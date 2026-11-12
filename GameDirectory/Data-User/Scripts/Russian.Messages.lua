--[[

	Строки сообщений для HourlyUpdate.lua (русская локализация).

	Загружается из C++ с префиксом языка, как undergroundsectornames.lua.

	Сохранять в UTF-8, БЕЗ BOM
	(Lua-парсер, используемый LetLuaHourlyQuestUpdate, не понимает BOM).

]]

Messages = {

	-- night ambush: screen message when a raiding party is dispatched
	NightAmbushInProgress	= "[LUA] Ночная засада: отряд уже выдвинулся, новый не создаётся",
	NightAmbushSpawned		= "[LUA] Ночная засада: отряд врагов (%d чел.) выдвинулся из %d/%d к вашему лагерю в %d/%d",

}
