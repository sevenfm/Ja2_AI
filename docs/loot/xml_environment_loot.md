# EnvironmentLoot.xml — краткий справочник

`TableData\Loot\EnvironmentLoot.xml` — master-конфигурация процедурного лута.
Файл читается через Expat в `Feature/ContainerLoot/ContainerLootXml.cpp`.

Полная инструкция по настройке: `docs/container_loot_configuration.md`.
Описание фичи: `docs/container_loot_feature.md`.

---

## 1. Структура файла

```xml
<ENVIRONMENT_LOOT version="1">
    <CONTAINER_SECURITY>...</CONTAINER_SECURITY>
    <SECTOR_SECURITY>...</SECTOR_SECURITY>
    <LOOT_TYPES>...</LOOT_TYPES>

    <LOOT_PROFILE id="..." ...>
        ...
    </LOOT_PROFILE>
</ENVIRONMENT_LOOT>
```

Порядок профилей в XML важен: при совпадении селекторов побеждает первый
подходящий профиль.

---

## 2. `LOOT_PROFILE`

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

    <TILES>...</TILES>
    <KINDS>...</KINDS>
    <EMPTY_CHANCE>0</EMPTY_CHANCE>
    <ROLLS min="1" max="3" />
    <PLACEMENTS>...</PLACEMENTS>
    <ITEM_POOL>...</ITEM_POOL>
</LOOT_PROFILE>
```

| Атрибут | Смысл | По умолчанию |
|---|---|---|
| `id` | Уникальный ID профиля | обязателен |
| `name` | Отображаемое имя | `id` |
| `priority` | Порядок обработки сайтов, меньше = раньше | 100 |
| `maxSites` | Лимит сайтов профиля за сектор; `0` = без лимита | 0 |
| `sectorTypes` | Whitelist типов/тегов секторов | пусто = везде |
| `fallbackProfile` | Резервный профиль | пусто |
| `minTier`, `maxTier` | Допустимый tier | 0..10 |
| `coolnessBonus` | Сдвиг допустимого coolness | 0 |
| `security` | ID security-профиля | `default` |
| `enabled` | `false` полностью отключает профиль | `true` |

---

## 3. Селекторы

### `TILES/TILE`

```xml
<TILE tileset="31" usIndex="2302" />
<TILE image="furn_6" />
<TILE image="furn_6" frame="0" kind="container" />
```

| Атрибут | Смысл |
|---|---|
| `tileset` + `usIndex` | Точный тайл |
| `image` | Имя STI без пути и расширения |
| `frame` | Номер кадра STI |
| `kind` | `container`, `prop`, `vehicle`, `nature`, `ground`, `body` |

### `KINDS/KIND`

```xml
<KINDS>
    <KIND value="container" />
</KINDS>
```

Резервный матч по типу объекта, если точный тайл/image не подошёл.

---

## 4. Пустота и количество

```xml
<EMPTY_CHANCE>40</EMPTY_CHANCE>
<ROLLS min="1" max="3" />
```

- `EMPTY_CHANCE` — 0..100.
- `ROLLS min/max` — 1..6.
- `EMPTY_CHANCE=100` не отключает профиль; для этого есть `enabled="false"`.

---

## 5. `PLACEMENTS`

```xml
<PLACEMENTS>
    <PLACEMENT type="open" weight="100" />
    <PLACEMENT type="hidden" weight="100" />
    <PLACEMENT type="surface" weight="50" />
</PLACEMENTS>
```

| Тип | Требование |
|---|---|
| `open` | `canOpen && !isOpen` |
| `hidden` | `canHide` |
| `surface` | `canSurface` |
| `corpse` | `canCorpse`; спавн трупов пока не реализован |

C++ повторно проверяет допустимость placement.

---

## 6. `ITEM_POOL`

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

| Запись | Назначение |
|---|---|
| `ITEM id` | Предмет из `Items.xml` |
| `MONEY` | Деньги; только внутри `ITEM_POOL` профиля |
| `RANDOM_ITEM class` | Случайный предмет из `RandomItem.xml` |
| `TYPE id` | Ссылка на `<LOOT_TYPE>` |
| `ITEM type="..."` | Альтернативная ссылка на `<LOOT_TYPE>` |

Общие атрибуты:

| Атрибут | Смысл | По умолчанию |
|---|---|---|
| `weight` | Относительный вес | 1 |
| `min`, `max` | Количество/диапазон суммы | 1 |
| `statusMin`, `statusMax` | Состояние предмета | 100 |
| `sectorTypes` | Ограничение по секторам | везде |
| `minTier`, `maxTier` | Ограничение по tier | 0..10 |
| `enabled` | `false` исключает запись из пула | `true` |

`weight="0"` отбрасывается парсером. Для временного отключения используй
`enabled="false"`.

---

## 7. `LOOT_TYPES`

```xml
<LOOT_TYPES>
    <LOOT_TYPE id="medical_supplies"
        sectorTypes="town,hospital,military,prison,airport,palace"
        minTier="0" maxTier="10">
        <ITEM id="37" weight="120" min="1" max="1" />
    </LOOT_TYPE>
</LOOT_TYPES>
```

`LOOT_TYPE` задаёт `sectorTypes`, `minTier`, `maxTier` и общий пул.
Ссылка из профиля (`TYPE` или `ITEM type`) умножает веса:

```text
итоговый_вес = вес_ссылки * вес_записи_в_LOOT_TYPE
```

---

## 8. `enabled`

```xml
<LOOT_PROFILE id="shop_fixture" enabled="false" />
<ITEM id="215" enabled="false" weight="20" min="1" max="1" />
```

- `LOOT_PROFILE enabled="false"` — профиль исключается из поиска и fallback.
- `ITEM`, `MONEY`, `RANDOM_ITEM`, `TYPE` с `enabled="false"` — запись остаётся
  в XML, но не попадает в пул.
- Если у профиля не осталось активных записей, срабатывает `fallbackProfile`.

---

## 9. Security

Security-блоки описывают замки и ловушки контейнеров:

```xml
<CONTAINER_SECURITY>
    <SECURITY_PROFILE id="default" lockChanceMin="20" ... />
</CONTAINER_SECURITY>

<SECTOR_SECURITY>
    <TAG name="sam" lockBonus="15" trapBonus="20" trapLevelBonus="1" />
</SECTOR_SECURITY>
```

Подробные формулы, допустимые trap ID и правила применения:
`docs/container_loot_configuration.md`.

---

## 10. Coverage

Для проверки покрытия тайлов профилями включи в `ContainerLootTypes.h`:

```cpp
#define CONTAINER_LOOT_SCAN_ALL_MAPS TRUE
```

Будут созданы:

- `ContainerLoot_allmaps.csv`
- `ContainerLoot_catalog_template.csv`
- `ContainerLoot_coverage.csv`

Статусы coverage:

- `assigned` — профиль найден;
- `generic` — сработал `KINDS/KIND`;
- `skip` — объект не является лут-кандидатом;
- `missing` — кандидат без профиля.

После проверки верни `CONTAINER_LOOT_SCAN_ALL_MAPS` в `FALSE` и пересобери проект.

---

## 11. INI

Глобальные коэффициенты находятся в `[Tactical container loot]`
файла `Ja2_Options.INI`. Полный список и значения по умолчанию:
`docs/container_loot_configuration.md`.

Основное:

```ini
[Tactical container loot]
MAX_SPAWNS_PER_SECTOR = 400
LOOT_PROGRESS_WEIGHT = 1.0
LOOT_SECTOR_COOLNESS_WEIGHT = 1.0
LOOT_MILITARY_TYPE_BONUS = 1.0
LOOT_DIFFICULTY_BONUS = 0.5
LOOT_PROGRESS_TIER_CAP_BONUS = 3.0
LOOT_MAX_ITEM_COOLNESS_BONUS = 2.0
LOOT_MONEY_PROGRESS_SCALE = 0.5
LOOT_EMPTY_CHANCE_SCALE = 1.0
```

---

## 12. Debug

- XML кэшируется статически; после правок нужен перезапуск игры.
- Лог в Debug-сборке: `ContainerLoot_debug.log`.
- Lua debug-фильтр: `DEBUG_FILTER_ENABLED` в `containerloot.lua`.
- `CONTAINER_LOOT_DEBUG_VISIBLE_HIDDEN` показывает скрытый лут.

Если XML не найден или не разобран, C++ пишет ошибку в лог и не генерирует лут.
