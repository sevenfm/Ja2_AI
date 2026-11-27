#ifndef __CONTAINER_LOOT_TYPES_H
#define __CONTAINER_LOOT_TYPES_H

#include "Types.h"
#include <string>
#include <vector>

// travnik: общие типы фичи ContainerLoot. Здесь нет игровой логики — только
// описание кандидатов на лут, способов размещения и контекста для Lua.

namespace ContainerLootData
{
	// travnik: тип объекта, в котором может лежать лут
	enum EKind
	{
		KIND_CONTAINER = 0,
		KIND_PROP,
		KIND_VEHICLE,
		KIND_NATURE,
		KIND_GROUND,
		KIND_BODY,
	};

	// travnik: способ размещения лута, который выбирает Lua
	enum EPlacement
	{
		PLACEMENT_NONE = 0,
		PLACEMENT_OPEN,
		PLACEMENT_HIDDEN,
		PLACEMENT_SURFACE,
		PLACEMENT_CORPSE,
	};

	// travnik: слой карты, на котором найден объект
	enum ELayer
	{
		LAYER_STRUCT = 0,
		LAYER_OBJECT,
		LAYER_TOPMOST,
		LAYER_LAND,
	};

	// travnik: диапазон tier/coolness для правил лута
	const INT32 MAX_LOOT_TIER = 10;
	const INT32 MAX_ITEM_COOLNESS = 10;
	const UINT8 MAX_TRAP_LEVEL = 10;
	const UINT8 MAX_LOCK_ID = 254;

	// travnik: элемент пула профиля из EnvironmentLoot.xml.
	struct LootItemDef
	{
		LootItemDef()
			: enabled(TRUE), item(0), weight(1), minAmount(1), maxAmount(1),
			  minStatus(100), maxStatus(100), isMoney(FALSE),
			  randomItemClass(0), isRandomItem(FALSE),
			  minTier(0), maxTier(MAX_LOOT_TIER)
		{
		}

		BOOLEAN	enabled;		// travnik: false disables this loot entry without removing it from XML.
		UINT16	item;
		UINT32	weight;
		UINT32	minAmount;
		UINT32	maxAmount;
		UINT8	minStatus;
		UINT8	maxStatus;
		BOOLEAN	isMoney;
		UINT16	randomItemClass;	// travnik: ссылка на RandomItem.xml
		BOOLEAN	isRandomItem;		// travnik: запись RANDOM_ITEM
		std::string	lootTypeId;	// travnik: ссылка на LOOT_TYPE/TYPE
		std::vector<std::string> sectorTypes;	// travnik: где разрешён предмет
		INT32	minTier;
		INT32	maxTier;
	};

	// travnik: именованный пул предметов для ITEM_POOL/TYPE.
	struct LootTypeDef
	{
		LootTypeDef()
			: minTier(0), maxTier(MAX_LOOT_TIER)
		{
		}

		std::string	id;
		std::vector<LootItemDef> items;
		std::vector<std::string> sectorTypes;
		INT32	minTier;
		INT32	maxTier;
	};

	// travnik: разрешённый способ размещения и его вес из XML.
	struct LootPlacementDef
	{
		LootPlacementDef()
			: type(PLACEMENT_NONE), weight(1)
		{
		}

		UINT8	type;
		UINT32	weight;
	};

	// travnik: селектор тайла по имени STI (+ необязательный кадр).
	struct LootImageSelector
	{
		LootImageSelector()
			: frame(0), useFrame(FALSE), kind(0), useKind(FALSE)
		{
		}

		std::string	image;
		UINT16		frame;
		BOOLEAN		useFrame;
		UINT8		kind;
		BOOLEAN		useKind;
	};

	// travnik: профиль лута из TableData\Loot\EnvironmentLoot.xml.
	// travnik: ��� ���������� �������� ��� ����������.
	struct TrapTypeDef
	{
		TrapTypeDef()
			: id(0), weight(1)
		{
		}

		UINT8	id;
		UINT32	weight;
	};

	// travnik: ���������� ������ ���������� ����������.
	struct ContainerSecurityProfile
	{
		ContainerSecurityProfile()
			: lockChanceMin(0), lockChanceMax(0), trapChanceMin(0), trapChanceMax(0),
			  lockChanceCap(100), trapChanceCap(100),
			  lockProgressWeight(0.0f), lockCoolnessWeight(0.0f),
			  trapProgressWeight(0.0f), trapCoolnessWeight(0.0f),
			  trapLevelMin(1), trapLevelMax(1),
			  trapLevelProgressWeight(0.0f), trapLevelCoolnessWeight(0.0f),
			  lockId(0)
		{
		}

		std::string	id;
		UINT8		lockChanceMin;
		UINT8		lockChanceMax;
		UINT8		trapChanceMin;
		UINT8		trapChanceMax;
		UINT8		lockChanceCap;
		UINT8		trapChanceCap;
		FLOAT		lockProgressWeight;
		FLOAT		lockCoolnessWeight;
		FLOAT		trapProgressWeight;
		FLOAT		trapCoolnessWeight;
		UINT8		trapLevelMin;
		UINT8		trapLevelMax;
		FLOAT		trapLevelProgressWeight;
		FLOAT		trapLevelCoolnessWeight;
		UINT8		lockId;
		std::vector<TrapTypeDef> trapTypes;
	};

	// travnik: �������� �� ����� ��������.
	struct SectorSecurityModifier
	{
		SectorSecurityModifier()
			: lockBonus(0), trapBonus(0), trapLevelBonus(0)
		{
		}

		std::string	tag;
		INT32		lockBonus;
		INT32		trapBonus;
		INT32		trapLevelBonus;
	};
	struct LootProfile
	{
		LootProfile()
			: enabled(TRUE), priority(100), maxSites(0), emptyChance(0), minRolls(1), maxRolls(1),
			  minTier(0), maxTier(MAX_LOOT_TIER), coolnessBonus(0)
		{
		}

		BOOLEAN	enabled;		// travnik: false makes the profile invisible to matching and fallback.
		std::string	id;
		std::string	name;
		std::vector<UINT32>	tileKeys;	// travnik: (tileset << 16) | usIndex
		std::vector<LootImageSelector> images;
		std::vector<UINT8> kinds;		// travnik: KIND_CONTAINER/KIND_NATURE/KIND_VEHICLE
		std::vector<LootPlacementDef> placements;
		std::vector<LootItemDef> items;
		INT32	priority;		// travnik: ??????? ????????? ??????? (?????? ? ??????)
		INT32	maxSites;		// travnik: ???????? ??????? ???????????? ?????? ?? ?????? (0 = ??? ??????)
		UINT8	emptyChance;
		UINT8	minRolls;
		UINT8	maxRolls;
		std::vector<std::string> sectorTypes;	// travnik: где разрешён контейнер
		std::string	fallbackProfile;	// travnik: профиль при запрете сектора
		std::string	securityProfile;	// travnik: security profile id for this loot profile.
		INT32	minTier;		// travnik: минимальный tier профиля
		INT32	maxTier;		// travnik: максимальный tier профиля
		INT32	coolnessBonus;		// travnik: сдвиг допустимого coolness предметов
	};

	// travnik: один кандидат на размещение лута
	struct Site
	{
		Site()
			: site(0), gridNo(0), lootGridNo(0), usIndex(0), frame(0), tileset(0),
			  kind(KIND_GROUND), layer(LAYER_STRUCT), isOpen(FALSE), height(0),
			  canOpen(FALSE), canHide(FALSE), canSurface(FALSE), canCorpse(FALSE),
			  priority(100), maxSites(0), randomKey(0)
		{
		}

		INT32		site;			// travnik: 1-based идентификатор для Lua
		INT32		gridNo;			// travnik: тайл-якорь объекта
		INT32		lootGridNo;		// travnik: куда реально класть предметы
		UINT16		usIndex;		// travnik: графика тайла
		UINT16		frame;			// travnik: кадр STI (usRegionIndex)
		UINT8		tileset;		// travnik: tileset
		UINT8		kind;			// travnik: EKind
		UINT8		layer;			// travnik: ELayer
		BOOLEAN		isOpen;			// travnik: для открывающихся контейнеров
		UINT8		height;			// travnik: подсказка для render height
		BOOLEAN		canOpen;		// travnik: можно открыть как контейнер
		BOOLEAN		canHide;		// travnik: можно сделать HIDDEN_ITEM
		BOOLEAN		canSurface;		// travnik: можно положить на поверхность
		BOOLEAN		canCorpse;		// travnik: можно создать труп
		std::string	subtype;		// travnik: barrel/tree/wreck/... (может быть пустым)
		std::string	profile;		// travnik: id ??????? ?? EnvironmentLoot.xml
		INT32		priority;		// travnik: ????????? ?? XML-???????
		INT32		maxSites;		// travnik: ????? ???????? ?????? ?? XML-???????
		UINT32		randomKey;		// travnik: ????????????????? ???? ?????????????
	};

	// travnik: план, который возвращает Lua. C++ только валидирует и применяет.
	// travnik: ?????? ??????? ?????? ??? C++ ? Lua: priority, randomKey, site id.
	inline BOOLEAN SiteSortLess(const Site& left, const Site& right)
	{
		if (left.priority != right.priority)
			return left.priority < right.priority;
		if (left.randomKey != right.randomKey)
			return left.randomKey < right.randomKey;
		return left.site < right.site;
	}

	struct PlanItem
	{
		PlanItem()
			: item(0), count(0), condition(100),
			  randomItemClass(0), maxCoolness(MAX_ITEM_COOLNESS)
		{
		}

		UINT16 item;
		UINT8 count;
		UINT8 condition;
		UINT16 randomItemClass;	// travnik: класс RandomItem.xml, 0 = обычный предмет
		UINT8 maxCoolness;		// travnik: предел coolness для random item
	};

	struct PlanEntry
	{
		PlanEntry()
			: site(0), placement(PLACEMENT_NONE), money(0), corpseItem(0), maxSites(-1),
			  hasLock(FALSE), lockId(0), hasTrap(FALSE), trapId(0), trapLevel(0)
		{
		}

		INT32 site;
		UINT8 placement;
		UINT32 money;
		UINT16 corpseItem;
		std::string profile;	// travnik: фактический профиль после fallback
		INT32 maxSites;		// travnik: лимит фактического профиля, -1 = из Site
		std::vector<PlanItem> items;
		BOOLEAN	hasLock;
		UINT8		lockId;
		BOOLEAN	hasTrap;
		UINT8		trapId;
		UINT8		trapLevel;
	};

	// travnik: контекст сектора и глобальные коэффициенты для Lua.
	struct Context
	{
		Context()
			: sectorX(0), sectorY(0), sectorZ(0), difficulty(0), progress(0),
			  currentProgress(0), sectorCoolness(0), weather(0), isUnderground(FALSE),
			  foodSystem(FALSE), townId(0), threat(0), wealth(0), traffic(0), enemyControlled(FALSE), worldDay(0),
			  progressWeight(1.0f), sectorCoolnessWeight(1.0f),
			  militaryTypeBonus(1.0f), difficultyBonus(0.5f),
			  progressTierCapBonus(3.0f), maxItemCoolnessBonus(2.0f),
			  moneyProgressScale(0.5f), emptyChanceScale(1.0f), coolnessNorm(0.0f)
		{
		}

		INT16		sectorX;
		INT16		sectorY;
		INT8		sectorZ;
		UINT8		difficulty;
		UINT8		progress;			// travnik: HighestPlayerProgressPercentage
		UINT8		currentProgress;	// travnik: CurrentPlayerProgressPercentage
		UINT32		sectorCoolness;
		UINT32		weather;
		BOOLEAN		isUnderground;
		BOOLEAN		foodSystem;		// travnik: в Lua нельзя выбирать еду, если food system выключен
		UINT8		townId;
		UINT8		threat;
		UINT8		wealth;
		UINT8		traffic;
		BOOLEAN		enemyControlled;
		UINT32		worldDay;
		std::string	sectorKey;
		std::string	sectorType;
		std::vector<std::string> sectorTags;
		std::vector<Site> sites;
		std::vector<LootProfile> profiles;	// travnik: статические профили из XML
		std::vector<LootTypeDef> lootTypes;	// travnik: именованные пулы TYPE
		std::vector<ContainerSecurityProfile> securityProfiles;
		std::vector<SectorSecurityModifier> sectorSecurity;

		// travnik: глобальные коэффициенты из Ja2_Options.ini
		FLOAT		progressWeight;
		FLOAT		sectorCoolnessWeight;
		FLOAT		militaryTypeBonus;
		FLOAT		difficultyBonus;
		FLOAT		progressTierCapBonus;
		FLOAT		maxItemCoolnessBonus;
		FLOAT		moneyProgressScale;
		FLOAT		emptyChanceScale;
		FLOAT		coolnessNorm;
	};

	// travnik: абсолютные предохранители, а не баланс. Баланс — в Lua.
	const INT32 MAX_ITEMS_PER_SITE = 6;
	const INT32 MAX_COUNT = 10;
	const UINT32 MAX_MONEY = 10000;
	const INT32 DEFAULT_PROFILE_PRIORITY = 100;
	const INT32 DEFAULT_PROFILE_MAX_SITES = 0;
	// travnik: ????? ????? ???????????? ???????? ?? ??????.
	const INT32 DEFAULT_MAX_SPAWNS_PER_SECTOR = 400;

	// travnik: временный общий выключатель фичи для контрольной сборки.
	// FALSE — hook и ProcessSector полностью отключены.
	#define CONTAINER_LOOT_ENABLED TRUE

	// travnik: на этапе 0-1 включён аудит карты; в release он отключён.
#ifdef _DEBUG
	#define CONTAINER_LOOT_AUDIT_ENABLED TRUE
#else
	#define CONTAINER_LOOT_AUDIT_ENABLED FALSE
#endif

	// travnik: debug-разрешение генерировать лут в уже посещённых секторах.
	// TRUE — можно тестировать на старых сейвах; FALSE — обычное поведение.
#ifdef _DEBUG
	#define CONTAINER_LOOT_ALLOW_VISITED_SECTORS TRUE
#else
	#define CONTAINER_LOOT_ALLOW_VISITED_SECTORS FALSE
#endif

	// travnik: одноразовый сброс SF_CONTAINER_LOOT_GENERATED для каждого сектора
	// в рамках текущего запуска игры. Нужен только для повторного теста лута.
#ifdef _DEBUG
	#define CONTAINER_LOOT_FORCE_REGENERATE TRUE
#else
	#define CONTAINER_LOOT_FORCE_REGENERATE FALSE
#endif

	// travnik: debug-инструмент для полного каталога контейнеров и пропсов.
	// При первом входе в новый мир один раз сканирует все карты и пишет
	// ContainerLoot_allmaps.csv + ContainerLoot_catalog_template.csv.
	// После успешного сканирования выключаем, чтобы не тратить время.
#define CONTAINER_LOOT_SCAN_ALL_MAPS FALSE

	// travnik: вместе со сканированием карт выгружает кадры STI в BMP
	// и собирает ContainerLoot_contact_sheets/index.html.
#define CONTAINER_LOOT_GENERATE_CONTACT_SHEETS FALSE

	// travnik: debug — временно показывать скрытый лут как обычный видимый.
	// Нужно, чтобы проверить, в какой клетке реально оказался предмет.
	// Для нормальной игры поставить FALSE.
#define CONTAINER_LOOT_DEBUG_VISIBLE_HIDDEN FALSE
}

#endif
