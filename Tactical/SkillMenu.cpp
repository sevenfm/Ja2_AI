/** 
 * @file
 * @author Flugente (bears-pit.com)
 */

#include "SkillMenu.h"
#include "Soldier Profile Type.h"
#include "Overhead.h"
#include "Text.h"
#include "Isometric Utils.h"
#include "DisplayCover.h"
#include "worldman.h"
#include "Rotting Corpses.h"
#include "Handle Items.h"
// sevenfm
#include "Interface.h"
#include "Cheats.h"
#include "HelpScreen.h"
//#include "renderworld.h"

// sevenfm: need this for correct calculation of traits menu position
extern INT16 gsInterfaceLevel;
// for calling help screen
extern BOOLEAN ShouldTheHelpScreenComeUp(UINT8 ubScreenID, BOOLEAN fForceHelpScreenToComeUp);
extern void HandleTBOptionsMenu(void);

UINT16 usTraitMenuPosX = 0;
UINT16 usTraitMenuPosY = 0;

INT32	sTraitsMenuTargetGridNo = NOWHERE;
INT8	bTraitsMenuTargetLevel = 0;

void Hide( void )
{
	// set off display of trait range/target gridno
	ToggleTraitRangeView(FALSE);

	// Dirty!
	fMapPanelDirty = TRUE;
}

// Turns an option off by always returning FALSE
BOOLEAN Popup_OptionOff( void )	{ return FALSE; }

void
SkillMenuItem::SetupPopup(CHAR* name)
{
	// create a popup
	gPopup = new POPUP(name);
	
	// add a callback that lets the keyboard handler know we're done (and ready to pop up again)
	gPopup->setCallback(POPUP_CALLBACK_HIDE, new popupCallbackFunction<void, void>( &Hide ) );
}

//	Different SkillItem classes. We need wrappers for each one, as we cannot pass a pointer to a member function (we need a concrete object)

/////////////////////////////// Trait Selection ////////////////////////////////////////////
TraitSelection	gTraitSelection;

void Wrapper_Setup_TraitSelection( UINT32 aVal)			{	gTraitSelection.Setup(aVal);	}
void Wrapper_Cancel_TraitSelection( UINT32 aVal )		{	gTraitSelection.Cancel();	}
/////////////////////////////// Trait Selection ////////////////////////////////////////////

/////////////////////////////// Skill Selection ////////////////////////////////////////////
SkillSelection	gSkillSelection;

void Wrapper_Function_SkillSelection( UINT32 aVal)		{	gSkillSelection.Functions(aVal);	}
void Wrapper_Setup_SkillSelection( UINT32 aVal)			{	gSkillSelection.Setup(aVal);	}
void Wrapper_Setup_HelpSelection(UINT32 aVal)
{ 
	gSkillSelection.Cancel();
	gTraitSelection.Cancel();
	//SetRenderFlags(RENDER_FLAG_FULL);
	//RenderWorld();
	ShouldTheHelpScreenComeUp(HELP_SCREEN_TACTICAL, TRUE);
}
void Wrapper_Setup_OptionsSelection(UINT32 aVal)
{
	gSkillSelection.Cancel();
	gTraitSelection.Cancel();
	
	HandleTBOptionsMenu();
}
void Wrapper_Cancel_SkillSelection( UINT32 aVal )		{	gSkillSelection.Cancel();	}
/////////////////////////////// Skill Selection ////////////////////////////////////////////

/////////////////////////////// Artillery Sector Selection ////////////////////////////////////////////
ArtillerySector	gArtillerySector;

//void Wrapper_Function_ArtillerySector( UINT32 aVal)		{	gArtillerySector.Functions(aVal);	}
void Wrapper_Setup_ArtillerySector( UINT32 aVal)		{	gArtillerySector.Setup(aVal);	}
void Wrapper_Cancel_ArtillerySector( UINT32 aVal )		{	gArtillerySector.Cancel();	}
/////////////////////////////// Artillery Sector Selection ////////////////////////////////////////////

/////////////////////////////// Artillery Team Selection ////////////////////////////////////////////
ArtilleryTeam	gArtilleryTeam;

void Wrapper_Function_ArtilleryTeam( UINT32 aVal)		{	gArtilleryTeam.Functions(aVal);	}
void Wrapper_Setup_ArtilleryTeam( UINT32 aVal)			{	gArtilleryTeam.Setup(aVal);	}
void Wrapper_Cancel_ArtilleryTeam( UINT32 aVal )		{	gArtilleryTeam.Cancel();	}
/////////////////////////////// Artillery Team Selection ////////////////////////////////////////////

/////////////////////////////// Reinforcement Sector Selection ////////////////////////////////////////////
ReinforcementSector	gReinforcementSector;

//void Wrapper_Function_ReinforcementSector( UINT32 aVal)		{	gReinforcementSector.Functions(aVal);	}
void Wrapper_Setup_ReinforcementSector( UINT32 aVal)		{	gReinforcementSector.Setup(aVal);	}
void Wrapper_Cancel_ReinforcementSector( UINT32 aVal )		{	gReinforcementSector.Cancel();	}
/////////////////////////////// Reinforcement Sector Selection ////////////////////////////////////////////

/////////////////////////////// Reinforcement Number Selection ////////////////////////////////////////////
ReinforcementNumber	gReinforcementNumber;

void Wrapper_Function_ReinforcementNumber( UINT32 aVal)		{	gReinforcementNumber.Functions(aVal);	}
void Wrapper_Setup_ReinforcementNumber( UINT32 aVal)		{	gReinforcementNumber.Setup(aVal);	}
void Wrapper_Cancel_ReinforcementNumber( UINT32 aVal )		{	gReinforcementNumber.Cancel();	}
/////////////////////////////// Reinforcement Number Selection ////////////////////////////////////////////

/////////////////////////////// Soldier Target Selection ////////////////////////////////////////////
SoldierSelection	gSoldierSelection;

void Wrapper_Function_SoldierSelection( UINT32 aVal)	{	gSoldierSelection.Functions(aVal);	}
void Wrapper_Setup_SoldierSelection( UINT32 aVal)		{	gSoldierSelection.Setup(aVal);	}
void Wrapper_Cancel_SoldierSelection( UINT32 aVal )		{	gSoldierSelection.Cancel();	}
/////////////////////////////// Soldier Target Selection ////////////////////////////////////////////

/////////////////////////////// Drag Selection ////////////////////////////////////////////
DragSelection	gDragSelection;

void Wrapper_Function_DragSelection(UINT32 aVal)		{ gDragSelection.Functions(aVal); }
void Wrapper_Function_DragSelection_GridNo(INT32 aVal){ gDragSelection.FunctionsGridNo(aVal); }
void Wrapper_Setup_DragSelection(UINT32 aVal)			{ gDragSelection.Setup(aVal); }
void Wrapper_Cancel_DragSelection(UINT32 aVal)		{ gDragSelection.Cancel(); }
/////////////////////////////// Drag Selection ////////////////////////////////////////////

/////////////////////////////// Covert Selection ////////////////////////////////////////////
CovertSelection	gCovertSelection;

void Wrapper_Function_CovertSelection(UINT32 aVal)		{ gCovertSelection.Functions(aVal); }
//void Wrapper_Function_CovertSelection_GridNo(INT32 aVal){ gCovertSelection.FunctionsGridNo(aVal); }
void Wrapper_Setup_CovertSelection(UINT32 aVal)			{ gCovertSelection.Setup(aVal); }
void Wrapper_Cancel_CovertSelection(UINT32 aVal)		{ gCovertSelection.Cancel(); }
/////////////////////////////// Covert Selection ////////////////////////////////////////////

/////////////////////////////// Driver Selection ////////////////////////////////////////////
DriverSelection	gDriverSelection;

void Wrapper_Function_DriverSelection(UINT32 aVal)		{ gDriverSelection.Functions(aVal); }
//void Wrapper_Function_DriverSelection_GridNo(INT32 aVal){ gDriverSelection.FunctionsGridNo(aVal); }
void Wrapper_Setup_DriverSelection(UINT32 aVal)			{ gDriverSelection.Setup(aVal); }
void Wrapper_Cancel_DriverSelection(UINT32 aVal)		{ gDriverSelection.Cancel(); }
/////////////////////////////// Driver Selection ////////////////////////////////////////////

/////////////////////////////// Officer Selection ////////////////////////////////////////////
OfficerSelection	gOfficerSelection;

void Wrapper_Function_OfficerSelection(UINT32 aVal)		{ gOfficerSelection.Functions(aVal); }
//void Wrapper_Function_OfficerSelection_GridNo(INT32 aVal){ gOfficerSelection.FunctionsGridNo(aVal); }
void Wrapper_Setup_OfficerSelection(UINT32 aVal)			{ gOfficerSelection.Setup(aVal); }
void Wrapper_Cancel_OfficerSelection(UINT32 aVal)		{ gOfficerSelection.Cancel(); }
/////////////////////////////// Driver Selection ////////////////////////////////////////////


/////////////////////////////// Driver Selection ////////////////////////////////////////////
void
TraitSelection::Setup( UINT32 aVal )
{
	Destroy();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier( &pSoldier, gusSelectedSoldier );

	if ( pSoldier == NULL )
		return;

	SetupPopup("TraitSelection");

	POPUP_OPTION *pOption;
	
	CHAR16 pStr[300];

	// create entries for the sub-menus for each trait
	UINT8 ubNumSkills = 10;

	//if(CHEATER_CHEAT_LEVEL())
		//ubNumSkills++;

	UINT8 traitarray[11];
	traitarray[0] = ACTIONSKILLS;
	traitarray[1] = WEAPONSKILLS;
	traitarray[2] = RADIO_OPERATOR_NT;
	traitarray[3] = VARIOUSSKILLS;
	traitarray[4] = SQUADSKILLS;
	traitarray[5] = SECTORSKILLS;
	traitarray[6] = INVENTORYSKILLS;
	traitarray[7] = DISPLAYSKILLS;
	traitarray[8] = HELPSKILLS;
	traitarray[9] = OPTIONSSKILLS;
	traitarray[10] = CHEATSKILLS;

	for ( int i = 0; i < ubNumSkills; ++i)
	{
		swprintf( pStr, gzMercSkillTextNew[traitarray[i]] );

		if (traitarray[i] == HELPSKILLS)
		{
			pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Setup_HelpSelection, traitarray[i]));
		}
		else if (traitarray[i] == OPTIONSSKILLS)
		{
			pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Setup_OptionsSelection, traitarray[i]));
		}
		else
		{
			pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Setup_SkillSelection, traitarray[i]));
		}		

		// if we cannot perform this skill, grey it out
		if ( !HAS_SKILL_TRAIT(pSoldier, traitarray[i]) )
		{
			// Set this option off.
			pOption->setAvail(new popupCallbackFunction<bool,void*>( &Popup_OptionOff, NULL ));
		}

		GetPopup()->addOption( *pOption );
	}

	// cancel option
	swprintf( pStr, pSkillMenuStrings[SKILLMENU_CANCEL] );
	pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void,UINT32>( &Wrapper_Cancel_TraitSelection, 0 ) );
	GetPopup()->addOption( *pOption );
		
	// grab soldier's x,y screen position
	INT16 sX, sY;
	// sevenfm: changed TraitsMenu position from soldier to mouse
	//GetSoldierScreenPos( pSoldier, &sX, &sY );
	GetGridNoScreenPos( sTraitsMenuTargetGridNo, (UINT8)gsInterfaceLevel, &sX, &sY );	
		
	if( sX < 0 ) sX = 0;
	if( sY < 0 ) sY = 0;

	usTraitMenuPosX = sX + 30;		
	usTraitMenuPosY = sY;

	if ( ( usTraitMenuPosX + 400 ) > SCREEN_WIDTH )
		usTraitMenuPosX = SCREEN_WIDTH - 400;

	if ( ( usTraitMenuPosY + 130 ) > SCREEN_HEIGHT )
		usTraitMenuPosY = SCREEN_HEIGHT - 190;

	SetPos(usTraitMenuPosX, usTraitMenuPosY);
}
/////////////////////////////// Trait Selection ////////////////////////////////////////////


/////////////////////////////// Skill Selection ////////////////////////////////////////////
void
SkillSelection::Setup( UINT32 aVal )
{
	Destroy();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier( &pSoldier, gusSelectedSoldier );

	if ( pSoldier == NULL )
		return;

	if ( HAS_SKILL_TRAIT(pSoldier, aVal) )
	{
		SetupPopup("SkillSelection");
		
		POPUP_OPTION *pOption;

		CHAR16 pStr[300];

		SetTraitToDisplay(aVal);
		SetGridNoForTraitDisplay(sTraitsMenuTargetGridNo);

		// this switch isn't really necessary. But dividing the skills into menus for each trait gives a better overview, and looks way better than a huge list
		switch ( aVal )
		{
		case RADIO_OPERATOR_NT:
			for (UINT32 uiCounter = SKILLS_RADIO_FIRST; uiCounter <= SKILLS_RADIO_LAST; ++uiCounter)
			{
				swprintf(pStr, pTraitSkillsMenuStrings[uiCounter]);

				if (uiCounter == SKILLS_RADIO_ARTILLERY)
					pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Setup_ArtillerySector, uiCounter));
				else if (uiCounter == SKILLS_RADIO_CALLREINFORCEMENTS)
					pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Setup_ReinforcementSector, uiCounter));
				else
					pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_SkillSelection, uiCounter));

				// if we cannot perform this skill, grey it out
				if (!(pSoldier->CanUseSkill(uiCounter, TRUE, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel)))
				{
					// Set this option off.
					pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
				}

				GetPopup()->addOption(*pOption);
			}
			break;

		case VARIOUSSKILLS:
			for (UINT32 uiCounter = SKILLS_VARIOUS_FIRST; uiCounter <= SKILLS_VARIOUS_LAST; ++uiCounter)
			{
				swprintf(pStr, pTraitSkillsMenuStrings[uiCounter]);

				if (uiCounter == SKILLS_DRAG)
					pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Setup_DragSelection, uiCounter));
				else if (uiCounter == SKILLS_COVERT)
					pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Setup_CovertSelection, uiCounter));
				else if (uiCounter == SKILLS_DRIVER)
					pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Setup_DriverSelection, uiCounter));
				else if (uiCounter == SKILLS_OFFICER)
					pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Setup_OfficerSelection, uiCounter));
				else
					pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_SkillSelection, uiCounter));

				// if we cannot perform this skill, grey it out
				if (!(pSoldier->CanUseSkill(uiCounter, TRUE, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel)))
				{
					// Set this option off.
					pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
				}

				GetPopup()->addOption(*pOption);
			}

			SetTraitToDisplay(VARIOUSSKILLS);
			SetGridNoForTraitDisplay(sTraitsMenuTargetGridNo);
			// sevenfm: disable showing trait range
			ToggleTraitRangeView(FALSE);
			break;

		case DISPLAYSKILLS:
			for (UINT32 uiCounter = SKILLS_DISPLAY_FIRST; uiCounter <= SKILLS_DISPLAY_LAST; ++uiCounter)
			{
				swprintf(pStr, pTraitSkillsMenuStrings[uiCounter]);

				pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_SkillSelection, uiCounter));

				// if we cannot perform this skill, grey it out
				if (!(pSoldier->CanUseSkill(uiCounter, TRUE, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel)))
				{
					// Set this option off.
					pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
				}

				GetPopup()->addOption(*pOption);
			}

			SetTraitToDisplay(DISPLAYSKILLS);
			SetGridNoForTraitDisplay(sTraitsMenuTargetGridNo);
			break;

		case ACTIONSKILLS:
			for (UINT32 uiCounter = SKILLS_ACTION_FIRST; uiCounter <= SKILLS_ACTION_LAST; ++uiCounter)
			{
				if (uiCounter == SKILLS_ACTION_DRAG && pSoldier->IsDragging() ||
					uiCounter == SKILLS_ACTION_STOP_DRAGGING && !pSoldier->IsDragging())
					continue;

				if ((uiCounter == SKILLS_ACTION_CLEAN_GUN ||
					uiCounter == SKILLS_ACTION_COWER || uiCounter == SKILLS_ACTION_STOP_COWERING ||
					uiCounter == SKILLS_ACTION_DROP_BACKPACK || uiCounter == SKILLS_ACTION_PICKUP_BACKPACK) &&
					!(pSoldier->CanUseSkill(uiCounter, FALSE, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel)))
					continue;

				if (uiCounter == SKILLS_ACTION_DRINK_CANTEEN && gGameOptions.fFoodSystem)
					continue;

				swprintf(pStr, pTraitSkillsMenuStrings[uiCounter]);

				pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_SkillSelection, uiCounter));

				// if we cannot perform this skill, grey it out
				if (!(pSoldier->CanUseSkill(uiCounter, TRUE, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel)))
				{
					// Set this option off.
					pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
				}

				GetPopup()->addOption(*pOption);
			}

			SetTraitToDisplay(ACTIONSKILLS);
			SetGridNoForTraitDisplay(sTraitsMenuTargetGridNo);
			break;

		case WEAPONSKILLS:
			for (UINT32 uiCounter = SKILLS_WEAPON_FIRST; uiCounter <= SKILLS_WEAPON_LAST; ++uiCounter)
			{
				swprintf(pStr, pTraitSkillsMenuStrings[uiCounter]);

				pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_SkillSelection, uiCounter));

				// if we cannot perform this skill, grey it out
				if (!(pSoldier->CanUseSkill(uiCounter, TRUE, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel)))
				{
					// Set this option off.
					pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
				}

				GetPopup()->addOption(*pOption);
			}

			SetTraitToDisplay(WEAPONSKILLS);
			SetGridNoForTraitDisplay(sTraitsMenuTargetGridNo);
			break;

		case SQUADSKILLS:
			for (UINT32 uiCounter = SKILLS_SQUAD_FIRST; uiCounter <= SKILLS_SQUAD_LAST; ++uiCounter)
			{
				swprintf(pStr, pTraitSkillsMenuStrings[uiCounter]);

				pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_SkillSelection, uiCounter));

				// if we cannot perform this skill, grey it out
				if (!(pSoldier->CanUseSkill(uiCounter, TRUE, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel)))
				{
					// Set this option off.
					pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
				}

				GetPopup()->addOption(*pOption);
			}

			SetTraitToDisplay(SQUADSKILLS);
			SetGridNoForTraitDisplay(sTraitsMenuTargetGridNo);
			break;

		case SECTORSKILLS:
			for (UINT32 uiCounter = SKILLS_SECTOR_FIRST; uiCounter <= SKILLS_SECTOR_LAST; ++uiCounter)
			{
				if ((uiCounter == SKILLS_SECTOR_CLEAN_ALL_GUNS || uiCounter == SKILLS_SECTOR_MILITIA_DROP_GEAR || uiCounter == SKILLS_SECTOR_MILITIA_TAKE_GEAR) &&
					!(pSoldier->CanUseSkill(uiCounter, FALSE, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel)))
					continue;

				swprintf(pStr, pTraitSkillsMenuStrings[uiCounter]);

				pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_SkillSelection, uiCounter));

				// if we cannot perform this skill, grey it out
				if (!(pSoldier->CanUseSkill(uiCounter, TRUE, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel)))
				{
					// Set this option off.
					pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
				}

				GetPopup()->addOption(*pOption);
			}

			SetTraitToDisplay(SECTORSKILLS);
			SetGridNoForTraitDisplay(sTraitsMenuTargetGridNo);
			break;

		case INVENTORYSKILLS:
			for (UINT32 uiCounter = SKILLS_INVENTORY_FIRST; uiCounter <= SKILLS_INVENTORY_LAST; ++uiCounter)
			{
				swprintf(pStr, pTraitSkillsMenuStrings[uiCounter]);

				pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_SkillSelection, uiCounter));

				// if we cannot perform this skill, grey it out
				if (!(pSoldier->CanUseSkill(uiCounter, TRUE, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel)))
				{
					// Set this option off.
					pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
				}

				GetPopup()->addOption(*pOption);
			}

			SetTraitToDisplay(INVENTORYSKILLS);
			SetGridNoForTraitDisplay(sTraitsMenuTargetGridNo);
			break;
		
		default:
			break;
		}
				
		// cancel option
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_CANCEL] );
		pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void,UINT32>( &Wrapper_Cancel_SkillSelection, 0 ) );
		GetPopup()->addOption( *pOption );
	}

	// same y, different x
	SetPos(gTraitSelection.GetMaxPosX(), usTraitMenuPosY);

	if (HAS_SKILL_TRAIT(pSoldier, aVal))
	{
		UINT8 cnt = 0;
		switch (aVal)
		{
		case RADIO_OPERATOR_NT:
			for (UINT32 uiCounter = SKILLS_RADIO_FIRST; uiCounter <= SKILLS_RADIO_LAST; ++uiCounter)
			{
				SetRegionFastHelpText(&(GetPopup()->MenuRegion[cnt++]), pSoldier->PrintSkillDesc(uiCounter));
			}
			break;

		case VARIOUSSKILLS:
			for (UINT32 uiCounter = SKILLS_VARIOUS_FIRST; uiCounter <= SKILLS_VARIOUS_LAST; ++uiCounter)
			{
				SetRegionFastHelpText(&(GetPopup()->MenuRegion[cnt++]), pSoldier->PrintSkillDesc(uiCounter));
			}
			break;

		case DISPLAYSKILLS:
			for (UINT32 uiCounter = SKILLS_DISPLAY_FIRST; uiCounter <= SKILLS_DISPLAY_LAST; ++uiCounter)
			{
				SetRegionFastHelpText(&(GetPopup()->MenuRegion[cnt++]), pSoldier->PrintSkillDesc(uiCounter));
			}
			break;

		case ACTIONSKILLS:
			for (UINT32 uiCounter = SKILLS_ACTION_FIRST; uiCounter <= SKILLS_ACTION_LAST; ++uiCounter)
			{
				if (uiCounter == SKILLS_ACTION_DRAG && pSoldier->IsDragging() ||
					uiCounter == SKILLS_ACTION_STOP_DRAGGING && !pSoldier->IsDragging())
					continue;

				if ((uiCounter == SKILLS_ACTION_CLEAN_GUN ||
					uiCounter == SKILLS_ACTION_COWER || uiCounter == SKILLS_ACTION_STOP_COWERING ||
					uiCounter == SKILLS_ACTION_DROP_BACKPACK || uiCounter == SKILLS_ACTION_PICKUP_BACKPACK) &&
					!(pSoldier->CanUseSkill(uiCounter, FALSE, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel)))
					continue;

				if (uiCounter == SKILLS_ACTION_DRINK_CANTEEN && gGameOptions.fFoodSystem)
					continue;

				SetRegionFastHelpText(&(GetPopup()->MenuRegion[cnt++]), pSoldier->PrintSkillDesc(uiCounter));
			}
			break;

		case WEAPONSKILLS:
			for (UINT32 uiCounter = SKILLS_WEAPON_FIRST; uiCounter <= SKILLS_WEAPON_LAST; ++uiCounter)
			{
				SetRegionFastHelpText(&(GetPopup()->MenuRegion[cnt++]), pSoldier->PrintSkillDesc(uiCounter));
			}

		case SQUADSKILLS:
			for (UINT32 uiCounter = SKILLS_SQUAD_FIRST; uiCounter <= SKILLS_SQUAD_LAST; ++uiCounter)
			{
				SetRegionFastHelpText(&(GetPopup()->MenuRegion[cnt++]), pSoldier->PrintSkillDesc(uiCounter));
			}
			break;
		case SECTORSKILLS:
			for (UINT32 uiCounter = SKILLS_SECTOR_FIRST; uiCounter <= SKILLS_SECTOR_LAST; ++uiCounter)
			{
				if ((uiCounter == SKILLS_SECTOR_CLEAN_ALL_GUNS || uiCounter == SKILLS_SECTOR_MILITIA_DROP_GEAR || uiCounter == SKILLS_SECTOR_MILITIA_TAKE_GEAR) &&
					!(pSoldier->CanUseSkill(uiCounter, FALSE, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel)))
					continue;
				SetRegionFastHelpText(&(GetPopup()->MenuRegion[cnt++]), pSoldier->PrintSkillDesc(uiCounter));
			}
			break;
		case INVENTORYSKILLS:
			for (UINT32 uiCounter = SKILLS_INVENTORY_FIRST; uiCounter <= SKILLS_INVENTORY_LAST; ++uiCounter)
			{
				SetRegionFastHelpText(&(GetPopup()->MenuRegion[cnt++]), pSoldier->PrintSkillDesc(uiCounter));
			}
			break;
		}
	}
}

void
SkillSelection::Functions( UINT32 aVal )
{
	Cancel();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier( &pSoldier, gusSelectedSoldier );

	if ( pSoldier == NULL )
		return;
	
	UINT8 ubID = WhoIsThere2(sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel);

	pSoldier->UseSkill(aVal, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel, ubID);
		
	Cancel();
	gTraitSelection.Cancel();
}
/////////////////////////////// Skill Selection ////////////////////////////////////////////

/////////////////////////////// Artillery Sector Selection ////////////////////////////////////////////
void
ArtillerySector::Setup( UINT32 aVal )
{
	Destroy();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier( &pSoldier, gusSelectedSoldier );

	if ( pSoldier == NULL )
		return;

	if ( pSoldier->CanUseSkill(SKILLS_RADIO_ARTILLERY, TRUE) )
	{
		SetupPopup("ArtillerySector");
		
		POPUP_OPTION *pOption;
			
		CHAR16 pStr[300];
				
		// check whether we can call artillery from the 4 adjacent sectors
		for (UINT8 i = 0; i < 4; ++i)
		{
			INT16 loopX = pSoldier->sSectorX;
			INT16 loopY = pSoldier->sSectorY;

			if ( i == 0 )		++loopY;
			else if ( i == 1 )	++loopX;
			else if ( i == 2 )	--loopY;
			else if ( i == 3 )	--loopX;

			if ( loopX < 1 || loopX >= MAP_WORLD_X - 1 || loopY < 1 || loopY >= MAP_WORLD_Y - 1 )
				continue;

			UINT32 sectornr = (UINT32)SECTOR( loopX, loopY );
		
			swprintf( pStr, L"%c%d", loopY + 'A' - 1, loopX );

			pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void, UINT32>( &Wrapper_Setup_ArtilleryTeam, sectornr ) );

			// grey out if no artillery can be called from this sector
			if ( !IsValidArtilleryOrderSector( loopX, loopY, pSoldier->bSectorZ, pSoldier->bTeam ) && !IsValidArtilleryOrderSector( loopX, loopY, pSoldier->bSectorZ, MILITIA_TEAM ) )
			{
				// Set this option off.
				pOption->setAvail(new popupCallbackFunction<bool,void*>( &Popup_OptionOff, NULL ));
			}
			GetPopup()->addOption( *pOption );
		}

		// cancel option
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_CANCEL] );
		pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void,UINT32>( &Wrapper_Cancel_ArtillerySector, 0 ) );
		GetPopup()->addOption( *pOption );

		// certain traits have skills whose effects depend on whether someone is near to us (squadleader, commissar). We therefore display our radius of effect while this display is open
		SetTraitToDisplay(RADIO_OPERATOR_NT);
		SetGridNoForTraitDisplay(sTraitsMenuTargetGridNo);
		// sevenfm: disable showing trait range
		ToggleTraitRangeView(FALSE);
	}

	// same y, different x
	SetPos(gSkillSelection.GetMaxPosX(), usTraitMenuPosY);
}
/////////////////////////////// Artillery Sector Selection ////////////////////////////////////////////

/////////////////////////////// Artillery Team Selection ////////////////////////////////////////////
void
ArtilleryTeam::Setup( UINT32 aVal )
{
	Destroy();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier( &pSoldier, gusSelectedSoldier );

	if ( pSoldier == NULL )
		return;

	if ( pSoldier->CanUseSkill(SKILLS_RADIO_ARTILLERY, TRUE) && pSoldier->bSectorZ == 0)
	{		
		usSector = aVal;
		
		SetupPopup("ArtilleryTeam");

		POPUP_OPTION *pOption;
			
		CHAR16 pStr[300];

		// determine sector coordinates
		INT16 sSectorX = SECTORX(usSector);
		INT16 sSectorY = SECTORY(usSector);
				
		// order artillery from militia
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_MILITIA] );

		pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void, UINT32>( &Wrapper_Function_ArtilleryTeam, MILITIA_TEAM ) );

		// grey out if no ArtilleryTeam can be called from this sector
		if ( !IsValidArtilleryOrderSector( sSectorX, sSectorY, pSoldier->bSectorZ, MILITIA_TEAM ) )
		{
			// Set this option off.
			pOption->setAvail(new popupCallbackFunction<bool,void*>( &Popup_OptionOff, NULL ));
		}
		GetPopup()->addOption( *pOption );

		// order artillery from our mercs
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_OTHERSQUADS] );

		pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void, INT8>( &Wrapper_Function_ArtilleryTeam, pSoldier->bTeam ) );

		// grey out if no ArtilleryTeam can be called from this sector
		if ( !IsValidArtilleryOrderSector( sSectorX, sSectorY, pSoldier->bSectorZ, pSoldier->bTeam ) )
		{
			// Set this option off.
			pOption->setAvail(new popupCallbackFunction<bool,void*>( &Popup_OptionOff, NULL ));
		}
		GetPopup()->addOption( *pOption );
		
		// cancel option
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_CANCEL] );
		pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void,UINT32>( &Wrapper_Cancel_ArtilleryTeam, 0 ) );
		GetPopup()->addOption( *pOption );

		// certain traits have skills whose effects depend on whether someone is near to us (squadleader, commissar). We therefore display our radius of effect while this display is open
		SetTraitToDisplay(RADIO_OPERATOR_NT);
		SetGridNoForTraitDisplay(sTraitsMenuTargetGridNo);
		// sevenfm: disable showing trait range
		ToggleTraitRangeView(FALSE);
	}

	// same y, different x
	SetPos(gArtillerySector.GetMaxPosX(), usTraitMenuPosY);
}

void
ArtilleryTeam::Functions( UINT32 aVal )
{
	Cancel();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier( &pSoldier, gusSelectedSoldier );

	if ( pSoldier == NULL || !pSoldier->CanUseSkill(SKILLS_RADIO_ARTILLERY, TRUE) )
		return;
	
	pSoldier->OrderArtilleryStrike(usSector, sTraitsMenuTargetGridNo, (UINT8)(aVal));

	Cancel();
	gArtillerySector.Cancel();
	gSkillSelection.Cancel();
	gTraitSelection.Cancel();
}
/////////////////////////////// Artillery Team Selection ////////////////////////////////////////////

/////////////////////////////// Reinforcement Sector Selection ////////////////////////////////////////////
extern UINT8 CountAllMilitiaInSector(INT16 sMapX, INT16 sMapY);

void
ReinforcementSector::Setup( UINT32 aVal )
{
	Destroy();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier( &pSoldier, gusSelectedSoldier );

	if ( pSoldier == NULL )
		return;

	if ( pSoldier->CanUseSkill(aVal, TRUE) )
	{
		SetupPopup("Reinforcements sector");
		
		POPUP_OPTION *pOption;

		CHAR16 pStr[300];
				
		// check whether we can call artillery from the 4 adjacent sectors
		for (UINT8 i = 0; i < 4; ++i)
		{
			INT16 loopX = pSoldier->sSectorX;
			INT16 loopY = pSoldier->sSectorY;

			if ( i == 0 )		++loopY;
			else if ( i == 1 )	++loopX;
			else if ( i == 2 )	--loopY;
			else if ( i == 3 )	--loopX;

			if ( loopX < 1 || loopX >= MAP_WORLD_X - 1 || loopY < 1 || loopY >= MAP_WORLD_Y - 1 )
				continue;

			UINT32 sectornr = (UINT32)SECTOR( loopX, loopY );
		
			swprintf( pStr, L"%c%d", loopY + 'A' - 1, loopX );

			pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void, UINT32>( &Wrapper_Setup_ReinforcementNumber, sectornr ) );

			// grey out if no reinforcements can be called from this sector
			if( !CountAllMilitiaInSector( loopX, loopY ) )
			{
				// Set this option off.
				pOption->setAvail(new popupCallbackFunction<bool,void*>( &Popup_OptionOff, NULL ));
			}
			GetPopup()->addOption( *pOption );
		}

		// cancel option
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_CANCEL] );
		pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void, UINT32>( &Wrapper_Cancel_ReinforcementSector, 0 ) );
		GetPopup()->addOption( *pOption );
	}

	// same y, different x
	SetPos(gSkillSelection.GetMaxPosX(), usTraitMenuPosY);
}
/////////////////////////////// Reinforcement Sector Selection ////////////////////////////////////////////

/////////////////////////////// Reinforcement Number Selection ////////////////////////////////////////////
void
ReinforcementNumber::Setup( UINT32 aVal )
{
	Destroy();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier( &pSoldier, gusSelectedSoldier );

	if ( pSoldier == NULL )
		return;

	if ( pSoldier->CanUseSkill(SKILLS_RADIO_CALLREINFORCEMENTS, TRUE) )
	{
		usSector = aVal;

		SetupPopup("Reinforcements number submenu");
	
		POPUP_OPTION *pOption;

		CHAR16 pStr[300];
		
		UINT8 numberofmilitia = CountAllMilitiaInSector( SECTORX(usSector), SECTORY(usSector) );
		
		// 5 militia option
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_X_MILITIA], 5 );
		pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void, UINT32>( &Wrapper_Function_ReinforcementNumber, 5 ) );
		
		if ( numberofmilitia < 5 )
		{
			// Set this option off.
			pOption->setAvail(new popupCallbackFunction<bool,void*>( &Popup_OptionOff, NULL ));
		}

		GetPopup()->addOption( *pOption );

		// 10 militia option
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_X_MILITIA], 10 );
		pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void, UINT32>( &Wrapper_Function_ReinforcementNumber, 10 ) );

		if ( numberofmilitia < 10 )
		{
			// Set this option off.
			pOption->setAvail(new popupCallbackFunction<bool,void*>( &Popup_OptionOff, NULL ));
		}

		GetPopup()->addOption( *pOption );

		// 15 militia option
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_X_MILITIA], 15 );
		pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void, UINT32>( &Wrapper_Function_ReinforcementNumber, 15 ) );

		if ( numberofmilitia < 15 )
		{
			// Set this option off.
			pOption->setAvail(new popupCallbackFunction<bool,void*>( &Popup_OptionOff, NULL ));
		}

		GetPopup()->addOption( *pOption );

		// 20 militia option
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_X_MILITIA], 20 );
		pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void, UINT32>( &Wrapper_Function_ReinforcementNumber, 20 ) );

		if ( numberofmilitia < 20 )
		{
			// Set this option off.
			pOption->setAvail(new popupCallbackFunction<bool,void*>( &Popup_OptionOff, NULL ));
		}

		// 30 militia option
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_X_MILITIA], 30 );
		pOption = new POPUP_OPTION( &std::wstring( pStr ), new popupCallbackFunction<void, UINT32>( &Wrapper_Function_ReinforcementNumber, 30 ) );

		if ( numberofmilitia < 30 )
		{
			// Set this option off.
			pOption->setAvail( new popupCallbackFunction<bool, void*>( &Popup_OptionOff, NULL ) );
		}

		// 40 militia option
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_X_MILITIA], 40 );
		pOption = new POPUP_OPTION( &std::wstring( pStr ), new popupCallbackFunction<void, UINT32>( &Wrapper_Function_ReinforcementNumber, 40 ) );

		if ( numberofmilitia < 40 )
		{
			// Set this option off.
			pOption->setAvail( new popupCallbackFunction<bool, void*>( &Popup_OptionOff, NULL ) );
		}

		GetPopup()->addOption( *pOption );

		// all militia option
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_ALL_MILITIA] );

		pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void, UINT32>( &Wrapper_Function_ReinforcementNumber, numberofmilitia ) );
		GetPopup()->addOption( *pOption );

		// cancel option
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_CANCEL] );
		pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void, UINT32>( &Wrapper_Cancel_ReinforcementNumber, 0 ) );
		GetPopup()->addOption( *pOption );
	}

	// same y, different x
	SetPos(gReinforcementSector.GetMaxPosX(), usTraitMenuPosY);
}

void
ReinforcementNumber::Functions( UINT32 aVal )
{
	Cancel();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier( &pSoldier, gusSelectedSoldier );
		
	if ( pSoldier == NULL || !pSoldier->CanUseSkill(SKILLS_RADIO_CALLREINFORCEMENTS, TRUE) )
		return;
	
	pSoldier->RadioCallReinforcements(usSector, aVal);

	Cancel();
	gReinforcementSector.Cancel();
	gSkillSelection.Cancel();
	gTraitSelection.Cancel();
}
/////////////////////////////// Reinforcement Number Selection ////////////////////////////////////////////

/////////////////////////////// Soldier Target Selection ////////////////////////////////////////////
void
SoldierSelection::Setup( UINT32 aVal )
{
	Destroy();
	
	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier( &pSoldier, gusSelectedSoldier );

	if ( pSoldier == NULL )
		return;

	if ( pSoldier->CanUseSkill(aVal, TRUE) )
	{
		usSkill = aVal;

		SetupPopup("SoldierSelection");
		
		POPUP_OPTION *pOption;
	
		CHAR16 pStr[300];

		// pretty simple: we find every soldier in a radius around the target position and add him to the list
		// loop through all soldiers around
		for ( UINT32 cnt = gTacticalStatus.Team[ OUR_TEAM ].bFirstID ; cnt <= gTacticalStatus.Team[ CIV_TEAM ].bLastID ; ++cnt )
		{
			INT32 iRange = GetRangeInCellCoordsFromGridNoDiff( sTraitsMenuTargetGridNo, MercPtrs[ cnt ]->sGridNo );

			if ( iRange < 100 )
			{
				if ( cnt != pSoldier->ubID )
				{
					swprintf( pStr, L"%s", MercPtrs[ cnt ]->GetName() );

					pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void, UINT8>( &Wrapper_Function_SoldierSelection, cnt ) );

					// grey out if no artillery can be called from this sector
					if ( 0 )
					{
						// Set this option off.
						pOption->setAvail(new popupCallbackFunction<bool,void*>( &Popup_OptionOff, NULL ));
					}
					GetPopup()->addOption( *pOption );
				}
			}
		}

		// cancel option
		swprintf( pStr, pSkillMenuStrings[SKILLMENU_CANCEL] );
		pOption = new POPUP_OPTION(&std::wstring( pStr ), new popupCallbackFunction<void,UINT32>( &Wrapper_Cancel_SoldierSelection, 0 ) );
		GetPopup()->addOption( *pOption );
	}

	// same y, different x
	SetPos(gSkillSelection.GetMaxPosX(), usTraitMenuPosY);
}

void
SoldierSelection::Functions( UINT32 aVal )
{
	Cancel();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier( &pSoldier, gusSelectedSoldier );

	if ( pSoldier == NULL )
		return;

	pSoldier->UseSkill(usSkill, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel, aVal);

	Cancel();
	gSkillSelection.Cancel();
	gTraitSelection.Cancel();
}
/////////////////////////////// Soldier Target Selection ////////////////////////////////////////////

/////////////////////////////// Drag Selection ////////////////////////////////////////////
void
DragSelection::Setup(UINT32 aVal)
{
	Destroy();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier(&pSoldier, gusSelectedSoldier);

	if (pSoldier == NULL)
		return;

	if (pSoldier->CanUseSkill(aVal, TRUE))
	{
		usSkill = aVal;

		SetupPopup("DragSelection");

		POPUP_OPTION *pOption;

		CHAR16 pStr[300];

		// pretty simple: we find every soldier in a radius around the target position and add him to the list
		// loop through all soldiers around
		for (UINT32 cnt = gTacticalStatus.Team[OUR_TEAM].bFirstID; cnt <= gTacticalStatus.Team[CIV_TEAM].bLastID; ++cnt)
		{
			if (cnt != pSoldier->ubID && pSoldier->CanDragPerson(cnt))
			{
				swprintf(pStr, L"%s", MercPtrs[cnt]->GetName());

				pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_DragSelection, cnt));

				GetPopup()->addOption(*pOption);
			}
		}

		// corpses
		std::vector<INT16> corpseids = GetCorpseIDsNearGridNo(pSoldier->sGridNo, pSoldier->pathing.bLevel, 1);

		for (std::vector<INT16>::iterator it = corpseids.begin(); it != corpseids.end(); ++it)
		{
			ROTTING_CORPSE* pCorpse = GetRottingCorpse((*it));

			if (pCorpse && pSoldier->CanDragCorpse(pCorpse->iID))
			{
				swprintf(pStr, pSkillMenuStrings[SKILLMENU_CORPSES], pCorpse->name);

				// we have to use an offset of NOBODY in order to differentiate between person and corpse
				pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_DragSelection, (*it) + NOBODY));

				GetPopup()->addOption(*pOption);
			}			
		}

		// gridno
		for (int ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ++ubDirection)
		{
			INT32 sTempGridNo = NewGridNo(pSoldier->sGridNo, DirectionInc(ubDirection));

			UINT32 tiletype;
			UINT16 structurenumber;
			UINT8 hitpoints;
			UINT8 decalflag;
			if (pSoldier->CanDragStructure(sTempGridNo)
				//&& IsDragStructurePresent(sTempGridNo, pSoldier->pathing.bLevel, tiletype, structurenumber))
				&& IsDragStructurePresent(sTempGridNo, pSoldier->pathing.bLevel, tiletype, structurenumber, hitpoints, decalflag))
			{
				int xmlentry;
				GetDragStructureXmlEntry(tiletype, structurenumber, xmlentry);

				if (xmlentry >= 0)
				{
					//swprintf(pStr, L"%hs %s", gStructureMovePossible[xmlentry].szTileSetDisplayName, FaceDirs[gOneCDirection[ubDirection]]);
					swprintf(pStr, L"%hs (%s)", gStructureMovePossible[xmlentry].szTileSetDisplayName, FaceDirs[gOneCDirection[ubDirection]]);

					// we have to use an offset of NOBODY in order to differentiate between person and corpse
					pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_DragSelection_GridNo, sTempGridNo));

					GetPopup()->addOption(*pOption);
				}
			}
		}

		// cancel option
		swprintf(pStr, pSkillMenuStrings[SKILLMENU_CANCEL]);
		pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Cancel_DragSelection, 0));
		GetPopup()->addOption(*pOption);
	}

	// same y, different x
	SetPos(gSkillSelection.GetMaxPosX(), usTraitMenuPosY);
}

void
DragSelection::Functions(UINT32 aVal)
{
	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier(&pSoldier, gusSelectedSoldier);

	if (pSoldier)
	{
		BOOLEAN result = pSoldier->UseSkill(usSkill, NOWHERE, 0, aVal);
	}

	Cancel();
	gSkillSelection.Cancel();
	gTraitSelection.Cancel();
}

void
DragSelection::FunctionsGridNo(INT32 aVal)
{
	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier(&pSoldier, gusSelectedSoldier);

	if (pSoldier)
	{
		BOOLEAN result = pSoldier->UseSkill(usSkill, aVal, 0, 0);

		// additional dialogue
		//AdditionalTacticalCharacterDialogue_CallsLua(pSoldier, ADE_SKILL_RESULT, usSkill, result);
	}

	Cancel();
	gSkillSelection.Cancel();
	gTraitSelection.Cancel();
}
/////////////////////////////// Drag Selection ////////////////////////////////////////////

/////////////////////////////// Covert Selection ////////////////////////////////////////////

extern BOOLEAN HandleTBCheckCanDisguise(void);
extern BOOLEAN HandleTBCheckCanLoseDisguise(void);
extern BOOLEAN HandleTBCheckCanTestDisguise(void);

void CovertSelection::Setup(UINT32 aVal)
{
	Destroy();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier(&pSoldier, gusSelectedSoldier);

	if (pSoldier == NULL)
		return;

	if (pSoldier->CanUseSkill(aVal, TRUE))
	{
		usSkill = aVal;

		SetupPopup("CovertSelection");

		POPUP_OPTION *pOption;
		CHAR16 pStr[300];

		swprintf(pStr, std::wstring(szActionNameStr[TACTICAL_ACTION_DISGUISE]).c_str());
		pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_CovertSelection, 1));		
		if (!HandleTBCheckCanDisguise())
			pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
		GetPopup()->addOption(*pOption);

		swprintf(pStr, std::wstring(szActionNameStr[TACTICAL_ACTION_LOSE_DISGUISE]).c_str());
		pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_CovertSelection, 2));
		if (!HandleTBCheckCanLoseDisguise())
			pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
		GetPopup()->addOption(*pOption);

		swprintf(pStr, std::wstring(szActionNameStr[TACTICAL_ACTION_TEST_DISGUISE]).c_str());
		pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_CovertSelection, 3));
		if (!HandleTBCheckCanTestDisguise())
			pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
		GetPopup()->addOption(*pOption);

		// cancel option
		swprintf(pStr, pSkillMenuStrings[SKILLMENU_CANCEL]);
		pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Cancel_CovertSelection, 0));
		GetPopup()->addOption(*pOption);
	}

	// same y, different x
	SetPos(gSkillSelection.GetMaxPosX(), usTraitMenuPosY);
}

void CovertSelection::Functions(UINT32 aVal)
{
	Cancel();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier(&pSoldier, gusSelectedSoldier);

	if (pSoldier == NULL)
		return;

	pSoldier->UseSkill(usSkill, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel, aVal);

	Cancel();
	gSkillSelection.Cancel();
	gTraitSelection.Cancel();
}
/////////////////////////////// Covert Selection ////////////////////////////////////////////

/////////////////////////////// Driver Selection ////////////////////////////////////////////

extern BOOLEAN HandleTBCheckCanDisguise(void);
extern BOOLEAN HandleTBCheckCanLoseDisguise(void);
extern BOOLEAN HandleTBCheckCanTestDisguise(void);

void DriverSelection::Setup(UINT32 aVal)
{
	Destroy();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier(&pSoldier, gusSelectedSoldier);

	if (pSoldier == NULL)
		return;

	if (pSoldier->CanUseSkill(aVal, TRUE))
	{
		usSkill = aVal;

		SetupPopup("DriverSelection");

		POPUP_OPTION *pOption;
		CHAR16 pStr[300];

		swprintf(pStr, pSkillMenuStrings[SKILLMENU_DRIVER_HORN]);
		pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_DriverSelection, 1));
		if (!pSoldier->IsDriver())
			pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
		GetPopup()->addOption(*pOption);

		swprintf(pStr, pSkillMenuStrings[SKILLMENU_DRIVER_LIGHTS]);
		pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_DriverSelection, 2));
		if (!pSoldier->IsDriver())
			pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
		GetPopup()->addOption(*pOption);

		swprintf(pStr, pSkillMenuStrings[SKILLMENU_DRIVER_RADIO]);
		pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_DriverSelection, 3));
		if (!pSoldier->IsDriver() || !gGameExternalOptions.fEnableSSA || gGameExternalOptions.ubVolumeSSA == 0)
			pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
		GetPopup()->addOption(*pOption);

		// cancel option
		swprintf(pStr, pSkillMenuStrings[SKILLMENU_CANCEL]);
		pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Cancel_DriverSelection, 0));
		GetPopup()->addOption(*pOption);
	}

	// same y, different x
	SetPos(gSkillSelection.GetMaxPosX(), usTraitMenuPosY);
}

void DriverSelection::Functions(UINT32 aVal)
{
	Cancel();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier(&pSoldier, gusSelectedSoldier);

	if (pSoldier == NULL)
		return;

	pSoldier->UseSkill(usSkill, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel, aVal);

	Cancel();
	gSkillSelection.Cancel();
	gTraitSelection.Cancel();
}
/////////////////////////////// Driver Selection ////////////////////////////////////////////

/////////////////////////////// Officer Selection ////////////////////////////////////////////

extern BOOLEAN HandleTBCheckCanDisguise(void);
extern BOOLEAN HandleTBCheckCanLoseDisguise(void);
extern BOOLEAN HandleTBCheckCanTestDisguise(void);

void OfficerSelection::Setup(UINT32 aVal)
{
	Destroy();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier(&pSoldier, gusSelectedSoldier);

	if (pSoldier == NULL)
		return;

	if (pSoldier->CanUseSkill(aVal, TRUE))
	{
		usSkill = aVal;

		SetupPopup("OfficerSelection");

		POPUP_OPTION *pOption;
		CHAR16 pStr[300];

		swprintf(pStr, pSkillMenuStrings[SKILLMENU_OFFICER_MILITIA]);
		pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_OfficerSelection, 1));
		if (!HAS_SKILL_TRAIT(pSoldier, SQUADLEADER_NT))
			pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
		GetPopup()->addOption(*pOption);

		/*swprintf(pStr, pSkillMenuStrings[SKILLMENU_DRIVER_LIGHTS]);
		pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_DriverSelection, 2));
		if (!pSoldier->IsDriver())
			pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
		GetPopup()->addOption(*pOption);

		swprintf(pStr, pSkillMenuStrings[SKILLMENU_DRIVER_RADIO]);
		pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Function_DriverSelection, 3));
		if (!pSoldier->IsDriver() || !gGameExternalOptions.fEnableSSA || gGameExternalOptions.ubVolumeSSA == 0)
			pOption->setAvail(new popupCallbackFunction<bool, void*>(&Popup_OptionOff, NULL));
		GetPopup()->addOption(*pOption);*/

		// cancel option
		swprintf(pStr, pSkillMenuStrings[SKILLMENU_CANCEL]);
		pOption = new POPUP_OPTION(&std::wstring(pStr), new popupCallbackFunction<void, UINT32>(&Wrapper_Cancel_OfficerSelection, 0));
		GetPopup()->addOption(*pOption);
	}

	// same y, different x
	SetPos(gSkillSelection.GetMaxPosX(), usTraitMenuPosY);
}

void OfficerSelection::Functions(UINT32 aVal)
{
	Cancel();

	SOLDIERTYPE * pSoldier = NULL;

	GetSoldier(&pSoldier, gusSelectedSoldier);

	if (pSoldier == NULL)
		return;

	pSoldier->UseSkill(usSkill, sTraitsMenuTargetGridNo, bTraitsMenuTargetLevel, aVal);

	Cancel();
	gSkillSelection.Cancel();
	gTraitSelection.Cancel();
}
/////////////////////////////// Officer Selection ////////////////////////////////////////////

/**
 *	Flugente: traits menu popup box
 *  This will create other boxes when needed
 */
void TraitsMenu( INT32 usMapPos )
{
	sTraitsMenuTargetGridNo = usMapPos;
	bTraitsMenuTargetLevel = (INT8)gsInterfaceLevel;

	gTraitSelection.Setup(0);
}
