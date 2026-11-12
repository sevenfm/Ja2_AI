-- log helper: shows a message in the message log (FONT_MCOLOR_LTYELLOW = 144)
function LogHookMessage( message )
	SetScreenMsg( MESSAGE_COLOR_LTYELLOW, "[LUA] " .. tostring( message ) )
end

Facts = 
{
	FACT_BROTHEL_OPEN = 251,
	FACT_MUSEUM_OPEN = 250,
	FACT_CLUB_OPEN = 252,
	FACT_PAST_CLUB_CLOSING_AND_PLAYER_WARNED = 107,
}

function HourlyQuestUpdate()
	cHour = GetWorldHour()

	if (cHour == 4) then
		SetFactFalse( Facts.FACT_BROTHEL_OPEN )
	elseif (cHour == 20) then
		SetFactTrue( Facts.FACT_BROTHEL_OPEN )
	end

	-- Bar/night club
	if ( cHour == 15 ) then
		SetFactTrue( Facts.FACT_CLUB_OPEN )
		SetFactFalse( Facts.FACT_PAST_CLUB_CLOSING_AND_PLAYER_WARNED )

		-- Reset boxes fought
		for i = 0,2 do
			-- Set false
			gfBoxerFought(i,false)
		end

		-- If # of boxing matches the player has won is a multiple of
		-- 3, and the boxers haven't rested, then make them rest
		
		if ( gfBoxersResting == true ) then
			-- Done resting now!
			--SetgfBoxersResting(false)
			--SetgubBoxersRests(gubBoxersRests + 1)
			gfBoxersResting = false
			gubBoxersRests = gubBoxersRests + 1
		elseif ( math.floor(gubBoxingMatchesWon / 3) > gubBoxersRests ) then
			-- Time for the boxers to rest!
			--SetgfBoxersResting(true)
			gfBoxersResting = true
		end
	elseif ( cHour == 2 ) then
		SetFactFalse( Facts.FACT_CLUB_OPEN )
	end

	-- travnik AI mod: night forest ambush check
	NightForestAmbush()

	-- Museum
	if ( cHour == 9 ) then
		SetFactTrue( Facts.FACT_MUSEUM_OPEN )
	elseif ( cHour == 18 ) then
		SetFactFalse( Facts.FACT_MUSEUM_OPEN )
	end

end

-- ====================================================================
-- travnik AI mod: night forest ambushes (скрытая засада + ночная засада в лесу)
-- ====================================================================
-- Сценарий: ночью, пока отряд игрока стоит лагерем в лесу (TROPICS/DENSE),
-- с некоторым шансом за каждый час из враждебного соседнего сектора выдвигается
-- карательный отряд. Его встреча с игроком станет засадой (90%).
--
-- Управление:
--   факт 431 = 1  - сценарий включён (мастер-ключ, персистентный)
--   факт 432      - счётчик ночных рейдов (для статистики/лимитов)
--
-- Все настройки - в таблице NightAmbush ниже.
-- ====================================================================

-- terrain codes (SECTORINFO.ubTraversability[4], см. Strategic\Campaign Types.h)
-- terrain codes (SECTORINFO.ubTraversability[4], see Strategic\Campaign Types.h)
-- NOTE: values must match the engine enum exactly (TOWN=0 ... SWAMP_ROAD=24)
Terrain = {
	TOWN = 0, ROAD = 1, PLAINS = 2, SAND = 3, SPARSE = 4, DENSE = 5,
	SWAMP = 6, WATER = 7, HILLS = 8, GROUNDBARRIER = 9,
	NS_RIVER = 10, EW_RIVER = 11, EDGEOFWORLD = 12,
	TROPICS = 13, FARMLAND = 14, PLAINS_ROAD = 15, SPARSE_ROAD = 16,
	FARMLAND_ROAD = 17, TROPICS_ROAD = 18, DENSE_ROAD = 19, COASTAL = 20,
	HILLS_ROAD = 21, COASTAL_ROAD = 22, SAND_ROAD = 23, SWAMP_ROAD = 24,
}

NightAmbush = {
	ChancePerHour   = 100,   -- % шанс проверки за каждый ночной час 
	RaidCooldownHours = 6,  -- часов до следующей засады после выдвижения отряда (0 - без ограничения)
	TravelMinutes   = 20,   -- минут на переход отряда до сектора игрока (прибытие в пределах того же часа)
	MinTroops       = 3,    -- мин. regular в рейдовом отряде
	MaxTroops       = 4,    -- макс. regular
	MinElites       = 1,    -- мин. elite
	MaxElites       = 2,    -- макс. elite
	LogMessages     = true, -- выводить [LUA] сообщения о выдвижении отряда
}

-- travnik: константы ночных рейдов

-- флаги групп для SetGroupFlag/GetGroupFlag (см. GROUPFLAG_* в Strategic Movement.h)
LUAGROUPFLAG_HIGH_POTENTIAL_FOR_AMBUSH = 0x10	-- встреча с отрядом станет засадой (90%)
LUAGROUPFLAG_RAID_GROUP                = 0x40	-- рейдовый отряд: нападение на сектор игрока станет засадой

-- факты (см. FACT_NIGHT_AMBUSH_* в LuaInitNPCs.h; движковые факты заканчиваются на 430, NUM_FACTS = 500)
FACT_NIGHT_AMBUSH_ENABLED = 431	-- 1 = ночные засады включены (NightAmbushControl)
FACT_NIGHT_AMBUSH_COUNT   = 432	-- счётчик проведённых рейдов
FACT_NIGHT_AMBUSH_TIME_LO = 433	-- время последнего выдвижения (абсолютные часы = день*24+час), младший байт
FACT_NIGHT_AMBUSH_TIME_HI = 434	-- время последнего выдвижения, старший байт

-- время
HOURS_PER_DAY          = 24	-- часов в игровых сутках
SECTORS_PER_MAP_ROW    = 16	-- размер тактической карты в секторах (1..16)
FACT_VALUE_MAX         = 256	-- ёмкость одного факта (8 бит) для упаковки времени
MESSAGE_COLOR_LTYELLOW = 144	-- цвет сообщений [LUA] (FONT_MCOLOR_LTYELLOW)

-- ночное время рейдов
NIGHT_START_HOUR = 22	-- рейды с 22:00
NIGHT_END_HOUR   = 5	-- до 05:59

-- линейный номер сектора (0..255) из координат 1..16
function SectorID( x, y )
	return ( y - 1 ) * SECTORS_PER_MAP_ROW + ( x - 1 )
end

-- стратегический индекс (StrategicMap[]) из координат 1..16
-- ВАЖНО: отличается от SectorID! Формат: x + y*18 (MAP_WORLD_X = 18)
function StrategicIndex( x, y )
	return x + y * 18
end

-- лес/дикие заросли? еще добавлен сектор пустыни
function IsForestSector( x, y )
	local t = GetSectorTerrain( x, y )
	-- debug log
	-- LogHookMessage( "Sector Type: "..t)
	return ( t == Terrain.TROPICS or t == Terrain.DENSE or t == Terrain.SAND_ROAD) 
end

-- сектор под контролем врага?
function IsEnemyControlled( x, y )
	return SectorEnemyControlled( StrategicIndex( x, y ) ) == true
end

-- есть ли бойцы игрока в секторе (поверхностный уровень)?
function HasPlayerMercs( x, y )
	return PlayerMercsInSector( x, y, 0 ) > 0
end

-- сектор внутри карты и не за краем мира
function IsValidSector( x, y )
	return x >= 1 and x <= 16 and y >= 1 and y <= 16
end

-- выбрать соседний вражеский сектор для спавна рейдеров
-- приоритет: сектор с наибольшим количеством вражеских войск (NumNonPlayerTeamInSector, ENEMY_TEAM = 1),
-- при отсутствии войск у всех соседей - случайный вражеский сектор
-- возвращает spawnX, spawnY или nil
function FindRaiderSpawnSector( x, y )
	local dirs = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } }
	-- случайный порядок перебора направлений
	local order = { 1, 2, 3, 4 }
	for i = 4, 2, -1 do
		local j = math.random( i )
		order[i], order[j] = order[j], order[i]
	end

	local bestX, bestY, bestNum = nil, nil, 0
	local anyX, anyY = nil, nil

	for k = 1, 4 do
		local nx = x + dirs[ order[k] ][1]
		local ny = y + dirs[ order[k] ][2]
		if ( IsValidSector( nx, ny ) and IsEnemyControlled( nx, ny ) ) then
			-- запоминаем первый вражеский сектор (fallback)
			if ( anyX == nil ) then
				anyX = nx
				anyY = ny
			end

			-- ищем сектор с наибольшим количеством вражеских войск
			local num = NumNonPlayerTeamInSector( nx, ny, 1 )
			-- debug log
			-- LogHookMessage( "NightForestAmbush: spawn candidate " .. nx .. "/" .. ny .. " enemy troops " .. num )
			if ( num > bestNum ) then
				bestNum = num
				bestX = nx
				bestY = ny
			end
		end
	end

	if ( bestX ~= nil and bestNum > 0 ) then
		return bestX, bestY
	end

	-- ни у одного соседа нет войск - fallback на случайный вражеский сектор
	return anyX, anyY
end

-- главная почасовая проверка
function NightForestAmbush()

	-- проверка включения ночных засад
	-- if ( CheckFact( FACT_NIGHT_AMBUSH_ENABLED, 0 ) == false ) then
		-- debug log
		-- LogHookMessage( "NightForestAmbush: disabled (fact " .. FACT_NIGHT_AMBUSH_ENABLED .. ")" )
	--	return
	-- end

	-- ночь: 22:00 - 05:59
	local h = GetWorldHour()
	if ( h < NIGHT_START_HOUR and h > NIGHT_END_HOUR ) then
		-- debug log
		-- LogHookMessage( "NightForestAmbush: not night, hour " .. h )
		return
	end

	-- шанс за час
	if ( math.random( 100 ) > NightAmbush.ChancePerHour ) then
		-- debug log
		-- LogHookMessage( "NightForestAmbush: chance failed" )
		return
	end

	-- защита от повторных отрядов: если предыдущий отряд ещё выдвигается (или кулдаун не истёк), новую засаду не начинаем
	-- время последнего выдвижения хранится в фактах FACT_NIGHT_AMBUSH_TIME_LO/HI (абсолютные часы = день*24+час, разбитые на младший/старший байт; СОХРАНЯЕТСЯ в сейве)
	local uiAbsNow = GetWorldDay() * HOURS_PER_DAY + GetWorldHour()
	local uiLastRaid = GetFact( FACT_NIGHT_AMBUSH_TIME_LO ) + GetFact( FACT_NIGHT_AMBUSH_TIME_HI ) * FACT_VALUE_MAX
	if ( uiLastRaid > 0 and uiAbsNow >= uiLastRaid ) then
		local uiElapsedHours = uiAbsNow - uiLastRaid
		if ( NightAmbush.RaidCooldownHours > 0 and uiElapsedHours < NightAmbush.RaidCooldownHours ) then
			-- debug log
			-- LogHookMessage( "NightForestAmbush: raid already in progress, elapsed " .. uiElapsedHours .. " h" )
			if ( Messages and Messages.NightAmbushInProgress ) then
				SetScreenMsg( MESSAGE_COLOR_LTYELLOW, tostring( Messages.NightAmbushInProgress ) )
			end
			return
		end
	end

	-- перебор всех поверхностных секторов
	for x = 1, SECTORS_PER_MAP_ROW do
		for y = 1, SECTORS_PER_MAP_ROW do
			-- лагерь игрока: есть бойцы, лес, нет врагов в секторе, сектор не вражий
			if ( HasPlayerMercs( x, y ) ) then
				-- debug log
				-- LogHookMessage( "NightForestAmbush: mercs in " .. x .. "/" .. y ..
				--	" forest=" .. tostring( IsForestSector( x, y ) ) ..
				--	" hostiles=" .. GetNumHostilesInSector( x, y, 0 ) ..
				--	" enemy=" .. tostring( IsEnemyControlled( x, y ) ) )
			end

			if ( HasPlayerMercs( x, y )
				and IsForestSector( x, y )
				and GetNumHostilesInSector( x, y, 0 ) == 0
				and not IsEnemyControlled( x, y ) ) then

				-- ищем откуда выдвинуть рейдеров
				local spawnX, spawnY = FindRaiderSpawnSector( x, y )
				if ( spawnX ~= nil ) then
					-- состав рейдового отряда
					local troops = NightAmbush.MinTroops + math.random( NightAmbush.MaxTroops - NightAmbush.MinTroops + 1 )
					local elites = NightAmbush.MinElites + math.random( NightAmbush.MaxElites - NightAmbush.MinElites + 1 )

					local grpID = CreateEnemyGroup( SectorID( spawnX, spawnY ), 0, troops, elites )
					if ( grpID ~= nil and grpID > 0 ) then
						-- помечаем отряд как рейдовый: нападение на сектор игрока станет засадой
						SetGroupFlag( grpID, LUAGROUPFLAG_RAID_GROUP, true )
						-- встреча станет засадой (90%)
						SetGroupHighAmbushPotential( grpID )
						-- быстрый переход: отряд прибудет к сектору игрока в пределах того же часа
						SendGroupToSectorNow( grpID, x, y, NightAmbush.TravelMinutes )

						-- статистика
						SetFact( FACT_NIGHT_AMBUSH_COUNT, GetFact( FACT_NIGHT_AMBUSH_COUNT ) + 1 )

						-- запоминаем время выдвижения (факты FACT_NIGHT_AMBUSH_TIME_LO/HI: абсолютные часы = день*24+час, младший/старший байт)
						SetFact( FACT_NIGHT_AMBUSH_TIME_LO, uiAbsNow % FACT_VALUE_MAX )
						SetFact( FACT_NIGHT_AMBUSH_TIME_HI, math.floor( uiAbsNow / FACT_VALUE_MAX ) )

						if ( NightAmbush.LogMessages ) then
							if ( Messages and Messages.NightAmbushSpawned ) then
								SetScreenMsg( MESSAGE_COLOR_LTYELLOW, string.format( Messages.NightAmbushSpawned, troops + elites, spawnX, spawnY, x, y ) )
							else
								-- debug log
								-- SetScreenMsg( MESSAGE_COLOR_LTYELLOW, "[LUA] Night ambush: enemy party (" .. (troops + elites) .. " men) dispatched from " .. spawnX .. "/" .. spawnY .. " to your camp in " .. x .. "/" .. y )
							end
						end
					end

					-- одна засада за час достаточно
					return
				end
			end
		end
	end
end
