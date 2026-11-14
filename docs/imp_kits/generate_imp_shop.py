# -*- coding: utf-8 -*-
"""
travnik: генератор IMPShopItems.xml для каждой папки мода.

Правила подбора (баланс):
  - только предметы с крутостью 1..4 из Items.xml мода (endgame не добавляется);
  - цена не переопределяется (usPrice = 0) - берётся из Items.xml;
  - fForceAvailable = 0 - действуют штатные фильтры ItemIsLegal;
  - ubMaxAmount ограничивает копии (броня/разгрузка - 1, медицина/граны - 2);
  - ubKit привязывает специалистов к наборам трейтов, остальное - 255 (всем);
  - патроны не добавляются: магазины подбираются автоматически при выборе оружия.

Скрипт читает Items.xml/IMPItemChoices.xml из папки игры (--game-root), а готовые
файлы пишет в репозиторий: GameDirectory/<мод>/TableData/Inventory/IMPShopItems.xml
(--out-root, по умолчанию <repo>/GameDirectory).

Запуск:
  python docs/imp_kits/generate_imp_shop.py [--game-root D:\\Projects\\JA2_Game] [--out-root <repo>\\GameDirectory]
"""

import argparse
import io
import os
import xml.etree.ElementTree as ET

KIT_ALWAYS = 255

# travnik: привязки к наборам IMPItemChoices (uiIndex)
KIT_NIGHT_OPS = 24
KIT_LOCKPICKING = 31
KIT_CAMOUFLAGE = 33
KIT_RADIO_OPERATOR = 51

# travnik: папка репозитория для готовых файлов (GameDirectory рядом с docs)
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
DEFAULT_OUT_ROOT = os.path.join(REPO_ROOT, "GameDirectory")


def e(item_id, max_amount=1, kit=KIT_ALWAYS):
    """Одна запись ассортимента: (id предмета, лимит копий, привязка к набору)."""
    return (item_id, max_amount, kit)


# travnik: подбор снаряжения по модам; группы нужны только для читаемости XML
MODS = {
    "Data": {
        "title": "Vanilla (Data)",
        "groups": [
            ("Пистолеты и револьверы", [e(1), e(2), e(3), e(8), e(7), e(6), e(56), e(5)]),
            ("Пистолеты-пулемёты и дробовики", [e(9), e(10), e(330), e(11), e(13), e(31)]),
            ("Винтовки", [e(22), e(17), e(16), e(14)]),
            ("Броня", [e(176), e(177), e(188), e(161), e(164), e(189)]),
            ("Медицина и инструменты", [e(201, 2), e(202), e(203), e(204, 1, KIT_LOCKPICKING)]),
            ("Лицевое снаряжение", [e(211, 1, KIT_NIGHT_OPS), e(213), e(212)]),
            ("Граны", [e(134, 2), e(131, 2), e(151, 2), e(135, 2)]),
        ],
    },
    "Data-1.13": {
        "title": "1.13 (Data-1.13)",
        "groups": [
            ("Пистолеты и револьверы", [e(1), e(2), e(658), e(3), e(8), e(631), e(7), e(637), e(675)]),
            ("Пистолеты-пулемёты", [e(9), e(342), e(343), e(330), e(692), e(720), e(10), e(677)]),
            ("Дробовики и винтовки", [e(31), e(621), e(17), e(1193), e(14)]),
            ("Броня", [e(176), e(177), e(302), e(188), e(161), e(164), e(838)]),
            ("Разгрузка (LBE)", [e(1090), e(1651), e(1098), e(1095), e(1083), e(1093), e(1092), e(1649)]),
            ("Медицина и инструменты", [e(201, 2), e(202), e(203), e(204, 1, KIT_LOCKPICKING)]),
            ("Лицевое снаряжение", [e(211, 1, KIT_NIGHT_OPS), e(213), e(212)]),
            ("Граны", [e(134, 2), e(131, 2), e(151, 2), e(135, 2)]),
            ("Рация (Vox Operator)", [e(1697, 1, KIT_RADIO_OPERATOR)]),
        ],
    },
    "Data-AFS": {
        "title": "AFS (Data-AFS)",
        "groups": [
            ("Пистолеты и револьверы", [e(3), e(8), e(7), e(6), e(5), e(5168), e(5169), e(4318), e(4360)]),
            ("Пистолеты-пулемёты", [e(9), e(4588), e(4578), e(10), e(4976)]),
            ("Дробовики и винтовки", [e(31), e(4627), e(17), e(4680), e(4739)]),
            ("Броня", [e(176), e(177), e(161), e(164), e(878)]),
            ("Разгрузка (LBE)", [e(717), e(1651), e(741), e(716), e(731), e(732), e(735), e(1646)]),
            ("Медицина и инструменты", [e(201, 2), e(202), e(203), e(204, 1, KIT_LOCKPICKING)]),
            ("Лицевое снаряжение", [e(211, 1, KIT_NIGHT_OPS), e(213), e(212)]),
            ("Граны", [e(134, 2), e(131, 2), e(151, 2), e(135, 2)]),
        ],
    },
    "Data-SDO": {
        "title": "SDO (Data-SDO)",
        "groups": [
            ("Пистолеты и револьверы", [e(1), e(3), e(8), e(631), e(635), e(636), e(689), e(738), e(7), e(637)]),
            ("Пистолеты-пулемёты", [e(342), e(343), e(345), e(692), e(720), e(677)]),
            ("Дробовики и винтовки", [e(31), e(775), e(17), e(22), e(347), e(1193)]),
            ("Броня", [e(176), e(177), e(302), e(161), e(164), e(170), e(189), e(838), e(2276)]),
            ("Разгрузка (LBE)", [e(1090), e(1095), e(1083), e(1092), e(1093), e(1099), e(2273), e(1691)]),
            ("Медицина и инструменты", [e(201, 2), e(202), e(808, 2), e(203), e(2299), e(204, 1, KIT_LOCKPICKING)]),
            ("Лицевое снаряжение", [e(211, 1, KIT_NIGHT_OPS), e(213), e(212), e(2306)]),
            ("Камуфляж", [e(1027, 1, KIT_CAMOUFLAGE), e(1028, 1, KIT_CAMOUFLAGE), e(205, 1, KIT_CAMOUFLAGE)]),
            ("Граны", [e(134, 2), e(131, 2), e(151, 2), e(135, 2)]),
            ("Рация (Vox Operator)", [e(1697, 1, KIT_RADIO_OPERATOR)]),
        ],
    },
}

HEADER = """<?xml version="1.0" encoding="utf-8"?>
<!--
\tIMPShopItems.xml - ассортимент магазина снаряжения IMP для мода {title}.

\tФайл дополняет пул предметов, который собирается из IMPItemChoices.xml.
\tПодробное описание формата - docs/wiki_imp_shop_assortment.md.

\tПравила, по которым заполнен файл:
\t\t- крутость (ubCoolness) у всех предметов 1..4, endgame-снаряжение не добавлено;
\t\t- цена не переопределяется (usPrice = 0): берётся цена из Items.xml,
\t\t  поэтому доплата за снаряжение сверх IMP_PROFILE_COST остаётся прежней;
\t\t- fForceAvailable = 0: продолжают действовать ItemIsLegal (Tons of Guns,
\t\t  Sci-Fi, система инвентаря и т.д.);
\t\t- ubMaxAmount ограничивает копии: броня и разгрузка - 1, медицина и граны - 2;
\t\t- ubKit привязывает специализированные вещи к набору трейта
\t\t  (24 = Night Ops, 31 = Lockpicking, 33 = Camouflage, 51 = Vox Operator),
\t\t  остальное доступно всем (255);
\t\t- патроны не добавлены: магазины подбираются автоматически при выборе оружия.
-->
<IMPSHOPITEMS>

\t<fRespectCoolness>0</fRespectCoolness>
"""


def load_items(path):
    tree = ET.parse(path)
    items = {}
    for node in tree.getroot():
        if node.tag != "ITEM":
            continue
        fields = {child.tag: (child.text or "").strip() for child in node}
        try:
            item_id = int(fields.get("uiIndex", ""))
        except ValueError:
            continue
        items[item_id] = {
            "name": fields.get("szItemName", ""),
            "price": int(fields.get("usPrice", "0") or 0),
            "cool": int(fields.get("ubCoolness", "0") or 0),
        }
    return items


def load_kits(path):
    tree = ET.parse(path)
    kits = set()
    for node in tree.getroot():
        if node.tag != "IMPITEMCHOICES":
            continue
        try:
            kits.add(int((node.findtext("uiIndex") or "").strip()))
        except ValueError:
            continue
    return kits


def xml_escape(text):
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def build_mod(game_root, out_root, mod, config):
    items_path = os.path.join(game_root, mod, "TableData", "Items", "Items.xml")
    kits_path = os.path.join(game_root, mod, "TableData", "Inventory", "IMPItemChoices.xml")
    items = load_items(items_path)
    kits = load_kits(kits_path)

    lines = [HEADER.format(title=config["title"])]
    index = 0
    errors = []

    for group_name, entries in config["groups"]:
        if index > 0:
            lines.append("")
        lines.append("\t<!-- %s -->" % group_name)

        for (item_id, max_amount, kit) in entries:
            if item_id not in items:
                errors.append("%s: предмет %d отсутствует в Items.xml" % (mod, item_id))
                continue

            data = items[item_id]

            # travnik: жёсткая проверка баланса - крутость только 1..4
            if not (1 <= data["cool"] <= 4):
                errors.append("%s: предмет %d (%s) имеет крутость %d вне 1..4"
                              % (mod, item_id, data["name"], data["cool"]))
                continue

            if kit != KIT_ALWAYS and kit not in kits:
                errors.append("%s: набор %d отсутствует для предмета %d" % (mod, kit, item_id))
                continue

            index += 1
            lines.append("\t<!-- %d: %s (крутость %d, %d$) -->" % (item_id, data["name"], data["cool"], data["price"]))
            lines.append("\t<IMPSHOPITEM>")
            lines.append("\t\t<uiIndex>%d</uiIndex>" % index)
            lines.append("\t\t<usItem>%d</usItem>" % item_id)
            lines.append("\t\t<usPrice>0</usPrice>")
            lines.append("\t\t<ubCoolness>%d</ubCoolness>" % data["cool"])
            lines.append("\t\t<ubKit>%d</ubKit>" % kit)
            lines.append("\t\t<ubMaxAmount>%d</ubMaxAmount>" % max_amount)
            lines.append("\t\t<fForceAvailable>0</fForceAvailable>")
            lines.append("\t\t<fRemove>0</fRemove>")
            lines.append("\t\t<szComment>%s</szComment>" % xml_escape(data["name"]))
            lines.append("\t</IMPSHOPITEM>")

    lines.append("")
    lines.append("</IMPSHOPITEMS>")
    lines.append("")

    out_path = os.path.join(out_root, mod, "TableData", "Inventory", "IMPShopItems.xml")

    out_dir = os.path.dirname(out_path)
    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)

    if errors:
        print("  ОШИБКИ:")
        for err in errors:
            print("    " + err)
        return None, index, errors

    with io.open(out_path, "w", encoding="utf-8", newline="\r\n") as handle:
        handle.write("\n".join(lines))

    return out_path, index, errors


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--game-root", default=r"D:\Projects\JA2_Game")
    parser.add_argument("--out-root", default=DEFAULT_OUT_ROOT)
    args = parser.parse_args()

    for mod, config in MODS.items():
        out_path, count, errors = build_mod(args.game_root, args.out_root, mod, config)
        if out_path:
            print("%s -> %s (%d записей)" % (mod, out_path, count))
        else:
            print("%s -> НЕ записан (%d ошибок)" % (mod, len(errors)))


if __name__ == "__main__":
    main()
