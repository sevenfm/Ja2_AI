# IMP: комплекты снаряжения - папка `Data`

> Источники: `Data\TableData\Inventory\IMPItemChoices.xml`, `Data\TableData\Items\Items.xml`, `Data\TableData\Items\Russian.Items.xml`
> Сгенерировано скриптом `docs\imp_kits\generate_imp_kits.ps1`.

## Как считается стоимость

Новый магазин IMP (`Laptop\IMP Gear.cpp`) складывает в пул **все** предметы из списка комплекта
(`bItemNo1..bItemNoN`, нулевые пропускаются), поэтому:

- **Уникальные предметы** - сколько разных вещей предлагает комплект;
- **Сумма (уник.)** - стоимость комплекта, если взять по одному предмету каждого вида;
- **Сумма (все позиции)** - сумма по всем строкам списка, включая повторы (так считает старая схема `GiveIMPItems`).

Стоимость предмета - поле `usPrice` из `Items.xml`, крутость - `ubCoolness`
(`ubCoolness = 0` - предмет не появляется в обычной продаже, только по скриптам и наградам).
Патроны к оружию в сумме не учтены: в новом магазине они выбираются отдельными ячейками и оплачиваются отдельно.

**Примечание:** в этом файле у 49 комплектов тег `<name/>` пустой - в таблицах подставлены
канонические имена из enum `IMP_ITEM_CHOICES` (`Tactical\Item Types.h`).

**Внимание:** 31 комплектов в этом файле пусты (нет предметов), новый магазин IMP
не покажет по ним ничего: 1 Random Items, 3 Dexterity, 4 Agility, 5 Strength, 6 Health, 7 Leadership, 12 Auto Weapons, 13 Heavy Weapons, 14 Sniper, 15 Ranger, 16 Gunslinger, 17 Martial Arts, 18 Squadleader, 19 Technician, 20 Doctor, 26 Athletics, 27 Bodybuilding, 28 Demolitions, 29 Teaching, 30 Scouting, 33 Camouflage, 34 Auto Weapons Exp, 35 Heavy Weapons Exp, 36 Sniper Exp, 37 Ranger Exp, 38 Gunslinger Exp, 39 Martial Arts Exp, 40 Squadleader Exp, 41 Technician Exp, 42 Doctor Exp, 47 Teaching Exp.

## Сводка по комплектам

| # | Комплект | Позиций | Уник. | Сумма (уник.), $ | Сумма (все позиции), $ |
|--:|----------|--------:|------:|-----------------:|-----------------------:|
| 0 | Default Items | 2 | 2 | 610 | 610 |
| 1 | Random Items | 0 | 0 | 0 | 0 |
| 2 | Wisdom | 1 | 1 | 100 | 100 |
| 3 | Dexterity | 0 | 0 | 0 | 0 |
| 4 | Agility | 0 | 0 | 0 | 0 |
| 5 | Strength | 0 | 0 | 0 | 0 |
| 6 | Health | 0 | 0 | 0 | 0 |
| 7 | Leadership | 0 | 0 | 0 | 0 |
| 8 | Marksmanship | 10 | 2 | 2,790 | 14,294 |
| 9 | Explosives | 11 | 2 | 600 | 3,500 |
| 10 | Mechanical | 1 | 1 | 500 | 500 |
| 11 | Medical | 8 | 2 | 400 | 1,800 |
| 12 | Auto Weapons | 0 | 0 | 0 | 0 |
| 13 | Heavy Weapons | 0 | 0 | 0 | 0 |
| 14 | Sniper | 0 | 0 | 0 | 0 |
| 15 | Ranger | 0 | 0 | 0 | 0 |
| 16 | Gunslinger | 0 | 0 | 0 | 0 |
| 17 | Martial Arts | 0 | 0 | 0 | 0 |
| 18 | Squadleader | 0 | 0 | 0 | 0 |
| 19 | Technician | 0 | 0 | 0 | 0 |
| 20 | Doctor | 0 | 0 | 0 | 0 |
| 21 | Ambidextrous | 1 | 1 | 763 | 763 |
| 22 | Melee | 1 | 1 | 20 | 20 |
| 23 | Throwing | 1 | 1 | 25 | 25 |
| 24 | Night Ops | 1 | 1 | 500 | 500 |
| 25 | Stealthy | 1 | 1 | 750 | 750 |
| 26 | Athletics | 0 | 0 | 0 | 0 |
| 27 | Bodybuilding | 0 | 0 | 0 | 0 |
| 28 | Demolitions | 0 | 0 | 0 | 0 |
| 29 | Teaching | 0 | 0 | 0 | 0 |
| 30 | Scouting | 0 | 0 | 0 | 0 |
| 31 | Lockpicking | 1 | 1 | 250 | 250 |
| 32 | Electronics | 1 | 1 | 200 | 200 |
| 33 | Camouflage | 0 | 0 | 0 | 0 |
| 34 | Auto Weapons Exp | 0 | 0 | 0 | 0 |
| 35 | Heavy Weapons Exp | 0 | 0 | 0 | 0 |
| 36 | Sniper Exp | 0 | 0 | 0 | 0 |
| 37 | Ranger Exp | 0 | 0 | 0 | 0 |
| 38 | Gunslinger Exp | 0 | 0 | 0 | 0 |
| 39 | Martial Arts Exp | 0 | 0 | 0 | 0 |
| 40 | Squadleader Exp | 0 | 0 | 0 | 0 |
| 41 | Technician Exp | 0 | 0 | 0 | 0 |
| 42 | Doctor Exp | 0 | 0 | 0 | 0 |
| 43 | Melee Exp | 1 | 1 | 20 | 20 |
| 44 | Throwing Exp | 1 | 1 | 25 | 25 |
| 45 | Night Ops Exp | 1 | 1 | 500 | 500 |
| 46 | Stealthy Exp | 1 | 1 | 750 | 750 |
| 47 | Teaching Exp | 0 | 0 | 0 | 0 |
| 48 | Lockpicking Exp | 1 | 1 | 250 | 250 |

## Детализация комплектов

### Базовые

#### 0. Default Items

uiIndex = 0 &middot; ubChoices = 2 &middot; ubNumItems = 2

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 161 | Flak Jacket | Жилет легкий | 2 | 600 |
| 2 | 214 | Canteen | Фляжка | 1 | 10 |

**Общая стоимость комплекта: 610 $**

#### 1. Random Items

uiIndex = 1 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

### Характеристики

#### 2. Wisdom

uiIndex = 2 &middot; ubChoices = 2 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 176 | Steel Helmet | Стальная каска | 2 | 100 |

**Общая стоимость комплекта: 100 $**

#### 3. Dexterity

uiIndex = 3 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 4. Agility

uiIndex = 4 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 5. Strength

uiIndex = 5 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 6. Health

uiIndex = 6 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 7. Leadership

uiIndex = 7 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 8. Marksmanship

uiIndex = 8 &middot; ubChoices = 10 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 4 | Beretta 93R | Беретта 93R | 3 | 1,481 |
| 2 | 4 | Beretta 93R | Беретта 93R | 3 | 1,481 |
| 3 | 4 | Beretta 93R | Беретта 93R | 3 | 1,481 |
| 4 | 4 | Beretta 93R | Беретта 93R | 3 | 1,481 |
| 5 | 4 | Beretta 93R | Беретта 93R | 3 | 1,481 |
| 6 | 4 | Beretta 93R | Беретта 93R | 3 | 1,481 |
| 7 | 4 | Beretta 93R | Беретта 93R | 3 | 1,481 |
| 8 | 9 | MP5KA4 | MP5KA4 | 3 | 1,309 |
| 9 | 9 | MP5KA4 | MP5KA4 | 3 | 1,309 |
| 10 | 9 | MP5KA4 | MP5KA4 | 3 | 1,309 |

**Общая стоимость комплекта: 2,790 $** (уникальные предметы; сумма всех 10 позиций - 14,294 $)

#### 9. Explosives

uiIndex = 9 &middot; ubChoices = 20 &middot; ubNumItems = 2

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 137 | TNT | Тротил | 3 | 500 |
| 2 | 223 | Detonator | Детонатор | 3 | 100 |
| 3 | 137 | TNT | Тротил | 3 | 500 |
| 4 | 223 | Detonator | Детонатор | 3 | 100 |
| 5 | 137 | TNT | Тротил | 3 | 500 |
| 6 | 137 | TNT | Тротил | 3 | 500 |
| 7 | 223 | Detonator | Детонатор | 3 | 100 |
| 8 | 137 | TNT | Тротил | 3 | 500 |
| 9 | 223 | Detonator | Детонатор | 3 | 100 |
| 10 | 137 | TNT | Тротил | 3 | 500 |
| 11 | 223 | Detonator | Детонатор | 3 | 100 |

**Общая стоимость комплекта: 600 $** (уникальные предметы; сумма всех 11 позиций - 3,500 $)

#### 10. Mechanical

uiIndex = 10 &middot; ubChoices = 2 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 203 | Tool Kit | Инструмент | 1 | 500 |

**Общая стоимость комплекта: 500 $**

#### 11. Medical

uiIndex = 11 &middot; ubChoices = 10 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 201 | 1st Aid Kit | Аптечка | 1 | 100 |
| 2 | 201 | 1st Aid Kit | Аптечка | 1 | 100 |
| 3 | 201 | 1st Aid Kit | Аптечка | 1 | 100 |
| 4 | 202 | Med Kit | Док. аптечка | 1 | 300 |
| 5 | 202 | Med Kit | Док. аптечка | 1 | 300 |
| 6 | 202 | Med Kit | Док. аптечка | 1 | 300 |
| 7 | 202 | Med Kit | Док. аптечка | 1 | 300 |
| 8 | 202 | Med Kit | Док. аптечка | 1 | 300 |

**Общая стоимость комплекта: 400 $** (уникальные предметы; сумма всех 8 позиций - 1,800 $)

### Навыки (обычные)

#### 12. Auto Weapons

uiIndex = 12 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 13. Heavy Weapons

uiIndex = 13 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 14. Sniper

uiIndex = 14 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 15. Ranger

uiIndex = 15 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 16. Gunslinger

uiIndex = 16 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 17. Martial Arts

uiIndex = 17 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 18. Squadleader

uiIndex = 18 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 19. Technician

uiIndex = 19 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 20. Doctor

uiIndex = 20 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 21. Ambidextrous

uiIndex = 21 &middot; ubChoices = 0 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 3 | Beretta 92F | Беретта 92F | 2 | 763 |

**Общая стоимость комплекта: 763 $**

#### 22. Melee

uiIndex = 22 &middot; ubChoices = 1 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 49 | Dusters | Кастет | 2 | 20 |

**Общая стоимость комплекта: 20 $**

#### 23. Throwing

uiIndex = 23 &middot; ubChoices = 1 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 38 | Throw. Knife | Мет. нож | 3 | 25 |

**Общая стоимость комплекта: 25 $**

#### 24. Night Ops

uiIndex = 24 &middot; ubChoices = 1 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 211 | NVG I | ПНВ I | 3 | 500 |

**Общая стоимость комплекта: 500 $**

#### 25. Stealthy

uiIndex = 25 &middot; ubChoices = 1 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 207 | Silencer | Глушитель | 4 | 750 |

**Общая стоимость комплекта: 750 $**

#### 26. Athletics

uiIndex = 26 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 27. Bodybuilding

uiIndex = 27 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 28. Demolitions

uiIndex = 28 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 29. Teaching

uiIndex = 29 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 30. Scouting

uiIndex = 30 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 31. Lockpicking

uiIndex = 31 &middot; ubChoices = 1 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 204 | Locksmith Kit | Отмычки | 3 | 250 |

**Общая стоимость комплекта: 250 $**

#### 32. Electronics

uiIndex = 32 &middot; ubChoices = 1 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 215 | Metal Detector | Металлоискатель | 4 | 200 |

**Общая стоимость комплекта: 200 $**

#### 33. Camouflage

uiIndex = 33 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

### Навыки (экспертные)

#### 34. Auto Weapons Exp

uiIndex = 34 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 35. Heavy Weapons Exp

uiIndex = 35 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 36. Sniper Exp

uiIndex = 36 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 37. Ranger Exp

uiIndex = 37 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 38. Gunslinger Exp

uiIndex = 38 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 39. Martial Arts Exp

uiIndex = 39 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 40. Squadleader Exp

uiIndex = 40 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 41. Technician Exp

uiIndex = 41 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 42. Doctor Exp

uiIndex = 42 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 43. Melee Exp

uiIndex = 43 &middot; ubChoices = 1 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 49 | Dusters | Кастет | 2 | 20 |

**Общая стоимость комплекта: 20 $**

#### 44. Throwing Exp

uiIndex = 44 &middot; ubChoices = 1 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 38 | Throw. Knife | Мет. нож | 3 | 25 |

**Общая стоимость комплекта: 25 $**

#### 45. Night Ops Exp

uiIndex = 45 &middot; ubChoices = 1 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 211 | NVG I | ПНВ I | 3 | 500 |

**Общая стоимость комплекта: 500 $**

#### 46. Stealthy Exp

uiIndex = 46 &middot; ubChoices = 1 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 207 | Silencer | Глушитель | 4 | 750 |

**Общая стоимость комплекта: 750 $**

#### 47. Teaching Exp

uiIndex = 47 &middot; ubChoices = 0 &middot; ubNumItems = 0

_Комплект пуст: предметы не заданы._

#### 48. Lockpicking Exp

uiIndex = 48 &middot; ubChoices = 1 &middot; ubNumItems = 1

| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |
|--:|---:|---------------|---------------|---------:|--------:|
| 1 | 204 | Locksmith Kit | Отмычки | 3 | 250 |

**Общая стоимость комплекта: 250 $**

