# Выбор снаряжения IMP (IMP Gear Selection)

> Порт фичи из основного репозитория JA2 1.13 (`1dot13/source`, автор Flugente).
> Позволяет игроку вручную выбрать стартовое снаряжение создаваемого IMP-наёмника
> вместо случайной выдачи по ответам анкеты.

---

## Игровое описание

После экрана распределения атрибутов появляется страница **«Как будет
выбираться снаряжение?»** с двумя кнопками:

- **Автоматически** (старый метод) — снаряжение выдаётся случайно согласно
  ответам анкеты, как в оригинале. Дополнение: IMP всегда получает пистолет
  из категории `Sidearm` (`IMPItemChoices.xml`, uiIndex 52), поэтому не может
  остаться без оружия.
- **Вручную** (новый метод) — экран выбора снаряжения: ~27 выпадающих списков,
  по одному на каждый слот снаряжения. Для каждого слота кнопками **+/−**
  задаётся количество предметов.

### Слоты выбора

| Группа | Слоты |
|--------|-------|
| LBE | жилет, боевой подсумок, рюкзак, 2 кобуры |
| Оружие | основное оружие, боеприпасы, второе оружие, доп. боеприпасы, оружие ближнего боя |
| Броня | шлем, бронежилет, штаны, 2 слота лицевого снаряжения |
| Доп. предметы | 7 произвольных слотов |

Список доступных предметов формируется из двух источников:

1. Все категории `IMPItemChoices.xml` кроме трейтовых (`IMP_DEFAULT`,
   `IMP_RANDOMDEFAULT`, характеристики, трейтовые наборы) — базовый пул;
2. Категории выбранных трейтов (обычных и экспертных) — добавляются к пулу.

Стоимость выбранных предметов суммируется. Бесплатная часть — `IMP_PROFILE_COST`
(по умолчанию 3000$, параметр `Ja2.ini`). Сверх этой суммы стоимость
**доплачивается отдельно** к цене профиля. На странице выбора стоимость
показывается только если она превышает бесплатную часть:
`Additional Cost: %d$ (%d$ prepaid)`.

---

## Поток выполнения (полная цепочка)

```
IMP Attribute Selection: кнопка Finish
  └─ iCurrentImpPage = IMP_GEAR_ENTRANCE, SetGeneratedCharacterAttributes()
IMP Gear Entrance: выбор метода
  ├─ «Автоматически» → fNewIMPGearMethodUsed = FALSE
  │    └─ iCurrentImpPage = IMP_FINISH, iCurrentProfileMode = IMP__FINISH
  └─ «Вручную» → fNewIMPGearMethodUsed = TRUE
       └─ iCurrentImpPage = IMP_GEAR
IMP Gear: выбор предметов по слотам
  └─ CalculatePossibleItems()      → пул предметов (XML + трейты)
  └─ DistributePossibleItemsOnDropDowns() → раскладка по дропдаунам
  └─ StoreSelectedIMPGear()        → gIMPGearSelectedItems (предмет → кол-во)
  └─ кнопка Finish → iCurrentImpPage = IMP_FINISH
IMP Confirm: подтверждение
  └─ GiveItemsToPC()
       ├─ IsIMPGearUsed() → GiveIMPSelectedGear(pProfile)
       │    └─ MakeProfileInvItemAnySlot() на каждый выбранный предмет
       └─ иначе стандартная выдача (+ GiveIMPRandomItems(IMP_SIDEARM))
  └─ DistributeInitialGear() — раскладка по карманам
  └─ оплата: iGetProfileCost(TRUE) = профиль + снаряжение сверх лимита
```

---

## Стоимость

| Функция | Назначение |
|---------|-----------|
| `GetIMPGearCost()` | Стоимость снаряжения **сверх лимита**: `max(0, gIMPGearCost - iIMPProfileCost)`, только при новом методе |
| `iGetProfileCost( aWithGearCost )` | Полная стоимость профиля. `aWithGearCost=TRUE` — при создании IMP (профиль + перерасход снаряжения), `FALSE` — при проверке баланса на главной странице IMP и при загрузке существующего профиля |
| `GetEquippedGearCost( pProfile )` | Стоимость всего инвентаря профиля (цена × количество по каждому слоту). Используется при загрузке существующего IMP — стоимость снаряжения не сохраняется в файле профиля, поэтому вычисляется по факту |

Логика оплаты при загрузке ранее созданного IMP (коммит b988df876):

```cpp
INT32 iIMPCost = iGetProfileCost( FALSE );
iIMPCost += max( 0, GetEquippedGearCost( &gMercProfiles[iProfileId] )
                  - gGameExternalOptions.iIMPProfileCost );
```

При динамической стоимости (`DYNAMIC_IMP_PROFILE_COST`) цена профиля
умножается на число занятых IMP-слотов, перерасход снаряжения добавляется
сверху.

---

## Конфигурация

### `Ja2.ini` → `[Recruitment Settings]`

| Параметр | По умолчанию | Назначение |
|----------|:---:|-----------|
| `IMP_PROFILE_COST` | 3000 | Бесплатная часть стоимости профиля/снаряжения |
| `DYNAMIC_IMP_PROFILE_COST` | FALSE | Цена профиля растёт с каждым занятым IMP-слотом |

### XML

- `TableData\IMPItemChoices.xml` — категории предметов. Существующие
  категории используются как есть; новая категория `Sidearm` (uiIndex 52)
  добавлена в выдачу старого метода. Полнота категорий на совести
  мод-сборки: пустые категории просто не дают предметов в пул.

### Графика

- `Interface\plusminusbuttons.sti` — кнопки +/− количества. В базовой
  поставке 7609 отсутствует, файл извлечён из upstream (r7529) и установлен
  в `Data-User\Interface\` (VFS загружает Data-User поверх SLF-архивов).
- `LAPTOP\button_2.sti` — кнопки страниц, уже есть в `LAPTOP.SLF`.

---

## Новые страницы IMP (CharProfile.h)

```cpp
enum{
    ...
    IMP_BACKGROUND,
    IMP_PREJUDICE,
    IMP_GEAR_ENTRANCE,   // выбор метода снаряжения
    IMP_GEAR,            // экран ручного выбора

    IMP_NUM_PAGES,
};
```

Обе страницы подключены во всех четырёх switch'ах `CharProfile.cpp`:
`HandleCharProfile`, `RenderCharProfile`, `ExitOldIMPMode`, `EnterNewIMPMode`.
Переход на них — из `IMP Attribute Selection.cpp`
(`BtnIMPAttributeFinishCallback`), возврат — в `IMP_FINISH` (страница Finish,
как и в upstream, пропускается кнопкой Finish экрана снаряжения).

---

## Расширение DropDown

`DropDownBase` (`Laptop\DropDown.h/.cpp`) дополнен API, который нужен фиче
(в upstream эти методы появились в соседних фичах, в базе 7609 их не было):

| Метод | Назначение |
|-------|-----------|
| `SetNthEntry( UINT8 )` | Выбрать n-ю запись (с защитой от выхода за границы) |
| `HasEntries()` | Есть ли записи вообще |
| `ClearEntries()` | Очистить список, сбросить выбор |
| `GetX()` / `GetY()` | Координаты дропдауна (для отрисовки превью предмета левее списка) |
| `GetSelectedEntryKey()` | Безопасная версия: `-1` для пустого списка (фикс upstream) |

В `enum definedDropDowns` добавлены 22 идентификатора `DROPDOWNNR_IMPGEAR_*`
— по одному на каждый дропдаун экрана (синглтоны `DropDownTemplate<N>`).

`IMP Gear.cpp` работает с дропдаунами через массив указателей
`DropDownBase* pIMPGEARDropDown[]`, что позволяет обрабатывать их циклом.

---

## Изменения в C++ коде

### Новые файлы

| Файл | Содержимое |
|------|-----------|
| `Laptop\IMP Gear.cpp` (926 строк) | Экран выбора: пул предметов, дропдауны, кнопки +/−, отрисовка превью предметов (`DisplayGear`), хранение выбора, стоимость |
| `Laptop\IMP Gear.h` | Интерфейс: `EnterIMPGear`, `GiveIMPSelectedGear`, `GetIMPGearCost`, `DisplayGear` |
| `Laptop\IMP Gear Entrance.cpp` | Страница выбора метода: `IsIMPGearUsed()`, флаг `fNewIMPGearMethodUsed` |
| `Laptop\IMP Gear Entrance.h` | Интерфейс страницы |

### Изменённые файлы

| Файл | Изменение |
|------|-----------|
| `Laptop\CharProfile.h/.cpp` | Две новые страницы в enum и switch'ах |
| `Laptop\IMP Attribute Selection.cpp` | Кнопка Finish → `IMP_GEAR_ENTRANCE` (ключевой стык!) |
| `Laptop\IMP Confirm.cpp` | Ветвление `GiveItemsToPC`, `GetEquippedGearCost`, перерасчёт стоимости при загрузке, `IMP_SIDEARM`, `ItemIsLegal` пропускает оружие |
| `Laptop\IMP Confirm.h` | `iGetProfileCost( BOOLEAN )`, `MakeProfileInvItemAnySlot`, `GetEquippedGearCost` |
| `Laptop\IMP MainPage.cpp` | Проверка баланса без gear-стоимости (`iGetProfileCost( FALSE )`) |
| `Laptop\IMP Text System.cpp` | Полная стоимость на странице подтверждения |
| `Laptop\DropDown.h/.cpp` | Расширение API (см. выше) |
| `Tactical\Item Types.h` | `IMP_SIDEARM` в enum категорий |
| `Utils\Text.h` + 8 локализаций | `szIMPGearWebSiteText`, `szIMPGearDropDownText`, `szIMPGearDropDownNoneText` |
| `Laptop\Laptop_VS2013.vcxproj` | Новые файлы в проекте |

---

## Хранение выбора

Выбор снаряжения живёт только в памяти (`gIMPGearSelectedItems`,
`std::map<UINT16, UINT8>`) и **не сохраняется** в файл IMP-профиля. В файл
попадают уже распределённые предметы (`gMercProfiles[].inv[]`), поэтому:

- перерасход стоимости при повторном найме/загрузке вычисляется по факту
  через `GetEquippedGearCost`;
- флаг метода (`fNewIMPGearMethodUsed`) сбрасывается при выходе с экрана
  подтверждения — это переменная создания персонажа, не профиля.

Дубликаты: один и тот же предмет в нескольких слотах учитывается один раз
(фикс upstream 200899593).

---

## Адаптация под базу 7609+AI

База 2014 года не содержит ряда upstream-изменений, учтённых при переносе:

| Отличие | Решение |
|---------|---------|
| Нет трейта SURVIVAL (появился в 2015) | Кейс `IMP_SKILL_TRAITS_NEW_SURVIVAL` и категория `IMP_SURVIVAL` убраны из `IMP Gear.cpp` |
| `iGetProfileCost()` без параметров (в upstream переименована в `GetProfileCost(BOOLEAN,BOOLEAN)`) | Имя сохранено, добавлен параметр `aWithGearCost` с default-значением; внутренняя логика `CountFilledIMPSlots(-1)` из вашей сборки сохранена |
| `DropDownBase` вашей сборки переработан (нет `WidgetBase`) | Расширение добавлено в существующий класс, а не заменой файла |
| В upstream `Laptop All.h` не включает IMP-страницы Flugente | Все новые файлы используют `#else`-ветку с явными инклудами (в вашем проекте PCH отключён — `NotUsing`) |
| Нет `IMP_SIDEARM` | Добавлен в конец enum (совместимо с существующими XML: несуществующая категория просто пуста) |
| `GetSelectedEntryKey` у вас без защиты | Добавлена защита из upstream (пустой список → `-1`) |

### Коммиты-источники (1dot13/source)

| Коммит | Что взято |
|--------|-----------|
| `7558c84bc` | Базовая фича (2014): файлы экранов, навигация, тексты |
| `a87e97776` | Неиспользуемые слоты не красные |
| `57f6e41d4` | Опциональный фон превью предметов |
| `1d8136aa0` | Sidearm-категория: IMP не остаётся без оружия |
| `200899593` | Дедупликация предметов между слотами |
| `46b34e55d` | `IMP_SIDEARM`, `ItemIsLegal` пропускает оружие |
| `b988df876` | Перерасход снаряжения вычитается всегда (в т.ч. при загрузке) |
| `f53ec6025` | Баланс на главной странице без gear-стоимости |
| `8c2c0dd71` | Стоимость показывается только при превышении лимита |

Бранч `feature/gear-imp` (старая попытка переноса, коммит `ca8e95d`) не
использовался — перенос выполнен заново из upstream.

---

## Известные ограничения

- Страница Finish после выбора снаряжения, как и в upstream, не показывается
  отдельно — она вызывается напрямую (`IMP_FINISH`), пользователь видит её
  как финальное подтверждение с полной стоимостью.
- Кол-во предметов ограничено 0–9 на слот (одна цифра в UI).
- Дропдаун показывает максимум 8 записей одновременно (`DROPDOWN_REGIONS`),
  остальные — через скроллбар (штатное поведение класса).
- Русская локализация выполнена; остальные 6 языков содержат английские
  строки с пометкой `TODO.Translate`.
- Стоимость снаряжения не сохраняется в IMP-файле — при загрузке профиля
  она вычисляется по текущим ценам `Items.xml`; изменение цен в XML меняет
  и доплату за старые профили.
- Кнопки +/− требуют `plusminusbuttons.sti`; без него экран выбора упадёт
  на `LoadButtonImage` (файл должен присутствовать в Data-User\Interface).
