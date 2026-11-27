# Генерация случайного лута в контейнерах (Container Loot)

Актуальное описание фичи для **JA2 1.13 r7609 + AI (Seven)**.

Связанные документы:

- `docs/loot/container_loot_configuration.md` — подробная инструкция по настройке.
- `docs/loot/xml_environment_loot.md` — краткий справочник по XML-схеме.

---

## 1. Назначение

Фича добавляет процедурный лут в объекты окружения при первом входе в сектор:

- шкафы, комоды, столы, холодильники, сейфы;
- ящики, патронные и оружейные ящики, аптечные шкафы;
- бочки, коробки, мусорные баки, торговые автоматы, кассы;
- разбитые машины и вертолёты;
- деревья, кусты, камни и другие природные объекты;
- поверхности столов, прилавков и полок.

Фича не заменяет:

- лут, заранее расставленный автором карты;
- инвентарь врагов и трупов, создаваемый другими системами;
- ассортимент торговцев;
- квестовые предметы и специальные контейнеры игры.

---

## 2. Что видит игрок

1. При первом входе в сектор часть контейнеров и объектов получает лут.
2. Закрытые открывающиеся контейнеры скрывают содержимое до открытия.
3. Бочки, коробки, кассы, автоматы, машины и природные объекты используют тайник, который ищется «красной рукой».
4. Часть подходящих закрытых контейнеров может быть заперта.
5. Часть запертых контейнеров может быть заминирована.
6. Ловушки на отдельных предметах больше не создаются: ловушка относится к контейнеру через `DoorTable`.
7. Ключи под процедурные замки автоматически не создаются.

---

## 3. Границы фичи

Фича работает только при входе в сектор, а не в редакторе, мультиплеере или режиме `meanwhile`.

Не обрабатываются:

- открытые открывающиеся контейнеры;
- контейнеры и тайлы, где уже есть `ITEM_POOL`;
- двери, окна, заборы, переключатели, стены, крыши;
- персонажи и трупы;
- специальные тайлы:
  - `OMERTA_LEAVE_EQUIP_GRIDNO`;
  - `BOBBYR_SHIPPING_DEST_GRIDNO`;
  - `MEDUNA_ITEM_DROP_OFF_GRIDNO_VALUE`;
  - `gModSettings.iAirportDropOff`;
- реальный транспорт Скайрайдера и машины игрока;
- квестовые и специальные контейнеры, если они уже содержат пул предметов.

---

## 4. Высокоуровневый поток

```text
EnterSector()
  -> ContainerLoot::ProcessSector()
      -> проверка координат и режима игры
      -> проверка SF_CONTAINER_LOOT_GENERATED
      -> ContainerLootSectorContext::Resolve()
      -> ContainerLootXml::EnsureLoaded()
      -> ContainerLootScanner::CollectSites()
      -> ContainerLootLua::Generate()
      -> ContainerLootSpawner::Apply()
          -> создание предметов и денег
          -> ContainerLootSecurity::Apply()
      -> SetSectorFlag(SF_CONTAINER_LOOT_GENERATED)
  -> SaveWorldItemsToTempItemFile()
```

Ключевое: генерация выполняется **до** сохранения temp-файла сектора, поэтому созданные предметы и `DoorTable` попадают в обычное сохранение сектора.

---

## 5. Компоненты

| Компонент | Ответственность |
|---|---|
| `ContainerLoot` | Точка входа `ProcessSector`, guard’ы, debug-лог, флаг сектора |
| `ContainerLootSectorContext` | Тип сектора, теги, прогресс, coolness, wealth/threat/traffic |
| `ContainerLootScanner` | Поиск сайтов и классификация объектов |
| `ContainerLootXml` | Загрузка `EnvironmentLoot.xml`, профили, fallback, security |
| `ContainerLootLua` | Сборка `ctx`, вызов `GenerateSectorLoot`, чтение плана |
| `ContainerLootSpawner` | Валидация плана, создание предметов/денег, размещение |
| `ContainerLootSecurity` | Замки и ловушки контейнеров через `DoorTable` |
| `ContainerLootRandomItems` | Выбор предмета для `RANDOM_ITEM` по coolness |
| `containerloot.lua` | Выбор профиля, пула, количества, состояния, размещения и security |
| `EnvironmentLoot.xml` | Все профили, пулы, секторные модификаторы и security |

---

## 6. Жизненный цикл сектора

### 6.1. Точка входа

Вызов находится в `Strategic/strategicmap.cpp` в `EnterSector()`, перед `SaveWorldItemsToTempItemFile()`:

```cpp
#if CONTAINER_LOOT_ENABLED
if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME))
{
    ContainerLoot::ProcessSector(sSectorX, sSectorY, (INT8)bSectorZ);
}
#endif
```

Такой выбор точки входа покрывает обычный вход в сектор и Tactical Placement GUI.

### 6.2. Guard’ы

`ProcessSector` не выполняется, если:

- координаты сектора невалидны;
- установлен `LOADING_SAVED_GAME`;
- мир ещё не загружен (`!gfWorldLoaded`);
- включён редактор (`gfEditMode`);
- включён мультиплеер (`is_networked`);
- активен `meanwhile`;
- в Release уже установлен `SF_CONTAINER_LOOT_GENERATED`;
- в Debug уже установлен `SF_CONTAINER_LOOT_GENERATED`, кроме первого входа в сектор после запуска процесса.

### 6.3. Флаг сектора

```cpp
#define SF_CONTAINER_LOOT_GENERATED 0x00010000
```

Флаг хранится в `SECTORINFO.uiFlags` и сохраняется вместе с сектором. Новые поля в save-структуры не добавляются.

Флаг ставится в конце `ContainerLootSpawner::Apply()` даже если план оказался пустым или часть записей была отклонена. Это защищает от повторной генерации при каждом входе.

Если Lua завершился ошибкой, `ContainerLootSpawner::Apply()` не вызывается, флаг не ставится, и генерация может быть повторена при следующем входе.

---

## 7. Контекст сектора

`ContainerLootSectorContext::Resolve()` заполняет `Context`:

| Поле | Источник | Смысл |
|---|---|---|
| `sectorX`, `sectorY`, `sectorZ` | `EnterSector` | Координаты |
| `sectorKey` | вычисляется | `"A9"` |
| `sectorType` | tileset и флаги | Основной тип сектора |
| `sectorTags` | tileset и флаги | Набор тегов |
| `difficulty` | `gGameOptions.ubDifficultyLevel` | Сложность |
| `progress` | `HighestPlayerProgressPercentage()` | Общий прогресс |
| `currentProgress` | `CurrentPlayerProgressPercentage()` | Текущий прогресс |
| `sectorCoolness` | `gCoolnessBySector[]` | Coolness сектора |
| `weather` | `guiEnvWeather` | Погода |
| `worldDay` | `GetWorldDay()` | День |
| `townId` | `StrategicMap[].bNameId` только для `sectorZ == 0`; для подземелий `BLANK_SECTOR` | Город |
| `threat` | Наземный гарнизон `SECTORINFO` для `sectorZ == 0`; `UNDERGROUND_SECTORINFO` для `sectorZ > 0` | 0..10 |
| `wealth` | город, аэропорт, военная база, шахта, трафик | 0..10 |
| `traffic` | `ubTravelRating` соответствующего уровня | 0..10 |
| `enemyControlled` | `StrategicMap[].fEnemyControlled` для `sectorZ == 0`; `!SectorInfo[].fPlayer[sectorZ]` для `sectorZ > 0` | Флаг |
| `isUnderground` | `sectorZ > 0` | Подземелье |
| `foodSystem` | `gGameOptions.fFoodSystem` | Включена ли еда |

Для `sectorZ > 0` используются `UNDERGROUND_SECTORINFO` и `SectorInfo[].fPlayer[sectorZ]`. Наземные флаги, threat и `enemyControlled` в подземный контекст не наследуются. `sectorCoolness` остаётся двумерным свойством сектора, потому что отдельного coolness по `Z` в r7609 нет.

### 7.1. Тип сектора

Основные значения:

```text
unknown
wilderness
farm
forest
swamp
coast
desert
town
sam
mine
military
prison
hospital
airport
industrial
underground
lawless
palace
```

Для наземных секторов тип определяется по `SF_SAM_SITE`, `SF_MINING_SITE`, городу и tileset. Для подземелий тип всегда `underground`, а теги строятся по tileset и `UNDERGROUND_SECTORINFO::uiFlags`.

### 7.2. Теги сектора

Теги используются для `sectorTypes` в XML и для `SECTOR_SECURITY`:

- `town`
- `sam`
- `mine`
- `military`
- `prison`
- `hospital`
- `airport`
- `palace`
- `lawless`
- `farm`, `agricultural`, `rural`
- `industrial`
- `underground`
- `wealthy`
- `traffic`

`isSectorAllowed()` в Lua разрешает запись, если `sectorTypes` пуст или содержит `ctx.sectorType` либо любой тег из `ctx.sectorTags`.

Для подземелий теги `town`, `sam`, `mine` и `airport` не добавляются автоматически с поверхности. Если профиль должен работать только на подземном уровне, используй `sectorType = underground` или тег `underground`.

---

## 8. Сканирование сайтов

`ContainerLootScanner::CollectSites()` проходит по структурам мира и создаёт сайты.

| Тип сайта | Признак | Возможности |
|---|---|---|
| `container` | `STRUCTURE_OPENABLE`, не дверь/окно/забор/переключатель | `canOpen`, иногда `canSurface` |
| `prop` | `STRUCTURE_GENERIC` или скрытый контейнер | `canHide`, иногда `canSurface` |
| `vehicle` | `STRUCTURE_VEHICLE` | `canHide` |
| `nature` | `STRUCTURE_TREE` и природные объекты | `canHide` |
| `surface` | `STRUCTURE_HASITEMONTOP` у generic | `canHide` + `canSurface` |

Сайт пропускается, если:

- у него нет `LEVELNODE`;
- тайл входит в список специальных;
- на тайле уже есть `ITEM_POOL`;
- структура входит в исключённые типы;
- открытый openable-контейнер уже открыт.

Специальные item-pool клетки Omerta, Bobby Ray и аэропорта проверяются вместе с координатами сектора, поэтому одноимённый `gridNo` в другом секторе не пропускается. Meduna drop-off в активном коде не используется и отдельно не исключается.

Для hidden-тайников дополнительно проверяется доступный тайл взаимодействия:

- сама клетка должна быть проходимой или рядом должна быть проходимая клетка;
- для multi-tile структур проверяются все occupied-тайлы;
- если доступного тайла нет, сайт пропускается, а в лог пишется `hiddenNoAccessSkipped`.

Сейчас это проверка локальной проходимости, а не полной достижимости от точки входа. Тайник в изолированной комнате всё ещё может быть выбран.

Скрытые объекты с `STRUCTURE_OPENABLE` получают `canOpen = TRUE`, но `canHide = FALSE`: hidden-placement им запрещён. Они могут участвовать в обычном open-placement и получать замок/ловушку.

---

## 9. Выбор профиля

Профиль ищется в `ContainerLootXml::FindProfileForSector()`:

1. точное совпадение `tileset + usIndex`;
2. совпадение `image` с учётом `frame` и `kind`;
3. резервный выбор по `KINDS/KIND`.

Если профиль не найден, сайт игнорируется.

Важно:

- профили перебираются **в порядке XML**; первый подходящий побеждает;
- `priority` не выбирает профиль, а задаёт порядок обработки сайтов;
- `enabled="false"` исключает профиль из поиска и из fallback;
- `fallbackProfile` проверяется на этапе C++ для `sectorTypes` и ещё раз в Lua для tier/сектора и пустого пула.

---

## 10. План Lua

`GenerateSectorLoot(ctx)` возвращает массив записей:

```lua
{
    site = 1,
    placement = "open",
    profile = "weapon_crate",
    maxSites = 0,
    money = 0,
    items = {
        { item = 123, count = 1, condition = 75, randomItemClass = 0, maxCoolness = 7 },
    },
    hasLock = true,
    lockId = 1,
    hasTrap = true,
    trapId = 4,
    trapLevel = 6,
}
```

### 10.1. Tier

```text
progressTier = (progress / 10) * progressWeight
sectorTier   = (sectorCoolness / 2) * sectorCoolnessWeight

tier = max(progressTier, sectorTier)
tier = min(tier, progressTier + progressTierCapBonus)
```

Далее:

- `military`, `sam`, `prison`: `+ militaryTypeBonus`;
- `palace`, `airport`: `+ militaryTypeBonus * 0.5`;
- `hospital`, `industrial`: `+ militaryTypeBonus * 0.25`;
- `+ difficulty * difficultyBonus`;
- clamp `0..10`.

`coolness` предмета:

```text
maxCoolness = round(tier + maxItemCoolnessBonus + profile.coolnessBonus)
maxCoolness = clamp(maxCoolness, 0, 10)
```

### 10.2. Пустота и количество

```text
progressLoss = max(0, progress - currentProgress)
emptyChance = profile.emptyChance * emptyChanceScale * (1 + progressLoss / 200)
emptyChance = clamp(emptyChance, 0, 100)
```

Если бросок `math.random(100) <= emptyChance`, профиль возвращает пустой результат.

Иначе:

```text
rolls = random(profile.minRolls, min(profile.maxRolls, 12))
```

Каждый roll выбирает одну запись из активного пула с учётом веса. Лимит `12` соответствует `MAX_ITEMS_PER_SITE` в C++; `MAX_COUNT = 10` ограничивает только количество копий одного item ID в одном стаке, а не число предметов в контейнере.

### 10.3. Пул

Запись попадает в пул, если:

- `enabled != false`;
- разрешена по `sectorTypes`;
- проходит по tier;
- для предмета: `item > 0` и `coolness <= maxCoolness`;
- для `MONEY` и `RANDOM_ITEM`: выполнены их собственные условия.

`TYPE`/`ITEM type` разворачивает `<LOOT_TYPE>`:

```text
weight = entry.weight * typeItem.weight
```

Для type-записей tier ограничивается пересечением профиля, типа и записи.

### 10.4. Деньги и предметы

```text
amount = random(min, max)
count = min(10, amount)
condition = random(statusMin, statusMax)
```

Деньги:

```text
multiplier = 1 + (progress / 100) * moneyProgressScale
money += floor(amount * multiplier)
```

Итоговые деньги ограничиваются `MAX_MONEY` (10000) в C++.

### 10.5. Размещение

`choosePlacement()` выбирает допустимый вариант по весам:

| `placement` | Требование |
|---|---|
| `open` | `canOpen && !isOpen` |
| `hidden` | `canHide && !canOpen` и есть доступный тайл взаимодействия |
| `surface` | `canSurface` |
| `corpse` | `canCorpse` |

`corpse` сейчас зарезервирован, но фактический спавн трупов ещё не реализован.

### 10.6. Security

`buildSecurity()` вызывается только для `site.kind == "container" && canOpen && !isOpen`.

Порядок:

1. Выбрать security-профиль по `profile.security` или `default`.
2. Развернуть шанс замка.
3. Если замок не выпал — выйти.
4. Развернуть шанс ловушки.
5. Если ловушка не выпала — вернуть только замок.
6. Выбрать тип ловушки и уровень.

Подробные формулы — в `docs/loot/container_loot_configuration.md`.

---

## 11. C++ спавн и валидация

`ContainerLootSpawner::Apply()`:

1. Сортирует план по `priority` и `randomKey` сайта.
2. Проверяет `site`, дубликаты и допустимость `placement`.
3. Проверяет `maxSites` профиля.
4. Отклоняет `corpse` до реализации трупов.
5. Проверяет лимиты денег и предметов.
6. Создаёт деньги через `CreateMoney()`.
7. Для каждого предмета:
   - разрешает `RANDOM_ITEM` через `ContainerLootRandomItems`;
   - проверяет ID, класс, `ItemIsLegal`, `defaultundroppable`, `IC_KEY`;
   - проверяет `count` и `condition`;
   - создаёт объект через `CreateItems()`;
   - добавляет в мир.
8. Учитывает общий лимит `MAX_SPAWNS_PER_SECTOR`.
9. Вызывает `ContainerLootSecurity::Apply()` только если реально создан хотя бы один объект.
10. Ставит `SF_CONTAINER_LOOT_GENERATED`.

### 11.1. Видимость

| `placement` | `bVisible` |
|---|---|
| `open` | `VISIBLE`; закрытый контейнер сам скрывает предмет через `HIDDEN_IN_OBJECT` |
| `hidden` | `HIDDEN_ITEM`; ищется «красной рукой» |
| `surface` | `VISIBLE` + высота рендера |
| `corpse` | пока не реализовано |

Для `hidden` спавнер дополнительно синхронизирует `ITEM_POOL::bVisible` и снимает `WORLD_ITEM_DONTRENDER`, чтобы `AddItemToPool()` не оставил предмет невидимым навсегда.

---

## 12. Замки и ловушки

### 12.1. Модель

Замок и ловушка контейнера хранятся в обычной `DoorTable`:

```cpp
struct DOOR
{
    INT32 sGridNo;
    BOOLEAN fLocked;
    UINT8 ubLockID;
    UINT8 ubTrapID;
    UINT8 ubTrapLevel;
    ...
};
```

Новые поля в `SOLDIERTYPE`, `OBJECTTYPE` или `SECTORINFO` не добавляются.

### 12.2. Условия применения

`ContainerLootSecurity::Apply()` применяет security только если:

- `entry.hasLock == TRUE`;
- сайт является `KIND_CONTAINER`;
- `canOpen == TRUE`;
- `isOpen == FALSE`;
- в контейнере реально что-то заспавнилось;
- `lockId` находится в диапазоне `1..254`; `lockId = 0` при `hasLock = TRUE` считается невалидным;
- нет чужого (map-authored) замка на этом тайле.

Если на тайле уже есть `DOOR`, созданный не этой фичей, запись не перезаписывается.

### 12.3. Ловушки

- Ловушка ставится только после успешного замка.
- Тип ловушки: `EXPLOSION`, `ELECTRIC`, `SIREN`, `SILENT_ALARM`, `BROTHEL_SIREN`, `SUPER_ELECTRIC`.
- Уровень ловушки: 1..10.
- Некорректный `trapId`/`trapLevel` отключает ловушку, но замок остаётся.
- Ловушки на отдельных предметах (`data.bTrap`) фичей больше не создаются.
- Старые предметы с `bTrap` из существующих сохранений не удаляются и не преобразуются.

### 12.4. Ключи

Ключи под процедурные замки не спавнятся. `lockId` ссылается на `Locks.xml`; при необходимости игрок должен найти соответствующий ключ или использовать отмычку/лом/взрывчатку.

### 12.5. Очистка

`ContainerLootSecurity::ClearGeneratedForSector()` удаляет только замки, созданные фичей в текущей сессии. Список хранится в памяти (`sGeneratedLocks`) и не сохраняется. Map-authored замки не трогаются.

Это влияет только на debug force-regenerate: после перезапуска процесса старые процедурные замки выглядят как «чужие» и сохраняются.

---

## 13. Сохранения и повторный вход

- `SF_CONTAINER_LOOT_GENERATED` хранится в `SECTORINFO`.
- Новые save-поля не добавляются.
- Seed кампании хранится в существующем `GENERAL_SAVE_INFO::uiSeedNumber`.
- При новой игре создаётся новый seed, при загрузке он восстанавливается. Для старых сейвов с `uiSeedNumber == 0` создаётся новый seed и записывается при следующем сохранении.
- Seed участвует в `MakeSiteRandomKey()`, поэтому разные кампании выбирают разные контейнеры при одинаковых лимитах.
- Предметы и деньги попадают в temp-файл сектора и обычное сохранение.
- `DoorTable` уже сохраняется движком, поэтому замки и ловушки контейнеров сохраняются без отдельной сериализации.
- При повторном входе в сектор генерация не повторяется.
- В Debug `CONTAINER_LOOT_ALLOW_VISITED_SECTORS = TRUE` и `CONTAINER_LOOT_FORCE_REGENERATE = TRUE`, поэтому можно попробовать повторную генерацию пустых контейнеров, но уже созданные предметы не удаляются.

---

## 14. Debug и диагностика

### 14.1. Лог

В Debug-сборке включён `CONTAINER_LOOT_AUDIT_ENABLED`, запись идёт в:

```text
ContainerLoot_debug.log
```

`Trace()` открывает файл один раз за процесс и пишет через буферизованный `std::ofstream`. `FlushTrace()` вызывается в конце `ProcessSector()` и в `ContainerLoot::Reset()`, чтобы последние строки не терялись при завершении игры или переходе между секторами. В Release `Trace()` и `FlushTrace()` не выполняют файловых операций.

Основные строки:

- `ProcessSector: enter ...`
- `ProcessSector: context ...`
- `CollectSites: ...`
- `ContainerLootLua: plan=... locked=... trapped=...`
- `ContainerLootSpawner: plan ...`
- `ContainerLootSecurity: applied ...`
- `ContainerLootSecurity: no lock ...`
- `ContainerLootSecurity: existing non-generated lock ...`

### 14.2. Debug-переключатели

`Feature/ContainerLoot/ContainerLootTypes.h`:

| Макрос | Debug | Release | Назначение |
|---|---:|---:|---|
| `CONTAINER_LOOT_ENABLED` | `TRUE` | `TRUE` | Компиляционное включение фичи |
| `CONTAINER_LOOT_AUDIT_ENABLED` | `TRUE` | `FALSE` | Запись `ContainerLoot_debug.log` |
| `CONTAINER_LOOT_ALLOW_VISITED_SECTORS` | `TRUE` | `FALSE` | Разрешить посещённые сектора |
| `CONTAINER_LOOT_FORCE_REGENERATE` | `TRUE` | `FALSE` | Сброс флага и процедурных замков один раз за сессию |
| `CONTAINER_LOOT_SCAN_ALL_MAPS` | `FALSE` | `FALSE` | Сканирование всех карт для coverage |
| `CONTAINER_LOOT_GENERATE_CONTACT_SHEETS` | `FALSE` | `FALSE` | Контактные листы тайлов |
| `CONTAINER_LOOT_DEBUG_VISIBLE_HIDDEN` | `FALSE` | `FALSE` | Показывать скрытый лут |

### 14.3. Lua debug-фильтр

В `containerloot.lua`:

```lua
local DEBUG_FILTER_ENABLED = false
local DEBUG_ONLY_SUBTYPES = {
    cash_register = true,
}
```

Это временный фильтр по ID профиля. Для production оставляй `false`.

### 14.4. Coverage

При `CONTAINER_LOOT_SCAN_ALL_MAPS = TRUE` создаются:

- `ContainerLoot_allmaps.csv`
- `ContainerLoot_catalog_template.csv`
- `ContainerLoot_coverage.csv`

Статусы coverage:

- `assigned` — профиль найден;
- `generic` — сработал `KINDS/KIND`;
- `skip` — объект не является лут-кандидатом;
- `missing` — кандидат без профиля.

После проверки вернуть `CONTAINER_LOOT_SCAN_ALL_MAPS` в `FALSE` и пересобрать проект.

---

## 15. Ограничения

1. `corpse` placement пока не реализован.
2. Очистка процедурных замков между перезапусками игры не сохраняется.
3. Force-regenerate не удаляет уже созданные предметы.
4. Процедурные ключи не спавнятся.
5. Старые item traps из прежних сохранений не мигрируются.
6. `DoorTable` имеет предел 255 записей; при переполнении замок не добавляется.
7. Профили перебираются в порядке XML; при конфликте селекторов это важнее `priority`.
8. Ловушки и замки применяются только к закрытым openable-контейнерам, в которые реально попал лут.
9. `maxSites="0"` означает «без ограничения», а не «отключено».
10. Для hidden-тайников проверяется только локальная проходимость соседней клетки, а не полная достижимость от точки входа. Изолированные комнаты требуют отдельной flood-fill/`FindBestPath` проверки.
11. `MAX_ITEMS_PER_SITE = 12`; превышение не отбрасывает сайт, а обрезает список предметов с warning в логе.

---

## 16. Карта файлов

| Файл | Роль |
|---|---|
| `Strategic/strategicmap.cpp` | Вызов `ProcessSector` в `EnterSector` |
| `Strategic/Campaign Types.h` | `SF_CONTAINER_LOOT_GENERATED` |
| `Feature/ContainerLoot/ContainerLoot.cpp` | Оркестрация и guard’ы |
| `Feature/ContainerLoot/ContainerLootScanner.cpp` | Поиск сайтов |
| `Feature/ContainerLoot/ContainerLootSectorContext.cpp` | Контекст сектора |
| `Feature/ContainerLoot/ContainerLootXml.cpp` | Парсер XML |
| `Feature/ContainerLoot/ContainerLootLua.cpp` | Мост C++ ↔ Lua |
| `Feature/ContainerLoot/ContainerLootSpawner.cpp` | Спавн и валидация |
| `Feature/ContainerLoot/ContainerLootSecurity.cpp` | Замки и ловушки |
| `Feature/ContainerLoot/ContainerLootRandomItems.cpp` | Выбор random item |
| `Tactical/Keys.cpp` | Безопасное расширение `DoorTable` |
| `GameDirectory/Data/TableData/Loot/EnvironmentLoot.xml` | Master-конфигурация |
| `GameDirectory/Data/Scripts/containerloot.lua` | Master-логика Lua |

---

## 17. Чек-лист проверки фичи

1. Запустить Debug-сборку.
2. Войти в новый сектор.
3. Проверить `ContainerLoot_debug.log`:
   - `ProcessSector: context ...`;
   - `CollectSites: ...`;
   - `ContainerLootLua: plan=...`;
   - `ContainerLootSpawner: spawned=...`.
4. Проверить, что в обычном секторе заперто примерно 20–30% подходящих контейнеров.
5. Проверить, что среди запертых примерно 30–40% с ловушкой.
6. Проверить `sam`/`military`: больше замков и ловушек, выше уровень.
7. Проверить открытие запертого контейнера: отмычка, лом, взрывчатка.
8. Проверить обнаружение и обезвреживание ловушки.
9. Сохранить игру, выйти, загрузить, проверить замок и ловушку.
10. Проверить, что старые предметы с `bTrap` не мешают новой логике.
11. Проверить seed: две новые игры должны давать разные наборы контейнеров при одинаковых `maxSites`/`maxSpawns`.
12. Сохраниться до генерации сектора, загрузиться и войти: выбранные сайты должны совпасть.
13. Проверить подземный сектор: threat/enemyControlled берутся с подземного уровня, наземные теги не наследуются.
14. Проверить `hiddenNoAccessSkipped` в логе и отсутствие hidden-лута в заведомо недоступных местах.
15. Проверить, что `MAX_ITEMS_PER_SITE = 12` не превышается.
