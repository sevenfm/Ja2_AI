-- travnik: ContainerLoot dynamic stage.
-- Profiles, tile selectors, item pools and chances are loaded from
-- TableData\Loot\EnvironmentLoot.xml. There is no Lua profile fallback:
-- a site without an XML profile is ignored.

-- travnik: Temporary debug filter. Set to false to enable all XML profiles.
local DEBUG_FILTER_ENABLED = false
local DEBUG_ONLY_SUBTYPES = {
    cash_register = true,
}

local MAX_LOOT_TIER = 10
local MAX_ITEM_COOLNESS = 10
local MAX_ROLLS = 6

local function clamp(value, minimum, maximum)
    if value < minimum then
        return minimum
    end
    if value > maximum then
        return maximum
    end
    return value
end

local function round(value)
    return math.floor(value + 0.5)
end

local function pickWeighted(list)
    local total = 0
    for _, entry in ipairs(list) do
        total = total + (entry.weight or 1)
    end

    if total <= 0 then
        return nil
    end

    local roll = math.random() * total
    for _, entry in ipairs(list) do
        roll = roll - (entry.weight or 1)
        if roll <= 0 then
            return entry
        end
    end

    return list[#list]
end

local function containsToken(tokens, value)
    if not tokens then
        return false
    end
    for _, token in ipairs(tokens) do
        if token == value then
            return true
        end
    end
    return false
end

local function isSectorAllowed(sectorTypes, ctx)
    if not sectorTypes or #sectorTypes == 0 then
        return true
    end

    if ctx.sectorType and containsToken(sectorTypes, ctx.sectorType) then
        return true
    end

    for _, tag in ipairs(ctx.sectorTags or {}) do
        if containsToken(sectorTypes, tag) then
            return true
        end
    end

    return false
end

local function sumSecurityTagBonus(ctx, field)
    local total = 0
    for _, modifier in ipairs(ctx.sectorSecurity or {}) do
        for _, tag in ipairs(ctx.sectorTags or {}) do
            if modifier.tag == tag then
                total = total + (modifier[field] or 0)
            end
        end
    end
    return total
end

local function chooseTrapType(security)
    local trap = pickWeighted(security.trapTypes or {})
    if trap then
        return trap.id or 1
    end
    return 1
end

local function buildSecurity(site, profile, ctx)
    local result = {
        hasLock = false,
        lockId = 0,
        hasTrap = false,
        trapId = 0,
        trapLevel = 0,
    }

    if not site or site.kind ~= "container" or not site.canOpen or site.isOpen then
        return result
    end

    local profiles = ctx.securityProfiles or {}
    local security = profiles[profile.security or "default"] or profiles["default"]
    if not security then
        return result
    end

    local progress = ctx.progress or 0
    local coolness = ctx.coolnessNorm or 0.0
    local lockMin = math.max(0, security.lockChanceMin or 0)
    local lockMax = math.max(lockMin, security.lockChanceMax or lockMin)
    local lockChance = math.random(lockMin, lockMax)
    lockChance = lockChance +
        progress * (security.lockProgressWeight or 0.0) +
        coolness * (security.lockCoolnessWeight or 0.0) +
        sumSecurityTagBonus(ctx, "lockBonus")
    lockChance = clamp(lockChance, 0, security.lockChanceCap or 100)

    if math.random(100) > lockChance then
        return result
    end

    result.hasLock = true
    result.lockId = security.lockId or 0

    local trapMin = math.max(0, security.trapChanceMin or 0)
    local trapMax = math.max(trapMin, security.trapChanceMax or trapMin)
    local trapChance = math.random(trapMin, trapMax)
    trapChance = trapChance +
        progress * (security.trapProgressWeight or 0.0) +
        coolness * (security.trapCoolnessWeight or 0.0) +
        sumSecurityTagBonus(ctx, "trapBonus")
    trapChance = clamp(trapChance, 0, security.trapChanceCap or 100)

    if math.random(100) > trapChance then
        return result
    end

    result.hasTrap = true
    result.trapId = chooseTrapType(security)

    local levelMin = math.max(1, security.trapLevelMin or 1)
    local levelMax = math.max(levelMin, security.trapLevelMax or levelMin)
    local level = math.random(levelMin, levelMax)
    level = level + math.floor(progress * (security.trapLevelProgressWeight or 0.0))
    level = level + math.floor(coolness * (security.trapLevelCoolnessWeight or 0.0))
    level = level + sumSecurityTagBonus(ctx, "trapLevelBonus")
    result.trapLevel = clamp(level, 1, 10)

    return result
end

local function computeTier(ctx)
    local progressTier = ((ctx.progress or 0) / 10) * (ctx.progressWeight or 1)
    local sectorTier = ((ctx.sectorCoolness or 0) / 2) * (ctx.sectorCoolnessWeight or 1)
    local tier = math.max(progressTier, sectorTier)

    tier = math.min(tier, progressTier + (ctx.progressTierCapBonus or 3))

    local typeBonus = ctx.militaryTypeBonus or 1
    if containsToken(ctx.sectorTags, "military") or
       containsToken(ctx.sectorTags, "sam") or
       containsToken(ctx.sectorTags, "prison") then
        tier = tier + typeBonus
    elseif containsToken(ctx.sectorTags, "palace") or
           containsToken(ctx.sectorTags, "airport") then
        tier = tier + typeBonus * 0.5
    elseif containsToken(ctx.sectorTags, "hospital") or
           containsToken(ctx.sectorTags, "industrial") then
        tier = tier + typeBonus * 0.25
    end

    tier = tier + (ctx.difficulty or 0) * (ctx.difficultyBonus or 0.5)
    return clamp(tier, 0, MAX_LOOT_TIER)
end

local function computeMaxCoolness(tier, profile, ctx)
    return clamp(round(tier + (ctx.maxItemCoolnessBonus or 2) + (profile.coolnessBonus or 0)),
        0, MAX_ITEM_COOLNESS)
end

local function profileTierAllowed(profile, tier)
    if not profile then
        return false
    end

    local minTier = profile.minTier or 0
    local maxTier = profile.maxTier or MAX_LOOT_TIER
    return tier >= minTier and tier <= maxTier
end

local function choosePlacement(site, profile)
    local placements = profile.placements or {}

    local total = 0
    for _, placement in ipairs(placements) do
        local allowed = false
        if placement.type == "open" then
            allowed = site.canOpen and not site.isOpen
        elseif placement.type == "hidden" then
            allowed = site.canHide
        elseif placement.type == "surface" then
            allowed = site.canSurface
        elseif placement.type == "corpse" then
            allowed = site.canCorpse
        end

        if allowed then
            total = total + (placement.weight or 1)
        end
    end

    if total <= 0 then
        return nil
    end

    local roll = math.random() * total
    for _, placement in ipairs(placements) do
        local allowed = false
        if placement.type == "open" then
            allowed = site.canOpen and not site.isOpen
        elseif placement.type == "hidden" then
            allowed = site.canHide
        elseif placement.type == "surface" then
            allowed = site.canSurface
        elseif placement.type == "corpse" then
            allowed = site.canCorpse
        end

        if allowed then
            roll = roll - (placement.weight or 1)
            if roll <= 0 then
                return placement.type
            end
        end
    end

    return nil
end

local function entryTierAllowed(entry, tier, minTier, maxTier)
    local entryMin = math.max(minTier or 0, entry.minTier or 0)
    local entryMax = math.min(maxTier or MAX_LOOT_TIER, entry.maxTier or MAX_LOOT_TIER)
    return tier >= entryMin and tier <= entryMax
end

local function entryAllowed(entry, ctx, tier, maxCoolness, minTier, maxTier, placement)
    if entry.enabled == false then
        return false
    end

    if not isSectorAllowed(entry.sectorTypes, ctx) then
        return false
    end

    if not entryTierAllowed(entry, tier, minTier, maxTier) then
        return false
    end

    if entry.money then
        return true
    end

    if entry.randomItem then
        return (entry.randomItemClass or 0) > 0
    end

    if (entry.item or 0) <= 0 then
        return false
    end

    if (entry.coolness or 0) > maxCoolness then
        return false
    end

    return true
end

local function buildPool(profile, ctx, tier, maxCoolness, placement, lootTypes)
    local pool = {}

    for _, entry in ipairs(profile.items or {}) do
        if entry.enabled ~= false then
            if entry.lootType and entry.lootType ~= "" then
                local lootType = lootTypes[entry.lootType]
                if lootType and isSectorAllowed(entry.sectorTypes, ctx) and
                   isSectorAllowed(lootType.sectorTypes, ctx) then
                    for _, typeItem in ipairs(lootType.items or {}) do
                        if entryAllowed(typeItem, ctx, tier, maxCoolness,
                            math.max(lootType.minTier or 0, entry.minTier or 0),
                            math.min(lootType.maxTier or MAX_LOOT_TIER, entry.maxTier or MAX_LOOT_TIER),
                            placement) then
                            local clone = {}
                            for key, value in pairs(typeItem) do
                                clone[key] = value
                            end
                            clone.weight = (entry.weight or 1) * (typeItem.weight or 1)
                            pool[#pool + 1] = clone
                        end
                    end
                end
            elseif entryAllowed(entry, ctx, tier, maxCoolness, 0, MAX_LOOT_TIER, placement) then
                pool[#pool + 1] = entry
            end
        end
    end

    return pool
end

local function makeLootForProfile(profile, ctx, tier, placement, lootTypes, profiles, depth)
    if not profile or depth > 4 then
        return {}, 0, profile
    end

    local maxCoolness = computeMaxCoolness(tier, profile, ctx)
    local pool = buildPool(profile, ctx, tier, maxCoolness, placement, lootTypes)

    if #pool == 0 and profile.fallbackProfile and profile.fallbackProfile ~= "" then
        local fallback = profiles[profile.fallbackProfile]
        if fallback and fallback.id ~= profile.id and
           profileTierAllowed(fallback, tier) and isSectorAllowed(fallback.sectorTypes, ctx) then
            local items, money, usedProfile = makeLootForProfile(fallback, ctx, tier, placement,
                lootTypes, profiles, depth + 1)
            if #items > 0 or money > 0 then
                return items, money, usedProfile
            end
        end
    end

    local items = {}
    local money = 0

    local progressLoss = math.max(0, (ctx.progress or 0) - (ctx.currentProgress or ctx.progress or 0))
    local emptyChance = (profile.emptyChance or 0) * (ctx.emptyChanceScale or 1) *
        (1 + progressLoss / 200)
    emptyChance = clamp(emptyChance, 0, 100)
    if emptyChance > 0 and math.random(100) <= emptyChance then
        return items, money, profile
    end

    local minRolls = math.max(1, profile.minRolls or 1)
    local maxRolls = math.max(minRolls, profile.maxRolls or minRolls)
    maxRolls = math.min(maxRolls, MAX_ROLLS)

    local rolls = math.random(minRolls, maxRolls)
    for _ = 1, rolls do
        local entry = pickWeighted(pool)
        if not entry then
            break
        end

        local minAmount = math.max(1, entry.min or 1)
        local maxAmount = math.max(minAmount, entry.max or minAmount)
        local amount = math.random(minAmount, maxAmount)

        if entry.money then
            local multiplier = 1 + ((ctx.progress or 0) / 100) * (ctx.moneyProgressScale or 0.5)
            money = money + math.floor(amount * multiplier)
        else
            local statusMin = math.max(1, math.min(100, entry.statusMin or 100))
            local statusMax = math.max(statusMin, math.min(100, entry.statusMax or statusMin))

            items[#items + 1] = {
                item = entry.item or 0,
                randomItemClass = entry.randomItemClass or 0,
                maxCoolness = maxCoolness,
                count = math.min(10, amount),
                condition = math.random(statusMin, statusMax),
            }
        end
    end

    return items, money, profile
end

local function selectProfile(profile, profiles, ctx, tier, depth)
    if not profile or depth > 4 then
        return nil
    end

    if profileTierAllowed(profile, tier) and isSectorAllowed(profile.sectorTypes, ctx) then
        return profile
    end

    if profile.fallbackProfile and profile.fallbackProfile ~= "" then
        local fallback = profiles[profile.fallbackProfile]
        if fallback and fallback.id ~= profile.id then
            return selectProfile(fallback, profiles, ctx, tier, depth + 1)
        end
    end

    return nil
end

local function isDebugAllowed(profileId)
    if not DEBUG_FILTER_ENABLED then
        return true
    end

    return DEBUG_ONLY_SUBTYPES[profileId] == true
end

function GenerateSectorLoot(ctx)
    local plan = {}
    local profiles = ctx.profiles or {}
    local lootTypes = ctx.lootTypes or {}
    local spawnedByProfile = {}
    local tier = computeTier(ctx)

    -- travnik: C++ already sorted sites by priority/randomKey.
    -- Lua chooses the concrete containers and spawn/no-spawn decisions.
    for _, site in ipairs(ctx.sites or {}) do
        local profileId = site.profile

        if profileId and profileId ~= "" and isDebugAllowed(profileId) then
            local profile = selectProfile(profiles[profileId], profiles, ctx, tier, 0)

            if profile then
                local maxSites = profile.maxSites or site.maxSites or 0
                local spawned = spawnedByProfile[profile.id] or 0

                if maxSites > 0 and spawned >= maxSites then
                    -- soft cap: keep scanning only if another profile can still spawn
                else
                    local placement = choosePlacement(site, profile)
                    if placement then
                        local items, money, usedProfile = makeLootForProfile(profile, ctx, tier,
                            placement, lootTypes, profiles, 0)

                        if #items > 0 or money > 0 then
                            local security = buildSecurity(site, usedProfile, ctx)
                            plan[#plan + 1] = {
                                site = site.site,
                                placement = placement,
                                items = items,
                                money = money,
                                profile = usedProfile.id,
                                maxSites = usedProfile.maxSites or 0,
                                hasLock = security.hasLock,
                                lockId = security.lockId,
                                hasTrap = security.hasTrap,
                                trapId = security.trapId,
                                trapLevel = security.trapLevel,
                            }
                            spawnedByProfile[usedProfile.id] = (spawnedByProfile[usedProfile.id] or 0) + 1
                        end
                    end
                end
            end
        end
    end

    return plan
end
