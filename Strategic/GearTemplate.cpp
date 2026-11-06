#include "GearTemplate.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <windows.h>

// Включения JA2 (скорректируйте пути под структуру вашего проекта)

#include "vfs/Core/vfs_file_raii.h"
#include "items.h"
#include "Action Items.h"
#include "handle Items.h"
#include "message.h"
#include "text.h"
#include "GameSettings.h"
#include "ShopKeeper Interface.h"
#include "Strategic Town Loyalty.h"
#include "Food.h"
#include "english.h"
#include "Interface Items.h"
#include "interface panels.h"
#include "Interface.h"
#include "Map Screen Interface Map Inventory.h"
#include "Map Screen Interface Map.h"
#include "Map Screen Interface.h"
#include "OverHead.h"
#include "SkillCheck.h"
#include "Sound Control.h"

// Внешние функции и переменные движка JA2
extern std::vector<WORLDITEM> pInventoryPoolList;
extern INT32 GetAttachmentInfoIndex(UINT16 usItem);
extern INT32 giItemDescAmmoButton;
extern BOOL GetBetterObject_InventoryPool(UINT16 usItem, INT16 status, UINT32& arLoop, UINT8& arIndex);
extern BOOL GetFittingAmmo_InventoryPool(UINT8 usCalibre, UINT8 usAmmoType, UINT32& arLoop);
extern BOOLEAN HandleNailsVestFetish(SOLDIERTYPE* pSoldier, UINT32 uiHandPos, UINT16 usReplaceItem);

// Глобальный вектор имен шаблонов
extern std::vector<std::string> gTemplateVector;

// Реализация оператора записи в поток для GearNode
std::ostream& operator<<(std::ostream& stream, GearNode const& data)
{
    stream << (int)(data.slot) << " " << (UINT16)(data.item) << " " << (UINT16)(data.ammoitem);
    for (UINT16 att : data.attachments)
    {
        stream << " " << att;
    }
    stream << std::endl;
    return stream;
}

GearTemplateManager::GearTemplateManager(const std::string& templateName)
    : m_templateName(templateName)
{
}

// Приватный метод низкоуровневой загрузки файла с диска
bool GearTemplateManager::IO_Load(const std::string& filename)
{
    m_nodes.clear();
    std::ifstream file(filename, std::ios::in);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file >> std::ws, line))
    {
        if (line.empty() || line[0] == '#' || line[0] == '/')
            continue;

        std::istringstream iss(line);
        GearNode node;
        int slotNum;

        if (iss >> slotNum && iss >> node.item)
        {
            node.slot = static_cast<INT8>(slotNum);

            if (iss >> node.ammoitem)
            {
                int attNum;
                while (iss >> attNum)
                {
                    node.attachments.push_back(static_cast<UINT16>(attNum));
                }
            }
            m_nodes.push_back(node);
        }
    }
    file.close();
    return true;
}

// Приватный метод низкоуровневого сохранения файла на диск
bool GearTemplateManager::IO_Save(const std::string& filename) const
{
    std::fstream binary_file(filename, std::ios::out);
    if (!binary_file.is_open()) return false;

    for (const auto& node : m_nodes)
    {
        binary_file << node;
    }
    binary_file.close();
    return true;
}

// Поиск файлов (Windows API)
std::vector<std::string> GearTemplateManager::GetFilesInFolder(const std::string& folder) const
{
    std::vector<std::string> names;
    std::string search_path = folder + "/*.*";
    WIN32_FIND_DATA fd;
    HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                names.push_back(fd.cFileName);
            }
        } while (::FindNextFile(hFind, &fd));

        ::FindClose(hFind);
    }
    return names;
}

// Обновление глобального gTemplateVector
std::vector<std::string> GearTemplateManager::RefreshTemplateList() const
{
    CHAR8 filename[MAX_PATH];
    sprintf(filename, "%sUpdate Gear.txt", EQUIPMENTTEMPLATE_DIRECTORY);

    vfs::Path vfsPath;
    vfs::COpenWriteFile rfile(filename, true);
    rfile->_getRealPath(vfsPath);

    vfs::Path dir, file;
    vfsPath.splitLast(dir, file);
    std::string folderPath = dir.to_string();

    gTemplateVector = GetFilesInFolder(folderPath);
    return gTemplateVector;
}

// Запись инвентаря наемника в шаблон
void GearTemplateManager::CreateFromSoldier(SOLDIERTYPE* pSoldier, STR16 name)
{
    if (!pSoldier) return;

    m_nodes.clear();
    INT8 invsize = static_cast<INT8>(pSoldier->inv.size());

    for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
    {
        if (pSoldier->inv[bLoop].exists())
        {
            UINT16 ammoitem = 0;
            if (Item[(pSoldier->inv[bLoop]).usItem].usItemClass & IC_GUN)
                ammoitem = (pSoldier->inv[bLoop])[0]->data.gun.usGunAmmoItem;

            GearNode node(bLoop, (pSoldier->inv[bLoop]).usItem, ammoitem);

            auto iterend = (pSoldier->inv[bLoop])[0]->attachments.end();
            for (auto iter = (pSoldier->inv[bLoop])[0]->attachments.begin(); iter != iterend; ++iter)
            {
                if (iter->exists())
                    node.attachments.push_back(iter->usItem);
            }
            m_nodes.push_back(node);
        }
    }

    CHAR8 filepath[MAX_PATH];
    sprintf(filepath, "%s%S.txt", EQUIPMENTTEMPLATE_DIRECTORY, name);

    vfs::Path vfsPath;
    vfs::COpenWriteFile rfile(filepath, true);
    rfile->_getRealPath(vfsPath);

    IO_Save(vfsPath.to_string());
}

// =========================================================================
// ГЛАВНЫЙ КОРНЕВОЙ МЕТОД: Управление шагами применения шаблона
// =========================================================================
void GearTemplateManager::ApplyToSoldier(SOLDIERTYPE* pSoldier, const std::string& name)
{
    if (!pSoldier) return;

    // Шаг 0: Валидация условий тактики и загрузка файла шаблона
    if (!LoadTemplateFile(pSoldier, name)) return;

    // Шаг 1: Раздевание наемника (освобождение слотов, где тип вещи не совпадает)
    DropMismatchedItems(pSoldier);

    // Шаг 2: Базовое одевание (оружие, броня, разгрузки из пула сектора)
    EquipBaseItems(pSoldier);

    // Шаг 3: Слияние/оптимизация стаков и обновление качества предметов
    OptimizeAndMergeStacks(pSoldier);

    // Шаг 4: Установка навесного оборудования и обвесов
    InstallWeaponAttachments(pSoldier);

    // Шаг 5: Секторная перезарядка оружия и наполнение фляг
    ReloadGunsAndRefillCanteens(pSoldier);

    // Шаг 6: Обновление графического интерфейса
    RefreshUI();
}

// =========================================================================
// ИЗОЛИРОВАННЫЕ ШАГИ РЕФАКТОРИНГА
// =========================================================================

bool GearTemplateManager::LoadTemplateFile(SOLDIERTYPE* pSoldier, const std::string& name)
{
    UINT16 color = FONT_MCOLOR_WHITE;

    if (pSoldier->sSectorX != sSelMapX || pSoldier->sSectorY != sSelMapY || pSoldier->bSectorZ != iCurrentMapSectorZ)
    {
        ScreenMsg(color, MSG_INTERFACE, szGearTemplateText[3], pSoldier->GetName());
        return false;
    }

    if ((gTacticalStatus.uiFlags & INCOMBAT || gTacticalStatus.fEnemyInSector))
    {
        ScreenMsg(color, MSG_INTERFACE, szGearTemplateText[1]);
        return false;
    }

    CHAR8 filepath[MAX_PATH];
    sprintf(filepath, "%s%s", EQUIPMENTTEMPLATE_DIRECTORY, name.c_str());

    vfs::Path vfsPath;
    vfs::COpenWriteFile rfile(filepath, true);
    rfile->_getRealPath(vfsPath);

    if (!IO_Load(vfsPath.to_string()))
    {
        ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"Gear Template '%S' NOT FOUND!", name.c_str());
        return false;
    }

    // Сообщение-подтверждение успешной загрузки узлов
    ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"Template '%S' loaded successfully. Nodes: %d", name.c_str(), m_nodes.size());
    return true;
}

void GearTemplateManager::DropMismatchedItems(SOLDIERTYPE* pSoldier)
{
    if (!pSoldier) return;

    // Проверяем каждый слот инвентаря наемника по отдельности
    for (int slot = HELMETPOS; slot < NUM_INV_SLOTS; ++slot)
    {
        // Ищем, есть ли данный слот в загруженном конфиге шаблона
        auto it = std::find_if(m_nodes.begin(), m_nodes.end(), [slot](const GearNode& node) {
            return node.slot == slot;
            });

        // Если слот ВООБЩЕ отсутствует в конфиге шаблона — ИГНОРИРУЕМ его (узел пропущен)
        if (it == m_nodes.end())
        {
            continue;
        }

        // Если мы дошли сюда, значит слот есть в конфиге. Получаем узел шаблона.
        const GearNode& templateNode = *it;

        // Логика очистки срабатывает, если у наемника в слоте что-то есть, И:
        // Либо в конфиге явно написано 0 (templateNode.item == NOTHING)
        // Либо предмет на наемнике не совпадает с тем, что указан в конфиге
        if (pSoldier->inv[slot].exists())
        {
            if (templateNode.item == NOTHING || pSoldier->inv[slot].usItem != templateNode.item)
            {
                // Перед очисткой разгрузочной системы (LBE), бережно вытряхиваем её карманы
                if (slot >= VESTPOCKPOS && slot <= BPACKPOCKPOS)
                {
                    std::vector<INT8> pocketKey;
                    GetLBESlots(slot, pocketKey);

                    for (INT8 slotId : pocketKey)
                    {
                        if (pSoldier->inv[slotId].exists())
                        {
                            AutoPlaceObjectInInventoryStash(&pSoldier->inv[slotId], pSoldier->sGridNo, pSoldier->pathing.bLevel);
                            DeleteObj(&pSoldier->inv[slotId]);
                        }
                    }
                }
                // Если очищаем основную руку, а оружие двуручное — убираем зависимость со второй руки
                else if (slot == HANDPOS && ItemIsTwoHanded(pSoldier->inv[HANDPOS].usItem) && pSoldier->inv[SECONDHANDPOS].exists())
                {
                    AutoPlaceObjectInInventoryStash(&pSoldier->inv[SECONDHANDPOS], pSoldier->sGridNo, pSoldier->pathing.bLevel);
                    DeleteObj(&pSoldier->inv[SECONDHANDPOS]);
                }
                // Проверка на квестовый жилет Нейлса
                else if (slot == VESTPOS && HandleNailsVestFetish(pSoldier, slot, NOTHING))
                {
                    continue;
                }

                // Выбрасываем предмет на склад/землю сектора и полностью удаляем его из слота наемника
                AutoPlaceObjectInInventoryStash(&pSoldier->inv[slot], pSoldier->sGridNo, pSoldier->pathing.bLevel);
                DeleteObj(&pSoldier->inv[slot]);
            }
        }
    }
}


void GearTemplateManager::EquipBaseItems(SOLDIERTYPE* pSoldier)
{
    UINT16 color = FONT_MCOLOR_WHITE;

    for (const auto& node : m_nodes)
    {
        if (node.slot >= 0 && node.slot < NUM_INV_SLOTS)
        {
            if (!pSoldier->inv[node.slot].exists() && node.item != NOTHING)
            {
                UINT32 poolslot = 0;
                UINT8 index = 0;
                if (GetBetterObject_InventoryPool(node.item, 0, poolslot, index))
                {
                    (pInventoryPoolList[poolslot].object).RemoveObjectAtIndex(index, &gItemPointer);

                    if (!PlaceObject(pSoldier, node.slot, &gItemPointer))
                    {
                    	ScreenMsg(color, MSG_INTERFACE, szGearTemplateText[4], pSoldier->GetName(), Item[node.item].szItemName);
                        AutoPlaceObjectInInventoryStash(&gItemPointer, pSoldier->sGridNo, pSoldier->pathing.bLevel);
                        DeleteObj(&gItemPointer);
                    }
                }
                else
                {
                    if (Item[node.item].usItemClass & IC_AMMO)
                    {
                        UINT16 usMagIndex = Item[node.item].ubClassIndex;
                        UINT16 neededammo = Magazine[usMagIndex].ubMagSize;
                        if (neededammo > 0)
                        {
                            UINT32 poolslotAlt = 0;
                            if (GetFittingAmmo_InventoryPool(Magazine[usMagIndex].ubCalibre, Magazine[usMagIndex].ubAmmoType, poolslotAlt))
                            {
                                UINT32 takeammo = min(neededammo, pInventoryPoolList[poolslotAlt].object[pInventoryPoolList[poolslotAlt].object.ubNumberOfObjects - 1]->data.ubShotsLeft);
                                pInventoryPoolList[poolslotAlt].object[pInventoryPoolList[poolslotAlt].object.ubNumberOfObjects - 1]->data.ubShotsLeft -= takeammo;
                                if (!pInventoryPoolList[poolslotAlt].object[pInventoryPoolList[poolslotAlt].object.ubNumberOfObjects - 1]->data.ubShotsLeft)
                                {
                                    pInventoryPoolList[poolslotAlt].object.RemoveObjectAtIndex(pInventoryPoolList[poolslotAlt].object.ubNumberOfObjects - 1);
                                }
                                CreateAmmo(node.item, &gItemPointer, takeammo);
                                if (!PlaceObject(pSoldier, node.slot, &gItemPointer))
                                {
                                    ScreenMsg(color, MSG_INTERFACE, szGearTemplateText[4], pSoldier->GetName(), Item[node.item].szItemName);
                                    AutoPlaceObjectInInventoryStash(&gItemPointer, pSoldier->sGridNo, pSoldier->pathing.bLevel);
                                    DeleteObj(&gItemPointer);
                                }
                            }
                        }
                    }
                }
            }
            // Ранняя установка обвесов для разгрузочных систем (LBE)
            if (pSoldier->inv[node.slot].exists() && node.slot >= VESTPOCKPOS && node.slot <= BPACKPOCKPOS)
            {
                if (!node.attachments.empty() && ((pSoldier->inv[node.slot]).usItem == node.item || NOTHING == node.item))
                {
                    OBJECTTYPE* pObj = &(pSoldier->inv[node.slot]);
                    for (INT16 i = 0; i < pObj->ubNumberOfObjects; ++i)
                    {
                        for (UINT16 item_attachment : node.attachments)
                        {
                            INT32 bAttachInfoIndex = GetAttachmentInfoIndex(item_attachment);
                            if (bAttachInfoIndex < 0 || AttachmentInfo[bAttachInfoIndex].bAttachmentSkillCheck == NO_CHECK)
                            {
                                UINT32 poolslotAttach = 0;
                                UINT8 indexAttach = 0;
                                if (GetBetterObject_InventoryPool(item_attachment, 0, poolslotAttach, indexAttach))
                                {
                                    (pInventoryPoolList[poolslotAttach].object).RemoveObjectAtIndex(indexAttach, &gItemPointer);
                                    if (!pObj->AttachObject(pSoldier, &gItemPointer, FALSE, i))
                                    {
                                        AutoPlaceObjectInInventoryStash(&gItemPointer, pSoldier->sGridNo, pSoldier->pathing.bLevel);
                                        DeleteObj(&gItemPointer);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
void GearTemplateManager::OptimizeAndMergeStacks(SOLDIERTYPE* pSoldier)
{
    for (int slot = HELMETPOS; slot < NUM_INV_SLOTS; ++slot)
    {
        if (pSoldier->inv[slot].exists())
        {
            OBJECTTYPE* pObj = &(pSoldier->inv[slot]);
            if (Item[pObj->usItem].usItemClass & (IC_KIT | IC_MEDKIT) && pObj->ubNumberOfObjects > 1)
            {
                CleanUpStack(pObj, pObj);
            }
            if ((Item[pObj->usItem].usItemClass & IC_AMMO) && pObj->ubNumberOfObjects > 1)
            {
                UINT16 usMagIndex = Item[pObj->usItem].ubClassIndex;
                UINT16 magsize = Magazine[usMagIndex].ubMagSize;
                if (magsize)
                {
                    UINT32 ammocount = 0;
                    for (INT16 i = 0; i < pObj->ubNumberOfObjects; ++i)
                        ammocount += (*pObj)[i]->data.ubShotsLeft;
                    UINT8 fullmags = ammocount / magsize;
                    UINT16 leftover = ammocount - fullmags * magsize;
                    UINT8 totalmags = fullmags + (leftover > 0 ? 1 : 0);
                    if (totalmags < pObj->ubNumberOfObjects)
                        pObj->RemoveObjectsFromStack(pObj->ubNumberOfObjects - totalmags);
                    for (INT16 i = 0; i < totalmags; ++i)
                        (*pObj)[i]->data.ubShotsLeft = magsize;
                    if (leftover > 0)
                        (*pObj)[0]->data.ubShotsLeft = leftover;
                }
            }
            // Повышение износа/качества предметов за счет лучшего эквивалента из сектора
            for (INT16 i = 0; i < pObj->ubNumberOfObjects; ++i)
            {
                UINT32 poolslotUpgrade = 0;
                UINT8 indexUpgrade = 0;
                if ((*pObj)[i]->data.objectStatus < 100 && GetBetterObject_InventoryPool(pObj->usItem, (*pObj)[i]->data.objectStatus, poolslotUpgrade, indexUpgrade))
                {
                    std::swap((*pObj)[i]->data.objectStatus, (pInventoryPoolList[poolslotUpgrade].object)[indexUpgrade]->data.objectStatus);
                    std::swap((*pObj)[i]->data.bTrap, (pInventoryPoolList[poolslotUpgrade].object)[indexUpgrade]->data.bTrap);
                    std::swap((*pObj)[i]->data.fUsed, (pInventoryPoolList[poolslotUpgrade].object)[indexUpgrade]->data.fUsed);
                    std::swap((*pObj)[i]->data.ubImprintID, (pInventoryPoolList[poolslotUpgrade].object)[indexUpgrade]->data.ubImprintID);
                    std::swap((*pObj)[i]->data.bTemperature, (pInventoryPoolList[poolslotUpgrade].object)[indexUpgrade]->data.bTemperature);
                    std::swap((*pObj)[i]->data.ubDirection, (pInventoryPoolList[poolslotUpgrade].object)[indexUpgrade]->data.ubDirection);
                    std::swap((*pObj)[i]->data.ubWireNetworkFlag, (pInventoryPoolList[poolslotUpgrade].object)[indexUpgrade]->data.ubWireNetworkFlag);
                    std::swap((*pObj)[i]->data.bDefuseFrequency, (pInventoryPoolList[poolslotUpgrade].object)[indexUpgrade]->data.bDefuseFrequency);
                    std::swap((*pObj)[i]->data.sRepairThreshold, (pInventoryPoolList[poolslotUpgrade].object)[indexUpgrade]->data.sRepairThreshold);
                    std::swap((*pObj)[i]->data.sObjectFlag, (pInventoryPoolList[poolslotUpgrade].object)[indexUpgrade]->data.sObjectFlag);
                }
            }
            // Заполнение неполных стеков до максимума
            UINT8 ubSlotLimit = ItemSlotLimit(pObj, slot, pSoldier, FALSE);
            while (pObj->ubNumberOfObjects < ubSlotLimit)
            {
                UINT32 poolslotFill = 0;
                UINT8 indexFill = 0;
                if (Item[pObj->usItem].usItemClass & IC_AMMO)
                {
                    UINT16 usMagIndex = Item[pObj->usItem].ubClassIndex;
                    UINT16 neededammo = Magazine[usMagIndex].ubMagSize - (*pObj)[pObj->ubNumberOfObjects - 1]->data.ubShotsLeft;
                    if (neededammo > 0 && GetFittingAmmo_InventoryPool(Magazine[usMagIndex].ubCalibre, Magazine[usMagIndex].ubAmmoType, poolslotFill))
                    {
                        for (INT16 j = pInventoryPoolList[poolslotFill].object.ubNumberOfObjects - 1; j >= 0; --j)
                        {
                            UINT32 takeammo = min(neededammo, pInventoryPoolList[poolslotFill].object[j]->data.ubShotsLeft);
                            neededammo -= takeammo;
                            pInventoryPoolList[poolslotFill].object[j]->data.ubShotsLeft -= takeammo;
                            (*pObj)[pObj->ubNumberOfObjects - 1]->data.ubShotsLeft += takeammo;
                            if (!pInventoryPoolList[poolslotFill].object[j]->data.ubShotsLeft)
                                pInventoryPoolList[poolslotFill].object.RemoveObjectAtIndex(j);
                            if (!neededammo) break;
                        }
                        continue;
                    }
                    break;
                }
                else if (GetBetterObject_InventoryPool(pObj->usItem, 0, poolslotFill, indexFill))
                {
                    (pInventoryPoolList[poolslotFill].object).RemoveObjectAtIndex(indexFill, &gItemPointer);
                    if (!TryToStackInSlot(pSoldier, &gItemPointer, slot))
                        break;
                }
                else
                {
                    break;
                }
            }
            // Апгрейд состояния встроенных обвесов
            for (INT16 i = 0; i < pObj->ubNumberOfObjects; ++i)
            {
                for (auto& att : (*pObj)[i]->attachments)
                {
                    if (att.exists())
                    {
                        UINT32 poolslotAttUpgrade = 0;
                        UINT8 indexAttUpgrade = 0;

                        // ИСПРАВЛЕНО: Заменено att->data на att[0]->data
                        if (att[0]->data.objectStatus < 100 && GetBetterObject_InventoryPool(att.usItem, att[0]->data.objectStatus, poolslotAttUpgrade, indexAttUpgrade))
                        {
                            // ИСПРАВЛЕНО: Везде заменено att->data на att[0]->data для корректного доступа к полям структуры предмета
                            std::swap(att[0]->data.objectStatus, (pInventoryPoolList[poolslotAttUpgrade].object)[indexAttUpgrade]->data.objectStatus);
                            std::swap(att[0]->data.bTrap, (pInventoryPoolList[poolslotAttUpgrade].object)[indexAttUpgrade]->data.bTrap);
                            std::swap(att[0]->data.fUsed, (pInventoryPoolList[poolslotAttUpgrade].object)[indexAttUpgrade]->data.fUsed);
                            std::swap(att[0]->data.ubImprintID, (pInventoryPoolList[poolslotAttUpgrade].object)[indexAttUpgrade]->data.ubImprintID);
                            std::swap(att[0]->data.bTemperature, (pInventoryPoolList[poolslotAttUpgrade].object)[indexAttUpgrade]->data.bTemperature);
                            std::swap(att[0]->data.ubDirection, (pInventoryPoolList[poolslotAttUpgrade].object)[indexAttUpgrade]->data.ubDirection);
                            std::swap(att[0]->data.ubWireNetworkFlag, (pInventoryPoolList[poolslotAttUpgrade].object)[indexAttUpgrade]->data.ubWireNetworkFlag);
                            std::swap(att[0]->data.bDefuseFrequency, (pInventoryPoolList[poolslotAttUpgrade].object)[indexAttUpgrade]->data.bDefuseFrequency);
                            std::swap(att[0]->data.sRepairThreshold, (pInventoryPoolList[poolslotAttUpgrade].object)[indexAttUpgrade]->data.sRepairThreshold);
                            std::swap(att[0]->data.sObjectFlag, (pInventoryPoolList[poolslotAttUpgrade].object)[indexAttUpgrade]->data.sObjectFlag);
                        }
                    }
                }
            }
        }
    }
}
void GearTemplateManager::InstallWeaponAttachments(SOLDIERTYPE* pSoldier)
{
    for (const auto& node : m_nodes)
    {
        if (node.slot >= 0 && node.slot < NUM_INV_SLOTS && (node.slot < VESTPOCKPOS || node.slot > BPACKPOCKPOS))
        {
            if (!node.attachments.empty() && pSoldier->inv[node.slot].exists() && ((pSoldier->inv[node.slot]).usItem == node.item || NOTHING == node.item))
            {
                OBJECTTYPE* pObj = &(pSoldier->inv[node.slot]);
                for (INT16 i = 0; i < pObj->ubNumberOfObjects; ++i)
                {
                    for (UINT16 item_attachment : node.attachments)
                    {
                        INT32 bAttachInfoIndex = GetAttachmentInfoIndex(item_attachment);
                        if (bAttachInfoIndex < 0 || AttachmentInfo[bAttachInfoIndex].bAttachmentSkillCheck == NO_CHECK)
                        {
                            UINT32 poolslotFinalAtt = 0;
                            UINT8 indexFinalAtt = 0;
                            if (GetBetterObject_InventoryPool(item_attachment, 0, poolslotFinalAtt, indexFinalAtt))
                            {
                                (pInventoryPoolList[poolslotFinalAtt].object).RemoveObjectAtIndex(indexFinalAtt, &gItemPointer);
                                if (!pObj->AttachObject(pSoldier, &gItemPointer, FALSE, i))
                                {
                                    AutoPlaceObjectInInventoryStash(&gItemPointer, pSoldier->sGridNo, pSoldier->pathing.bLevel);
                                    DeleteObj(&gItemPointer);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
void GearTemplateManager::ReloadGunsAndRefillCanteens(SOLDIERTYPE* pSoldier)
{
    UINT16 usReloadSound = 0;
    UINT8 waterquality = GetWaterQuality(sSelMapX, sSelMapY, iCurrentMapSectorZ);
    BOOLEAN refillwaterfromsector = (waterquality == WATER_DRINKABLE);
    FLOAT wateraddtemperature = OVERHEATING_MAX_TEMPERATURE;
    for (int slot = HELMETPOS; slot < NUM_INV_SLOTS; ++slot)
    {
        if (pSoldier->inv[slot].exists())
        {
            if (Item[(pSoldier->inv[slot]).usItem].usItemClass & (IC_KIT | IC_MEDKIT))
            {
                UINT32 poolslotKit = 0;
                UINT8 indexKit = 0;
                if (GetBetterObject_InventoryPool((pSoldier->inv[slot]).usItem, 0, poolslotKit, indexKit))
                {
                    DistributeStatus(&(pInventoryPoolList[poolslotKit].object), &(pSoldier->inv[slot]), 100);
                }
            }
            if (refillwaterfromsector && ItemIsCanteen(pSoldier->inv[slot].usItem) && Food[Item[(pSoldier->inv[slot]).usItem].foodtype].bDrinkPoints > 0)
            {
                OBJECTTYPE* pObj = &(pSoldier->inv[slot]);
                for (INT16 i = 0; i < pObj->ubNumberOfObjects; ++i)
                {
                    UINT16 status = (*pObj)[i]->data.objectStatus;
                    UINT16 statusmmissing = max(0, 100 - status);
                    FLOAT temperature = (*pObj)[i]->data.bTemperature;
                    (*pObj)[i]->data.objectStatus = 100;
                    (*pObj)[i]->data.bTemperature = (status * temperature + statusmmissing * wateraddtemperature) / 100;
                }
            }
            if ((Item[(pSoldier->inv[slot]).usItem].usItemClass & (IC_GUN | IC_AMMO)))
            {
                OBJECTTYPE* pObj = &(pSoldier->inv[slot]);
                for (INT16 i = 0; i < pObj->ubNumberOfObjects; ++i)
                {
                    UINT16 ammoitem = (pSoldier->inv[slot]).usItem;
                    if ((Item[(pSoldier->inv[slot]).usItem].usItemClass & IC_GUN))
                    {
                        UINT8 calibre = Weapon[pObj->usItem].ubCalibre;
                        UINT16 magsize = GetMagSize(pObj, i);
                        ammoitem = (*pObj)[i]->data.gun.usGunAmmoItem;
                        for (const auto& n : m_nodes)
                        {
                            if (n.slot == slot && n.ammoitem != NOTHING && (n.item == pObj->usItem || n.item == NOTHING))
                            {
                                ammoitem = n.ammoitem;
                                if (n.item == pObj->usItem) break;
                            }
                        }
                        if (!ammoitem) ammoitem = DefaultMagazine(pObj->usItem);
                        UINT16 usMagIndex = Item[ammoitem].ubClassIndex;
                        if (calibre != Magazine[usMagIndex].ubCalibre || magsize != Magazine[usMagIndex].ubMagSize)
                        {
                            ammoitem = FindReplacementMagazine(calibre, magsize, Magazine[usMagIndex].ubAmmoType);
                            usMagIndex = Item[ammoitem].ubClassIndex;
                        }
                        if ((*pObj)[i]->data.gun.usGunAmmoItem != ammoitem && (*pObj)[i]->data.gun.usGunAmmoItem != NOTHING && ammoitem != NOTHING)
                        {
                            CreateAmmo((*pObj)[i]->data.gun.usGunAmmoItem, &gItemPointer, (*pObj)[i]->data.gun.ubGunShotsLeft);
                            AutoPlaceObjectInInventoryStash(&gItemPointer, pSoldier->sGridNo, pSoldier->pathing.bLevel);
                            DeleteObj(&gItemPointer);
                            (*pObj)[i]->data.gun.usGunAmmoItem = NOTHING;
                            (*pObj)[i]->data.gun.ubGunShotsLeft = 0;
                            (*pObj)[i]->data.gun.ubGunAmmoType = 0;
                        }
                    }
                    UINT16 usMagIndex = Item[ammoitem].ubClassIndex;
                    UINT16 neededammo = (Item[(pSoldier->inv[slot]).usItem].usItemClass & IC_GUN) ?
                        Magazine[usMagIndex].ubMagSize - (*pObj)[i]->data.gun.ubGunShotsLeft :
                        Magazine[usMagIndex].ubMagSize - (*pObj)[i]->data.ubShotsLeft;
                    while (neededammo > 0)
                    {
                        UINT32 poolslotAmmoFinal = 0;
                        if (GetFittingAmmo_InventoryPool(Magazine[usMagIndex].ubCalibre, Magazine[usMagIndex].ubAmmoType, poolslotAmmoFinal))
                        {
                            for (INT16 j = pInventoryPoolList[poolslotAmmoFinal].object.ubNumberOfObjects - 1; j >= 0; --j)
                            {
                                UINT32 takeammo = min(neededammo, pInventoryPoolList[poolslotAmmoFinal].object[j]->data.ubShotsLeft);
                                neededammo -= takeammo;
                                pInventoryPoolList[poolslotAmmoFinal].object[j]->data.ubShotsLeft -= takeammo;
                                if ((Item[(pSoldier->inv[slot]).usItem].usItemClass & IC_GUN))
                                {
                                    (*pObj)[i]->data.gun.usGunAmmoItem = ammoitem;
                                    (*pObj)[i]->data.gun.ubGunAmmoType = Magazine[usMagIndex].ubAmmoType;
                                    (*pObj)[i]->data.gun.ubGunShotsLeft += takeammo;
                                    usReloadSound = Weapon[pObj->usItem].sReloadSound;
                                }
                                else
                                {
                                    (*pObj)[i]->data.ubShotsLeft += takeammo;
                                }
                                if (!pInventoryPoolList[poolslotAmmoFinal].object[j]->data.ubShotsLeft)
                                {
                                    pInventoryPoolList[poolslotAmmoFinal].object.RemoveObjectAtIndex(j);
                                }
                                if (!neededammo) break;
                            }
                        }
                        else break;
                        neededammo = (Item[(pSoldier->inv[slot]).usItem].usItemClass & IC_GUN) ?
                            Magazine[usMagIndex].ubMagSize - (*pObj)[i]->data.gun.ubGunShotsLeft :
                            Magazine[usMagIndex].ubMagSize - (*pObj)[i]->data.ubShotsLeft;
                    }
                }
            }
        }
    }
    if (usReloadSound != 0)
        PlayJA2Sample(usReloadSound, RATE_11025, HIGHVOLUME, 1, MIDDLEPAN);
}

void GearTemplateManager::RefreshUI()
{
    fTeamPanelDirty = TRUE;
    fMapPanelDirty = TRUE;
    fInterfacePanelDirty = DIRTYLEVEL2;
    fCharacterInfoPanelDirty = TRUE;
    if (giItemDescAmmoButton > -1)
        MarkAButtonDirty(giItemDescAmmoButton);
}