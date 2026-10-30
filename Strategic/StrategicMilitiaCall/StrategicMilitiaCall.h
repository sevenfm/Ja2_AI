#ifndef _STRATEGIC_MILITIA_CALL_H
#define _STRATEGIC_MILITIA_CALL_H

#include "Types.h"
#include "Soldier Control.h"

namespace MilitiaCall {
	// Константы для линий под-меню смежных секторов
	enum {
		LINE_SOUTH = 0,
		LINE_EAST,
		LINE_NORTH,
		LINE_WEST,
		LINE_CANCEL,
		NUM_SECTOR_LINES = 4,
		MAX_SECTOR_REGIONS = 5
	};

	// Константы для меню выбора количества
	enum {
		NUM_MENU_5 = 0,
		NUM_MENU_10,
		NUM_MENU_15,
		NUM_MENU_20,
		NUM_MENU_30,
		NUM_MENU_40,
		NUM_MENU_ALL,
		NUM_MENU_CANCEL,
		MAX_NUMBER_REGIONS = 8
	};
}

class StrategicMilitiaCall {
private:
	// Идентификаторы всплывающих окон PopUpBox
	INT32 m_hRadioBox;
	INT32 m_hRadioSectorBox;
	INT32 m_hRadioNumberBox;

	// Внутреннее состояние сессии вызова
	UINT16 m_usRadioSectorLine[MilitiaCall::NUM_SECTOR_LINES];
	UINT16 m_usRadioSelectedSector;
	INT32  m_iRadioSectorMenuHighLine;

	// Позиции окон для сохранения координат
	SGPPoint m_RadioPosition;
	SGPPoint m_RadioSectorPosition;
	SGPPoint m_RadioNumberPosition;

	// Приватные хелперы для верстки окон (ИСПРАВЛЕНО: перенесены в класс)
	void SetupRadioBoxCommon(INT32 hBox);
	void FinishRadioBoxSetup(INT32 hBox);

public:
	StrategicMilitiaCall();
	~StrategicMilitiaCall();

	// Инициализация дескрипторов
	void Init();

	// --- Слой MODEL (Логика и проверка условий) ---
	BOOLEAN CanCharacterCallMilitia(SOLDIERTYPE* pSoldier);
	UINT16  CountAllMilitiaInSector(INT16 sMapX, INT16 sMapY);
	BOOLEAN ExecuteStrategicMilitiaCall(SOLDIERTYPE* pSoldier, UINT16 usNumber);

	// --- Слой VIEW (Отрисовка и визуальное обновление) ---
	void CreateRadioBox();
	void CreateRadioSectorBox();
	void CreateRadioNumberBox();
	void RebuildRadioSectorBox(SOLDIERTYPE* pSoldier);
	void PositionMenus(INT16 sBaseX, INT16 sBaseY);
	void HandleShadingOfLinesForRadioMenu(SOLDIERTYPE* pSoldier);
	void HandleShadingOfLinesForRadioSectorMenu();
	void HandleShadingOfLinesForRadioNumberMenu();
	void CloseAllMenus();

	// Геттеры состояния для внешних интерфейсов и контроллеров
	INT32  GetRadioBoxID() const { return m_hRadioBox; }
	INT32  GetSectorBoxID() const { return m_hRadioSectorBox; }
	INT32  GetNumberBoxID() const { return m_hRadioNumberBox; }
	UINT16 GetRadioSectorLine(INT32 iLine) const { return m_usRadioSectorLine[iLine]; }

	void   SetRadioSelectedSector(UINT16 usSector) { m_usRadioSelectedSector = usSector; }
	UINT16 GetRadioSelectedSector() const { return m_usRadioSelectedSector; }

	void   SetSectorMenuHighLine(INT32 iLine) { m_iRadioSectorMenuHighLine = iLine; }
	INT32  GetSectorMenuHighLine() const { return m_iRadioSectorMenuHighLine; }
};

// Объявляем глобальный объект фичи для использования в Assignments/Interface
extern StrategicMilitiaCall g_StrategicMilitiaCall;

#endif
