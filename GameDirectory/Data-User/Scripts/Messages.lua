--[[

	Message strings for HourlyUpdate.lua.

	Localization: this file is loaded from C++ with a language prefix,
	just like undergroundsectornames.lua. English version has no prefix,
	Russian version is "Russian.Messages.lua", etc.

	Make sure to save this file as UTF-8, WITHOUT BOM signature
	(the Lua parser used by LetLuaHourlyQuestUpdate dislikes BOMs).

]]

Messages = {

	-- night ambush: screen message when a raiding party is dispatched
	NightAmbushInProgress	= "[LUA] Night ambush: raid already in progress, skipping",
	NightAmbushSpawned		= "[LUA] Night ambush: enemy party (%d men) dispatched from %d/%d to your camp in %d/%d",

}
