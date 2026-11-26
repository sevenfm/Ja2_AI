# Настройка конфигурации Container Loot

Подробная инструкция по настройке процедурного лута в `EnvironmentLoot.xml`, `containerloot.lua` и `Ja2_Options.INI`.

См. также:

- `docs/container_loot_feature.md` — описание фичи и потока данных.
- `docs/xml_environment_loot.md` — краткий справочник по XML-схеме.
- `.opencode/skills/loot-balance/SKILL.md` — рабочий skill для балансных правок.

---

## 1. Где что лежит

### 1.1. Master-файлы

| Файл | Назначение |
|---|---|
| `GameDirectory/Data/TableData/Loot/EnvironmentLoot.xml` | Профили, пулы, security |
| `GameDirectory/Data/Scripts/containerloot.lua` | Логика выбора и планирования |
| `D:\Projects\JA2_Game\Data\Ja2_Options.INI` | Глобальные коэффициенты |
| `Feature/ContainerLoot/ContainerLootXml.cpp` | Фактический парсер XML |
| `Feature/ContainerLoot/ContainerLootTypes.h` | Структуры и лимиты |

### 1.2. Активные VFS-копии

Игра читает данные через VFS, поэтому после правки master нужно синхронизировать
именно те копии, которые реально участвуют в выбранном VFS-профиле.

Проверь активный конфиг в `D:\Projects\JA2_Game\Ja2.ini`:

```ini
VFS_CONFIG_INI = vfs_config.AR+SDO.ini
```

Для профиля `AR+SDO` порядок профилей такой:

```text
SlfLibs, Vanilla, v113, SDO, SDO-ru, AR, AR+SDO, User, UserProf
```

Файлы читаются справа налево, поэтому приоритет выше у `UserProf`, `User`,
`AR+SDO`, `AR`, `SDO-ru`, `SDO`, `v113`.

Практическое следствие:

- XML может приходить из `Data-SDO`, а не из `GameDirectory`;
- Lua может приходить из `Data-User`, если там есть `containerloot.lua`;
- `GameDirectory` — это repo master, но не обязательно файл, который читает игра.

Найди реальные копии:

```powershell
Get-ChildItem 'D:\Projects\JA2_Game' -Recurse -File -Filter 'EnvironmentLoot.xml'
Get-ChildItem 'D:\Projects\JA2_Game' -Recurse -File -Filter 'containerloot.lua'
```

Правило: сначала определи, какой VFS-профиль является целевым для релиза,
затем выбери один source of truth и синхронизируй все активные копии.
Не копируй маленький `GameDirectory`-файл поверх более полного `Data-SDO`
без сравнения профилей и профилей security.

---

## 2. Порядок изменения

1. Определи цель в измеримых терминах.
2. Сделай baseline: текущие веса, шансы и ожидаемые доли.
3. Измени **одну** группу параметров.
4. Проверь XML на well-formed.
5. Проверь Lua-синтаксис, если менялся Lua.
6. Синхронизируй VFS-копии.
7. Перезапусти игру, потому что XML кэшируется статически.
8. Проверь новую генерацию на новом секторе или через Debug force-regenerate.
9. Сверь `ContainerLoot_debug.log`.

---

## 3. Структура `EnvironmentLoot.xml`

Текущий порядок блоков:

```xml
<ENVIRONMENT_LOOT version="1">
    <CONTAINER_SECURITY>...</CONTAINER_SECURITY>
    <SECTOR_SECURITY>...</SECTOR_SECURITY>
    <LOOT_TYPES>...</LOOT_TYPES>
    <LOOT_PROFILE ...>...</LOOT_PROFILE>
    ...
</ENVIRONMENT_LOOT>
```

### 3.1. Замечания по загрузке

- XML читается через Expat в `ContainerLootXml.cpp`.
- `EnsureLoaded()` кэширует результат в статических векторах.
- После изменения XML нужен перезапуск игры.
- Lua-скрипт вычисляется заново при генерации сектора, но уже сгенерированные сектора не пересоздаются без сброса флага.
- Если XML не загрузился, лут не генерируется и в лог пишется ошибка.

---

## 4. Security: замки и ловушки контейнеров

### 4.1. Общая схема

```xml
<CONTAINER_SECURITY>
    <SECURITY_PROFILE id="default"
        lockChanceMin="20" lockChanceMax="30"
        trapChanceMin="30" trapChanceMax="40"
        lockChanceCap="60" trapChanceCap="80"
        lockProgressWeight="0.10" lockCoolnessWeight="0.10"
        trapProgressWeight="0.10" trapCoolnessWeight="0.10"
        trapLevelMin="1" trapLevelMax="4"
        trapLevelProgressWeight="0.02" trapLevelCoolnessWeight="1.0"
        lockId="0">
        <TRAP_TYPES>
            <TRAP_TYPE id="EXPLOSION" weight="60" />
            <TRAP_TYPE id="ELECTRIC" weight="40" />
        </TRAP_TYPES>
    </SECURITY_PROFILE>
</CONTAINER_SECURITY>
```

### 4.2. Параметры security-профиля

| Атрибут | Смысл | Диапазон |
|---|---|---|
| `id` | ID профиля | строка |
| `lockChanceMin`, `lockChanceMax` | Базовый шанс замка, % | 0..100 |
| `trapChanceMin`, `trapChanceMax` | Шанс ловушки при наличии замка, % | 0..100 |
| `lockChanceCap` | Потолок итогового шанса замка | 0..100 |
| `trapChanceCap` | Потолок итогового шанса ловушки | 0..100 |
| `lockProgressWeight` | Вклад прогресса в шанс замка | float |
| `lockCoolnessWeight` | Вклад coolness в шанс замка | float |
| `trapProgressWeight` | Вклад прогресса в шанс ловушки | float |
| `trapCoolnessWeight` | Вклад coolness в шанс ловушки | float |
| `trapLevelMin`, `trapLevelMax` | Базовый уровень ловушки | 1..10 |
| `trapLevelProgressWeight` | Вклад прогресса в уровень | float |
| `trapLevelCoolnessWeight` | Вклад coolness в уровень | float |
| `lockId` | ID замка из `Locks.xml` | 0..254 |

`lockChanceMin/Max` и `trapChanceMin/Max` — это диапазон, из которого берётся случайное базовое значение, а не фиксированный процент.

### 4.3. Формула шанса замка

```text
lockChance = random(lockChanceMin, lockChanceMax)
           + progress * lockProgressWeight
           + coolnessNorm * lockCoolnessWeight
           + sum(lockBonus по тегам сектора)

lockChance = clamp(lockChance, 0, lockChanceCap)
```

Проверка:

```text
if random(100) > lockChance: замка нет
```

`coolnessNorm` — это `min(100, sectorCoolness) / 100`, то есть 0..1.

Важно: при `lockCoolnessWeight = 0.10` максимальный вклад coolness составит всего `+0.1` процентного пункта. Если нужен заметный вклад, используй значения порядка `5..15`.

### 4.4. Формула шанса ловушки

Ловушка проверяется только после успешного замка:

```text
trapChance = random(trapChanceMin, trapChanceMax)
           + progress * trapProgressWeight
           + coolnessNorm * trapCoolnessWeight
           + sum(trapBonus по тегам сектора)

trapChance = clamp(trapChance, 0, trapChanceCap)
```

### 4.5. Формула уровня ловушки

```text
trapLevel = random(trapLevelMin, trapLevelMax)
          + floor(progress * trapLevelProgressWeight)
          + floor(coolnessNorm * trapLevelCoolnessWeight)
          + sum(trapLevelBonus по тегам сектора)

trapLevel = clamp(trapLevel, 1, 10)
```

### 4.6. Типы ловушек

```xml
<TRAP_TYPES>
    <TRAP_TYPE id="EXPLOSION" weight="40" />
    <TRAP_TYPE id="SILENT_ALARM" weight="30" />
    <TRAP_TYPE id="SUPER_ELECTRIC" weight="30" />
</TRAP_TYPES>
```

Допустимые ID:

- `EXPLOSION`
- `ELECTRIC`
- `SIREN`
- `SILENT_ALARM`
- `BROTHEL_SIREN`
- `SUPER_ELECTRIC`

Веса относительные. Если `TRAP_TYPES` пуст, парсер добавляет `EXPLOSION`.

### 4.7. Секторные модификаторы

```xml
<SECTOR_SECURITY>
    <TAG name="sam" lockBonus="15" trapBonus="20" trapLevelBonus="1" />
    <TAG name="military" lockBonus="8" trapBonus="10" trapLevelBonus="0" />
    <TAG name="wealthy" lockBonus="5" trapBonus="5" trapLevelBonus="0" />
    <TAG name="underground" lockBonus="5" trapBonus="5" trapLevelBonus="0" />
    <TAG name="mine" lockBonus="3" trapBonus="3" trapLevelBonus="0" />
</SECTOR_SECURITY>
```

`name` должен совпадать с тегом из `ctx.sectorTags`:

```text
town, sam, mine, military, prison, hospital, airport, palace,
lawless, farm, agricultural, rural, industrial, underground,
wealthy, traffic
```

Если несколько тегов сектора совпали с разными `TAG`, бонусы складываются.

### 4.8. Apply-условия

`ContainerLootSecurity::Apply()` применяет security только если:

- контейнер закрыт (`canOpen && !isOpen`);
- это `KIND_CONTAINER`;
- в контейнер реально что-то заспавнилось;
- `hasLock == true`;
- `lockId <= 254`;
- на тайле нет чужого map-authored замка.

Ловушка ставится только после замка. Если `trapId` или `trapLevel` некорректны, ловушка отключается, а замок остаётся.

### 4.9. Ключи

Ключи не спавнятся. `lockId` только ссылается на запись из `Locks.xml`.

Текущие базовые ID:

- `0` — `Crate key`
- `1` — `Tough crate`

Если добавляешь новый `lockId`, проверь его в `Locks.xml` и убедись, что игрок может открыть контейнер отмычкой, ломом или взрывчаткой.

---

## 5. `LOOT_TYPES`

`LOOT_TYPES` — общие пулы предметов, на которые ссылаются профили.

```xml
<LOOT_TYPES>
    <LOOT_TYPE id="medical_supplies"
        sectorTypes="town,hospital,military,prison,airport,palace"
        minTier="0" maxTier="10">
        <ITEM id="37" weight="120" min="1" max="1"
              statusMin="50" statusMax="100" enabled="true" />
        <ITEM id="54" weight="80" min="1" max="1"
              statusMin="50" statusMax="100" enabled="true" />
    </LOOT_TYPE>
</LOOT_TYPES>
```

Параметры `LOOT_TYPE`:

| Атрибут | Смысл |
|---|---|
| `id` | ID типа |
| `sectorTypes` | Ограничение по секторам |
| `minTier`, `maxTier` | Ограничение по tier |
| `enabled` | Для всего `LOOT_TYPE` пока не поддерживается |

Внутри можно использовать те же записи, что и в `ITEM_POOL`: `ITEM`, `RANDOM_ITEM`.

Ссылка из профиля:

```xml
<ITEM_POOL>
    <TYPE id="medical_supplies" weight="2" />
</ITEM_POOL>
```

или:

```xml
<ITEM_POOL>
    <ITEM type="medical_supplies" weight="2" />
</ITEM_POOL>
```

Вес итоговой записи:

```text
entry.weight * typeItem.weight
```

---

## 6. `LOOT_PROFILE`

### 6.1. Полный пример

```xml
<LOOT_PROFILE
    id="weapon_crate"
    name="Weapon Crate"
    priority="10"
    maxSites="0"
    sectorTypes="sam,military,prison,airport,palace"
    fallbackProfile="crate"
    minTier="0"
    maxTier="10"
    coolnessBonus="1"
    security="high_value"
    enabled="true">

    <TILES>
        <TILE image="wp_stuf1" />
        <TILE tileset="31" usIndex="2302" />
    </TILES>

    <KINDS>
        <KIND value="container" />
    </KINDS>

    <EMPTY_CHANCE>0</EMPTY_CHANCE>
    <ROLLS min="1" max="3" />

    <PLACEMENTS>
        <PLACEMENT type="open" weight="100" />
        <PLACEMENT type="hidden" weight="100" />
    </PLACEMENTS>

    <ITEM_POOL>
        <ITEM id="215" weight="100" min="1" max="1"
              statusMin="50" statusMax="100" />
    </ITEM_POOL>
</LOOT_PROFILE>
```

### 6.2. Атрибуты профиля

| Атрибут | Смысл | По умолчанию |
|---|---|---|
| `id` | Уникальный ID | обязателен |
| `name` | Отображаемое имя | `id` |
| `priority` | Порядок обработки сайтов; меньше = раньше | 100 |
| `maxSites` | Лимит сайтов профиля за сектор; `0` = без лимита | 0 |
| `sectorTypes` | Whitelist типов/тегов; пусто = везде | пусто |
| `fallbackProfile` | Резервный профиль | пусто |
| `minTier`, `maxTier` | Допустимый tier | 0..10 |
| `coolnessBonus` | Сдвиг максимального coolness предметов | 0 |
| `security` | ID security-профиля | `default` |
| `enabled` | `false` полностью отключает профиль | `true` |

### 6.3. Приоритет и порядок XML

- Профили проверяются **в порядке XML**.
- Первый подходящий профиль побеждает.
- `priority` используется позже, при сортировке сайтов для Lua.
- Если два профиля матчат один и тот же тайл, ставь более специфичный выше в XML.

### 6.4. Селекторы

```xml
<TILES>
    <TILE tileset="31" usIndex="2302" />
    <TILE image="furn_6" />
    <TILE image="furn_6" frame="0" kind="container" />
</TILES>
```

| Атрибут | Смысл |
|---|---|
| `tileset` + `usIndex` | Точный тайл |
| `image` | Имя STI без пути и расширения |
| `frame` | Номер кадра STI |
| `kind` | `container`, `prop`, `vehicle`, `nature`, `ground`, `body` |

`KINDS/KIND` — резервный матч по типу объекта, если точный тайл/image не подошёл.

### 6.5. `EMPTY_CHANCE` и `ROLLS`

```xml
<EMPTY_CHANCE>40</EMPTY_CHANCE>
<ROLLS min="1" max="3" />
```

- `EMPTY_CHANCE` — 0..100.
- `ROLLS min/max` — 1..6; C++ и Lua дополнительно ограничивают значение.
- Не используй `EMPTY_CHANCE=100` для отключения профиля: используй `enabled="false"`.

### 6.6. `PLACEMENTS`

```xml
<PLACEMENTS>
    <PLACEMENT type="open" weight="100" />
    <PLACEMENT type="hidden" weight="100" />
</PLACEMENTS>
```

| Тип | Требование сайта |
|---|---|
| `open` | `canOpen && !isOpen` |
| `hidden` | `canHide` |
| `surface` | `canSurface` |
| `corpse` | `canCorpse`; пока не реализовано |

C++ повторно проверяет placement и отклоняет недопустимый вариант.

### 6.7. `ITEM_POOL`

```xml
<ITEM_POOL>
    <MONEY weight="100" min="10" max="100" enabled="true" />

    <ITEM id="215" weight="40" min="1" max="1"
          statusMin="50" statusMax="100"
          sectorTypes="sam,military"
          minTier="2" maxTier="8"
          enabled="true" />

    <RANDOM_ITEM class="3" weight="20" min="1" max="1"
                 statusMin="50" statusMax="100"
                 enabled="true" />

    <TYPE id="medical_supplies" weight="2" enabled="true" />
</ITEM_POOL>
```

Поддерживаемые записи:

| Запись | Назначение |
|---|---|
| `ITEM id` | Конкретный предмет из `Items.xml` |
| `MONEY` | Деньги |
| `RANDOM_ITEM class` | Случайный предмет из класса `RandomItem.xml` |
| `TYPE id` | Ссылка на `<LOOT_TYPE>` |
| `ITEM type="..."` | Альтернативная ссылка на `<LOOT_TYPE>` |

Общие атрибуты записей:

| Атрибут | Смысл | По умолчанию |
|---|---|---|
| `weight` | Относительный вес | 1 |
| `min`, `max` | Количество/диапазон суммы | 1 |
| `statusMin`, `statusMax` | Состояние предмета | 100 |
| `sectorTypes` | Ограничение по секторам | везде |
| `minTier`, `maxTier` | Ограничение по tier | 0..10 |
| `enabled` | `false` исключает запись | `true` |

`MONEY` работает только внутри `ITEM_POOL` профиля. Если добавить `MONEY` в `LOOT_TYPE`, парсер её не создаст.

`ITEM`/`TYPE` с `weight="0"` отбрасываются парсером. Для временного отключения предпочтительнее `enabled="false"`.

### 6.8. Fallback

```xml
<LOOT_PROFILE id="medical_cabinet" fallbackProfile="cabinet" ... />
```

Fallback используется в двух местах:

1. C++ `FindAllowedProfile()` — если профиль не подходит по `sectorTypes`.
2. Lua:
   - `selectProfile()` — если профиль не подходит по tier/сектору;
   - `makeLootForProfile()` — если пул оказался пустым.

Fallback не используется, если целевой профиль отключён через `enabled="false"`.

### 6.9. `enabled`

```xml
<LOOT_PROFILE id="shop_fixture" enabled="false" />
```

- `LOOT_PROFILE enabled="false"` — профиль исчезает из поиска и fallback.
- `ITEM`, `MONEY`, `RANDOM_ITEM`, `TYPE`, `ITEM type` с `enabled="false"` — запись остаётся в XML, но не попадает в пул.
- Если у профиля не осталось активных записей, срабатывает fallback.
- `enabled` не удаляет данные, поэтому его удобно использовать для временного отключения.

---

## 7. INI: `[Tactical container loot]`

Секция читается в `GameSettings.cpp` при старте игры.

```ini
[Tactical container loot]
; Максимальное количество созданных объектов за сектор
MAX_SPAWNS_PER_SECTOR = 400

; Вес прогресса игрока при расчёте tier
LOOT_PROGRESS_WEIGHT = 1.0

; Вес coolness сектора при расчёте tier
LOOT_SECTOR_COOLNESS_WEIGHT = 1.0

; Бонус tier для military/SAM/prison
LOOT_MILITARY_TYPE_BONUS = 1.0

; Бонус tier за уровень сложности
LOOT_DIFFICULTY_BONUS = 0.5

; Максимальный tier выше progress tier
LOOT_PROGRESS_TIER_CAP_BONUS = 3.0

; Дополнительный coolness предметов выше tier
LOOT_MAX_ITEM_COOLNESS_BONUS = 2.0

; Множитель денег от прогресса
LOOT_MONEY_PROGRESS_SCALE = 0.5

; Множитель EMPTY_CHANCE
LOOT_EMPTY_CHANCE_SCALE = 1.0
```

| Параметр | Диапазон | По умолчанию | Влияние |
|---|---:|---:|---|
| `MAX_SPAWNS_PER_SECTOR` | 1..4096 | 400 | Жёсткий предел созданных объектов за сектор |
| `LOOT_PROGRESS_WEIGHT` | 0..10 | 1.0 | Вклад прогресса в tier |
| `LOOT_SECTOR_COOLNESS_WEIGHT` | 0..10 | 1.0 | Вклад coolness сектора в tier |
| `LOOT_MILITARY_TYPE_BONUS` | 0..10 | 1.0 | Бонус tier для военных типов |
| `LOOT_DIFFICULTY_BONUS` | 0..10 | 0.5 | Бонус tier за сложность |
| `LOOT_PROGRESS_TIER_CAP_BONUS` | 0..10 | 3.0 | Ограничение роста tier от coolness |
| `LOOT_MAX_ITEM_COOLNESS_BONUS` | 0..10 | 2.0 | Допуск coolness предметов |
| `LOOT_MONEY_PROGRESS_SCALE` | 0..10 | 0.5 | Рост денег к late-game |
| `LOOT_EMPTY_CHANCE_SCALE` | 0..10 | 1.0 | Общий множитель пустоты |

Важно:

- `CONTAINER_LOOT_ENABLED` — compile-time макрос в `ContainerLootTypes.h`, а не INI.
- `MAX_SITES` в INI больше не используется.
- Изменения INI применяются после перезапуска игры.

---

## 8. Lua: что можно настраивать

Основные балансные значения должны быть в XML. Lua содержит алгоритм.

В `containerloot.lua` можно временно использовать debug-фильтр:

```lua
local DEBUG_FILTER_ENABLED = false
local DEBUG_ONLY_SUBTYPES = {
    cash_register = true,
}
```

Для production оставляй `false`.

Если нужно изменить формулу или добавить новое поле — это уже задача `ja2-dev`, а не только правка конфигурации.

Полезные точки в Lua:

| Функция | Что делает |
|---|---|
| `isSectorAllowed` | Проверяет `sectorTypes` |
| `computeTier` | Считает tier |
| `computeMaxCoolness` | Считает допустимый coolness |
| `buildPool` | Собирает пул по весам и ограничениям |
| `makeLootForProfile` | Выбирает пустоту, rolls, количество и fallback |
| `choosePlacement` | Выбирает `open/hidden/surface/corpse` |
| `buildSecurity` | Считает замок, ловушку и уровень |
| `GenerateSectorLoot` | Главный цикл по сайтам |

---

## 9. Рецепты настройки

### 9.1. Полностью отключить профиль

```xml
<LOOT_PROFILE id="shop_fixture" enabled="false">
```

### 9.2. Временно убрать предмет из пула

```xml
<ITEM id="215" enabled="false" weight="20" min="1" max="1" />
```

### 9.3. Уменьшить долю предмета

Было:

```xml
<ITEM id="215" weight="300" ... />
<ITEM id="216" weight="100" ... />
```

Стало:

```xml
<ITEM id="215" weight="150" ... />
<ITEM id="216" weight="100" ... />
```

Сумма весов уменьшилась, поэтому доля `215` упала сильнее, чем в 2 раза. Считай итоговые проценты.

### 9.4. Сделать контейнеры чаще пустыми

```xml
<EMPTY_CHANCE>60</EMPTY_CHANCE>
```

или глобально:

```ini
LOOT_EMPTY_CHANCE_SCALE = 1.5
```

### 9.5. Увеличить количество предметов

```xml
<ROLLS min="2" max="4" />
```

Помни про `MAX_ITEMS_PER_SITE = 6` и `MAX_SPAWNS_PER_SECTOR`.

### 9.6. Ограничить профиль секторами

```xml
<LOOT_PROFILE id="weapon_crate"
    sectorTypes="sam,military,prison,airport,palace" ... />
```

Пустой `sectorTypes` означает «везде».

### 9.7. Усилить замки и ловушки на SAM

```xml
<SECTOR_SECURITY>
    <TAG name="sam" lockBonus="20" trapBonus="25" trapLevelBonus="2" />
</SECTOR_SECURITY>
```

Не забывай, что итог ограничивается `lockChanceCap` и `trapChanceCap`.

### 9.8. Сделать high-value контейнеры сложнее

```xml
<SECURITY_PROFILE id="high_value"
    lockChanceMin="40" lockChanceMax="55"
    trapChanceMin="50" trapChanceMax="65"
    lockChanceCap="80" trapChanceCap="90"
    trapLevelMin="4" trapLevelMax="7">
```

### 9.9. Добавить новый профиль

1. Добавь `<LOOT_PROFILE>` в XML.
2. Укажи `TILES` или `KINDS`.
3. Проверь `sectorTypes`.
4. Задай `fallbackProfile`.
5. Добавь `ITEM_POOL`.
6. Проверь `priority` и порядок XML.
7. Проверь coverage на новом тайле.
8. Синхронизируй VFS.

### 9.10. Добавить новый тип лута

1. Добавь `<LOOT_TYPE>` в `<LOOT_TYPES>`.
2. Проверь `sectorTypes` и tier.
3. Добавь ссылку `<TYPE id="..."/>` в нужный профиль.
4. Помни, что итоговый вес — произведение весов ссылки и записи.

---

## 10. Проверка изменений

### 10.1. XML

```powershell
[xml](Get-Content 'GameDirectory\Data\TableData\Loot\EnvironmentLoot.xml' -Raw) | Out-Null
```

### 10.2. Lua

```powershell
python -c "import lupa; from lupa import LuaRuntime; lua=LuaRuntime(); lua.execute(open(r'GameDirectory\Data\Scripts\containerloot.lua',encoding='utf-8').read()); print('lua syntax OK')"
```

### 10.3. Синхронизация

```powershell
Get-FileHash 'GameDirectory\Data\TableData\Loot\EnvironmentLoot.xml',
             'D:\Projects\JA2_Game\Data\TableData\Loot\EnvironmentLoot.xml',
             'D:\Projects\JA2_Game\Data-1.13\TableData\Loot\EnvironmentLoot.xml',
             'D:\Projects\JA2_Game\Data-User\TableData\Loot\EnvironmentLoot.xml' -Algorithm SHA256
```

Хэши должны совпадать.

### 10.4. Игра

1. Перезапустить игру.
2. Войти в новый сектор или сбросить флаг в Debug.
3. Проверить `ContainerLoot_debug.log`.
4. Проверить фактическое поведение в игре.

---

## 11. Диагностика проблем

| Симптом | Вероятная причина | Что проверить |
|---|---|---|
| Лута нет вообще | XML не загрузился | `ContainerLootXml: loaded ...`, путь VFS |
| Лута нет в конкретном контейнере | Профиль не найден или пустой пул | `CollectSites`, `profile`, `enabled`, fallback |
| Все контейнеры пустые | Высокий `EMPTY_CHANCE` или `ROLLS` | XML и `LOOT_EMPTY_CHANCE_SCALE` |
| Предмет никогда не выпадает | `coolness` выше `maxCoolness` или tier | `computeTier`, `minTier/maxTier` |
| Изменения не применяются | Не синхронизирован VFS или XML закэширован | Хэши, перезапуск |
| Сектор не regenerates | Установлен `SF_CONTAINER_LOOT_GENERATED` | Debug force-regenerate, новый сектор |
| Нет замков | Нет security-профиля или `hasLock=false` | `ContainerLootLua: locked=...`, лог security |
| Замок есть, ловушки нет | Не прошёл `trapChance` или неверный trap ID | `ContainerLootSecurity: invalid trap...` |
| Замок не появился | `DoorTable` полон или чужой замок | `door table is full`, `existing non-generated lock` |
| Ловушки на предметах остались | Это старый сейв | Новые ловушки создаются только на контейнерах |
| Профиль исчез | `enabled="false"` или нет items | XML, `disabled=` в логе |

---

## 12. Текущая карта профилей

Это снимок master-конфигурации. При изменении XML обновляй таблицу.

| Профиль | priority | maxSites | sectorTypes | fallback | security |
|---|---:|---:|---|---|---|
| `cash_register` | 0 | 0 | town | generic | default |
| `barrel` | 30 | 0 | — | — | default |
| `alcohol_cache` | 20 | 0 | lawless,town,underground | barrel | high_value |
| `fridge` | 40 | 0 | town,farm,hospital,military,airport,prison,palace,lawless | generic | default |
| `cabinet` | 70 | 0 | town,farm,hospital,military,airport,prison,palace,lawless | generic | default |
| `table` | 80 | 0 | town,farm,hospital,military,airport,prison,palace,lawless | generic | default |
| `weapon_crate` | 10 | 0 | sam,military,prison,airport,palace | crate | high_value |
| `crate` | 75 | 0 | — | — | default |
| `locker` | 60 | 0 | town,military,prison,airport,hospital,sam,palace | crate | high_value |
| `medical_cabinet` | 50 | 0 | town,hospital,military,prison,airport,palace | cabinet | high_value |
| `medical_crate` | 55 | 0 | town,hospital,military,prison,airport,palace | crate | high_value |
| `toolbox` | 65 | 0 | town,mine,military,sam,airport,prison,farm | crate | high_value |
| `soda_can` | 105 | 0 | town,airport,military,prison,hospital,sam,lawless | generic | default |
| `debris` | 100 | 16 | — | — | default |
| `vehicle` | 90 | 0 | — | — | default |
| `nature` | 110 | 8 | wilderness,farm,mine | debris | default |
| `shop_fixture` | 86 | 0 | town,airport,military,prison,palace | cabinet | default |
| `vending_machine` | 85 | 0 | town,airport,military,prison,hospital,sam,lawless | generic | default |
| `generic` | 120 | 0 | — | — | default |

---

## 13. Чек-лист перед коммитом

- [ ] Цель изменения сформулирована измеримо.
- [ ] Изменён только master XML/Lua.
- [ ] XML well-formed.
- [ ] Lua-синтаксис проверен, если менялся.
- [ ] Fallback и `enabled` не сломаны.
- [ ] ID предметов проверены по `Items.xml`.
- [ ] `lockId` проверен по `Locks.xml`.
- [ ] Все VFS-копии синхронизированы.
- [ ] Хэши совпадают.
- [ ] Для сложных изменений есть расчёт или симуляция «до/после».
- [ ] В отчёте указано, что нужен перезапуск игры.
