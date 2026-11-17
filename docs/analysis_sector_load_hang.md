# Отчёт: зависание загрузки сектора в Debug (вход в тактику I8 → I9)

> travnik: отчёт по дефекту. Дата: 18.11.2026.
> Ветка: JA2 1.13 r7609 + AI (Seven), Visual Studio 2022, Win32, сборка `Debug`.
> Область: загрузка сектора (`EnterSector()` → `LoadWorld()` → `TrashWorld()`), структуры тайлов, трупы (`ROTTING_CORPSE`).

---

## 1. Кратко

| | |
|---|---|
| **Симптом** | В Debug-сборке при входе в тактический сектор появляется `Assertion Failure` в `Strategic/strategicmap.cpp`, после чего игрока «выбрасывает» на стратегическую карту; сектор фактически не загружается, дальнейшая игра невозможна |
| **В Release** | Ошибка не проявляется |
| **Первопричина** | `RemoveCorpse()` удалял структуру трупа **дважды**: `DeleteAniTile()` уже удаляет её через level node, а следом старый код повторно искал «последнюю» corpse-структуру в тайле и удалял её. Если в одном тайле два трупа, удалялась **чужая** структура, а в level node второго трупа оставался указатель на освобождённую память. Дальше обход списка структур уходил в мусор → загрузка сектора зависала |
| **Вторичное** | Во время очистки мира происходил повторный (вложенный) вызов `TrashWorld()` |
| **Исправлено** | `Tactical/Rotting Corpses.cpp`, `TileEngine/worlddef.cpp`, `TileEngine/structure.cpp`, `TileEngine/Tile Animation.cpp`, `TileEngine/worldman.cpp`, `Strategic/strategicmap.cpp`, `Strategic/Map Screen Interface Bottom.cpp`, `Standard Gaming Platform/DEBUG.*` |
| **Проверено** | Debug и Release собираются; вход I8 → I9 → I8 проходит без assert'ов; очистка мира при загрузке I9 ≈ 60 мс |

---

## 2. Как воспроизводилось

1. Загрузить сейв, сделанный в секторе **I8**.
2. Отряд 2 путешествует I8 → I9; отряд 3 остаётся в I8.
3. На стратегической карте кликнуть I9 и нажать «войти» (вход в тактический режим).

В Debug-сборке: сообщение `Assertion Failure` (в отчёте пользователя строка была прочитана как 2460, затем 2454 — фактически **2160**, позже **2155**; достоверный источник — `Profiles\assert.log`), затем игра оказывается на стратегической карте, наёмников в секторе нет. В Release: ошибок нет, сектор грузится.

---

## 3. Диагностика

### 3.1. Инструментация

Чтобы не гадать, в движок были добавлены (только для `_DEBUG`):

| Что | Где | Назначение |
|---|---|---|
| Логирование assert'ов в `Profiles\assert.log` | `Standard Gaming Platform/DEBUG.cpp` (`_FailMessage`) | Раньше текст assert'а терялся в диалоге; теперь пишется в файл с тиком, файлом и строкой |
| `LogAssertDiagnostic( const char* )` | `Standard Gaming Platform/DEBUG.cpp` / `DEBUG.H` | Единая точка для диагностических сообщений в тот же лог |
| Трассировка стека `Profiles\stack_trace.log` | `Standard Gaming Platform/debug_win_util.cpp` (`ENABLE_STACK_TRACE` в `_DEBUG`) | Позволяет увидеть путь вызова в момент assert'а |
| Пошаговые метки `LoadWorld` / `TrashWorld` / `RemoveCorpse` / `DeleteAniTile` / `RemoveStructFromLevelNode` | `worlddef.cpp`, `Rotting Corpses.cpp`, `Tile Animation.cpp`, `worldman.cpp` | Локализация места зависания (после исправления пошаговые метки удалены, см. п. 6) |

Отдельно пришлось исправить формирование текста assert'а: `%S` с кириллицей во `_snprintf`/`vsprintf` возвращает `-1` и сообщение теряется целиком, поэтому описание собирается через `std::ostringstream` + `GetNameS()`.

### 3.2. Найденная хронология

Фрагмент `Profiles\assert.log` (одна из диагностических сборок):

```
[27.19]  LoadWorld: begin file=I9.DAT force=-
[27.19]  LoadWorld: file size=350967
[27.19]  LoadWorld: read 350967 bytes
[27.20]  LoadWorld: progress set, calling TrashWorld
[27.20]  TrashWorld: begin depth=1
[27.20]  TrashWorld: trashing 300 world items
[27.20]  TrashWorld: items trashed
[27.20]  TrashWorld: overhead map trashed
[27.20]  TrashWorld: smoke/light effects reset
[27.20]  TrashWorld: soldiers processed
[27.20]  RemoveCorpses: count=87
...      corpses id=0..51 удаляются нормально (~1.5 мс каждый)
[44.58]  RemoveCorpse: id=52 gridNo=9734 begin
[44.58]  DeleteAniTile: begin tile=2DDA4DD0 level=4 grid=9734 flags=0x5C08200
[44.58]  DeleteAniTile: tile found in list
         <зависание>
```

Выводы по цепочке:

1. Файл `I9.DAT` читается корректно (350 967 байт), проблема не в карте.
2. Зависание — **внутри `RemoveCorpses()`**, на трупе `id=52` в тайле `9734`.
3. Внутри `DeleteAniTile()` найдена ani-tile (`level=4` = `ANI_STRUCT_LEVEL`, `ANITILE_EXISTINGTILE` не выставлен), далее вызов `RemoveStructFromLevelNode(9734, node)`:
   `node found` → `deleting structure` → **зависание в `DeleteStructureFromWorld( pStruct->pStructureData )`**.
4. В том же прогоне зафиксирован ещё один факт: два трупа лежали в одном тайле — `id=12` и `id=40`, оба `gridNo=17062`. Именно на трупе `id=40` возникла 3,45-секундная пауза и **вложенный вызов `TrashWorld()`** (в файле два `RemoveCorpses: count=87` и два `TrashWorld: begin` без промежуточного `end`). Это уже следствие повреждения памяти.

### 3.3. Где именно ломалось

```
TrashWorld()
└─ RemoveCorpses()
   └─ RemoveCorpse(52)
      ├─ DeleteAniTile(pAniTile)                      // первый раз удаляет структуру трупа
      │  └─ RemoveStructFromLevelNode(9734, node)
      │     └─ DeleteStructureFromWorld(node->pStructureData)   // <-- зависание
      └─ FindLastStructure(9734, STRUCTURE_CORPSE)     // старый код, второй раз
         └─ DeleteStructureFromWorld(...)              // удаляет структуру «соседнего» трупа
```

---

## 4. Первопричина

### 4.1. Почему сначала падал assert о размещении солдат

Первый симптом (`strategicmap.cpp`, строки 2155/2160) — проверка «все бойцы помещены в загружаемый сектор». В логе: бойцы отряда 3 (`assignment = SQUAD_3`, группа 47) остаются в I8 (`8,9,0`) с `bInSector = 1`, но `sGridNo = -1` после `RemoveMercsInSector()` (вызывается из `EnterSector()` перед `LoadWorld()`). Флаг `bInSector` остаётся «висячим» после удаления бойцов из сектора.

Это **не является ошибкой при загрузке другого сектора**: бойцы действительно ещё принадлежат старому сектору и будут расставлены при возврате в него. Поэтому вместо безусловного assert сделана проверка с учётом сектора — `AssertAllSoldiersPlacedInSector( sMapX, sMapY, bMapZ )`: бойцы, чьи `sSectorX/Y/Z` отличаются от загружаемого сектора, только пишутся в лог (`DIAG: sector X,Y,Z loaded, but these soldiers still have bInSector=1 from another sector:`), а assert остаётся только для реально непомещённых бойцов загружаемого сектора.

Но это был лишь первый симптом. Настоящая причина «выбрасывания» на стратегическую карту — зависание загрузки (см. ниже): загрузка не доходит до конца, и игрок остаётся на карте.

### 4.2. Настоящая причина: двойное удаление структуры трупа

В исходном коде `RemoveCorpse()`:

```cpp
gRottingCorpse[iCorpseID].fActivated = FALSE;

DeleteAniTile(gRottingCorpse[iCorpseID].pAniTile);   // (1)
FreeCorpsePalettes(&(gRottingCorpse[iCorpseID]));

// remove corpse structure from world when removing corpse (by Asdow)
const auto sGridNo = gRottingCorpse[iCorpseID].def.sGridNo;
auto pStructure = FindLastStructure(sGridNo, STRUCTURE_CORPSE);
DeleteStructureFromWorld(pStructure);                // (2)
```

Труп создаётся в `AddRottingCorpse()`: `CreateAnimationTile()` (уровень `ANI_STRUCT_LEVEL`) + `CheckForAndAddTileCacheStructInfo()`, которая через `AddStructureToWorld()` привязывает к level node структуру с флагом `STRUCTURE_CORPSE` (`pLevelNode->pStructureData`).

Далее `DeleteStructureFromTile()` делает `MemFree(pStructure)` — структура действительно освобождается. Значит:

* шаг (1) уже удаляет структуру **этого** трупа и освобождает память;
* шаг (2) ищет в тайле **последнюю** corpse-структуру. Если в тайле один труп — поиск ничего не находит (и вызов безопасно выходит по `CHECKF`). Если трупов два (наложение трупов — штатная ситуация), шаг (2) удаляет структуру **второго** трупа, но его level node продолжает на неё ссылаться.

Результат: `pLevelNode->pStructureData` второго трупа — висячий указатель. При его удалении `DeleteStructureFromWorld()` разыменовывает освобождённую память, `FindBaseStructure()`/`FindStructureByID()` уходят в мусорный список → **бесконечный обход и зависание загрузки**.

Подтверждение из лога: `id=12` и `id=40` в одном тайле `17062`; пауза и вложенный `TrashWorld()` возникли именно на `id=40`, а «смертельное» зависание — на `id=52` (тайл `9734`).

### 4.3. Исправление

Структура трупа удаляется **по своему указателю** из его level node, указатель обнуляется, повторного удаления нет:

```cpp
// travnik: структуру трупа удаляем по указателю из его level node и обнуляем ссылку,
// иначе DeleteAniTile удаляет её повторно (висячий указатель) и обход списка структур зависает
ANITILE* pAniTile = gRottingCorpse[iCorpseID].pAniTile;
LEVELNODE* pCorpseLevelNode = (pAniTile != NULL) ? pAniTile->pLevelNode : NULL;
BOOLEAN fStructureRemoved = FALSE;

if (pCorpseLevelNode != NULL && pCorpseLevelNode->pStructureData != NULL)
{
    fStructureRemoved = DeleteStructureFromWorld(pCorpseLevelNode->pStructureData);
    pCorpseLevelNode->pStructureData = NULL;
}

DeleteAniTile(pAniTile);
FreeCorpsePalettes(&(gRottingCorpse[iCorpseID]));

// remove corpse structure from world when removing corpse (by Asdow)
// travnik: если структура не была привязана к level node, ищем corpse-структуру в тайле, как раньше
if (!fStructureRemoved)
{
    const auto sGridNo = gRottingCorpse[iCorpseID].def.sGridNo;
    auto pStructure = FindLastStructure(sGridNo, STRUCTURE_CORPSE);

    if (pStructure != NULL)
    {
        DeleteStructureFromWorld(pStructure);
    }
}
```

Запасной поиск `FindLastStructure()` сохранён только для случаев, когда структура не была привязана к level node (например, тайл трупа пришёл из карты как обычный), и теперь дополнительно защищён проверкой на `NULL`.

---

## 5. Прочие изменения

### 5.1. `TileEngine/worlddef.cpp`

* **Защита от повторного входа в `TrashWorld()`**: статический счётчик глубины. Вложенный вызов пишет `TrashWorld: nested call ignored` и мир не трогает (повторная очистка посреди уже идущей оставляет висячие указатели). Счётчик корректно уменьшается на всех выходах, включая ранний `if( !gfWorldLoaded )`.
* **Проверка размера файла карты** в `LoadWorld()`: пустой файл или файл больше 32 МБ (`MAP_FILE_SIZE_LIMIT`) не читается, загрузка аккуратно прекращается вместо падения/зависания.

### 5.2. `TileEngine/structure.cpp`

* `FindStructureByID()`: проверка `TileIsOutOfBounds( sGridNo )` и ограничение обхода списка (`MAX_STRUCTURE_SEARCH_STEPS`).
* `FindLastStructure()`: ограничение обхода списка.
* `DeleteStructureFromWorld()`: отбраковка мусорного/освобождённого указателя — если `sGridNo` базовой структуры вне карты, удаление пропускается с сообщением `DeleteStructureFromWorld: invalid structure grid, skipped`.

### 5.3. `TileEngine/Tile Animation.cpp`, `TileEngine/worldman.cpp`

Ограничение обхода связных списков (ani-tile и struct-нод) — `MAX_ANI_TILE_SEARCH_STEPS` / `MAX_LEVEL_NODE_SEARCH_STEPS`, с предупреждением `... LOOKS CYCLIC, aborting search`. Это «страховка»: при повреждённых данных загрузка продолжится, а в лог попадёт явное сообщение вместо зависания.

### 5.4. `Strategic/strategicmap.cpp`

* `AssertAllSoldiersPlacedInSector()` вместо двух безусловных assert-циклов (см. п. 4.1); бойцы чужого сектора логируются, а не роняют загрузку.
* `SectorHasUnplacedPlayerMercs()` + условие `gfWorldLoaded` в «быстром» пути: если сектор уже загружен, но в нём есть непомещённые бойцы — выполняется полная перезагрузка.
* Защита от повторного входа в `SetCurrentWorldSector()` (RAII-флаг `gfInSetCurrentWorldSector`): рекурсивный вызов игнорируется с сообщением `SetCurrentWorldSector: recursive call ignored`.
* Диагностические строки жизненного цикла: `SetCurrentWorldSector: requested=... selected=... world=...`, `EnterSector: sector=... map=... exists=...`, `EnterSector: completed`, `PrepareLoadedSector: begin`.

### 5.5. `Strategic/Map Screen Interface Bottom.cpp`

`HandleExitsFromMapScreen()` теперь сразу переносит `gbExitingMapScreenToWhere` в локальную переменную и сбрасывает его в `-1` **до** обработки перехода — это устраняло повторный вход в обработку выхода с экрана карты.

### 5.6. `Standard Gaming Platform/DEBUG.cpp`, `DEBUG.H`, `debug_win_util.cpp`

* `_FailMessage()` пишет assert в `Profiles\assert.log` (тик, файл, строка, функция, текст) — диалог больше не единственный источник информации.
* Добавлен `LogAssertDiagnostic()`.
* Исправлено формирование текста assert'а (см. п. 3.1).
* В `_DEBUG` включена трассировка стека в `Profiles\stack_trace.log`.

### 5.7. Метки источника вызова `TrashWorld()`

Перед каждым вызовом добавлена одна строка вида `TrashWorld call: ...`:

| Файл | Метка |
|---|---|
| `SaveLoadGame.cpp` | `LoadSavedGame` |
| `Strategic/Game Init.cpp` | `InitGame` |
| `Strategic/Meanwhile.cpp` | `Meanwhile done fade out` |
| `Strategic/strategicmap.cpp` | `CheckAndHandleUnloadingOfCurrentWorld`, `... autoresolve` |
| `TileEngine/worlddef.cpp` | `DeinitializeWorld`, `NewWorld` |

Нужны, чтобы в следующий раз сразу видеть, кто вмешался в загрузку (вложенный вызов) — точный источник того вложенного вызова в диагностическом прогоне зафиксировать не удалось, после исправления первопричины он не воспроизводится.

---

## 6. Что осталось в коде для диагностики

Пошаговые метки по каждому трупу/структуре/ani-tile удалены: на секторе с 87 трупами они давали ~390 КБ лога и заметно замедляли загрузку. Осталось только то, что помогает и не мешает:

| Сообщение | Смысл |
|---|---|
| `TrashWorld: begin depth=N` / `TrashWorld: end` | Начало/конец очистки мира; `depth` > 1 быть не должно |
| `TrashWorld: nested call ignored` | Повторный вход во время очистки — диагностический признак |
| `... LOOKS CYCLIC, aborting search` | Зацикленный список ani-tile / struct-нод / структур |
| `FindStructureByID: structure list looks cyclic, aborting` | То же для поиска структуры по ID |
| `DeleteStructureFromWorld: invalid structure grid, skipped` | Мусорный или уже освобождённый указатель структуры |
| `RemoveCorpse: deleting loose structure` | Сработал запасной путь удаления corpse-структуры |
| `SetCurrentWorldSector: recursive call ignored` | Повторный вход в загрузку сектора |
| `Assertion Failure [...]` | Штатное сообщение assert'а (пишется в лог) |

Логи: `Profiles\assert.log` (assert'ы + диагностика), `Profiles\stack_trace.log` (стек в момент assert'а), `Profiles\DebugMessage.txt` (`ENTER SECTOR TIME` и прочее).

---

## 7. Проверка

| Прогон | Результат |
|---|---|
| Диагностическая сборка: сейв I8 → вход I9 | Загрузка I9 завершилась (`TrashWorld: end` → `LoadWorld: finished ok` → `EnterSector: completed`), ранее висела |
| Тот же прогон: I9 → I8 → I9 | Все три входа завершены, assert'ов нет |
| Финальная сборка (без пошаговых меток): I8 → I9 → I8 | `EnterSector: completed` во всех случаях; очистка мира при входе в I9 ≈ 60 мс; `assert.log` ≈ 6 КБ на сессию |
| Сборка | `Debug` — OK; полный `Release` (`ja2_VS2022.sln`, Win32) — OK |

---

## 8. Известные смежные проблемы (в этот фикс не входят)

`Profiles\DebugMessage.txt` периодически содержит:

```
SynchronizeItemTempFile()  Error!  Reported 0, should be 252
```

Это отдельная давняя проблема синхронизации временного файла предметов (`Tactical/World Items.cpp`), к зависанию загрузки отношения не имеет и игру не ломает. Требует отдельного разбора.

---

## 9. Приложение: ключевые фрагменты лога

Начало загрузки I9 и завершение после исправления:

```
[33.46] EnterSector: sector=9,9,0 map=I9.DAT exists=1 placeholder=0
[33.46] LoadWorld: begin file=I9.DAT force=-
[33.49] TrashWorld: begin depth=1
[33.55] TrashWorld: end
[33.84] LoadWorld: finished ok
[35.32] EnterSector: completed
```

Зависание до исправления (последние строки перед hang):

```
[44.58] RemoveCorpse: id=52 gridNo=9734 begin
[44.58] DeleteAniTile: begin tile=2DDA4DD0 level=4 grid=9734 flags=0x5C08200
[44.58] DeleteAniTile: tile found in list
[44.58] RemoveStructFromLevelNode: begin grid=9734 node=350D8D48 usIndex=36001
[44.58] RemoveStructFromLevelNode: node found
[44.58] RemoveStructFromLevelNode: deleting structure
        <hang>
```

Расшифровка `flags=0x5C08200`: `ubLevelID = 4` = `ANI_STRUCT_LEVEL`; `ANITILE_EXISTINGTILE` (0x400) не выставлен, значит удаление идёт через уровень структур, а не через «существующий тайл» карты.
