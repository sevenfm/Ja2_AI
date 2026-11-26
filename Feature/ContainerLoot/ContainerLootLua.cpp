#include "Feature/ContainerLoot/ContainerLootLua.h"
#include "Feature/ContainerLoot/ContainerLoot.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "lua_state.h"
#include "Items.h"
#include "Item Types.h"

#include <algorithm>
#include <string>
#include <stdio.h>

// travnik: этап 3. Мост C++ -> Lua -> план размещения.
// C++ передаёт только контекст и сайты; шансы и выбор предметов делает Lua.

namespace
{
	const char* LuaScriptName = "scripts\\containerloot.lua";
	const char* LuaFunctionName = "GenerateSectorLoot";

	const char* KindName(UINT8 kind)
	{
		switch (kind)
		{
		case ContainerLootData::KIND_CONTAINER: return "container";
		case ContainerLootData::KIND_PROP: return "prop";
		case ContainerLootData::KIND_VEHICLE: return "vehicle";
		case ContainerLootData::KIND_NATURE: return "nature";
		case ContainerLootData::KIND_GROUND: return "ground";
		case ContainerLootData::KIND_BODY: return "body";
		default: return "unknown";
		}
	}

	const char* LayerName(UINT8 layer)
	{
		switch (layer)
		{
		case ContainerLootData::LAYER_OBJECT: return "object";
		case ContainerLootData::LAYER_TOPMOST: return "topmost";
		case ContainerLootData::LAYER_LAND: return "land";
		default: return "struct";
		}
	}

	void SetInt(lua_State* L, const char* key, int value)
	{
		lua_pushinteger(L, value);
		lua_setfield(L, -2, key);
	}

	void SetString(lua_State* L, const char* key, const std::string& value)
	{
		lua_pushstring(L, value.c_str());
		lua_setfield(L, -2, key);
	}

	void SetBool(lua_State* L, const char* key, bool value)
	{
		lua_pushboolean(L, value ? 1 : 0);
		lua_setfield(L, -2, key);
	}

	void SetFloat(lua_State* L, const char* key, float value)
	{
		lua_pushnumber(L, (lua_Number)value);
		lua_setfield(L, -2, key);
	}

	void PushStringList(lua_State* L, const char* key, const std::vector<std::string>& values)
	{
		lua_newtable(L);
		for (size_t i = 0; i < values.size(); ++i)
		{
			lua_pushstring(L, values[i].c_str());
			lua_rawseti(L, -2, (int)i + 1);
		}
		lua_setfield(L, -2, key);
	}

	int GetIntField(lua_State* L, int tableIndex, const char* key)
	{
		lua_getfield(L, tableIndex, key);
		const int value = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);
		return value;
	}

	BOOLEAN GetBoolField(lua_State* L, int tableIndex, const char* key)
	{
		lua_getfield(L, tableIndex, key);
		const BOOLEAN value = lua_toboolean(L, -1) ? TRUE : FALSE;
		lua_pop(L, 1);
		return value;
	}

	std::string GetStringField(lua_State* L, int tableIndex, const char* key)
	{
		lua_getfield(L, tableIndex, key);
		const char* value = lua_tostring(L, -1);
		std::string result = value ? value : "";
		lua_pop(L, 1);
		return result;
	}

	UINT8 PlacementFromString(const std::string& value)
	{
		if (value == "open") return ContainerLootData::PLACEMENT_OPEN;
		if (value == "hidden") return ContainerLootData::PLACEMENT_HIDDEN;
		if (value == "surface") return ContainerLootData::PLACEMENT_SURFACE;
		if (value == "corpse") return ContainerLootData::PLACEMENT_CORPSE;
		return ContainerLootData::PLACEMENT_NONE;
	}

	const char* PlacementName(UINT8 placement)
	{
		switch (placement)
		{
		case ContainerLootData::PLACEMENT_OPEN: return "open";
		case ContainerLootData::PLACEMENT_HIDDEN: return "hidden";
		case ContainerLootData::PLACEMENT_SURFACE: return "surface";
		case ContainerLootData::PLACEMENT_CORPSE: return "corpse";
		default: return "none";
		}
	}

	void PushLootItem(lua_State* L, const ContainerLootData::LootItemDef& item)
	{
		INT32 coolness = 0;
		INT32 itemClass = 0;
		if (item.item > 0 && item.item < gMAXITEMS_READ)
		{
			coolness = (INT32)Item[item.item].ubCoolness;
			itemClass = (INT32)Item[item.item].usItemClass;
		}

		lua_newtable(L);
		SetInt(L, "item", item.item);
		SetInt(L, "weight", (int)item.weight);
		SetInt(L, "min", (int)item.minAmount);
		SetInt(L, "max", (int)item.maxAmount);
		SetInt(L, "statusMin", item.minStatus);
		SetInt(L, "statusMax", item.maxStatus);
		SetBool(L, "money", item.isMoney ? true : false);
		SetBool(L, "randomItem", item.isRandomItem ? true : false);
		SetInt(L, "randomItemClass", (int)item.randomItemClass);
		SetString(L, "lootType", item.lootTypeId);
		SetInt(L, "coolness", coolness);
		SetInt(L, "itemClass", itemClass);
		SetInt(L, "minTier", item.minTier);
		SetInt(L, "maxTier", item.maxTier);
		PushStringList(L, "sectorTypes", item.sectorTypes);
	}

	void PushProfiles(lua_State* L, const std::vector<ContainerLootData::LootProfile>& profiles)
	{
		lua_newtable(L);

		for (std::vector<ContainerLootData::LootProfile>::const_iterator it = profiles.begin();
			it != profiles.end(); ++it)
		{
			const ContainerLootData::LootProfile& profile = *it;

			// travnik: disabled profiles must not be available to Lua fallback logic.
			if (!profile.enabled)
				continue;

			lua_newtable(L);
			SetString(L, "id", profile.id);
			SetString(L, "name", profile.name);
			SetInt(L, "priority", profile.priority);
			SetInt(L, "maxSites", profile.maxSites);
			SetInt(L, "emptyChance", profile.emptyChance);
			SetInt(L, "minRolls", profile.minRolls);
			SetInt(L, "maxRolls", profile.maxRolls);
			SetInt(L, "minTier", profile.minTier);
			SetInt(L, "maxTier", profile.maxTier);
			SetInt(L, "coolnessBonus", profile.coolnessBonus);
			SetString(L, "fallbackProfile", profile.fallbackProfile);
			SetString(L, "security", profile.securityProfile);
			PushStringList(L, "sectorTypes", profile.sectorTypes);

			lua_newtable(L);
			for (size_t i = 0; i < profile.placements.size(); ++i)
			{
				lua_newtable(L);
				SetString(L, "type", PlacementName(profile.placements[i].type));
				SetInt(L, "weight", (int)profile.placements[i].weight);
				lua_rawseti(L, -2, (int)i + 1);
			}
			lua_setfield(L, -2, "placements");

			lua_newtable(L);
			for (size_t i = 0; i < profile.items.size(); ++i)
			{
				PushLootItem(L, profile.items[i]);
				lua_rawseti(L, -2, (int)i + 1);
			}
			lua_setfield(L, -2, "items");

			lua_setfield(L, -2, profile.id.c_str());
		}

		lua_setfield(L, -2, "profiles");
	}

	void PushLootTypes(lua_State* L, const std::vector<ContainerLootData::LootTypeDef>& lootTypes)
	{
		lua_newtable(L);

		for (std::vector<ContainerLootData::LootTypeDef>::const_iterator it = lootTypes.begin();
			it != lootTypes.end(); ++it)
		{
			const ContainerLootData::LootTypeDef& type = *it;

			lua_newtable(L);
			SetString(L, "id", type.id);
			SetInt(L, "minTier", type.minTier);
			SetInt(L, "maxTier", type.maxTier);
			PushStringList(L, "sectorTypes", type.sectorTypes);

			lua_newtable(L);
			for (size_t i = 0; i < type.items.size(); ++i)
			{
				PushLootItem(L, type.items[i]);
				lua_rawseti(L, -2, (int)i + 1);
			}
			lua_setfield(L, -2, "items");

			lua_setfield(L, -2, type.id.c_str());
		}

		lua_setfield(L, -2, "lootTypes");
	}

	void PushSecurityProfiles(lua_State* L,
		const std::vector<ContainerLootData::ContainerSecurityProfile>& profiles)
	{
		lua_newtable(L);

		for (std::vector<ContainerLootData::ContainerSecurityProfile>::const_iterator it = profiles.begin();
			it != profiles.end(); ++it)
		{
			const ContainerLootData::ContainerSecurityProfile& profile = *it;

			lua_newtable(L);
			SetString(L, "id", profile.id);
			SetInt(L, "lockChanceMin", profile.lockChanceMin);
			SetInt(L, "lockChanceMax", profile.lockChanceMax);
			SetInt(L, "trapChanceMin", profile.trapChanceMin);
			SetInt(L, "trapChanceMax", profile.trapChanceMax);
			SetInt(L, "lockChanceCap", profile.lockChanceCap);
			SetInt(L, "trapChanceCap", profile.trapChanceCap);
			SetFloat(L, "lockProgressWeight", profile.lockProgressWeight);
			SetFloat(L, "lockCoolnessWeight", profile.lockCoolnessWeight);
			SetFloat(L, "trapProgressWeight", profile.trapProgressWeight);
			SetFloat(L, "trapCoolnessWeight", profile.trapCoolnessWeight);
			SetInt(L, "trapLevelMin", profile.trapLevelMin);
			SetInt(L, "trapLevelMax", profile.trapLevelMax);
			SetFloat(L, "trapLevelProgressWeight", profile.trapLevelProgressWeight);
			SetFloat(L, "trapLevelCoolnessWeight", profile.trapLevelCoolnessWeight);
			SetInt(L, "lockId", profile.lockId);

			lua_newtable(L);
			for (size_t i = 0; i < profile.trapTypes.size(); ++i)
			{
				lua_newtable(L);
				SetInt(L, "id", profile.trapTypes[i].id);
				SetInt(L, "weight", (int)profile.trapTypes[i].weight);
				lua_rawseti(L, -2, (int)i + 1);
			}
			lua_setfield(L, -2, "trapTypes");

			lua_setfield(L, -2, profile.id.c_str());
		}

		lua_setfield(L, -2, "securityProfiles");
	}

	void PushSectorSecurity(lua_State* L,
		const std::vector<ContainerLootData::SectorSecurityModifier>& modifiers)
	{
		lua_newtable(L);

		for (size_t i = 0; i < modifiers.size(); ++i)
		{
			lua_newtable(L);
			SetString(L, "tag", modifiers[i].tag);
			SetInt(L, "lockBonus", modifiers[i].lockBonus);
			SetInt(L, "trapBonus", modifiers[i].trapBonus);
			SetInt(L, "trapLevelBonus", modifiers[i].trapLevelBonus);
			lua_rawseti(L, -2, (int)i + 1);
		}

		lua_setfield(L, -2, "sectorSecurity");
	}

	void PushContext(lua_State* L, const ContainerLootData::Context& ctx)
	{
		lua_newtable(L);

		PushProfiles(L, ctx.profiles);
		PushLootTypes(L, ctx.lootTypes);
		PushSecurityProfiles(L, ctx.securityProfiles);
		PushSectorSecurity(L, ctx.sectorSecurity);

		SetString(L, "sectorKey", ctx.sectorKey);
		SetString(L, "sectorType", ctx.sectorType);
		PushStringList(L, "sectorTags", ctx.sectorTags);
		SetInt(L, "sectorX", ctx.sectorX);
		SetInt(L, "sectorY", ctx.sectorY);
		SetInt(L, "sectorZ", ctx.sectorZ);
		SetInt(L, "difficulty", ctx.difficulty);
		SetInt(L, "progress", ctx.progress);
		SetInt(L, "currentProgress", ctx.currentProgress);
		SetInt(L, "sectorCoolness", ctx.sectorCoolness);
		SetInt(L, "weather", ctx.weather);
		SetInt(L, "townId", ctx.townId);
		SetInt(L, "threat", ctx.threat);
		SetInt(L, "wealth", ctx.wealth);
		SetInt(L, "traffic", ctx.traffic);
		SetBool(L, "enemyControlled", ctx.enemyControlled ? true : false);
		SetInt(L, "worldDay", (int)ctx.worldDay);
		SetBool(L, "isUnderground", ctx.isUnderground ? true : false);
		SetBool(L, "foodSystem", ctx.foodSystem ? true : false);

		SetFloat(L, "progressWeight", ctx.progressWeight);
		SetFloat(L, "sectorCoolnessWeight", ctx.sectorCoolnessWeight);
		SetFloat(L, "militaryTypeBonus", ctx.militaryTypeBonus);
		SetFloat(L, "difficultyBonus", ctx.difficultyBonus);
		SetFloat(L, "progressTierCapBonus", ctx.progressTierCapBonus);
		SetFloat(L, "maxItemCoolnessBonus", ctx.maxItemCoolnessBonus);
		SetFloat(L, "moneyProgressScale", ctx.moneyProgressScale);
		SetFloat(L, "emptyChanceScale", ctx.emptyChanceScale);
		SetFloat(L, "coolnessNorm", ctx.coolnessNorm);

		// travnik: ??????? ?????? ?????? C++; Lua ???????? ??? ??????????????? ??????.
		std::vector<const ContainerLootData::Site*> orderedSites;
		orderedSites.reserve(ctx.sites.size());
		for (std::vector<ContainerLootData::Site>::const_iterator it = ctx.sites.begin(); it != ctx.sites.end(); ++it)
			orderedSites.push_back(&*it);

		std::sort(orderedSites.begin(), orderedSites.end(),
			[](const ContainerLootData::Site* left, const ContainerLootData::Site* right)
		{
			return ContainerLootData::SiteSortLess(*left, *right);
		});

		lua_newtable(L);
		for (size_t i = 0; i < orderedSites.size(); ++i)
		{
			const ContainerLootData::Site& site = *orderedSites[i];

			lua_newtable(L);
			SetInt(L, "site", site.site);
			SetString(L, "kind", KindName(site.kind));
			SetString(L, "subtype", site.subtype);
			SetString(L, "profile", site.profile);
			SetInt(L, "priority", site.priority);
			SetInt(L, "maxSites", site.maxSites);
			lua_pushinteger(L, (lua_Integer)site.randomKey);
			lua_setfield(L, -2, "randomKey");
			SetInt(L, "gridNo", site.gridNo);
			SetInt(L, "lootGridNo", site.lootGridNo);
			SetInt(L, "usIndex", site.usIndex);
			SetInt(L, "frame", site.frame);
			SetInt(L, "tileset", site.tileset);
			SetString(L, "layer", LayerName(site.layer));
			SetBool(L, "isOpen", site.isOpen ? true : false);
			SetInt(L, "height", site.height);
			SetBool(L, "canOpen", site.canOpen ? true : false);
			SetBool(L, "canHide", site.canHide ? true : false);
			SetBool(L, "canSurface", site.canSurface ? true : false);
			SetBool(L, "canCorpse", site.canCorpse ? true : false);
			lua_rawseti(L, -2, (int)i + 1);
		}
		lua_setfield(L, -2, "sites");
	}

	BOOLEAN ReadPlan(lua_State* L, std::vector<ContainerLootData::PlanEntry>& plan)
	{
		if (!lua_istable(L, -1))
		{
			ContainerLoot::Trace("ContainerLootLua: GenerateSectorLoot returned non-table");
			return FALSE;
		}

		const int count = (int)lua_objlen(L, -1);
		for (int i = 1; i <= count; ++i)
		{
			lua_rawgeti(L, -1, i);
			if (!lua_istable(L, -1))
			{
				lua_pop(L, 1);
				continue;
			}

			ContainerLootData::PlanEntry entry;
			entry.site = GetIntField(L, -1, "site");
			entry.placement = PlacementFromString(GetStringField(L, -1, "placement"));
			entry.money = (UINT32)GetIntField(L, -1, "money");
			entry.corpseItem = (UINT16)GetIntField(L, -1, "corpseItem");
			entry.profile = GetStringField(L, -1, "profile");
			if (!entry.profile.empty())
				entry.maxSites = GetIntField(L, -1, "maxSites");
			entry.hasLock = GetBoolField(L, -1, "hasLock");
			entry.lockId = (UINT8)GetIntField(L, -1, "lockId");
			entry.hasTrap = GetBoolField(L, -1, "hasTrap");
			entry.trapId = (UINT8)GetIntField(L, -1, "trapId");
			entry.trapLevel = (UINT8)GetIntField(L, -1, "trapLevel");

			lua_getfield(L, -1, "items");
			if (lua_istable(L, -1))
			{
				const int itemCount = (int)lua_objlen(L, -1);
				for (int j = 1; j <= itemCount; ++j)
				{
					lua_rawgeti(L, -1, j);
					if (lua_istable(L, -1))
					{
						ContainerLootData::PlanItem item;
						item.item = (UINT16)GetIntField(L, -1, "item");
						item.count = (UINT8)GetIntField(L, -1, "count");
						item.condition = (UINT8)GetIntField(L, -1, "condition");
						item.randomItemClass = (UINT16)GetIntField(L, -1, "randomItemClass");
						item.maxCoolness = (UINT8)GetIntField(L, -1, "maxCoolness");
						if (item.maxCoolness > ContainerLootData::MAX_ITEM_COOLNESS)
							item.maxCoolness = ContainerLootData::MAX_ITEM_COOLNESS;
						entry.items.push_back(item);
					}
					lua_pop(L, 1);
				}
			}
			lua_pop(L, 1);

			plan.push_back(entry);
			lua_pop(L, 1);
		}

		return TRUE;
	}
}

void ContainerLootLua::Reset()
{
}

BOOLEAN ContainerLootLua::Generate(const ContainerLootData::Context& ctx,
	std::vector<ContainerLootData::PlanEntry>& plan)
{
	plan.clear();

	LuaState state(LuaState::GET(lua::LUA_STATE_STRATEGIC_MAP));
	lua_State* L = state();
	if (!L)
	{
		ContainerLoot::Trace("ContainerLootLua: strategic Lua state is not initialized");
		return FALSE;
	}

	if (!state.EvalFile(LuaScriptName))
	{
		ContainerLoot::Trace("ContainerLootLua: cannot evaluate scripts\\containerloot.lua");
		return FALSE;
	}

	PushContext(L, ctx);

	lua_getglobal(L, LuaFunctionName);
	if (!lua_isfunction(L, -1))
	{
		ContainerLoot::Trace("ContainerLootLua: GenerateSectorLoot is not a function");
		lua_pop(L, 2);
		return FALSE;
	}

	lua_pushvalue(L, -2);
	if (lua_pcall(L, 1, 1, 0) != 0)
	{
		const char* error = lua_tostring(L, -1);
		CHAR8 message[512];
		sprintf(message, "ContainerLootLua: Lua error: %s", error ? error : "unknown");
		ContainerLoot::Trace(message);
		lua_pop(L, 2);
		return FALSE;
	}

	const BOOLEAN result = ReadPlan(L, plan);
	lua_pop(L, 2);

	if (result)
	{
		INT32 locked = 0;
		INT32 trapped = 0;
		for (std::vector<ContainerLootData::PlanEntry>::const_iterator it = plan.begin(); it != plan.end(); ++it)
		{
			if (it->hasLock)
				++locked;
			if (it->hasTrap)
				++trapped;
		}

		CHAR8 message[192];
		sprintf(message, "ContainerLootLua: plan=%d securityProfiles=%d locked=%d trapped=%d",
			(INT32)plan.size(), (INT32)ctx.securityProfiles.size(), locked, trapped);
		ContainerLoot::Trace(message);
	}

	return result;
}
