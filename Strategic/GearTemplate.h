#pragma once

#include <string>
#include <vector>
#include <iostream>
#include "types.h"
#include "isometric utils.h"

#define EQUIPMENTTEMPLATE_DIRECTORY "GearTemplate\\"

// Класс-узел для хранения данных о конкретном слоте инвентаря
class GearNode
{
public:
    GearNode(INT8 aSlot, UINT16 aItem, UINT16 aAmmoItem) :
        slot(aSlot), item(aItem), ammoitem(aAmmoItem) {
    }

    GearNode() :
        slot(NOWHERE), item(0), ammoitem(0) {
    }

    void swap(GearNode& other)
    {
        std::swap(slot, other.slot);
        std::swap(item, other.item);
        std::swap(ammoitem, other.ammoitem);
        attachments.swap(other.attachments);
    }

    INT8 slot;
    UINT16 item;
    UINT16 ammoitem;
    std::vector<UINT16> attachments;

    // Оператор вывода для сериализации в файл
    friend std::ostream& operator<<(std::ostream& stream, GearNode const& data);
};

// Основной менеджер шаблонов экипировки
class GearTemplateManager
{
private:
    std::string m_templateName;
    std::vector<GearNode> m_nodes;

    bool IO_Load(const std::string& filename);
    bool IO_Save(const std::string& filename) const;
    std::vector<std::string> GetFilesInFolder(const std::string& folder) const;

    // НОВЫЕ ПРИВАТНЫЕ МЕТОДЫ ДЛЯ СЕГМЕНТАЦИИ:
    bool LoadTemplateFile(SOLDIERTYPE* pSoldier, const std::string& name);
    void DropMismatchedItems(SOLDIERTYPE* pSoldier);
    void EquipBaseItems(SOLDIERTYPE* pSoldier);
    void OptimizeAndMergeStacks(SOLDIERTYPE* pSoldier);
    void InstallWeaponAttachments(SOLDIERTYPE* pSoldier);
    void ReloadGunsAndRefillCanteens(SOLDIERTYPE* pSoldier);
    void RefreshUI();

public:
    GearTemplateManager() = default;
    GearTemplateManager(const std::string& templateName);

    // Управление списком доступных шаблонов (бывшая GetEquipmentTemplates)
    std::vector<std::string> RefreshTemplateList() const;

    // Создание шаблона на основе текущего инвентаря наемника (бывшая WriteEquipmentTemplate)
    void CreateFromSoldier(SOLDIERTYPE* pSoldier, STR16 name);

    // Применение загруженного шаблона к наемнику (бывшая ReadEquipmentTable)
    void ApplyToSoldier(SOLDIERTYPE* pSoldier, const std::string& name);

    // Геттеры
    const std::vector<GearNode>& GetNodes() const { return m_nodes; }
    std::string GetTemplateName() const { return m_templateName; }
};
