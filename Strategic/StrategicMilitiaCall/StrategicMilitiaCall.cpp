#include "StrategicMilitiaCall.h"
#include "PopUpBox.h"
#include "Font Control.h"
#include "Assignments.h"
#include "Strategic Movement.h"
#include "strategicmap.h"
#include "Town Militia.h"
#include "Campaign Types.h"
#include "Text.h"
#include "Message.h"
#include "Sound Control.h"
#include "Map Screen Interface.h"
#include "Map Screen Helicopter.h"
#include "Interface Control.h"
#include "Interface Panels.h"
#include "GameSettings.h"
#include "English.h"
#include "MilitiaSquads.h"

StrategicMilitiaCall g_StrategicMilitiaCall;

StrategicMilitiaCall::StrategicMilitiaCall() {
	m_hRadioBox = -1;
	m_hRadioSectorBox = -1;
	m_hRadioNumberBox = -1;
	m_usRadioSelectedSector = 0xFFFF;
	m_iRadioSectorMenuHighLine = -1;

	for (int i = 0; i < MilitiaCall::NUM_SECTOR_LINES; i++) {
		m_usRadioSectorLine[i] = 0xFFFF;
	}
}

StrategicMilitiaCall::~StrategicMilitiaCall() {
	CloseAllMenus();
}

void StrategicMilitiaCall::Init() {
	m_usRadioSelectedSector = 0xFFFF;
	m_iRadioSectorMenuHighLine = -1;
}

void StrategicMilitiaCall::SetupRadioBoxCommon(INT32 hBox) {
	SetBoxBuffer(hBox, FRAME_BUFFER);
	SetBorderType(hBox, guiPOPUPBORDERS);
	SetBackGroundSurface(hBox, guiPOPUPTEX);
	SetMargins(hBox, 6, 6, 4, 4);
	SetLineSpace(hBox, 2);
	SetCurrentBox(hBox);
}

void StrategicMilitiaCall::FinishRadioBoxSetup(INT32 hBox) {
	SetBoxFont(hBox, MAP_SCREEN_FONT);
	SetBoxHighLight(hBox, FONT_WHITE);
	SetBoxForeground(hBox, FONT_LTGREEN);
	SetBoxBackground(hBox, FONT_BLACK);
	SetBoxShade(hBox, FONT_GRAY7);
	ResizeBoxToText(hBox);
}

BOOLEAN StrategicMilitiaCall::CanCharacterCallMilitia(SOLDIERTYPE* pSoldier) {
	if (!pSoldier || !gGameExternalOptions.gfAllowReinforcements) return FALSE;
	if (pSoldier->stats.bLife < OKLIFE || pSoldier->bSectorZ != 0) return FALSE;
	if (IsCharacterInTransit(pSoldier) || pSoldier->flags.fBetweenSectors) return FALSE;
	if (pSoldier->ubWhatKindOfMercAmI == MERC_TYPE__EPC) return FALSE;

	if (pSoldier->bAssignment == VEHICLE) {
		if ((iHelicopterVehicleId != -1) && (pSoldier->iVehicleId == iHelicopterVehicleId)) {
			return FALSE;
		}
	}
	return pSoldier->CanUseRadio(FALSE);
}

UINT16 StrategicMilitiaCall::CountAllMilitiaInSector(INT16 sMapX, INT16 sMapY) {
	SECTORINFO* pSectorInfo = &(SectorInfo[SECTOR(sMapX, sMapY)]);
	return pSectorInfo->ubNumberOfCivsAtLevel[GREEN_MILITIA] +
		pSectorInfo->ubNumberOfCivsAtLevel[REGULAR_MILITIA] +
		pSectorInfo->ubNumberOfCivsAtLevel[ELITE_MILITIA];
}

BOOLEAN StrategicMilitiaCall::ExecuteStrategicMilitiaCall(SOLDIERTYPE* pSoldier, UINT16 usNumber) {
	if (!pSoldier || usNumber == 0 || m_usRadioSelectedSector == 0xFFFF) return FALSE;

	INT16 sSrcX = SECTORX(m_usRadioSelectedSector);
	INT16 sSrcY = SECTORY(m_usRadioSelectedSector);

	if (abs(sSrcX - pSoldier->sSectorX) + abs(sSrcY - pSoldier->sSectorY) != 1) return FALSE;

	BOOLEAN fOperatorInLoadedSector = (pSoldier->sSectorX == gWorldSectorX) &&
		(pSoldier->sSectorY == gWorldSectorY) &&
		(pSoldier->bSectorZ == gbWorldSectorZ);

	if (fOperatorInLoadedSector) {
		return pSoldier->RadioCallReinforcements(m_usRadioSelectedSector, usNumber);
	}

	if (CountAllMilitiaInSector(sSrcX, sSrcY) == 0) return FALSE;

	if (!pSoldier->UseRadio(SKILLS_RADIO_CALLREINFORCEMENTS)) {
		ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, New113Message[MSG113_RADIO_ACTION_FAILED]);
		PlayJA2SampleFromFile("Sounds\\radioerror.wav", RATE_11025, MIDVOLUME, 1, MIDDLEPAN);
		return FALSE;
	}

	pSoldier->SwitchOffRadio();

	if (!CallMilitiaReinforcementsNoTactical(pSoldier->sSectorX, pSoldier->sSectorY, sSrcX, sSrcY, usNumber)) {
		return FALSE;
	}

	CHAR16 pStr[64];
	GetSectorIDString(sSrcX, sSrcY, 0, pStr, FALSE);
	ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, New113Message[MSG113_ORDERS_REINFORCEMENTS], pSoldier->name, pStr);
	PlayJA2SampleFromFile("Sounds\\scan1.wav", RATE_11025, MIDVOLUME, 1, MIDDLEPAN);

	return TRUE;
}

void StrategicMilitiaCall::CreateRadioBox() {
	UINT32 hStringHandle;
	CreatePopUpBox(&m_hRadioBox, RadioDimensions, RadioPosition, (POPUP_BOX_FLAG_CLIP_TEXT | POPUP_BOX_FLAG_CENTER_TEXT | POPUP_BOX_FLAG_RESIZE));
	ghRadioBox = m_hRadioBox;

	SetupRadioBoxCommon(m_hRadioBox);

	for (UINT32 i = 0; i < MAX_RADIO_MENU_STRING_COUNT; i++) {
		AddMonoString(&hStringHandle, pRadioMenuStrings[i]);
		UnHighLightLine(hStringHandle);
	}

	FinishRadioBoxSetup(m_hRadioBox);
	DetermineBoxPositions();
}

void StrategicMilitiaCall::CreateRadioSectorBox() {
	UINT32 hStringHandle;
	CreatePopUpBox(&m_hRadioSectorBox, RadioSectorDimensions, RadioSectorPosition, (POPUP_BOX_FLAG_CLIP_TEXT | POPUP_BOX_FLAG_CENTER_TEXT | POPUP_BOX_FLAG_RESIZE));
	ghRadioSectorBox = m_hRadioSectorBox;

	SetupRadioBoxCommon(m_hRadioSectorBox);

	for (UINT32 i = 0; i < MilitiaCall::NUM_SECTOR_LINES; i++) {
		AddMonoString(&hStringHandle, L"-");
		UnHighLightLine(hStringHandle);
	}
	AddMonoString(&hStringHandle, pRadioMenuStrings[RADIO_MENU_CANCEL]);
	UnHighLightLine(hStringHandle);

	FinishRadioBoxSetup(m_hRadioSectorBox);
	DetermineBoxPositions();
}

void StrategicMilitiaCall::CreateRadioNumberBox() {
	UINT32 hStringHandle;
	CreatePopUpBox(&m_hRadioNumberBox, RadioNumberDimensions, RadioNumberPosition, (POPUP_BOX_FLAG_CLIP_TEXT | POPUP_BOX_FLAG_CENTER_TEXT | POPUP_BOX_FLAG_RESIZE));
	ghRadioNumberBox = m_hRadioNumberBox;

	SetupRadioBoxCommon(m_hRadioNumberBox);

	for (UINT32 i = 0; i < MAX_RADIO_NUMBER_MENU_STRING_COUNT; i++) {
		AddMonoString(&hStringHandle, pRadioNumberMenuStrings[i]);
		UnHighLightLine(hStringHandle);
	}

	FinishRadioBoxSetup(m_hRadioNumberBox);
	DetermineBoxPositions();
}

void StrategicMilitiaCall::RebuildRadioSectorBox(SOLDIERTYPE* pSoldier) {
	UINT32 hStringHandle;
	CHAR16 sString[64];
	static const INT16 sDeltaX[MilitiaCall::NUM_SECTOR_LINES] = { 0, 1, 0, -1 };
	static const INT16 sDeltaY[MilitiaCall::NUM_SECTOR_LINES] = { 1, 0, -1, 0 };

	if ((pSoldier == NULL) || (m_hRadioSectorBox == -1)) return;

	SetCurrentBox(m_hRadioSectorBox);
	RemoveAllCurrentBoxStrings();

	SetupRadioBoxCommon(m_hRadioSectorBox);

	for (INT32 i = 0; i < MilitiaCall::NUM_SECTOR_LINES; i++) {
		INT16 sLoopX = pSoldier->sSectorX + sDeltaX[i];
		INT16 sLoopY = pSoldier->sSectorY + sDeltaY[i];

		if (sLoopX < 1 || sLoopX >= MAP_WORLD_X - 1 || sLoopY < 1 || sLoopY >= MAP_WORLD_Y - 1) {
			swprintf(sString, L"-");
			m_usRadioSectorLine[i] = 0xFFFF;
		}
		else {
			swprintf(sString, L"%c%d (%d)", sLoopY + 'A' - 1, sLoopX, CountAllMilitiaInSector(sLoopX, sLoopY));
			m_usRadioSectorLine[i] = SECTOR(sLoopX, sLoopY);
		}
		AddMonoString(&hStringHandle, sString);
		UnHighLightLine(hStringHandle);
	}

	AddMonoString(&hStringHandle, pRadioMenuStrings[RADIO_MENU_CANCEL]);
	UnHighLightLine(hStringHandle);

	FinishRadioBoxSetup(m_hRadioSectorBox);
}

void StrategicMilitiaCall::PositionMenus(INT16 sBaseX, INT16 sBaseY) {
	SGPRect pDim1, pDim3;
	SGPPoint pPos1, pPos3, pNewPoint;

	if (m_hRadioBox == -1) return;

	GetBoxSize(ghAssignmentBox, &pDim1);
	pNewPoint.iX = sBaseX + pDim1.iRight;
	pNewPoint.iY = sBaseY + ((GetFontHeight(MAP_SCREEN_FONT) + 2) * ASSIGN_MENU_RADIO_SCAN);

	SetBoxPosition(m_hRadioBox, pNewPoint);
	ResizeBoxToText(m_hRadioBox);
	m_RadioPosition = pNewPoint;

	GetBoxPosition(m_hRadioBox, &pPos1);
	GetBoxSize(m_hRadioBox, &pDim1);

	if ((fShowRadioSectorMenu == TRUE) && (m_hRadioSectorBox != -1)) {
		pNewPoint.iX = pPos1.iX + pDim1.iRight;
		pNewPoint.iY = pPos1.iY + ((GetFontHeight(MAP_SCREEN_FONT) + 2) * RADIO_MENU_CALL_MILITIA);

		SetBoxPosition(m_hRadioSectorBox, pNewPoint);
		ResizeBoxToText(m_hRadioSectorBox);
		m_RadioSectorPosition = pNewPoint;

		ForceUpDateOfBox(m_hRadioBox);
		ForceUpDateOfBox(m_hRadioSectorBox);

		GetBoxPosition(m_hRadioSectorBox, &pPos3);
		GetBoxSize(m_hRadioSectorBox, &pDim3);

		if ((fShowRadioNumberMenu == TRUE) && (m_hRadioNumberBox != -1)) {
			pNewPoint.iX = pPos3.iX + pDim3.iRight;
			pNewPoint.iY = pPos3.iY;

			SetBoxPosition(m_hRadioNumberBox, pNewPoint);
			ResizeBoxToText(m_hRadioNumberBox);
			m_RadioNumberPosition = pNewPoint;

			ForceUpDateOfBox(m_hRadioNumberBox);
		}
	}
	MarkAllBoxesAsAltered();
}

void StrategicMilitiaCall::HandleShadingOfLinesForRadioMenu(SOLDIERTYPE* pSoldier) {
	if ((fShowRadioMenu == FALSE) || (m_hRadioBox == -1)) return;

	if (pSoldier && pSoldier->CanUseRadio()) {
		UnShadeStringInBox(m_hRadioBox, RADIO_MENU_SCAN);
	}
	else {
		ShadeStringInBox(m_hRadioBox, RADIO_MENU_SCAN);
	}

	if (pSoldier && CanCharacterCallMilitia(pSoldier)) {
		UnShadeStringInBox(m_hRadioBox, RADIO_MENU_CALL_MILITIA);
	}
	else {
		ShadeStringInBox(m_hRadioBox, RADIO_MENU_CALL_MILITIA);
	}
}

void StrategicMilitiaCall::HandleShadingOfLinesForRadioSectorMenu() {
	if (m_hRadioSectorBox == -1) return;

	for (INT32 i = 0; i < MilitiaCall::NUM_SECTOR_LINES; i++) {
		UINT16 usSector = m_usRadioSectorLine[i];
		if ((usSector == 0xFFFF) || (CountAllMilitiaInSector(SECTORX(usSector), SECTORY(usSector)) == 0)) {
			ShadeStringInBox(m_hRadioSectorBox, i);
		}
		else {
			UnShadeStringInBox(m_hRadioSectorBox, i);
		}
	}
}

void StrategicMilitiaCall::HandleShadingOfLinesForRadioNumberMenu() {
	static const UINT16 usRadioNumbers[MilitiaCall::NUM_MENU_ALL] = { 5, 10, 15, 20, 30, 40 };
	UINT32 uiMilitia = 0;

	if (m_hRadioNumberBox == -1) return;

	if (m_usRadioSelectedSector != 0xFFFF) {
		uiMilitia = CountAllMilitiaInSector(SECTORX(m_usRadioSelectedSector), SECTORY(m_usRadioSelectedSector));
	}

	for (INT32 i = 0; i < MilitiaCall::NUM_MENU_ALL; i++) {
		if (uiMilitia < usRadioNumbers[i]) {
			ShadeStringInBox(m_hRadioNumberBox, i);
		}
		else {
			UnShadeStringInBox(m_hRadioNumberBox, i);
		}
	}

	if (uiMilitia == 0) {
		ShadeStringInBox(m_hRadioNumberBox, MilitiaCall::NUM_MENU_ALL);
	}
	else {
		UnShadeStringInBox(m_hRadioNumberBox, MilitiaCall::NUM_MENU_ALL);
	}
}

void StrategicMilitiaCall::CloseAllMenus() {
	if (m_hRadioBox != -1) { HideBox(m_hRadioBox); }
	if (m_hRadioSectorBox != -1) { HideBox(m_hRadioSectorBox); }
	if (m_hRadioNumberBox != -1) { HideBox(m_hRadioNumberBox); }
	ghRadioBox = -1;
	ghRadioSectorBox = -1;
	ghRadioNumberBox = -1;
}