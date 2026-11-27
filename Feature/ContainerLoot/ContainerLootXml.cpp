#include "Feature/ContainerLoot/ContainerLootXml.h"
#include "Feature/ContainerLoot/ContainerLoot.h"

#include "expat.h"
#include "FileMan.h"
#include "MemMan.h"
#include "Debug.h"
#include "Keys.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <string>
#include <vector>

// travnik: отдельный парсер EnvironmentLoot.xml на Expat, по образцу
// остальных XML-загрузчиков проекта (FileOpen/FileRead + XML_Parser).
namespace
{
	const char* const ENVIRONMENT_LOOT_FILE = "TableData\\Loot\\EnvironmentLoot.xml";

	// travnik: sector-specificity weights used only for profile selection.
	const INT32 PROFILE_EXACT_SECTOR_BONUS = 100;
	const INT32 PROFILE_STRONG_TAG_BONUS = 5;
	const INT32 PROFILE_MEDIUM_TAG_BONUS = 3;
	const INT32 PROFILE_WEAK_TAG_BONUS = 1;

	enum EXmlState
	{
		XML_STATE_ROOT = 0,
		XML_STATE_PROFILE,
		XML_STATE_TILES,
		XML_STATE_TILE,
		XML_STATE_KINDS,
		XML_STATE_KIND,
		XML_STATE_PLACEMENTS,
		XML_STATE_PLACEMENT,
		XML_STATE_SECURITY_PROFILES,
		XML_STATE_SECURITY_PROFILE,
		XML_STATE_TRAP_TYPES,
		XML_STATE_TRAP_TYPE,
		XML_STATE_SECTOR_SECURITY,
		XML_STATE_SECTOR_TAG,
		XML_STATE_ITEM_POOL,
		XML_STATE_ITEM,
		XML_STATE_MONEY,
		XML_STATE_EMPTY_CHANCE,
		XML_STATE_LOOT_TYPES,
		XML_STATE_LOOT_TYPE,
	};

	struct ParseData
	{
		ParseData()
			: state(XML_STATE_ROOT), profiles(NULL), lootTypes(NULL),
			  securityProfiles(NULL), sectorSecurity(NULL),
			  profileIndex(-1), lootTypeIndex(-1), securityProfileIndex(-1), sawRoot(FALSE),
			  inLootType(FALSE)
		{
		}

		EXmlState	state;
		std::vector<ContainerLootData::LootProfile>* profiles;
		std::vector<ContainerLootData::LootTypeDef>* lootTypes;
		std::vector<ContainerLootData::ContainerSecurityProfile>* securityProfiles;
		std::vector<ContainerLootData::SectorSecurityModifier>* sectorSecurity;
		INT32		profileIndex;
		INT32		lootTypeIndex;
		INT32		securityProfileIndex;
		BOOLEAN		sawRoot;
		BOOLEAN		inLootType;
		std::string	charData;
	};

	const char* GetAttr(const XML_Char** atts, const char* name)
	{
		if (!atts)
			return NULL;

		for (INT32 i = 0; atts[i]; i += 2)
		{
			if (strcmp(atts[i], name) == 0)
				return atts[i + 1];
		}

		return NULL;
	}

	INT32 ToInt(const char* value, INT32 defaultValue)
	{
		if (!value || !*value)
			return defaultValue;

		char* end = NULL;
		const long parsed = strtol(value, &end, 10);
		if (end == value)
			return defaultValue;
		if (parsed > INT_MAX)
			return INT_MAX;
		if (parsed < INT_MIN)
			return INT_MIN;

		return (INT32)parsed;
	}

	INT32 ToClampedInt(const char* value, INT32 defaultValue, INT32 minValue, INT32 maxValue)
	{
		INT32 parsed = ToInt(value, defaultValue);
		if (parsed < minValue)
			parsed = minValue;
		if (parsed > maxValue)
			parsed = maxValue;
		return parsed;
	}

	UINT8 ToUInt8(const char* value, UINT8 defaultValue)
	{
		return (UINT8)ToClampedInt(value, (INT32)defaultValue, 0, 255);
	}

	UINT16 ToUInt16(const char* value, UINT16 defaultValue)
	{
		return (UINT16)ToClampedInt(value, (INT32)defaultValue, 0, 65535);
	}

	UINT32 ToUInt32(const char* value, UINT32 defaultValue)
	{
		return (UINT32)ToClampedInt(value, (INT32)defaultValue, 0, INT_MAX);
	}

	BOOLEAN ToBool(const char* value, BOOLEAN defaultValue)
	{
		if (!value || !*value)
			return defaultValue;

		return (strcmp(value, "1") == 0 ||
			_stricmp(value, "true") == 0 ||
			_stricmp(value, "yes") == 0) ? TRUE : FALSE;
	}

	std::string NormalizeToken(const std::string& value)
	{
		std::string token = value;
		const size_t first = token.find_first_not_of(" \t\r\n");
		const size_t last = token.find_last_not_of(" \t\r\n");
		if (first == std::string::npos)
			return "";

		token = token.substr(first, last - first + 1);
		for (size_t i = 0; i < token.size(); ++i)
			token[i] = (char)tolower((unsigned char)token[i]);

		return token;
	}

	std::vector<std::string> SplitList(const char* value)
	{
		std::vector<std::string> result;
		if (!value || !*value)
			return result;

		const std::string text(value);
		size_t start = 0;
		while (start <= text.size())
		{
			const size_t comma = text.find(',', start);
			const std::string token = NormalizeToken(text.substr(start,
				comma == std::string::npos ? std::string::npos : comma - start));
			if (!token.empty())
				result.push_back(token);

			if (comma == std::string::npos)
				break;

			start = comma + 1;
		}

		return result;
	}

	bool ContainsToken(const std::vector<std::string>& tokens, const std::string& value)
	{
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			if (tokens[i] == value)
				return true;
		}
		return false;
	}

	bool SectorListAllows(const std::vector<std::string>& sectorTypes,
		const std::string& sectorType,
		const std::vector<std::string>& sectorTags)
	{
		if (sectorTypes.empty())
			return true;

		if (!sectorType.empty() && ContainsToken(sectorTypes, sectorType))
			return true;

		for (size_t i = 0; i < sectorTags.size(); ++i)
		{
			if (ContainsToken(sectorTypes, sectorTags[i]))
				return true;
		}

		return false;
	}

	std::string NormalizeImageName(const std::string& value)
	{
		std::string image = value;
		const size_t slash = image.find_last_of("\\/");
		if (slash != std::string::npos)
			image = image.substr(slash + 1);

		const size_t dot = image.find_last_of('.');
		if (dot != std::string::npos)
			image = image.substr(0, dot);

		for (size_t i = 0; i < image.size(); ++i)
			image[i] = (char)tolower((unsigned char)image[i]);

		return image;
	}

	BOOLEAN ImageEquals(const std::string& left, const std::string& right)
	{
		return _stricmp(NormalizeImageName(left).c_str(), NormalizeImageName(right).c_str()) == 0;
	}

	UINT8 PlacementFromXml(const char* value)
	{
		if (!value)
			return ContainerLootData::PLACEMENT_NONE;

		if (_stricmp(value, "open") == 0)
			return ContainerLootData::PLACEMENT_OPEN;
		if (_stricmp(value, "hidden") == 0)
			return ContainerLootData::PLACEMENT_HIDDEN;
		if (_stricmp(value, "surface") == 0)
			return ContainerLootData::PLACEMENT_SURFACE;
		if (_stricmp(value, "corpse") == 0)
			return ContainerLootData::PLACEMENT_CORPSE;

		return ContainerLootData::PLACEMENT_NONE;
	}

	UINT8 KindFromXml(const char* value)
	{
		if (!value)
			return 0xFF;

		if (_stricmp(value, "container") == 0)
			return ContainerLootData::KIND_CONTAINER;
		if (_stricmp(value, "prop") == 0)
			return ContainerLootData::KIND_PROP;
		if (_stricmp(value, "vehicle") == 0)
			return ContainerLootData::KIND_VEHICLE;
		if (_stricmp(value, "nature") == 0)
			return ContainerLootData::KIND_NATURE;
		if (_stricmp(value, "ground") == 0)
			return ContainerLootData::KIND_GROUND;
		if (_stricmp(value, "body") == 0)
			return ContainerLootData::KIND_BODY;

		return 0xFF;
	}

	ContainerLootData::LootProfile* CurrentProfile(ParseData* data)
	{
		if (!data || !data->profiles || data->profileIndex < 0)
			return NULL;

		if (data->profileIndex >= (INT32)data->profiles->size())
			return NULL;

		return &(*data->profiles)[data->profileIndex];
	}

	ContainerLootData::LootTypeDef* CurrentLootType(ParseData* data)
	{
		if (!data || !data->lootTypes || data->lootTypeIndex < 0)
			return NULL;

		if (data->lootTypeIndex >= (INT32)data->lootTypes->size())
			return NULL;

		return &(*data->lootTypes)[data->lootTypeIndex];
	}

	ContainerLootData::ContainerSecurityProfile* CurrentSecurityProfile(ParseData* data)
	{
		if (!data || !data->securityProfiles || data->securityProfileIndex < 0)
			return NULL;

		if (data->securityProfileIndex >= (INT32)data->securityProfiles->size())
			return NULL;

		return &(*data->securityProfiles)[data->securityProfileIndex];
	}

	UINT8 TrapTypeFromXml(const char* value)
	{
		if (!value)
			return NO_TRAP;

		if (_stricmp(value, "EXPLOSION") == 0)
			return EXPLOSION;
		if (_stricmp(value, "ELECTRIC") == 0)
			return ELECTRIC;
		if (_stricmp(value, "SIREN") == 0)
			return SIREN;
		if (_stricmp(value, "SILENT_ALARM") == 0)
			return SILENT_ALARM;
		if (_stricmp(value, "BROTHEL_SIREN") == 0)
			return BROTHEL_SIREN;
		if (_stricmp(value, "SUPER_ELECTRIC") == 0)
			return SUPER_ELECTRIC;

		return NO_TRAP;
	}

	void ParseTile(ParseData* data, const XML_Char** atts)
	{
		ContainerLootData::LootProfile* profile = CurrentProfile(data);
		if (!profile)
			return;

		const char* tilesetValue = GetAttr(atts, "tileset");
		const char* usIndexValue = GetAttr(atts, "usIndex");
		const char* imageValue = GetAttr(atts, "image");
		const char* frameValue = GetAttr(atts, "frame");
		const char* kindValue = GetAttr(atts, "kind");

		if (tilesetValue && usIndexValue)
		{
			const UINT8 tileset = ToUInt8(tilesetValue, 0);
			const UINT16 usIndex = ToUInt16(usIndexValue, 0);
			profile->tileKeys.push_back(((UINT32)tileset << 16) | usIndex);
			return;
		}

		if (imageValue && *imageValue)
		{
			ContainerLootData::LootImageSelector selector;
			selector.image = NormalizeImageName(imageValue);
			if (frameValue && *frameValue)
			{
				selector.frame = ToUInt16(frameValue, 0);
				selector.useFrame = TRUE;
			}
			if (kindValue && *kindValue)
			{
				const UINT8 kind = KindFromXml(kindValue);
				if (kind != 0xFF)
				{
					selector.kind = kind;
					selector.useKind = TRUE;
				}
			}
			profile->images.push_back(selector);
		}
	}

	void ParseKind(ParseData* data, const XML_Char** atts)
	{
		ContainerLootData::LootProfile* profile = CurrentProfile(data);
		if (!profile)
			return;

		const UINT8 kind = KindFromXml(GetAttr(atts, "value"));
		if (kind != 0xFF)
			profile->kinds.push_back(kind);
	}

	void ParsePlacement(ParseData* data, const XML_Char** atts)
	{
		ContainerLootData::LootProfile* profile = CurrentProfile(data);
		if (!profile)
			return;

		ContainerLootData::LootPlacementDef placement;
		placement.type = PlacementFromXml(GetAttr(atts, "type"));
		placement.weight = ToUInt32(GetAttr(atts, "weight"), 1);

		if (placement.type != ContainerLootData::PLACEMENT_NONE && placement.weight > 0)
			profile->placements.push_back(placement);
	}

	void ParseSecurityProfile(ParseData* data, const XML_Char** atts)
	{
		if (!data->securityProfiles)
			return;

		ContainerLootData::ContainerSecurityProfile profile;
		profile.id = NormalizeToken(GetAttr(atts, "id") ? GetAttr(atts, "id") : "");
		profile.lockChanceMin = ToUInt8(GetAttr(atts, "lockChanceMin"), 20);
		profile.lockChanceMax = ToUInt8(GetAttr(atts, "lockChanceMax"), 30);
		profile.trapChanceMin = ToUInt8(GetAttr(atts, "trapChanceMin"), 30);
		profile.trapChanceMax = ToUInt8(GetAttr(atts, "trapChanceMax"), 40);
		profile.lockChanceCap = ToUInt8(GetAttr(atts, "lockChanceCap"), 60);
		profile.trapChanceCap = ToUInt8(GetAttr(atts, "trapChanceCap"), 80);
		profile.lockProgressWeight = (FLOAT)atof(GetAttr(atts, "lockProgressWeight") ? GetAttr(atts, "lockProgressWeight") : "0.1");
		profile.lockCoolnessWeight = (FLOAT)atof(GetAttr(atts, "lockCoolnessWeight") ? GetAttr(atts, "lockCoolnessWeight") : "0.1");
		profile.trapProgressWeight = (FLOAT)atof(GetAttr(atts, "trapProgressWeight") ? GetAttr(atts, "trapProgressWeight") : "0.1");
		profile.trapCoolnessWeight = (FLOAT)atof(GetAttr(atts, "trapCoolnessWeight") ? GetAttr(atts, "trapCoolnessWeight") : "0.1");
		profile.trapLevelMin = ToUInt8(GetAttr(atts, "trapLevelMin"), 1);
		profile.trapLevelMax = ToUInt8(GetAttr(atts, "trapLevelMax"), 4);
		profile.trapLevelProgressWeight = (FLOAT)atof(GetAttr(atts, "trapLevelProgressWeight") ? GetAttr(atts, "trapLevelProgressWeight") : "0.02");
		profile.trapLevelCoolnessWeight = (FLOAT)atof(GetAttr(atts, "trapLevelCoolnessWeight") ? GetAttr(atts, "trapLevelCoolnessWeight") : "1.0");
		profile.lockId = ToUInt8(GetAttr(atts, "lockId"), 0);

		if (profile.id.empty())
			return;

		data->securityProfiles->push_back(profile);
		data->securityProfileIndex = (INT32)data->securityProfiles->size() - 1;
	}

	void ParseTrapType(ParseData* data, const XML_Char** atts)
	{
		ContainerLootData::ContainerSecurityProfile* profile = CurrentSecurityProfile(data);
		if (!profile)
			return;

		ContainerLootData::TrapTypeDef trap;
		trap.id = TrapTypeFromXml(GetAttr(atts, "id"));
		trap.weight = ToUInt32(GetAttr(atts, "weight"), 1);

		if (trap.id != NO_TRAP && trap.id < NUM_DOOR_TRAPS && trap.weight > 0)
			profile->trapTypes.push_back(trap);
	}

	void ParseSectorSecurityTag(ParseData* data, const XML_Char** atts)
	{
		if (!data->sectorSecurity)
			return;

		ContainerLootData::SectorSecurityModifier modifier;
		modifier.tag = NormalizeToken(GetAttr(atts, "name") ? GetAttr(atts, "name") : "");
		modifier.lockBonus = ToInt(GetAttr(atts, "lockBonus"), 0);
		modifier.trapBonus = ToInt(GetAttr(atts, "trapBonus"), 0);
		modifier.trapLevelBonus = ToInt(GetAttr(atts, "trapLevelBonus"), 0);

		if (!modifier.tag.empty())
			data->sectorSecurity->push_back(modifier);
	}

	void PushLootItem(ParseData* data, const ContainerLootData::LootItemDef& item)
	{
		// travnik: disabled XML entries must not reach the Lua pool.
		if (!item.enabled || item.weight == 0)
			return;

		ContainerLootData::LootTypeDef* type = CurrentLootType(data);
		if (type)
		{
			type->items.push_back(item);
			return;
		}

		ContainerLootData::LootProfile* profile = CurrentProfile(data);
		if (profile)
			profile->items.push_back(item);
	}

	void NormalizeLootItem(ContainerLootData::LootItemDef& item)
	{
		if (item.minAmount < 1)
			item.minAmount = 1;
		if (item.maxAmount < item.minAmount)
			item.maxAmount = item.minAmount;

		if (item.minStatus < 1)
			item.minStatus = 1;
		if (item.minStatus > 100)
			item.minStatus = 100;
		if (item.maxStatus < item.minStatus)
			item.maxStatus = item.minStatus;
		if (item.maxStatus > 100)
			item.maxStatus = 100;

		if (item.minTier < 0)
			item.minTier = 0;
		if (item.minTier > ContainerLootData::MAX_LOOT_TIER)
			item.minTier = ContainerLootData::MAX_LOOT_TIER;
		if (item.maxTier < item.minTier)
			item.maxTier = item.minTier;
		if (item.maxTier > ContainerLootData::MAX_LOOT_TIER)
			item.maxTier = ContainerLootData::MAX_LOOT_TIER;
	}

	void ParseItem(ParseData* data, const XML_Char** atts)
	{
		ContainerLootData::LootItemDef item;
		item.enabled = ToBool(GetAttr(atts, "enabled"), TRUE);
		item.item = ToUInt16(GetAttr(atts, "id"), 0);
		item.lootTypeId = NormalizeToken(GetAttr(atts, "type") ? GetAttr(atts, "type") : "");
		item.weight = ToUInt32(GetAttr(atts, "weight"), 1);
		item.minAmount = ToUInt32(GetAttr(atts, "min"), 1);
		item.maxAmount = ToUInt32(GetAttr(atts, "max"), 1);
		item.minStatus = ToUInt8(GetAttr(atts, "statusMin"), 100);
		item.maxStatus = ToUInt8(GetAttr(atts, "statusMax"), 100);
		item.isMoney = ToBool(GetAttr(atts, "money"), FALSE);
		item.sectorTypes = SplitList(GetAttr(atts, "sectorTypes"));
		item.minTier = ToClampedInt(GetAttr(atts, "minTier"), 0, 0, ContainerLootData::MAX_LOOT_TIER);
		item.maxTier = ToClampedInt(GetAttr(atts, "maxTier"), ContainerLootData::MAX_LOOT_TIER,
			0, ContainerLootData::MAX_LOOT_TIER);
		NormalizeLootItem(item);

		PushLootItem(data, item);
	}

	void ParseMoney(ParseData* data, const XML_Char** atts)
	{
		ContainerLootData::LootItemDef item;
		item.enabled = ToBool(GetAttr(atts, "enabled"), TRUE);
		item.isMoney = TRUE;
		item.weight = ToUInt32(GetAttr(atts, "weight"), 1);
		item.minAmount = ToUInt32(GetAttr(atts, "min"), 1);
		item.maxAmount = ToUInt32(GetAttr(atts, "max"), 1);
		item.minStatus = 100;
		item.maxStatus = 100;
		item.sectorTypes = SplitList(GetAttr(atts, "sectorTypes"));
		item.minTier = ToClampedInt(GetAttr(atts, "minTier"), 0, 0, ContainerLootData::MAX_LOOT_TIER);
		item.maxTier = ToClampedInt(GetAttr(atts, "maxTier"), ContainerLootData::MAX_LOOT_TIER,
			0, ContainerLootData::MAX_LOOT_TIER);
		NormalizeLootItem(item);

		PushLootItem(data, item);
	}

	void ParseRandomItem(ParseData* data, const XML_Char** atts)
	{
		ContainerLootData::LootItemDef item;
		item.enabled = ToBool(GetAttr(atts, "enabled"), TRUE);
		item.isRandomItem = TRUE;
		item.randomItemClass = ToUInt16(GetAttr(atts, "class"), 0);
		item.weight = ToUInt32(GetAttr(atts, "weight"), 1);
		item.minAmount = ToUInt32(GetAttr(atts, "min"), 1);
		item.maxAmount = ToUInt32(GetAttr(atts, "max"), 1);
		item.minStatus = ToUInt8(GetAttr(atts, "statusMin"), 100);
		item.maxStatus = ToUInt8(GetAttr(atts, "statusMax"), 100);
		item.sectorTypes = SplitList(GetAttr(atts, "sectorTypes"));
		item.minTier = ToClampedInt(GetAttr(atts, "minTier"), 0, 0, ContainerLootData::MAX_LOOT_TIER);
		item.maxTier = ToClampedInt(GetAttr(atts, "maxTier"), ContainerLootData::MAX_LOOT_TIER,
			0, ContainerLootData::MAX_LOOT_TIER);
		NormalizeLootItem(item);

		PushLootItem(data, item);
	}

	void XMLCALL StartElement(void* userData, const XML_Char* name, const XML_Char** atts)
	{
		ParseData* data = (ParseData*)userData;

		if (strcmp(name, "ENVIRONMENT_LOOT") == 0)
		{
			if (data->state == XML_STATE_ROOT)
			{
				data->sawRoot = TRUE;
				data->state = XML_STATE_ROOT;
			}
			return;
		}

		if (strcmp(name, "LOOT_TYPES") == 0 && data->state == XML_STATE_ROOT)
		{
			data->state = XML_STATE_LOOT_TYPES;
			return;
		}

		if (strcmp(name, "LOOT_TYPE") == 0 && data->state == XML_STATE_LOOT_TYPES)
		{
			ContainerLootData::LootTypeDef type;
			type.id = NormalizeToken(GetAttr(atts, "id") ? GetAttr(atts, "id") : "");
			type.sectorTypes = SplitList(GetAttr(atts, "sectorTypes"));
			type.minTier = ToInt(GetAttr(atts, "minTier"), 0);
			type.maxTier = ToInt(GetAttr(atts, "maxTier"), ContainerLootData::MAX_LOOT_TIER);

			data->lootTypeIndex = -1;
			if (!type.id.empty())
			{
				data->lootTypes->push_back(type);
				data->lootTypeIndex = (INT32)data->lootTypes->size() - 1;
			}

			data->inLootType = TRUE;
			data->state = XML_STATE_LOOT_TYPE;
			return;
		}

		if (strcmp(name, "CONTAINER_SECURITY") == 0 && data->state == XML_STATE_ROOT)
		{
			data->state = XML_STATE_SECURITY_PROFILES;
			return;
		}

		if (strcmp(name, "SECURITY_PROFILE") == 0 && data->state == XML_STATE_SECURITY_PROFILES)
		{
			ParseSecurityProfile(data, atts);
			data->state = XML_STATE_SECURITY_PROFILE;
			return;
		}

		if (strcmp(name, "TRAP_TYPES") == 0 && data->state == XML_STATE_SECURITY_PROFILE)
		{
			data->state = XML_STATE_TRAP_TYPES;
			return;
		}

		if (strcmp(name, "TRAP_TYPE") == 0 && data->state == XML_STATE_TRAP_TYPES)
		{
			ParseTrapType(data, atts);
			data->state = XML_STATE_TRAP_TYPE;
			return;
		}

		if (strcmp(name, "SECTOR_SECURITY") == 0 && data->state == XML_STATE_ROOT)
		{
			data->state = XML_STATE_SECTOR_SECURITY;
			return;
		}

		if (strcmp(name, "TAG") == 0 && data->state == XML_STATE_SECTOR_SECURITY)
		{
			ParseSectorSecurityTag(data, atts);
			data->state = XML_STATE_SECTOR_TAG;
			return;
		}

		if (strcmp(name, "LOOT_PROFILE") == 0 && data->state == XML_STATE_ROOT)
		{
			ContainerLootData::LootProfile profile;
			profile.enabled = ToBool(GetAttr(atts, "enabled"), TRUE);
			// travnik: profile ids and fallbackProfile share the same normalized key.
			profile.id = NormalizeToken(GetAttr(atts, "id") ? GetAttr(atts, "id") : "");
			profile.name = GetAttr(atts, "name") ? GetAttr(atts, "name") : profile.id;
			profile.priority = ToInt(GetAttr(atts, "priority"), ContainerLootData::DEFAULT_PROFILE_PRIORITY);
			profile.maxSites = ToInt(GetAttr(atts, "maxSites"), ContainerLootData::DEFAULT_PROFILE_MAX_SITES);
			profile.sectorTypes = SplitList(GetAttr(atts, "sectorTypes"));
			profile.fallbackProfile = NormalizeToken(GetAttr(atts, "fallbackProfile") ? GetAttr(atts, "fallbackProfile") : "");
			profile.securityProfile = NormalizeToken(GetAttr(atts, "security") ? GetAttr(atts, "security") : "");
			profile.minTier = ToInt(GetAttr(atts, "minTier"), 0);
			profile.maxTier = ToInt(GetAttr(atts, "maxTier"), ContainerLootData::MAX_LOOT_TIER);
			profile.coolnessBonus = ToInt(GetAttr(atts, "coolnessBonus"), 0);
			if (profile.maxSites < 0)
				profile.maxSites = 0;

			data->profiles->push_back(profile);
			data->profileIndex = (INT32)data->profiles->size() - 1;
			data->state = XML_STATE_PROFILE;
			return;
		}

		if (strcmp(name, "TILES") == 0 && data->state == XML_STATE_PROFILE)
		{
			data->state = XML_STATE_TILES;
			return;
		}

		if (strcmp(name, "TILE") == 0 && data->state == XML_STATE_TILES)
		{
			ParseTile(data, atts);
			data->state = XML_STATE_TILE;
			return;
		}

		if (strcmp(name, "KINDS") == 0 && data->state == XML_STATE_PROFILE)
		{
			data->state = XML_STATE_KINDS;
			return;
		}

		if (strcmp(name, "KIND") == 0 && data->state == XML_STATE_KINDS)
		{
			ParseKind(data, atts);
			data->state = XML_STATE_KIND;
			return;
		}

		if (strcmp(name, "PLACEMENTS") == 0 && data->state == XML_STATE_PROFILE)
		{
			data->state = XML_STATE_PLACEMENTS;
			return;
		}

		if (strcmp(name, "PLACEMENT") == 0 && data->state == XML_STATE_PLACEMENTS)
		{
			ParsePlacement(data, atts);
			data->state = XML_STATE_PLACEMENT;
			return;
		}

		if (strcmp(name, "ITEM_POOL") == 0 && data->state == XML_STATE_PROFILE)
		{
			data->state = XML_STATE_ITEM_POOL;
			return;
		}

		if (strcmp(name, "ITEM") == 0 &&
			(data->state == XML_STATE_ITEM_POOL || data->state == XML_STATE_LOOT_TYPE))
		{
			ParseItem(data, atts);
			data->state = XML_STATE_ITEM;
			return;
		}

		if (strcmp(name, "MONEY") == 0 &&
			(data->state == XML_STATE_ITEM_POOL || data->state == XML_STATE_LOOT_TYPE))
		{
			ParseMoney(data, atts);
			data->state = XML_STATE_MONEY;
			return;
		}

		if (strcmp(name, "TYPE") == 0 &&
			(data->state == XML_STATE_ITEM_POOL || data->state == XML_STATE_LOOT_TYPE))
		{
			ContainerLootData::LootItemDef item;
			item.enabled = ToBool(GetAttr(atts, "enabled"), TRUE);
			item.lootTypeId = NormalizeToken(GetAttr(atts, "id") ? GetAttr(atts, "id") : "");
			item.weight = ToUInt32(GetAttr(atts, "weight"), 1);
			item.minAmount = ToUInt32(GetAttr(atts, "min"), 1);
			item.maxAmount = ToUInt32(GetAttr(atts, "max"), 1);
			item.minStatus = ToUInt8(GetAttr(atts, "statusMin"), 100);
			item.maxStatus = ToUInt8(GetAttr(atts, "statusMax"), 100);
			item.sectorTypes = SplitList(GetAttr(atts, "sectorTypes"));
			item.minTier = ToClampedInt(GetAttr(atts, "minTier"), 0, 0, ContainerLootData::MAX_LOOT_TIER);
			item.maxTier = ToClampedInt(GetAttr(atts, "maxTier"), ContainerLootData::MAX_LOOT_TIER,
				0, ContainerLootData::MAX_LOOT_TIER);
			NormalizeLootItem(item);
			PushLootItem(data, item);
			data->state = XML_STATE_ITEM;
			return;
		}

		if (strcmp(name, "RANDOM_ITEM") == 0 &&
			(data->state == XML_STATE_ITEM_POOL || data->state == XML_STATE_LOOT_TYPE))
		{
			ParseRandomItem(data, atts);
			data->state = XML_STATE_ITEM;
			return;
		}

		if (strcmp(name, "EMPTY_CHANCE") == 0 && data->state == XML_STATE_PROFILE)
		{
			data->charData.clear();
			data->state = XML_STATE_EMPTY_CHANCE;
			return;
		}

		if (strcmp(name, "ROLLS") == 0 && data->state == XML_STATE_PROFILE)
		{
			ContainerLootData::LootProfile* profile = CurrentProfile(data);
			if (profile)
			{
				profile->minRolls = ToUInt8(GetAttr(atts, "min"), 1);
				profile->maxRolls = ToUInt8(GetAttr(atts, "max"), 1);
			}
			return;
		}
	}

	void XMLCALL EndElement(void* userData, const XML_Char* name)
	{
		ParseData* data = (ParseData*)userData;

		if (strcmp(name, "TILE") == 0)
		{
			data->state = XML_STATE_TILES;
			return;
		}

		if (strcmp(name, "TILES") == 0)
		{
			data->state = XML_STATE_PROFILE;
			return;
		}

		if (strcmp(name, "KIND") == 0)
		{
			data->state = XML_STATE_KINDS;
			return;
		}

		if (strcmp(name, "KINDS") == 0)
		{
			data->state = XML_STATE_PROFILE;
			return;
		}

		if (strcmp(name, "PLACEMENT") == 0)
		{
			data->state = XML_STATE_PLACEMENTS;
			return;
		}

		if (strcmp(name, "PLACEMENTS") == 0)
		{
			data->state = XML_STATE_PROFILE;
			return;
		}

		if (strcmp(name, "TRAP_TYPE") == 0)
		{
			data->state = XML_STATE_TRAP_TYPES;
			return;
		}

		if (strcmp(name, "TRAP_TYPES") == 0)
		{
			data->state = XML_STATE_SECURITY_PROFILE;
			return;
		}

		if (strcmp(name, "SECURITY_PROFILE") == 0)
		{
			ContainerLootData::ContainerSecurityProfile* profile = CurrentSecurityProfile(data);
			if (profile)
			{
				if (profile->lockChanceMin > 100) profile->lockChanceMin = 100;
				if (profile->lockChanceMax > 100) profile->lockChanceMax = 100;
				if (profile->lockChanceMax < profile->lockChanceMin) profile->lockChanceMax = profile->lockChanceMin;
				if (profile->trapChanceMin > 100) profile->trapChanceMin = 100;
				if (profile->trapChanceMax > 100) profile->trapChanceMax = 100;
				if (profile->trapChanceMax < profile->trapChanceMin) profile->trapChanceMax = profile->trapChanceMin;
				if (profile->lockChanceCap > 100) profile->lockChanceCap = 100;
				if (profile->trapChanceCap > 100) profile->trapChanceCap = 100;
				if (profile->trapLevelMin < 1) profile->trapLevelMin = 1;
				if (profile->trapLevelMax < profile->trapLevelMin) profile->trapLevelMax = profile->trapLevelMin;
				if (profile->trapLevelMax > ContainerLootData::MAX_TRAP_LEVEL) profile->trapLevelMax = ContainerLootData::MAX_TRAP_LEVEL;
				if (profile->lockId > ContainerLootData::MAX_LOCK_ID) profile->lockId = 0;

				if (profile->trapTypes.empty())
				{
					ContainerLootData::TrapTypeDef fallbackTrap;
					fallbackTrap.id = EXPLOSION;
					fallbackTrap.weight = 1;
					profile->trapTypes.push_back(fallbackTrap);
				}
			}

			data->securityProfileIndex = -1;
			data->state = XML_STATE_SECURITY_PROFILES;
			return;
		}

		if (strcmp(name, "CONTAINER_SECURITY") == 0)
		{
			data->state = XML_STATE_ROOT;
			return;
		}

		if (strcmp(name, "TAG") == 0)
		{
			data->state = XML_STATE_SECTOR_SECURITY;
			return;
		}

		if (strcmp(name, "SECTOR_SECURITY") == 0)
		{
			data->state = XML_STATE_ROOT;
			return;
		}

		if (strcmp(name, "ITEM") == 0 || strcmp(name, "MONEY") == 0 ||
			strcmp(name, "TYPE") == 0 || strcmp(name, "RANDOM_ITEM") == 0)
		{
			data->state = data->inLootType ? XML_STATE_LOOT_TYPE : XML_STATE_ITEM_POOL;
			return;
		}

		if (strcmp(name, "ITEM_POOL") == 0)
		{
			data->state = XML_STATE_PROFILE;
			return;
		}

		if (strcmp(name, "LOOT_TYPE") == 0)
		{
			ContainerLootData::LootTypeDef* type = CurrentLootType(data);
			if (type)
			{
				if (type->minTier < 0)
					type->minTier = 0;
				if (type->maxTier < type->minTier)
					type->maxTier = type->minTier;
				if (type->maxTier > ContainerLootData::MAX_LOOT_TIER)
					type->maxTier = ContainerLootData::MAX_LOOT_TIER;
			}

			if (type && type->items.empty() && data->lootTypes && !data->lootTypes->empty())
				data->lootTypes->pop_back();

			data->lootTypeIndex = -1;
			data->inLootType = FALSE;
			data->state = XML_STATE_LOOT_TYPES;
			return;
		}

		if (strcmp(name, "LOOT_TYPES") == 0)
		{
			data->state = XML_STATE_ROOT;
			return;
		}

		if (strcmp(name, "EMPTY_CHANCE") == 0)
		{
			ContainerLootData::LootProfile* profile = CurrentProfile(data);
			if (profile)
				profile->emptyChance = ToUInt8(data->charData.c_str(), 0);

			data->state = XML_STATE_PROFILE;
			return;
		}

		if (strcmp(name, "LOOT_PROFILE") == 0)
		{
			ContainerLootData::LootProfile* profile = CurrentProfile(data);

			// travnik: пустые/некорректные профили не оставляем в таблице.
			if (!profile || profile->id.empty() || profile->items.empty())
			{
				if (profile && !data->profiles->empty())
					data->profiles->pop_back();
			}
			else
			{
				if (profile->emptyChance > 100)
					profile->emptyChance = 100;

				if (profile->minRolls < 1)
					profile->minRolls = 1;
				if (profile->maxRolls < profile->minRolls)
					profile->maxRolls = profile->minRolls;
				if (profile->maxRolls > ContainerLootData::MAX_ITEMS_PER_SITE)
					profile->maxRolls = (UINT8)ContainerLootData::MAX_ITEMS_PER_SITE;

				if (profile->minTier < 0)
					profile->minTier = 0;
				if (profile->maxTier < profile->minTier)
					profile->maxTier = profile->minTier;
				if (profile->maxTier > ContainerLootData::MAX_LOOT_TIER)
					profile->maxTier = ContainerLootData::MAX_LOOT_TIER;
				if (profile->coolnessBonus < -ContainerLootData::MAX_ITEM_COOLNESS)
					profile->coolnessBonus = -ContainerLootData::MAX_ITEM_COOLNESS;
				if (profile->coolnessBonus > ContainerLootData::MAX_ITEM_COOLNESS)
					profile->coolnessBonus = ContainerLootData::MAX_ITEM_COOLNESS;
			}

			data->profileIndex = -1;
			data->state = XML_STATE_ROOT;
			return;
		}
	}

	void XMLCALL CharacterData(void* userData, const XML_Char* str, int len)
	{
		ParseData* data = (ParseData*)userData;
		if (data->state == XML_STATE_EMPTY_CHANCE && data->charData.size() < 32)
			data->charData.append(str, len);
	}

	BOOLEAN LoadFile(std::vector<ContainerLootData::LootProfile>& profiles,
		std::vector<ContainerLootData::LootTypeDef>& lootTypes,
		std::vector<ContainerLootData::ContainerSecurityProfile>& securityProfiles,
		std::vector<ContainerLootData::SectorSecurityModifier>& sectorSecurity)
	{
		profiles.clear();
		lootTypes.clear();
		securityProfiles.clear();
		sectorSecurity.clear();

		HWFILE hFile = FileOpen((STR)ENVIRONMENT_LOOT_FILE, FILE_ACCESS_READ, FALSE);
		if (!hFile)
		{
			ContainerLoot::Trace("ContainerLootXml: cannot open TableData\\Loot\\EnvironmentLoot.xml");
			return FALSE;
		}

		const UINT32 size = FileGetSize(hFile);
		CHAR8* buffer = (CHAR8*)MemAlloc(size + 1);
		if (!buffer)
		{
			FileClose(hFile);
			return FALSE;
		}

		UINT32 bytesRead = 0;
		if (!FileRead(hFile, buffer, size, &bytesRead))
		{
			MemFree(buffer);
			FileClose(hFile);
			return FALSE;
		}

		buffer[size] = 0;
		FileClose(hFile);

		XML_Parser parser = XML_ParserCreate(NULL);
		if (!parser)
		{
			MemFree(buffer);
			return FALSE;
		}

		ParseData data;
		data.profiles = &profiles;
		data.lootTypes = &lootTypes;
		data.securityProfiles = &securityProfiles;
		data.sectorSecurity = &sectorSecurity;

		XML_SetUserData(parser, &data);
		XML_SetElementHandler(parser, StartElement, EndElement);
		XML_SetCharacterDataHandler(parser, CharacterData);

		if (!XML_Parse(parser, buffer, size, TRUE))
		{
			CHAR8 message[512];
			sprintf_s(message, "ContainerLootXml: parse error in EnvironmentLoot.xml: %s at line %d",
				XML_ErrorString(XML_GetErrorCode(parser)),
				XML_GetCurrentLineNumber(parser));
			ContainerLoot::Trace(message);

			XML_ParserFree(parser);
			MemFree(buffer);
			profiles.clear();
			lootTypes.clear();
			securityProfiles.clear();
			sectorSecurity.clear();
			return FALSE;
		}

		XML_ParserFree(parser);
		MemFree(buffer);

		// travnik: a typo in security="..." must be visible in the debug log.
		for (size_t profileIndex = 0; profileIndex < profiles.size(); ++profileIndex)
		{
			const std::string& securityId = profiles[profileIndex].securityProfile;
			if (securityId.empty() || securityId == "none")
				continue;

			BOOLEAN found = FALSE;
			for (size_t securityIndex = 0; securityIndex < securityProfiles.size(); ++securityIndex)
			{
				if (securityProfiles[securityIndex].id == securityId)
				{
					found = TRUE;
					break;
				}
			}

			if (!found)
			{
				CHAR8 message[320];
				sprintf_s(message, "ContainerLootSecurity: unknown security profile '%s' in loot profile '%s'",
					securityId.c_str(), profiles[profileIndex].id.c_str());
				ContainerLoot::Trace(message);
			}
		}

		return data.sawRoot ? TRUE : FALSE;
	}

	bool HasTileKey(const ContainerLootData::LootProfile& profile, UINT32 key)
	{
		for (size_t i = 0; i < profile.tileKeys.size(); ++i)
		{
			if (profile.tileKeys[i] == key)
				return true;
		}

		return false;
	}
}

namespace
{
	const ContainerLootData::LootProfile* FindProfileById(
		const std::vector<ContainerLootData::LootProfile>& profiles,
		const std::string& id)
	{
		if (id.empty())
			return NULL;

		for (size_t i = 0; i < profiles.size(); ++i)
		{
			// travnik: disabled profiles must not be reachable through fallbackProfile.
			if (profiles[i].enabled && profiles[i].id == id)
				return &profiles[i];
		}

		return NULL;
	}

	INT32 SectorTokenSpecificity(const std::string& token)
	{
		if (token == "military" || token == "sam" || token == "prison")
			return PROFILE_STRONG_TAG_BONUS;

		if (token == "palace" || token == "airport" || token == "hospital" ||
			token == "industrial" || token == "mine" || token == "underground" ||
			token == "lawless")
			return PROFILE_MEDIUM_TAG_BONUS;

		if (token == "wealthy" || token == "traffic" || token == "farm" ||
			token == "agricultural" || token == "rural")
			return PROFILE_WEAK_TAG_BONUS;

		return 0;
	}

	INT32 ProfileSectorSpecificity(const ContainerLootData::LootProfile& profile,
		const std::string* sectorType,
		const std::vector<std::string>* sectorTags)
	{
		if (!sectorType || !sectorTags)
			return 0;

		INT32 score = 0;
		if (ContainsToken(profile.sectorTypes, *sectorType))
			score += PROFILE_EXACT_SECTOR_BONUS;

		for (size_t i = 0; i < sectorTags->size(); ++i)
		{
			if (ContainsToken(profile.sectorTypes, (*sectorTags)[i]))
				score += SectorTokenSpecificity((*sectorTags)[i]);
		}

		return score;
	}

	void ConsiderProfile(const ContainerLootData::LootProfile* allowed,
		const std::string* sectorType,
		const std::vector<std::string>* sectorTags,
		const ContainerLootData::LootProfile** best,
		INT32* bestScore)
	{
		if (!allowed)
			return;

		const INT32 score = ProfileSectorSpecificity(*allowed, sectorType, sectorTags);
		if (score > *bestScore)
		{
			*best = allowed;
			*bestScore = score;
		}
	}

	const ContainerLootData::LootProfile* FindAllowedProfile(
		const std::vector<ContainerLootData::LootProfile>& profiles,
		const ContainerLootData::LootProfile* profile,
		const std::string* sectorType,
		const std::vector<std::string>* sectorTags,
		INT32 depth)
	{
		if (!profile || !profile->enabled || depth > 8)
			return NULL;

		// travnik: старый FindProfile без контекста не фильтрует сектор
		if (!sectorType || !sectorTags)
			return profile;

		if (SectorListAllows(profile->sectorTypes, *sectorType, *sectorTags))
			return profile;

		if (profile->fallbackProfile.empty())
			return NULL;

		const ContainerLootData::LootProfile* fallback =
			FindProfileById(profiles, profile->fallbackProfile);

		if (!fallback || fallback == profile)
			return NULL;

		return FindAllowedProfile(profiles, fallback, sectorType, sectorTags, depth + 1);
	}

	const ContainerLootData::LootProfile* FindProfileCore(
		const std::vector<ContainerLootData::LootProfile>& profiles,
		UINT8 tileset,
		UINT16 usIndex,
		const std::string& image,
		UINT16 frame,
		UINT8 kind,
		const std::string* sectorType,
		const std::vector<std::string>* sectorTags)
	{
		const UINT32 key = ((UINT32)tileset << 16) | usIndex;

		const ContainerLootData::LootProfile* best = NULL;
		INT32 bestScore = -1;

		for (size_t i = 0; i < profiles.size(); ++i)
		{
			if (!HasTileKey(profiles[i], key))
				continue;

			const ContainerLootData::LootProfile* allowed =
				FindAllowedProfile(profiles, &profiles[i], sectorType, sectorTags, 0);
			ConsiderProfile(allowed, sectorType, sectorTags, &best, &bestScore);
		}

		if (best)
			return best;

		best = NULL;
		bestScore = -1;

		for (size_t i = 0; i < profiles.size(); ++i)
		{
			const ContainerLootData::LootProfile& profile = profiles[i];

			for (size_t j = 0; j < profile.images.size(); ++j)
			{
				const ContainerLootData::LootImageSelector& selector = profile.images[j];

				if (!ImageEquals(selector.image, image))
					continue;

				if (selector.useFrame && selector.frame != frame)
					continue;

				if (selector.useKind && selector.kind != kind)
					continue;

				const ContainerLootData::LootProfile* allowed =
					FindAllowedProfile(profiles, &profile, sectorType, sectorTags, 0);
				ConsiderProfile(allowed, sectorType, sectorTags, &best, &bestScore);
			}
		}

		if (best)
			return best;

		best = NULL;
		bestScore = -1;

		for (size_t i = 0; i < profiles.size(); ++i)
		{
			const ContainerLootData::LootProfile& profile = profiles[i];

			for (size_t j = 0; j < profile.kinds.size(); ++j)
			{
				if (profile.kinds[j] != kind)
					continue;

				const ContainerLootData::LootProfile* allowed =
					FindAllowedProfile(profiles, &profile, sectorType, sectorTags, 0);
				ConsiderProfile(allowed, sectorType, sectorTags, &best, &bestScore);
			}
		}

		return best;
	}
}

namespace ContainerLootXml
{
	BOOLEAN Reload(std::vector<ContainerLootData::LootProfile>& profiles,
		std::vector<ContainerLootData::LootTypeDef>& lootTypes,
		std::vector<ContainerLootData::ContainerSecurityProfile>& securityProfiles,
		std::vector<ContainerLootData::SectorSecurityModifier>& sectorSecurity)
	{
		return EnsureLoaded(profiles, lootTypes, securityProfiles, sectorSecurity, TRUE);
	}

	BOOLEAN EnsureLoaded(std::vector<ContainerLootData::LootProfile>& profiles,
		std::vector<ContainerLootData::LootTypeDef>& lootTypes,
		std::vector<ContainerLootData::ContainerSecurityProfile>& securityProfiles,
		std::vector<ContainerLootData::SectorSecurityModifier>& sectorSecurity,
		BOOLEAN forceReload)
	{
		static BOOLEAN loaded = FALSE;
		static std::vector<ContainerLootData::LootProfile> cachedProfiles;
		static std::vector<ContainerLootData::LootTypeDef> cachedTypes;
		static std::vector<ContainerLootData::ContainerSecurityProfile> cachedSecurity;
		static std::vector<ContainerLootData::SectorSecurityModifier> cachedSectorSecurity;

		if (!loaded || forceReload)
		{
			if (!LoadFile(cachedProfiles, cachedTypes, cachedSecurity, cachedSectorSecurity))
			{
				cachedProfiles.clear();
				cachedTypes.clear();
				cachedSecurity.clear();
				cachedSectorSecurity.clear();
				loaded = FALSE;
				// travnik: keep the cache retryable if the file was temporarily unavailable.
				ContainerLoot::Trace("ContainerLootXml: EnvironmentLoot.xml is not loaded");
			}
			else
			{
				loaded = TRUE;
				INT32 disabledProfiles = 0;
				for (std::vector<ContainerLootData::LootProfile>::const_iterator it = cachedProfiles.begin();
					it != cachedProfiles.end(); ++it)
				{
					if (!it->enabled)
						++disabledProfiles;
				}

				CHAR8 message[192];
				sprintf_s(message, "ContainerLootXml: loaded profiles=%d disabled=%d lootTypes=%d security=%d sectorTags=%d",
					(INT32)cachedProfiles.size(), disabledProfiles, (INT32)cachedTypes.size(),
					(INT32)cachedSecurity.size(), (INT32)cachedSectorSecurity.size());
				ContainerLoot::Trace(message);
			}
		}

		profiles = cachedProfiles;
		lootTypes = cachedTypes;
		securityProfiles = cachedSecurity;
		sectorSecurity = cachedSectorSecurity;
		return cachedProfiles.empty() ? FALSE : TRUE;
	}

	const ContainerLootData::LootProfile* FindProfile(
		const std::vector<ContainerLootData::LootProfile>& profiles,
		UINT8 tileset,
		UINT16 usIndex,
		const std::string& image,
		UINT16 frame,
		UINT8 kind)
	{
		return FindProfileCore(profiles, tileset, usIndex, image, frame, kind, NULL, NULL);
	}

	const ContainerLootData::LootProfile* FindProfileForSector(
		const std::vector<ContainerLootData::LootProfile>& profiles,
		UINT8 tileset,
		UINT16 usIndex,
		const std::string& image,
		UINT16 frame,
		UINT8 kind,
		const std::string& sectorType,
		const std::vector<std::string>& sectorTags)
	{
		return FindProfileCore(profiles, tileset, usIndex, image, frame, kind,
			&sectorType, &sectorTags);
	}
}
