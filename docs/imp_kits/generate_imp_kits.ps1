# travnik: генерация справочных таблиц по комплектам снаряжения IMP
# travnik: источники - IMPItemChoices.xml + Items.xml + Russian.Items.xml из каждой папки игры
# travnik: запуск: powershell -ExecutionPolicy Bypass -File generate_imp_kits.ps1

param(
    [string]$GameRoot = "D:\Projects\JA2_Game",
    [string]$OutDir   = $PSScriptRoot
)

$ErrorActionPreference = "Stop"
$inv = [System.Globalization.CultureInfo]::InvariantCulture

# travnik: канонические имена комплектов из enum IMP_ITEM_CHOICES (Tactical\Item Types.h)
$KitNames = @{
    0  = "Default Items";          1  = "Random Items";
    2  = "Wisdom";                 3  = "Dexterity";
    4  = "Agility";                5  = "Strength";
    6  = "Health";                 7  = "Leadership";
    8  = "Marksmanship";           9  = "Explosives";
    10 = "Mechanical";             11 = "Medical";
    12 = "Auto Weapons";           13 = "Heavy Weapons";
    14 = "Sniper";                 15 = "Ranger";
    16 = "Gunslinger";             17 = "Martial Arts";
    18 = "Squadleader";            19 = "Technician";
    20 = "Doctor";                 21 = "Ambidextrous";
    22 = "Melee";                  23 = "Throwing";
    24 = "Night Ops";              25 = "Stealthy";
    26 = "Athletics";              27 = "Bodybuilding";
    28 = "Demolitions";            29 = "Teaching";
    30 = "Scouting";               31 = "Lockpicking";
    32 = "Electronics";            33 = "Camouflage";
    34 = "Auto Weapons Exp";       35 = "Heavy Weapons Exp";
    36 = "Sniper Exp";             37 = "Ranger Exp";
    38 = "Gunslinger Exp";         39 = "Martial Arts Exp";
    40 = "Squadleader Exp";        41 = "Technician Exp";
    42 = "Doctor Exp";             43 = "Melee Exp";
    44 = "Throwing Exp";           45 = "Night Ops Exp";
    46 = "Stealthy Exp";           47 = "Teaching Exp";
    48 = "Lockpicking Exp";        49 = "Covert Ops";
    50 = "Covert Ops Exp";         51 = "Vox Operator";
    52 = "Sidearm"
}

# travnik: группы комплектов для навигации внутри файла
$KitGroups = @(
    @{ Title = "Базовые";                    From = 0;  To = 1  },
    @{ Title = "Характеристики";             From = 2;  To = 11 },
    @{ Title = "Навыки (обычные)";           From = 12; To = 33 },
    @{ Title = "Навыки (экспертные)";        From = 34; To = 48 },
    @{ Title = "Специальные";                From = 49; To = 52 }
)

$Folders = @("Data", "Data-1.13", "Data-AFS", "Data-SDO")

function Load-XmlDoc([string]$Path) {
    $doc = New-Object System.Xml.XmlDocument
    $doc.Load($Path)
    return $doc
}

function Read-ItemTable([string]$Path) {
    $doc = Load-XmlDoc $Path
    $map = @{}
    foreach ($node in $doc.DocumentElement.ChildNodes) {
        if ($node.NodeType -ne "Element") { continue }
        $idNode = $node.SelectSingleNode("uiIndex")
        if ($null -eq $idNode) { continue }
        $id = [int]$idNode.InnerText.Trim()
        $nameNode = $node.SelectSingleNode("szItemName")
        $name = if ($nameNode) { $nameNode.InnerText.Trim() } else { "" }
        $priceNode = $node.SelectSingleNode("usPrice")
        $coolNode = $node.SelectSingleNode("ubCoolness")
        $classNode = $node.SelectSingleNode("usItemClass")
        $map[$id] = [pscustomobject]@{
            Name     = $name
            Price    = if ($priceNode) { [int]$priceNode.InnerText.Trim() } else { 0 }
            Coolness = if ($coolNode) { [int]$coolNode.InnerText.Trim() } else { 0 }
            Class    = if ($classNode) { [int]$classNode.InnerText.Trim() } else { 0 }
        }
    }
    return $map
}

function Read-RussianNames([string]$Path) {
    $doc = Load-XmlDoc $Path
    $map = @{}
    foreach ($node in $doc.DocumentElement.ChildNodes) {
        if ($node.NodeType -ne "Element") { continue }
        $idNode = $node.SelectSingleNode("uiIndex")
        if ($null -eq $idNode) { continue }
        $id = [int]$idNode.InnerText.Trim()
        $nameNode = $node.SelectSingleNode("szItemName")
        if ($nameNode -and $nameNode.InnerText.Trim().Length -gt 0) {
            $map[$id] = $nameNode.InnerText.Trim()
        }
    }
    return $map
}

function Read-Kits([string]$Path) {
    $doc = Load-XmlDoc $Path
    $kits = @()
    foreach ($node in $doc.DocumentElement.ChildNodes) {
        if ($node.NodeType -ne "Element") { continue }
        $idx = [int]$node.SelectSingleNode("uiIndex").InnerText.Trim()
        $nameNode = $node.SelectSingleNode("name")
        $name = if ($nameNode) { $nameNode.InnerText.Trim() } else { "" }
        $choices = if ($node.SelectSingleNode("ubChoices")) { [int]$node.SelectSingleNode("ubChoices").InnerText.Trim() } else { 0 }
        $numItems = if ($node.SelectSingleNode("ubNumItems")) { [int]$node.SelectSingleNode("ubNumItems").InnerText.Trim() } else { 0 }

        $slots = @()
        for ($i = 1; $i -le 100; $i++) {
            $n = $node.SelectSingleNode("bItemNo$i")
            if ($null -eq $n) { break }
            $v = [int]$n.InnerText.Trim()
            if ($v -gt 0) { $slots += $v }
        }

        $kits += [pscustomobject]@{
            Index    = $idx
            Name     = $name
            Choices  = $choices
            NumItems = $numItems
            Slots    = $slots
        }
    }
    return ($kits | Sort-Object Index)
}

function Esc([string]$s) {
    if ($null -eq $s) { return "" }
    return ($s -replace "\|", "\|")
}

function Fmt([int]$v) { return $v.ToString("N0", $inv) }

function Build-FolderReport([string]$Folder) {
    $base = Join-Path $GameRoot $Folder
    $impPath  = Join-Path $base "TableData\Inventory\IMPItemChoices.xml"
    $itemPath = Join-Path $base "TableData\Items\Items.xml"
    $ruPath   = Join-Path $base "TableData\Items\Russian.Items.xml"

    foreach ($p in @($impPath, $itemPath, $ruPath)) {
        if (-not (Test-Path $p)) { throw "Не найден файл: $p" }
    }

    $items = Read-ItemTable $itemPath
    $ru    = Read-RussianNames $ruPath
    $kits  = Read-Kits $impPath

    $sb = New-Object System.Text.StringBuilder
    $null = $sb.AppendLine("# IMP: комплекты снаряжения - папка ``$Folder``")
    $null = $sb.AppendLine("")
    $null = $sb.AppendLine("> Источники: ``$Folder\TableData\Inventory\IMPItemChoices.xml``, ``$Folder\TableData\Items\Items.xml``, ``$Folder\TableData\Items\Russian.Items.xml``")
    $null = $sb.AppendLine("> Сгенерировано скриптом ``docs\imp_kits\generate_imp_kits.ps1``.")
    $null = $sb.AppendLine("")
    $null = $sb.AppendLine("## Как считается стоимость")
    $null = $sb.AppendLine("")
    $null = $sb.AppendLine("Новый магазин IMP (``Laptop\IMP Gear.cpp``) складывает в пул **все** предметы из списка комплекта")
    $null = $sb.AppendLine("(``bItemNo1..bItemNoN``, нулевые пропускаются), поэтому:")
    $null = $sb.AppendLine("")
    $null = $sb.AppendLine("- **Уникальные предметы** - сколько разных вещей предлагает комплект;")
    $null = $sb.AppendLine("- **Сумма (уник.)** - стоимость комплекта, если взять по одному предмету каждого вида;")
    $null = $sb.AppendLine("- **Сумма (все позиции)** - сумма по всем строкам списка, включая повторы (так считает старая схема ``GiveIMPItems``).")
    $null = $sb.AppendLine("")
    $null = $sb.AppendLine("Стоимость предмета - поле ``usPrice`` из ``Items.xml``, крутость - ``ubCoolness``")
    $null = $sb.AppendLine("(``ubCoolness = 0`` - предмет не появляется в обычной продаже, только по скриптам и наградам).")
    $null = $sb.AppendLine("Патроны к оружию в сумме не учтены: в новом магазине они выбираются отдельными ячейками и оплачиваются отдельно.")
    $null = $sb.AppendLine("")

    # travnik: в Data\IMPItemChoices.xml тег <name/> пустой - используем имена из enum
    $emptyNames = @($kits | Where-Object { -not $_.Name })
    if ($emptyNames.Count -gt 0) {
        $null = $sb.AppendLine("**Примечание:** в этом файле у $($emptyNames.Count) комплектов тег ``<name/>`` пустой - в таблицах подставлены")
        $null = $sb.AppendLine("канонические имена из enum ``IMP_ITEM_CHOICES`` (``Tactical\Item Types.h``).")
        $null = $sb.AppendLine("")
    }

    $emptyKits = @($kits | Where-Object { $_.Slots.Count -eq 0 })
    if ($emptyKits.Count -gt 0) {
        $null = $sb.AppendLine("**Внимание:** $($emptyKits.Count) комплектов в этом файле пусты (нет предметов), новый магазин IMP")
        $null = $sb.AppendLine("не покажет по ним ничего: " + (($emptyKits | ForEach-Object { "$($_.Index) " + $(if ($_.Name) { $_.Name } else { $KitNames[$_.Index] }) }) -join ", ") + ".")
        $null = $sb.AppendLine("")
    }

    # travnik: сводная таблица по всем комплектам
    $null = $sb.AppendLine("## Сводка по комплектам")
    $null = $sb.AppendLine("")
    $null = $sb.AppendLine("| # | Комплект | Позиций | Уник. | Сумма (уник.), $ | Сумма (все позиции), $ |")
    $null = $sb.AppendLine("|--:|----------|--------:|------:|-----------------:|-----------------------:|")

    $rows = @()
    foreach ($kit in $kits) {
        $name = if ($kit.Name) { $kit.Name } else { $KitNames[$kit.Index] }
        $unique = @($kit.Slots | Sort-Object -Unique)
        $sumUnique = 0
        foreach ($u in $unique) { if ($items.ContainsKey($u)) { $sumUnique += $items[$u].Price } }
        $sumAll = 0
        foreach ($s in $kit.Slots) { if ($items.ContainsKey($s)) { $sumAll += $items[$s].Price } }
        $rows += [pscustomobject]@{
            Index = $kit.Index; Name = $name; Slots = $kit.Slots.Count
            Unique = $unique.Count; SumUnique = $sumUnique; SumAll = $sumAll
        }
        $null = $sb.AppendLine("| $($kit.Index) | $(Esc $name) | $($kit.Slots.Count) | $($unique.Count) | $(Fmt $sumUnique) | $(Fmt $sumAll) |")
    }
    $null = $sb.AppendLine("")

    # travnik: детальные таблицы
    $null = $sb.AppendLine("## Детализация комплектов")
    $null = $sb.AppendLine("")

    foreach ($group in $KitGroups) {
        $inGroup = $kits | Where-Object { $_.Index -ge $group.From -and $_.Index -le $group.To }
        if (-not $inGroup) { continue }

        $null = $sb.AppendLine("### $($group.Title)")
        $null = $sb.AppendLine("")

        foreach ($kit in $inGroup) {
            $name = if ($kit.Name) { $kit.Name } else { $KitNames[$kit.Index] }
            $r = $rows | Where-Object { $_.Index -eq $kit.Index }

            $null = $sb.AppendLine("#### $($kit.Index). $name")
            $null = $sb.AppendLine("")
            $null = $sb.AppendLine("uiIndex = $($kit.Index) &middot; ubChoices = $($kit.Choices) &middot; ubNumItems = $($kit.NumItems)")
            $null = $sb.AppendLine("")

            if ($kit.Slots.Count -eq 0) {
                $null = $sb.AppendLine("_Комплект пуст: предметы не заданы._")
                $null = $sb.AppendLine("")
                continue
            }

            $null = $sb.AppendLine("| # | ID | Название (EN) | Название (RU) | Крутость | Цена, $ |")
            $null = $sb.AppendLine("|--:|---:|---------------|---------------|---------:|--------:|")

            $pos = 0
            foreach ($slot in $kit.Slots) {
                $pos++
                if ($items.ContainsKey($slot)) {
                    $it = $items[$slot]
                    $en = if ($it.Name) { $it.Name } else { "(нет в Items.xml)" }
                    $cool = $it.Coolness
                    $price = Fmt $it.Price
                } else {
                    $en = "(нет в Items.xml)"; $cool = "-"; $price = "-"
                }
                if ($ru.ContainsKey($slot)) { $ruName = $ru[$slot] } else { $ruName = "—" }
                $null = $sb.AppendLine("| $pos | $slot | $(Esc $en) | $(Esc $ruName) | $cool | $price |")
            }

            $null = $sb.AppendLine("")
            if ($r.Unique -eq $r.Slots) {
                $null = $sb.AppendLine("**Общая стоимость комплекта: $(Fmt $r.SumUnique) $**")
            } else {
                $null = $sb.AppendLine("**Общая стоимость комплекта: $(Fmt $r.SumUnique) $** (уникальные предметы; сумма всех $($r.Slots) позиций - $(Fmt $r.SumAll) $)")
            }
            $null = $sb.AppendLine("")
        }
    }

    return [pscustomobject]@{
        Folder = $Folder
        Rows   = $rows
        Text   = $sb.ToString()
        KitCount = $kits.Count
    }
}

$reports = @()
foreach ($folder in $Folders) {
    Write-Host "Обработка $folder ..."
    $reports += Build-FolderReport $folder
}

$utf8 = New-Object System.Text.UTF8Encoding($false)

foreach ($rep in $reports) {
    $path = Join-Path $OutDir ("IMP_Kits_" + $rep.Folder + ".md")
    [System.IO.File]::WriteAllText($path, $rep.Text, $utf8)
    Write-Host "  -> $path"
}

# travnik: сводный файл по всем папкам
$sb = New-Object System.Text.StringBuilder
$null = $sb.AppendLine("# IMP: сводка стоимости комплектов по папкам игры")
$null = $sb.AppendLine("")
$null = $sb.AppendLine("Стоимость - сумма ``usPrice`` по уникальным предметам комплекта (как их складывает новый магазин IMP).")
$null = $sb.AppendLine("")
$null = $sb.AppendLine("| # | Комплект | " + (($Folders | ForEach-Object { "``$_``" }) -join " | ") + " |")
$null = $sb.AppendLine("|--:|----------|" + (($Folders | ForEach-Object { "--------:" }) -join "|") + "|")

$allIndexes = @()
foreach ($rep in $reports) { $allIndexes += ($rep.Rows | ForEach-Object { $_.Index }) }
$allIndexes = $allIndexes | Sort-Object -Unique

foreach ($idx in $allIndexes) {
    $name = $KitNames[$idx]
    $cells = @()
    foreach ($folder in $Folders) {
        $rep = $reports | Where-Object { $_.Folder -eq $folder }
        $row = $rep.Rows | Where-Object { $_.Index -eq $idx }
        if ($row) { $cells += (Fmt $row.SumUnique) } else { $cells += "—" }
    }
    $null = $sb.AppendLine("| $idx | $name | " + ($cells -join " | ") + " |")
}

$null = $sb.AppendLine("")
$null = $sb.AppendLine("## Итоги по папкам")
$null = $sb.AppendLine("")
$null = $sb.AppendLine("| Папка | Комплектов | Сумма всех уникальных комплектов, $ |")
$null = $sb.AppendLine("|-------|-----------:|----------------------------------:|")
foreach ($rep in $reports) {
    $total = 0
    foreach ($r in $rep.Rows) { $total += $r.SumUnique }
    $null = $sb.AppendLine("| ``$($rep.Folder)`` | $($rep.KitCount) | $(Fmt $total) |")
}

$null = $sb.AppendLine("")
$null = $sb.AppendLine("## Наблюдения для балансировки")
$null = $sb.AppendLine("")
$null = $sb.AppendLine("- Комплект ``Sidearm`` (uiIndex 52) отсутствует во всех папках, хотя ``AddItemCategoryToPossibleGear( IMP_SIDEARM )``")
$null = $sb.AppendLine("  в ``Laptop\IMP Gear.cpp`` вызывается всегда - пул «запасного оружия» сейчас пуст.")
foreach ($rep in $reports) {
    $filled = @($rep.Rows | Where-Object { $_.Slots -gt 0 })
    $empty  = @($rep.Rows | Where-Object { $_.Slots -eq 0 })
    $maxRow = $filled | Sort-Object SumUnique -Descending | Select-Object -First 1
    $minRow = $filled | Sort-Object SumUnique | Select-Object -First 1
    $null = $sb.AppendLine("- ``$($rep.Folder)``: пустых комплектов - $($empty.Count); самый дорогой - $($maxRow.Index) $($maxRow.Name) ($(Fmt $maxRow.SumUnique) $), самый дешёвый - $($minRow.Index) $($minRow.Name) ($(Fmt $minRow.SumUnique) $).")
}
$null = $sb.AppendLine("")

$null = $sb.AppendLine("## Файлы")
$null = $sb.AppendLine("")
foreach ($rep in $reports) {
    $null = $sb.AppendLine("- ``IMP_Kits_$($rep.Folder).md``")
}

[System.IO.File]::WriteAllText((Join-Path $OutDir "README.md"), $sb.ToString(), $utf8)
Write-Host "  -> $(Join-Path $OutDir 'README.md')"
Write-Host "Готово."
