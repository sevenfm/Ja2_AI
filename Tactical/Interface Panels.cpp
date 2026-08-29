#ifdef PRECOMPILEDHEADERS
	#include "Tactical All.h"
#else
	#include "builddefines.h"
	#include <stdio.h>
	#include <time.h>
	#include "sgp.h"
	#include "gameloop.h"
	#include "himage.h"
	#include "vobject.h"
	#include "sysutil.h"
	#include "overhead.h"
	#include "mousesystem.h"
	#include "Button System.h"
	#include "interface.h"
	#include "vsurface.h"
	#include "wcheck.h"
	#include "input.h"
	#include "Handle UI.h"
	#include "renderworld.h"
	#include "cursors.h"
	#include "radar screen.h"
	#include "Font Control.h"
	#include "render dirty.h"
	#include "utilities.h"
	#include "Sound Control.h"
	#include "Interface Panels.h"
	#include "Animation Control.h"
	#include "Soldier Control.h"
	#include "pathai.h"
	#include "weapons.h"
	#include "lighting.h"
	#include "faces.h"
	#include "mapscreen.h"
	#include "message.h"
	#include "text.h"
	#include "Interface Items.h"
	#include "interface control.h"
	#include "interface utils.h"
	#include "Game Clock.h"
	#include "mapscreen.h"
	#include "Soldier Macros.h"
	#include "strategicmap.h"
	#include "soldier functions.h"
	#include "gamescreen.h"
	#include "Assignments.h"
	#include "points.h"
	#include "Squads.h"
	#include "strategic.h"
	#include "Map Screen Interface Map.h"
	#include "overhead map.h"
	#include "Map Screen Interface.h"
	#include "Options Screen.h"
	#include "ShopKeeper Interface.h"
	#include "english.h"
	#include "keys.h"
	#include "Soldier Add.h"
	#include "vehicles.h"
	#include "gamesettings.h"
	#include "dialogue control.h"
	#include "Items.h"
	#include "drugs and alcohol.h"
	#include "los.h"
	#include "opplist.h"
	#include "vobject_blitters.h"
	#include "Finances.h"
	#include "LaptopSave.h"
	#include "Cursor Control.h"
	#include "MessageBoxScreen.h"
	#include "wordwrap.h"
	#include "Boxing.h"
	#include "personnel.h"
	// HEADROCK HAM 3.6: This is required for Stat Progress Bars
	#include "Campaign.h"
	#include "Food.h"	// added by Flugente
	// sevenfm
	#include  "Interface Cursors.h"
	#include "Map Screen Interface Bottom.h"
	#include "message.h"
	#include "Auto Resolve.h"
	#include "cheats.h"
	#include "Queen Command.h"
	#include "MilitiaSquads.h"
	#include "SkillMenu.h"
	#include "HelpScreen.h"
	#include "DisplayCover.h"
	#include "Reinforcement.h"
	#include "SkillCheck.h"
#endif

// sevenfm: Visual Leak Detector
#ifdef _DEBUG
//#include "vld.h"
#endif

//legion by Jazz
#include "Interface Utils.h"
//forward declarations of common classes to eliminate includes
class OBJECTTYPE;
class SOLDIERTYPE;

// anv - extended panels
#include "Merc Contract.h"
#include "personnel.h"
#include "_Ja25Englishtext.h"
extern INT32 CalcTimeLeftOnMercContract( SOLDIERTYPE *pSoldier );
extern INT8 CalculateMercsAchievemntPercentage( INT32 IMercId );

// sevenfm
extern BOOLEAN fAllowBigRadarMap;
extern INT8 GetStealth(SOLDIERTYPE* pSoldier);
extern BOOLEAN TeamHasActiveRadioOperator(INT8 bTeam);
extern INT8	gbItemPointerSrcSlot;
extern INT16 uiNIVSlotType[NUM_INV_SLOTS];

/*
 *
 *								SINGLE MERC BAR COORDS
 *
 *
 *	My opinion about this bars and GUI
 *	i would be very happy if we had c++ classes then we could made something like modules
 *	and peoples could very easily make their own panels, button, labels and other
 *	i have got some ideas but it will take some time to implement then.
 *	it will be hard and time consuming but maybe i will show ya all direction how i would
 *	do it and peoples will continue that path :) joker
 */

/*	Ok so this array values had a constant initializer INV_INTERFACE_START_Y which was constant
 *	we need do load SCREEN_WIDTH _HEIGHT from file to make JA more customizable so if we
 *	will make SCREEN_xxx variable then xxx_INTERFACE_xxx will be also variable.
 *	When we declare static array like here (below) we need constant initializer but we need dynamic
 *	array so i declare only array and i will initialize it in run-time.
 *	any questions?
 *	joker
 */

// CHRISL: Change key value to use dynamically generated NUM_INV_SLOTS instead of static setting
INV_REGION_DESC gSMInvPocketXY[NUM_INV_SLOTS];	// ARRAY FOR INV PANEL INTERFACE ITEM POSITIONS
INV_REGION_DESC gSMCamoXY;			// X, Y Location of cammo region

/*	So this coords buttons, bars etc. also ware declared here as constant and we also need them
 *	more customizable in future so i just declare them and i will initialize them later
 *	any questions? joker
 */
int SM_ITEMDESC_START_X;
int SM_ITEMDESC_START_Y;
int SM_ITEMDESC_HEIGHT;
int SM_ITEMDESC_WIDTH;
extern INT16 ITEMDESC_START_X;
extern INT16 ITEMDESC_START_Y;

int SM_SELMERC_AP_X;
int SM_SELMERC_AP_Y;
int SM_SELMERC_AP_HEIGHT;
int SM_SELMERC_AP_WIDTH;

int SM_SELMERC_HEALTH_X;
int SM_SELMERC_HEALTH_Y;
int SM_SELMERC_HEALTH_WIDTH;
int SM_SELMERC_HEALTH_HEIGHT;

int SM_SELMERC_BREATH_X;
int SM_SELMERC_BREATH_Y;
int SM_SELMERC_BREATH_WIDTH;
int SM_SELMERC_BREATH_HEIGHT;

int SM_SELMERC_MORALE_X;
int SM_SELMERC_MORALE_Y;
int SM_SELMERC_MORALE_WIDTH;
int SM_SELMERC_MORALE_HEIGHT;

int SM_SELMERC_BARS_TIP_X;
int SM_SELMERC_BARS_TIP_Y;
int SM_SELMERC_BARS_TIP_WIDTH;
int SM_SELMERC_BARS_TIP_HEIGHT;

int SM_SELMERCNAME_X;
int SM_SELMERCNAME_Y;
int SM_SELMERCNAME_WIDTH;
int SM_SELMERCNAME_HEIGHT;

int SM_SELMERC_FACE_X;
int SM_SELMERC_FACE_Y;
int SM_SELMERC_FACE_HEIGHT;
int SM_SELMERC_FACE_WIDTH;

int SM_SELMERC_PLATE_X;
int SM_SELMERC_PLATE_Y;
int SM_SELMERC_PLATE_HEIGHT;
int SM_SELMERC_PLATE_WIDTH;

int SM_BODYINV_X;
int SM_BODYINV_Y;

int STATS_TITLE_FONT_COLOR;
int STATS_TEXT_FONT_COLOR;

int SM_TALKB_X;
int SM_TALKB_Y;
int SM_MUTEB_X;
int SM_MUTEB_Y;
int SM_STANCEUPB_X;
int SM_STANCEUPB_Y;
int SM_UPDOWNB_X;
int SM_UPDOWNB_Y;
int SM_CLIMBB_X;
int SM_CLIMBB_Y;
int SM_STANCEDOWNB_X;
int SM_STANCEDOWNB_Y;
int SM_HANDCURSORB_X;
int SM_HANDCURSORB_Y;
int SM_PREVMERCB_X;
int SM_PREVMERCB_Y;
int SM_NEXTMERCB_X;
int SM_NEXTMERCB_Y;
int SM_OPTIONSB_X;
int SM_OPTIONSB_Y;
int SM_BURSTMODEB_X;
int SM_BURSTMODEB_Y;
int SM_LOOKB_X;
int SM_LOOKB_Y;
int SM_STEALTHMODE_X;
int SM_STEALTHMODE_Y;
int SM_DONE_X;
int SM_DONE_Y;
int SM_MAPSCREEN_X;
int SM_MAPSCREEN_Y;
// sevenfm: extra buttons
int SM_SKILLMENU_X;
int SM_SKILLMENU_Y;
int SM_SECTORMENU_X;
int SM_SECTORMENU_Y;
// CHRISL: backpack buttons
int SM_DROPPACK_X;
int SM_DROPPACK_Y;
int SM_ZIPPER_X;
int SM_ZIPPER_Y;
extern int INV_BDROP_X;
extern int INV_BDROP_Y;

int SM_POSITIONB_X;
int SM_POSITIONB_Y;
int SM_POSITIONB_WIDTH;
int SM_POSITIONB_HEIGHT;

int SM_PERCENT_WIDTH;
int SM_PERCENT_HEIGHT;
int SM_ARMOR_X;
int SM_ARMOR_Y;
int SM_ARMOR_LABEL_X;
int SM_ARMOR_LABEL_Y;
int SM_ARMOR_PERCENT_X;
int SM_ARMOR_PERCENT_Y;

int SM_WEIGHT_LABEL_X;
int SM_WEIGHT_LABEL_Y;
int SM_WEIGHT_PERCENT_X;
int SM_WEIGHT_PERCENT_Y;
int SM_WEIGHT_X;
int SM_WEIGHT_Y;

int SM_CAMMO_LABEL_X;
int SM_CAMMO_LABEL_Y;
int SM_CAMMO_PERCENT_X;
int SM_CAMMO_PERCENT_Y;
int SM_CAMMO_X;
int SM_CAMMO_Y;

int SM_STATS_WIDTH;
int SM_STATS_HEIGHT;
int SM_AGI_X;
int SM_AGI_Y;
int SM_DEX_X;
int SM_DEX_Y;
int SM_STR_X;
int SM_STR_Y;
int SM_CHAR_X;
int SM_CHAR_Y;
int SM_WIS_X;
int SM_WIS_Y;
int SM_EXPLVL_X;
int SM_EXPLVL_Y;
int SM_MRKM_X;
int SM_MRKM_Y;
int SM_MECH_X;
int SM_MECH_Y;
int SM_EXPL_X;
int SM_EXPL_Y;
int SM_MED_X;
int SM_MED_Y;

int MONEY_X;
int MONEY_Y;
int MONEY_WIDTH;
int MONEY_HEIGHT;

/*
 *
 *								TEAM BAR COORDS
 *
 */

/*
 *	Now we have only 6 mercs in team but maby we will make it CUSTOMIZABLE hehe i like that word
 *	why we declare array with 12 items? coz we got 2 valuse x and y maby we can work it out
 *	and change INT16 to INV_REGION_DESC for ex.
 */
const int TeamInterfaceMax = NUMBER_OF_SOLDIERS_PER_SQUAD * 2;	// WDS
INT16 sTEAMAPPanelXY[TeamInterfaceMax];
INT16 sTEAMFacesXY[TeamInterfaceMax];
INT16 sTEAMNamesXY[TeamInterfaceMax];
INT16 sTEAMFaceHighlXY[TeamInterfaceMax];
INT16 sTEAMLifeXY[TeamInterfaceMax];
INT16 sTEAMBreathXY[TeamInterfaceMax];
INT16 sTEAMMoraleXY[TeamInterfaceMax];
INT16 sTEAMApXY[TeamInterfaceMax];
INT16 sTEAMBarsXY[TeamInterfaceMax];
INT16 sTEAMHandInvXY[TeamInterfaceMax];

int TM_FACE_WIDTH;
int TM_FACE_HEIGHT;
int TM_APPANEL_HEIGHT;
int TM_APPANEL_WIDTH;

int TM_ENDTURN_X;
int TM_ENDTURN_Y;
int TM_ROSTERMODE_X;
int TM_ROSTERMODE_Y;
int TM_DISK_X;
int TM_DISK_Y;

int TM_NAME_WIDTH;
int TM_NAME_HEIGHT;
int TM_LIFEBAR_WIDTH;
int TM_LIFEBAR_HEIGHT;
int TM_FACEHIGHTL_WIDTH;
int TM_FACEHIGHTL_HEIGHT;
int TM_AP_HEIGHT;
int TM_AP_WIDTH;

int TM_INV_WIDTH;
int TM_INV_HEIGHT;
int TM_INV_HAND1STARTX;
int TM_INV_HAND1STARTY;
int TM_INV_HAND2STARTX;
int TM_INV_HAND2STARTY;
int TM_INV_HAND_SEP;
int TM_INV_HAND_SEPY;

int TM_BARS_REGION_HEIGHT;
int TM_BARS_REGION_WIDTH;

int INDICATOR_BOX_WIDTH;
int INDICATOR_BOX_HEIGHT;

MOUSE_REGION	gSMPanelRegion;
INT8		gbNewItem[ NUM_INV_SLOTS ];
TEAM_PANEL_SLOTS_TYPE	gTeamPanel[ NUM_TEAM_SLOTS ];
/*
 *
 *							VARIOUS PANELS COORDS
 *
 *	now its only clock and location name we will also make them ???
 *	yes yes yes MORE CUSTOMIZABLE :P for moders
 *	i will work with radar screen to allow moving it. or maby someone else will do it
 *	any questions? joker
 */
int	LOCATION_NAME_X;
int	LOCATION_NAME_Y;

/* CHRISL: Added new "TM" variables to allow team and inventory screens to place the location name
independently of each other */
int	LOCATION_NAME_TM_X;
int	LOCATION_NAME_TM_Y;

// CHRISL: Keyring coords moved from Interface Items.cpp
int KEYRING_X;
int KEYRING_Y;
extern UINT32 guiCurrentItemDescriptionScreen;

extern UINT32 guiCurrentItemDescriptionScreen;


typedef enum
{
	STANCEUP_IMAGES = 0,
	UPDOWN_IMAGES,
	CLIMB_IMAGES,
	STANCEDOWN_IMAGES,
	HANDCURSOR_IMAGES,
	PREVMERC_IMAGES,
	NEXTMERC_IMAGES,
	OPTIONS_IMAGES,
//	BURSTMODE_IMAGES,
	LOOK_IMAGES,
	TALK_IMAGES,
	MUTE_IMAGES,
	STANCE_IMAGES,
	DONE_IMAGES,
	MAPSCREEN_IMAGES,
	BACKPACK_IMAGES,
	// sevenfm
	SKILL_MENU_IMAGES,
	SECTOR_MENU_IMAGES,
	//HOP_IMAGES, //legion by Jazz
	NUM_SM_BUTTON_IMAGES
};

typedef enum
{
	ENDTURN_IMAGES = 0,
	ROSTERMODE_IMAGES,
	DISK_IMAGES,
	NUM_TEAM_BUTTON_IMAGES
};

INT32		iSMPanelImages		[ NUM_SM_BUTTON_IMAGES ];
INT32		iBurstButtonImages	[ NUM_WEAPON_MODES ];
INT32		iTEAMPanelImages	[ NUM_TEAM_BUTTON_IMAGES ];

INT32		giSMStealthImages	= -1;
INT32		giSMStealthButton	= -1;

// CHRISL: Backpack buttons
INT32		giSMDropPackImages			= -1;
INT32		giSMDropPackButton			= -1;
INT32		giSMZipperImages			= -1;
INT32		giSMZipperButton			= -1;

BOOLEAN		gfSwitchPanel		= FALSE;
UINT8		gbNewPanel			= SM_PANEL;
UINT8		gubNewPanelParam	= 0;

BOOLEAN		gfUIStanceDifferent						= FALSE;
BOOLEAN		gfAllDisabled							= FALSE;
BOOLEAN		gfSMDisableForItems						= FALSE;
BOOLEAN		gfDisableTacticalPanelButtons			= FALSE;
BOOLEAN		gfAddingMoneyToMercFromPlayersAccount	= FALSE;

BOOLEAN		gfCheckForMouseOverItem				= FALSE;
UINT32		guiMouseOverItemTime				= 0;
INT8		gbCheckForMouseOverItemPos			= 0;
UINT8		gubSelectSMPanelToMerc				= NOBODY;
BOOLEAN		gfReEvaluateDisabledINVPanelButtons = FALSE;

UINT32		guiBrownBackgroundForTeamPanel;
UINT32		guiGoldBackgroundForTeamPanel;

// sevenfm: panel background
BOOLEAN		gfBackgroundPanel = FALSE;
UINT32		guiBackgroundPanel;
BOOLEAN		gfBackgroundLeft = FALSE;
UINT32		guiBackgroundLeft;
BOOLEAN		gfBackgroundRight = FALSE;
UINT32		guiBackgroundRight;

extern	BOOLEAN		gfRerenderInterfaceFromHelpText;
extern	BOOLEAN		gfInKeyRingPopup;
extern	UINT32		guiVEHINV;
extern	INT32		giMapInvDoneButton;
extern	BOOLEAN		gfBeginEndTurn;
extern	SOLDIERTYPE	*gpItemDescSoldier;
extern	BOOLEAN		gfInItemPickupMenu;

extern	void HandleAnyMercInSquadHasCompatibleStuff( UINT8 ubSquad, OBJECTTYPE *pObject, BOOLEAN fReset );
extern	void SetNewItem( SOLDIERTYPE *pSoldier, UINT8 ubInvPos, BOOLEAN fNewItem );
extern	BOOLEAN InternalInitItemDescriptionBox( OBJECTTYPE *pObject, INT16 sX, INT16 sY, UINT8 ubStatusIndex, SOLDIERTYPE *pSoldier, UINT8 ubPosition );
extern	BOOLEAN InternalHandleCompatibleAmmoUI( SOLDIERTYPE *pSoldier, OBJECTTYPE *pTestObject, BOOLEAN fOn	);
extern	BOOLEAN CompatibleAmmoForGun( OBJECTTYPE *pTryObject, OBJECTTYPE *pTestObject );//dnl ch66 070913

BOOLEAN IsMouseInRegion( MOUSE_REGION *pRegion );
void HandleMouseOverSoldierFaceForContMove( SOLDIERTYPE *pSoldier, BOOLEAN fOn );
void HandlePlayerTeamMemberDeathAfterSkullAnimation( SOLDIERTYPE *pSoldier );
void EnableButtonsForInItemBox( BOOLEAN fDisable );
void ConfirmationToDepositMoneyToPlayersAccount( UINT8 ubExitValue );
void MergeMessageBoxCallBack( UINT8 ubExitValue );

UINT8	gubHandPos;
UINT16	gusOldItemIndex;
UINT16	gusNewItemIndex;
BOOLEAN gfDeductPoints;

// HEADROCK HAM 4: This variable tells the program what "page" of the description box we are currently looking at.
// By saving this in memory, we tell the DescBox which background and values to draw, and make sure that we end up
// on the same page every time we open the description box. UDB buttons allow switching between pages.
UINT8 gubDescBoxPage;
// silversurfer: Now we also have a secondary page on the general tab for weapons so we can look at their secondary attributes.
UINT8 gubDescGenPage;
// Record which line we're looking at. This only applied to the Advanced tab in description boxes.
UINT8 gubDescBoxLine;
// Record how many lines this item has to display. This only applies to the Advanced tab.
UINT8 gubDescBoxTotalAdvLines;

INT32	iSMPanelButtons[ NUM_SM_BUTTONS ];
INT32	iTEAMPanelButtons[ NUM_TEAM_BUTTONS ];

// Video Surface for Single Merc Panel
UINT32	guiSMPanel;
UINT32	guiSMObjects;
UINT32	guiSMObjects2;
UINT32	guiSecItemHiddenVO;
UINT32	guiTEAMPanel;
UINT32	guiTEAMObjects;

// Globals for various mouse regions
MOUSE_REGION	gSM_SELMERCPanelRegion;
MOUSE_REGION	gSM_SELMERCBarsRegion;
MOUSE_REGION	gSM_SELMERCMoneyRegion;
MOUSE_REGION	gSM_SELMERCEnemyIndicatorRegion;
MOUSE_REGION	gTEAM_PanelRegion;
MOUSE_REGION	gTEAM_FaceRegions[ NUMBER_OF_SOLDIERS_PER_SQUAD ];
MOUSE_REGION	gTEAM_BarsRegions[ NUMBER_OF_SOLDIERS_PER_SQUAD ];
MOUSE_REGION	gTEAM_LeftBarsRegions[ NUMBER_OF_SOLDIERS_PER_SQUAD ];
MOUSE_REGION	gTEAM_FirstHandInv[ NUMBER_OF_SOLDIERS_PER_SQUAD ];
MOUSE_REGION	gTEAM_SecondHandInv[ NUMBER_OF_SOLDIERS_PER_SQUAD ];
MOUSE_REGION	gTEAM_EnemyIndicator[ NUMBER_OF_SOLDIERS_PER_SQUAD ];

// sevenfm: extended tactical panel
MOUSE_REGION	gTEAM_LeftPanelRegion;
MOUSE_REGION	gTEAM_RightPanelRegion;
MOUSE_REGION	gTEAM_ExtTacticsRegion;
//MOUSE_REGION	gTEAM_ExtScrollRegion;

MOUSE_REGION	gSM_LeftPanelRegion;
MOUSE_REGION	gSM_RightPanelRegion;
//MOUSE_REGION	g_RightPanelRegion;

class ActionData
{
public:
	UINT32 id;
	std::wstring name;
	std::wstring tooltip;

	ActionData(UINT32 new_id, std::wstring new_name, std::wstring new_tooltip)
	{
		id = new_id;
		name = new_name;
		tooltip = new_tooltip;
	}

	ActionData(UINT32 new_id, UINT32 str_id)
	{
		id = new_id;
		name = std::wstring(szActionNameStr[str_id]);
		tooltip = std::wstring(szActionTooltipStr[str_id]);
	}

	ActionData(UINT32 new_id)
	{
		AssertMsg(new_id < TACTICAL_ACTION_MAX, String("new_id %d should be <= TACTICAL_ACTION_MAX %d", new_id, TACTICAL_ACTION_MAX));

		id = new_id;
		name = std::wstring(szActionNameStr[new_id]);
		tooltip = std::wstring(szActionTooltipStr[new_id]);
	}
};

std::vector<ActionData> gActionDataVector;
std::vector<ActionData> gActionDataVector2;

MOUSE_REGION gActionRegionVector[1024];
UINT16 gsActionRegionNum = 0;

MOUSE_REGION	gSM_SELMERCWeightRegion; // Added by Flugente
MOUSE_REGION	gSM_SELMERCCamoRegion; // Added by Sandro

BOOLEAN			gfTEAM_HandInvDispText[ NUMBER_OF_SOLDIERS_PER_SQUAD ][ NUM_INV_SLOTS ];
BOOLEAN			gfSM_HandInvDispText[ NUM_INV_SLOTS ];

void HelpTextDoneCallback( void );

// Globals - for one - the current merc here
INT8				gbSMCurStanceObj;
UINT16				gusSMCurrentMerc = 0;
SOLDIERTYPE			*gpSMCurrentMerc = NULL;
// CHRISL:
extern	INT8		gbCompatibleApplyItem;
extern	SOLDIERTYPE *gpItemPopupSoldier;

INT8		gbStanceButPos[2][3][3] =
{
	// NON-STEALTH
	16,		14,		15,
	10,		8,		9,
	22,		20,		21,
	// STEALTH MODE
	13,		11,		12,
	7,		5,		6,
	19,		17,		18
};

//CHRISL: New arrays to deal with backpack button positions
INT8		gbDropPackButPos[2][2] =
{
	2,	3,
	3,	2
};
INT8		gbZipperButPos[2][2] =
{
	0,	1,
	1,	0
};

//Legion by Jazz
//void BtnOknoCallback(GUI_BUTTON *btn,INT32 reason); 
//void BtnFenceCallback(GUI_BUTTON *btn,INT32 reason); 

// Mouse button and region callbacks
//void BtnPositionCallback( GUI_BUTTON *btn, INT32 reason );
//void BtnMovementCallback( GUI_BUTTON *btn, INT32 reason );
void SelectedMercButtonCallback( MOUSE_REGION * pRegion, INT32 iReason );
void SelectedMercButtonMoveCallback( MOUSE_REGION *pRegion, INT32 iReason );
void SelectedMercEnemyIndicatorCallback( MOUSE_REGION * pRegion, INT32 iReason );
void SMInvMoveCallback( MOUSE_REGION * pRegion, INT32 iReason );
void SMInvClickCallback( MOUSE_REGION * pRegion, INT32 iReason );
void SMInvClickCamoCallback( MOUSE_REGION * pRegion, INT32 iReason );
void SMInvMoveCammoCallback( MOUSE_REGION * pRegion, INT32 iReason );
void SMInvMoneyButtonCallback( MOUSE_REGION * pRegion, INT32 iReason );

// SINGLE MERC PANEL BUTTON CALLBACKS
void BtnStealthModeCallback(GUI_BUTTON *btn,INT32 reason);
void BtnStanceUpCallback(GUI_BUTTON *btn,INT32 reason);
void BtnUpdownCallback(GUI_BUTTON *btn,INT32 reason);
void BtnClimbCallback(GUI_BUTTON *btn,INT32 reason);
void BtnStanceDownCallback(GUI_BUTTON *btn,INT32 reason);
void BtnHandCursorCallback(GUI_BUTTON *btn,INT32 reason);
void BtnTalkCallback(GUI_BUTTON *btn,INT32 reason);
void BtnMuteCallback(GUI_BUTTON *btn,INT32 reason);
void BtnSMDoneCallback(GUI_BUTTON *btn,INT32 reason);
void BtnMapScreenCallback(GUI_BUTTON *btn,INT32 reason);
// sevenfm
void BtnSkillMenuCallback(GUI_BUTTON *btn, INT32 reason);
void BtnSectorMenuCallback(GUI_BUTTON *btn, INT32 reason);

// CHRISL: New callback functions for backpack buttons
void BtnDropPackCallback(GUI_BUTTON *btn,INT32 reason);
void BtnZipperCallback(GUI_BUTTON *btn,INT32 reason);
BOOLEAN ChangeZipperStatus(SOLDIERTYPE *pSoldier, BOOLEAN newStatus);

void BtnPrevMercCallback(GUI_BUTTON *btn,INT32 reason);
void BtnNextMercCallback(GUI_BUTTON *btn,INT32 reason);
void BtnOptionsCallback(GUI_BUTTON *btn,INT32 reason);
void BtnBurstModeCallback(GUI_BUTTON *btn,INT32 reason);
void BtnLookCallback(GUI_BUTTON *btn,INT32 reason);
void BtnPositionShowCallback(GUI_BUTTON *btn,INT32 reason);
void InvPanelButtonClickCallback( MOUSE_REGION * pRegion, INT32 iReason );

// TEAM PANEL BUTTON CALLBACKS
void BtnEndTurnCallback(GUI_BUTTON *btn,INT32 reason);
void BtnRostermodeCallback(GUI_BUTTON *btn,INT32 reason);
void BtnSquadCallback(GUI_BUTTON *btn,INT32 reason);
void MercFacePanelCallback( MOUSE_REGION * pRegion, INT32 iReason );
void MercFacePanelMoveCallback( MOUSE_REGION * pRegion, INT32 iReason );
void TMFirstHandInvCallback( MOUSE_REGION * pRegion, INT32 iReason );
void TMClickFirstHandInvCallback( MOUSE_REGION * pRegion, INT32 iReason );
void TMClickSecondHandInvCallback( MOUSE_REGION * pRegion, INT32 iReason );
void EnemyIndicatorClickCallback( MOUSE_REGION * pRegion, INT32 iReason );

// sevenfm: scroll area for tactical panel
//void ScrollPanelCallback(MOUSE_REGION * pRegion, INT32 iReason);

// sevenfm: extended tactical panel
void TacticalPanelCallback( MOUSE_REGION * pRegion, INT32 iReason );
void LeftPanelCallback(MOUSE_REGION * pRegion, INT32 iReason);
void RightPanelCallback(MOUSE_REGION * pRegion, INT32 iReason);
INT32 gsTacticalMessagesOffset = 0;
INT32 gsTacticalActionOffset = 0;

UINT8 ubTacticalMessagePanelMode = LEFT_PANEL_MODE_STATISTICS;

INT16 gsPanelWideWidth, gsPanelWideHeight, gsPanelWideX, gsPanelWideY;
INT16 gsPanelTotalWidth;
INT16 gsPanelWindowX, gsPanelWindowWidth;
INT16 gsPanelWindowY, gsPanelWindowHeight;

extern std::vector<std::wstring> gTacticalMessageVector;
extern std::vector<UINT16> gusTacticalMessageColorVector;
extern std::vector<std::wstring> gTacticalDebugMessageVector;
extern std::vector<std::wstring> gTacticalAIMessageVector;
extern std::vector<std::wstring> gTacticalHelpMessageVector;

void RenderSoldierTeamInv( SOLDIERTYPE *pSoldier, INT16 sX, INT16 sY, UINT8 ubPanelNum, BOOLEAN fDirty );

// keyring stuff
void KeyRingItemPanelButtonCallback( MOUSE_REGION * pRegion, INT32 iReason );

void UpdateSelectedSoldier( UINT16 usSoldierID, BOOLEAN fSelect );

void CheckForFacePanelStartAnims( SOLDIERTYPE *pSoldier, INT16 sPanelX, INT16 sPanelY );
void HandleSoldierFaceFlash( SOLDIERTYPE *pSoldier, INT16 sFaceX, INT16 sFaceY );
BOOLEAN PlayerExistsInSlot( UINT8 ubID );
void UpdateStatColor( UINT32 uiTimer, BOOLEAN fIncrease, BOOLEAN fDamaged, BOOLEAN fAugmented, BOOLEAN fInShock = FALSE, BOOLEAN fInspired = FALSE, BOOLEAN fLeader = FALSE, BOOLEAN fCarryWeight = FALSE);

extern void UpdateItemHatches();

extern SOLDIERTYPE *FindNextActiveSquad( SOLDIERTYPE *pSoldier );
extern SOLDIERTYPE *FindPrevActiveSquad(SOLDIERTYPE *pSoldier);

extern BOOLEAN gfMouseLockedOnBorder;

// Wraps up check for AP-s get from a different soldier for in a vehicle...
INT16 GetUIApsToDisplay( SOLDIERTYPE *pSoldier )
{
	return pSoldier->bActionPoints;
}

void CheckForDisabledForGiveItem( )
{
	INT16			sDist;
	INT32 sDestGridNo;
	INT8			bDestLevel;
	INT32			cnt;
	SOLDIERTYPE		*pSoldier;
	UINT8			ubSrcSoldier;

	// CHRISL: If Merc hasn't been set, default to first merc
	if(gpSMCurrentMerc==NULL)
		gpSMCurrentMerc = MercPtrs[ 0 ];
	Assert( gpSMCurrentMerc != NULL);

	if ( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
	{
		gfSMDisableForItems = !CanMercInteractWithSelectedShopkeeper( gpSMCurrentMerc );
		return;
	}

	// Default to true
	gfSMDisableForItems = TRUE;

	// ATE: Is the current merc unconscious.....
	if ( gpSMCurrentMerc->stats.bLife < OKLIFE && gpItemPointer != NULL )
	{
		// Go through each merc and see if there is one closeby....
		cnt = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;
		for ( pSoldier = MercPtrs[ cnt ]; cnt <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; cnt++,pSoldier++)
		{
			if ( pSoldier->bActive && pSoldier->stats.bLife >= OKLIFE && !( pSoldier->flags.uiStatusFlags & SOLDIER_VEHICLE ) && !AM_A_ROBOT( pSoldier ) && pSoldier->bInSector && IsMercOnCurrentSquad( pSoldier ) )
			{
				sDist = PythSpacesAway( gpSMCurrentMerc->sGridNo, pSoldier->sGridNo );

				// Check LOS....
				if ( SoldierTo3DLocationLineOfSightTest( pSoldier, gpSMCurrentMerc->sGridNo,  gpSMCurrentMerc->pathing.bLevel, 3, TRUE, CALC_FROM_ALL_DIRS ) )
				{
					if ( sDist <= PASSING_ITEM_DISTANCE_NOTOKLIFE )
					{
						gfSMDisableForItems = FALSE;
						break;	// found one, no need to keep looking
					}
				}
			}
		}
	}
	else
	{
		ubSrcSoldier = (UINT8)gusSelectedSoldier;

		if ( gpItemPointer != NULL )
		{
			ubSrcSoldier = gpItemPointerSoldier->ubID;
		}

		// OK buddy, check our currently selected merc and disable/enable if not close enough...
		if ( ubSrcSoldier != NOBODY )
		{
			if ( gusSMCurrentMerc != ubSrcSoldier )
			{
				sDestGridNo = MercPtrs[ gusSMCurrentMerc ]->sGridNo;
				bDestLevel	= MercPtrs[ gusSMCurrentMerc ]->pathing.bLevel;

				// Get distance....
				sDist = PythSpacesAway( MercPtrs[ ubSrcSoldier ]->sGridNo, sDestGridNo );

				// Check LOS....
				if ( SoldierTo3DLocationLineOfSightTest( MercPtrs[ ubSrcSoldier ], sDestGridNo,  bDestLevel, 3, TRUE, CALC_FROM_ALL_DIRS )  )
				{
					// UNCONSCIOUS GUYS ONLY 1 tile AWAY
					if ( MercPtrs[ gusSMCurrentMerc ]->stats.bLife < CONSCIOUSNESS )
					{
						if ( sDist <= PASSING_ITEM_DISTANCE_NOTOKLIFE )
						{
							gfSMDisableForItems = FALSE;
						}
					}
					else if ( sDist <= PASSING_ITEM_DISTANCE_OKLIFE )
					{
						gfSMDisableForItems = FALSE;
					}
				}
				// anv: passengers in the same vehicle can pass items freely
				else if (MercPtrs[ubSrcSoldier]->iVehicleId != -1 && MercPtrs[ubSrcSoldier]->iVehicleId == MercPtrs[gusSMCurrentMerc]->iVehicleId)
				{
					gfSMDisableForItems = FALSE;
				}
			}
			else
			{
				gfSMDisableForItems = FALSE;
			}
		}
		else
		{
			gfSMDisableForItems = FALSE;
		}
	}
}

void SetSMPanelCurrentMerc( UINT8 ubNewID )
{
	gubSelectSMPanelToMerc = NOBODY;

	gusSMCurrentMerc = ubNewID;

	gpSMCurrentMerc = MercPtrs[ ubNewID ];

	// Disable all faces
	SetAllAutoFacesInactive( );

	// Turn off compat ammo....
	HandleCompatibleAmmoUI(gpSMCurrentMerc, NO_SLOT, FALSE);
	if (gpItemPointer != NULL)
	{
		gfCheckForMouseOverItem = TRUE;
	}

	// Remove item desc panel if one up....
	if ( gfInItemDescBox )
	{
		DeleteItemDescriptionBox( );
	}

	// Remove stack popup panel if one up....
	if ( InItemStackPopup( ) || InSectorStackPopup( ) )
	{
		DeleteItemStackPopup( );
	}

	// Remove keyring popup panel if one up....
	if ( InKeyRingPopup( ) )
	{
		DeleteKeyRingPopup( );
	}

	if ( gfInItemPickupMenu )
	{
		gfSMDisableForItems = TRUE;
	}
	else
	{
		if ( ( gpItemPointer != NULL || guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE || gpSMCurrentMerc->stats.bLife < OKLIFE ) )
		{
			CheckForDisabledForGiveItem( );
		}
		else
		{
			gfSMDisableForItems = FALSE;
		}
	}

	if ( gpItemPointer != NULL )
	{
		ReevaluateItemHatches( gpSMCurrentMerc, FALSE );
	}


	DisableInvRegions( gfSMDisableForItems );

	fInterfacePanelDirty = DIRTYLEVEL2;

	gfUIStanceDifferent = TRUE;

	UpdateSMPanel( );	
}


void UpdateForContOverPortrait( SOLDIERTYPE *pSoldier, BOOLEAN fOn )
{
	INT32 cnt;

	if ( gsCurInterfacePanel == SM_PANEL )
	{
		if ( gpSMCurrentMerc != NULL )
		{
			// Check if mouse is in region and if so, adjust...
			if ( IsMouseInRegion( &gSM_SELMERCPanelRegion ) )
			{
				HandleMouseOverSoldierFaceForContMove( gpSMCurrentMerc, fOn );
			}
		}
	}
	else
	{
		for ( cnt = 0; cnt < NUMBER_OF_SOLDIERS_PER_SQUAD; cnt++ )
		{
			if ( gTeamPanel[ cnt ].ubID == pSoldier->ubID )
			{
				if ( IsMouseInRegion( &gTEAM_FaceRegions[ cnt ] ) )
				{
					HandleMouseOverSoldierFaceForContMove( pSoldier, fOn );
				}
			}
		}
	}
}


void UpdateSMPanel()
{
	BOOLEAN						fNearHeigherLevel;
	BOOLEAN						fNearLowerLevel;
	INT8							bDirection;
	UINT8							ubStanceState;
	
	//jump window by Jazz
	//BOOLEAN						fNearLowerLevelOkno;
	//BOOLEAN						fNearHeigherLevelOkno;

	if (TileIsOutOfBounds(gpSMCurrentMerc->sGridNo))
	{
		return;
	}

	// Stance
	ubStanceState = gpSMCurrentMerc->ubDesiredHeight;

	if ( ubStanceState == NO_DESIRED_HEIGHT )
	{
		ubStanceState = gAnimControl[ gpSMCurrentMerc->usAnimState ].ubEndHeight;
	}


	switch( ubStanceState )
	{
		case ANIM_STAND:

			gbSMCurStanceObj = 0;
			DisableButton( iSMPanelButtons[ STANCEUP_BUTTON ] );
			EnableButton( iSMPanelButtons[ STANCEDOWN_BUTTON ] );

			// Disable if we cannot do this!
			if ( !IsValidStance( gpSMCurrentMerc, ANIM_CROUCH ) )
			{
				DisableButton( iSMPanelButtons[ STANCEDOWN_BUTTON ] );
			}
			break;

		case ANIM_PRONE:
			gbSMCurStanceObj = 2;
			DisableButton( iSMPanelButtons[ STANCEDOWN_BUTTON ] );
			EnableButton( iSMPanelButtons[ STANCEUP_BUTTON ] );
			break;

		case ANIM_CROUCH:
			gbSMCurStanceObj = 1;
			EnableButton( iSMPanelButtons[ STANCEUP_BUTTON ] );
			EnableButton( iSMPanelButtons[ STANCEDOWN_BUTTON ] );

			// Disable if we cannot do this!
			if ( !IsValidStance( gpSMCurrentMerc, ANIM_PRONE ) )
			{
				DisableButton( iSMPanelButtons[ STANCEDOWN_BUTTON ] );
			}
			break;
	}

	// Stance button done wether we're disabled or not...
	if ( gfUIStanceDifferent )
	{
		//Remove old
		if ( giSMStealthButton != -1 )
		{
				RemoveButton( giSMStealthButton );
		}
		if ( giSMStealthImages != -1 )
		{
			UnloadButtonImage( giSMStealthImages );
		}

		// Make new
		giSMStealthImages = UseLoadedButtonImage( iSMPanelImages[ STANCE_IMAGES	] ,gbStanceButPos[ gpSMCurrentMerc->bStealthMode ][gbSMCurStanceObj][0] ,gbStanceButPos[ gpSMCurrentMerc->bStealthMode ][gbSMCurStanceObj][1],-1,gbStanceButPos[ gpSMCurrentMerc->bStealthMode ][gbSMCurStanceObj][2],-1 );

		giSMStealthButton = QuickCreateButton( giSMStealthImages, SM_STEALTHMODE_X, SM_STEALTHMODE_Y,
										BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
										DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnStealthModeCallback );
		RenderBackpackButtons(ACTIVATE_BUTTON);	/* CHRISL: Needed for new inventory backpack buttons */

		SetButtonFastHelpText( giSMStealthButton, TacticalStr[ TOGGLE_STEALTH_MODE_POPUPTEXT ] );

		gfUIStanceDifferent = FALSE;

		if ( gfAllDisabled )
		{
			if ( giSMStealthButton != -1 )
			{
					DisableButton( giSMStealthButton );
			}
			RenderBackpackButtons(DISABLE_BUTTON);	/* CHRISL: Needed for new inventory backpack buttons */
		}
	}

	if ( gfAllDisabled )
	{
		return;
	}

	CheckForReEvaluateDisabledINVPanelButtons( );

	// Check for any newly added items we need.....
	if ( gpSMCurrentMerc->flags.fCheckForNewlyAddedItems )
	{
		// Startup any newly added items....
		CheckForAnyNewlyAddedItems( gpSMCurrentMerc );

		gpSMCurrentMerc->flags.fCheckForNewlyAddedItems = FALSE;
	}

	// Set Disable /Enable UI based on buddy's stats
	if (ButtonList[ iSMPanelButtons[ BURSTMODE_BUTTON ] ]->ImageNum != (UINT32) iBurstButtonImages[ gpSMCurrentMerc->bWeaponMode ] )
	{
		ButtonList[ iSMPanelButtons[ BURSTMODE_BUTTON ] ]->ImageNum = iBurstButtonImages[ gpSMCurrentMerc->bWeaponMode ];
		ButtonList[ iSMPanelButtons[ BURSTMODE_BUTTON ] ]->uiFlags |= BUTTON_DIRTY;
	}

	/*
	if ( gpSMCurrentMerc->bDoBurst )
	{
		if ( !ButtonList[ iSMPanelButtons[ BURSTMODE_BUTTON ] ]->ubToggleButtonActivated )
		{
			ButtonList[ iSMPanelButtons[ BURSTMODE_BUTTON ] ]->uiFlags |= BUTTON_CLICKED_ON;
		}
	}
	else
	{
		if ( !ButtonList[ iSMPanelButtons[ BURSTMODE_BUTTON ] ]->ubToggleButtonActivated )
		{
			ButtonList[ iSMPanelButtons[ BURSTMODE_BUTTON ] ]->uiFlags &= ( ~BUTTON_CLICKED_ON );
		}
	}
	*/

	// Toggle MUTE button...
	if ( gpSMCurrentMerc->flags.uiStatusFlags & SOLDIER_MUTE )
	{
		if ( !ButtonList[ iSMPanelButtons[ MUTE_BUTTON ] ]->ubToggleButtonActivated )
		{
			ButtonList[ iSMPanelButtons[ MUTE_BUTTON ] ]->uiFlags |= BUTTON_CLICKED_ON;
		}
	}
	else
	{
		if ( !ButtonList[ iSMPanelButtons[ MUTE_BUTTON ] ]->ubToggleButtonActivated )
		{
			ButtonList[ iSMPanelButtons[ MUTE_BUTTON ] ]->uiFlags &= ( ~BUTTON_CLICKED_ON );
		}
	}

	DisableButton( iSMPanelButtons[ CLIMB_BUTTON ] );
	
	GetMercClimbDirection( gpSMCurrentMerc->ubID, &fNearLowerLevel, &fNearHeigherLevel );

	if (fNearLowerLevel || fNearHeigherLevel)
	{
		if (IsValidStance(gpSMCurrentMerc, ANIM_CROUCH) && EnoughPoints(gpSMCurrentMerc, GetAPsToClimbRoof(gpSMCurrentMerc, fNearLowerLevel), GetBPsToClimbRoof(gpSMCurrentMerc, fNearLowerLevel), FALSE))
		{
			EnableButton(iSMPanelButtons[CLIMB_BUTTON]);
		}
	}
	
	if (gGameExternalOptions.fCanClimbOnWalls)
	{
		if (FindWallJumpDirection(gpSMCurrentMerc, gpSMCurrentMerc->sGridNo, gpSMCurrentMerc->ubDirection, &bDirection))
		{
			if (IsValidStance(gpSMCurrentMerc, ANIM_CROUCH) && EnoughPoints(gpSMCurrentMerc, GetAPsToJumpWall(gpSMCurrentMerc, FALSE), GetBPsToJumpWall(gpSMCurrentMerc, FALSE), FALSE))
			{
				EnableButton(iSMPanelButtons[CLIMB_BUTTON]);
			}
		}
	}

	// sevenfm: disabled
	/*if ( FindFenceJumpDirection( gpSMCurrentMerc, gpSMCurrentMerc->sGridNo, gpSMCurrentMerc->ubDirection, &bDirection ) )
	{
		if (IsValidStance(gpSMCurrentMerc, ANIM_CROUCH) && EnoughPoints( gpSMCurrentMerc, GetAPsToJumpFence( gpSMCurrentMerc, FALSE ), 0, FALSE ) )
		{
			EnableButton( iSMPanelButtons[ CLIMB_BUTTON ] );
		}
	}*/

	if ( (gTacticalStatus.ubCurrentTeam != gbPlayerNum) || (gTacticalStatus.uiFlags & REALTIME ) || !(gTacticalStatus.uiFlags & INCOMBAT ) )
	{
		DisableButton( iSMPanelButtons[ SM_DONE_BUTTON ] );
	}
	else if ( !gfAllDisabled )
	{
		EnableButton( iSMPanelButtons[ SM_DONE_BUTTON ] );
	}

//	if ( gpSMCurrentMerc->bUIInterfaceLevel > 0 )
	if ( gsInterfaceLevel > 0 )
	{
		if ( !ButtonList[ iSMPanelButtons[ UPDOWN_BUTTON ] ]->ubToggleButtonActivated )
		{
			ButtonList[ iSMPanelButtons[ UPDOWN_BUTTON ] ]->uiFlags |= (BUTTON_CLICKED_ON);
		}
	}
	else
	{
		if ( !ButtonList[ iSMPanelButtons[ UPDOWN_BUTTON ] ]->ubToggleButtonActivated )
		{
			ButtonList[ iSMPanelButtons[ UPDOWN_BUTTON ] ]->uiFlags &= ( ~BUTTON_CLICKED_ON );
		}
	}

	if ( gCurrentUIMode == HANDCURSOR_MODE )
	{
		if ( !ButtonList[ iSMPanelButtons[ HANDCURSOR_BUTTON ] ]->ubToggleButtonActivated )
		{
			ButtonList[ iSMPanelButtons[ HANDCURSOR_BUTTON ] ]->uiFlags |= BUTTON_CLICKED_ON;
		}
	}
	else
	{
		if ( !ButtonList[ iSMPanelButtons[ HANDCURSOR_BUTTON ] ]->ubToggleButtonActivated )
		{
			ButtonList[ iSMPanelButtons[ HANDCURSOR_BUTTON ] ]->uiFlags &= ( ~BUTTON_CLICKED_ON );
		}
	}

	if ( gCurrentUIMode == TALKCURSOR_MODE )
	{
		if ( !ButtonList[ iSMPanelButtons[ TALK_BUTTON ] ]->ubToggleButtonActivated )
		{
			ButtonList[ iSMPanelButtons[ TALK_BUTTON ] ]->uiFlags |= BUTTON_CLICKED_ON;
		}
	}
	else
	{
		if ( !ButtonList[ iSMPanelButtons[ TALK_BUTTON ] ]->ubToggleButtonActivated )
		{
			ButtonList[ iSMPanelButtons[ TALK_BUTTON ] ]->uiFlags &= ( ~BUTTON_CLICKED_ON );
		}
	}

	if ( gCurrentUIMode == LOOKCURSOR_MODE )
	{
		if ( !ButtonList[ iSMPanelButtons[ LOOK_BUTTON ] ]->ubToggleButtonActivated )
		{
			ButtonList[ iSMPanelButtons[ LOOK_BUTTON ] ]->uiFlags |= BUTTON_CLICKED_ON;
		}
	}
	else
	{
		if ( !ButtonList[ iSMPanelButtons[ LOOK_BUTTON ] ]->ubToggleButtonActivated )
		{
			ButtonList[ iSMPanelButtons[ LOOK_BUTTON ] ]->uiFlags &= ( ~BUTTON_CLICKED_ON );
		}
	}

	// If not selected ( or dead ), disable/gray some buttons
	if ( gusSelectedSoldier != gpSMCurrentMerc->ubID || ( gpSMCurrentMerc->stats.bLife < OKLIFE ) || (gTacticalStatus.ubCurrentTeam != gbPlayerNum) || gfSMDisableForItems )
	{
		DisableButton( iSMPanelButtons[ CLIMB_BUTTON ] );
		DisableButton( iSMPanelButtons[ BURSTMODE_BUTTON ] );
		DisableButton( iSMPanelButtons[ STANCEUP_BUTTON ] );
		DisableButton( iSMPanelButtons[ STANCEDOWN_BUTTON ] );
		DisableButton( iSMPanelButtons[ LOOK_BUTTON ] );
		DisableButton( iSMPanelButtons[ UPDOWN_BUTTON ] );
		DisableButton( iSMPanelButtons[ HANDCURSOR_BUTTON ] );
		if ( giSMStealthButton != -1 )
		{
				DisableButton( giSMStealthButton );
		}
		RenderBackpackButtons(DISABLE_BUTTON);	/* CHRISL: Needed for new inventory backpack buttons */
	}
	else
	{
		// Enable some buttons!
		// r8700
		if (((IsGunAutofireCapable(&gpSMCurrentMerc->inv[HANDPOS]) || IsGunBurstCapable(&gpSMCurrentMerc->inv[HANDPOS], FALSE, gpSMCurrentMerc)) && !Weapon[gpSMCurrentMerc->inv[HANDPOS].usItem].NoSemiAuto)
			|| IsGrenadeLauncherAttached(&(gpSMCurrentMerc->inv[HANDPOS]))
			|| IsWeaponAttached(&(gpSMCurrentMerc->inv[HANDPOS]), IC_GUN)
			|| IsWeaponAttached(&(gpSMCurrentMerc->inv[HANDPOS]), IC_BLADE))
		{
			EnableButton( iSMPanelButtons[ BURSTMODE_BUTTON ] );
		}
		else
		{
			DisableButton( iSMPanelButtons[ BURSTMODE_BUTTON ] );
		}
		EnableButton( iSMPanelButtons[ LOOK_BUTTON ] );
		EnableButton( iSMPanelButtons[ UPDOWN_BUTTON ] );
		EnableButton( iSMPanelButtons[ HANDCURSOR_BUTTON ] );

		if ( giSMStealthButton != -1 )
		{
				EnableButton( giSMStealthButton );
		}
		if(!gfInItemDescBox)
			RenderBackpackButtons(ENABLE_BUTTON);	/* CHRISL: Needed for new inventory backpack buttons */
	}

	// CJC Dec 4 2002: or if item pickup menu is up
	if ( (gTacticalStatus.uiFlags & ENGAGED_IN_CONV) || gfInItemPickupMenu)
	{
		DisableButton( iSMPanelButtons[ SM_MAP_SCREEN_BUTTON ] );

		if (UsingNewInventorySystem())
		{
			DisableButton(iSMPanelButtons[SM_SKILL_MENU_BUTTON]);
			DisableButton(iSMPanelButtons[SM_SECTOR_MENU_BUTTON]);
		}
	}
	else
	{
		EnableButton( iSMPanelButtons[ SM_MAP_SCREEN_BUTTON ] );

		if (UsingNewInventorySystem())
		{
			EnableButton(iSMPanelButtons[SM_SKILL_MENU_BUTTON]);
			EnableButton(iSMPanelButtons[SM_SECTOR_MENU_BUTTON]);
		}
	}

}

extern INT8		gbInvalidPlacementSlot[ NUM_INV_SLOTS ];


void ReevaluateItemHatches( SOLDIERTYPE *pSoldier, BOOLEAN fAllValid )
{
	UINT32	cnt;

	// if there's an item in the cursor and we're not supposed to just make them all valid
	if ( ( gpItemPointer != NULL ) && !fAllValid )
	{
		// check all inventory positions and mark the ones where cursor item won't fit as invalid
		UINT32 invsize = pSoldier->inv.size();
		for ( cnt = 0; cnt < invsize; ++cnt )
		{
			gbInvalidPlacementSlot[ cnt ] = !CanItemFitInPosition( pSoldier, gpItemPointer, (INT8)cnt, FALSE );

			// !!! ATTACHING/MERGING ITEMS IN MAP SCREEN IS NOT SUPPORTED !!!
			// CJC: seems to be supported now...
			//if( guiCurrentScreen != MAP_SCREEN )
			{
				// Check attachments, override to valid placement if valid merge...
				if ( ValidAttachment( gpItemPointer->usItem, &(pSoldier->inv[ cnt ]) ) )
				{
					gbInvalidPlacementSlot[ cnt ] = FALSE;
				}

				if ( ValidMerge( gpItemPointer->usItem, pSoldier->inv[ cnt ].usItem ) )
				{
					gbInvalidPlacementSlot[ cnt ] = FALSE;
				}
			}
		}
	}
	else
	{
		// mark all inventory positions as valid
		for ( cnt = 0; cnt < NUM_INV_SLOTS; ++cnt )
		{
			gbInvalidPlacementSlot[ cnt ] = FALSE;
		}
	}

	fInterfacePanelDirty = DIRTYLEVEL2;
}

//CHRISL: Function to handle backpack button rendering
void RenderBackpackButtons(int bpAction)
{
	// Only run function if we're using new inventory system
	if((UsingNewInventorySystem() == false))
		return;
	// Only run this if we're not on the strategic screen
	if(guiCurrentItemDescriptionScreen == MAP_SCREEN)
		return;
	// Only run if the Tactical Inventory Panel is open
	if(gsCurInterfacePanel != SM_PANEL)
		return;
	// Don't run if keyring is open
	if(gfInKeyRingPopup)
		return;
	// If Merc hasn't been set, default to first merc
	if(gpSMCurrentMerc==NULL)
		gpSMCurrentMerc = MercPtrs[ 0 ];
	Assert(gpSMCurrentMerc!=NULL);
	// Deal with buttons depending on which screen is currently active
	switch (bpAction)
	{
		case ACTIVATE_BUTTON:
			// Activate buttons
			if(giSMZipperButton != -1)
				RemoveButton( giSMZipperButton );
			if(giSMZipperImages != -1)
				UnloadButtonImage( giSMZipperImages );
			if(giSMDropPackButton != -1)
				RemoveButton( giSMDropPackButton );
			if(giSMDropPackImages != -1)
				UnloadButtonImage( giSMDropPackImages );
			gpSMCurrentMerc->flags.DropPackFlag = FALSE;
			if(gpSMCurrentMerc->inv[BPACKPOCKPOS].exists() == FALSE)
			{
				for(unsigned int wi = 0; wi < guiNumWorldItems; wi++)
				{
					if(gWorldItems[wi].soldierID == gpSMCurrentMerc->ubID && gWorldItems[wi].object.exists() == true)
					{
						gpSMCurrentMerc->flags.DropPackFlag = TRUE;
						break;
					}
				}
			}
			giSMZipperImages	= UseLoadedButtonImage( iSMPanelImages[ BACKPACK_IMAGES  ] ,-1 ,gbZipperButPos[ gpSMCurrentMerc->flags.ZipperFlag ][0],-1,gbZipperButPos[ gpSMCurrentMerc->flags.ZipperFlag ][1],-1 );
			giSMDropPackImages	= UseLoadedButtonImage( iSMPanelImages[ BACKPACK_IMAGES  ] ,-1 ,gbDropPackButPos[ gpSMCurrentMerc->flags.DropPackFlag ][0],-1,gbDropPackButPos[ gpSMCurrentMerc->flags.DropPackFlag ][1],-1 );
			//giSMDropPackImages	= UseLoadedButtonImage( iSMPanelImages[ BACKPACK_IMAGES  ] ,gbDropPackButPos[ gpSMCurrentMerc->flags.DropPackFlag ][0] ,gbDropPackButPos[ gpSMCurrentMerc->flags.DropPackFlag ][0],-1,gbDropPackButPos[ gpSMCurrentMerc->flags.DropPackFlag ][1],-1 );

			giSMZipperButton	= QuickCreateButton( giSMZipperImages, SM_ZIPPER_X, SM_ZIPPER_Y,
													BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
													DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnZipperCallback );
			giSMDropPackButton	= QuickCreateButton( giSMDropPackImages, SM_DROPPACK_X, SM_DROPPACK_Y,
													BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
													DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnDropPackCallback );
			//SetButtonFastHelpText( giSMZipperButton, TacticalStr[  ] );
			//SetButtonFastHelpText( giSMDropPackButton, TacticalStr[  ] );
			break;
		case DEACTIVATE_BUTTON:
			// Deactivate buttons
			if(giSMZipperButton != -1)
				RemoveButton( giSMZipperButton );
			if(giSMDropPackButton != -1)
				RemoveButton( giSMDropPackButton );
			RenderBackpackButtons(UNLOAD_BUTTON);
			giSMDropPackImages	= -1;
			giSMDropPackButton	= -1;
			giSMZipperImages	= -1;
			giSMZipperButton	= -1;
			break;
		case ENABLE_BUTTON:
			// Enable buttons
			if(giSMDropPackButton == -1 || giSMZipperButton == -1)
			{
				RenderBackpackButtons(DEACTIVATE_BUTTON);
				RenderBackpackButtons(ACTIVATE_BUTTON);
			}
			if(giSMDropPackButton != -1)
				EnableButton( giSMDropPackButton );
			if(giSMZipperButton != -1)
				EnableButton( giSMZipperButton );
			break;
		case DISABLE_BUTTON:
			// Disable buttons
			if(giSMDropPackButton != -1)
				DisableButton( giSMDropPackButton );
			if(giSMZipperButton != -1)
				DisableButton( giSMZipperButton );
			break;
		case UNLOAD_BUTTON:
			if(giSMZipperImages != -1)
				UnloadButtonImage( giSMZipperImages );
			if(giSMDropPackImages != -1)
				UnloadButtonImage( giSMDropPackImages );
			break;
	}
}


void EnableSMPanelButtons( BOOLEAN fEnable , BOOLEAN fFromItemPickup )
{
	if ( fFromItemPickup )
	{
		// If we have the item pointer up...
		// CJC Dec 4 2002: or if item pickup menu is up
		//if ( gpItemPointer != NULL )
		if ( gpItemPointer != NULL || gfInItemPickupMenu )
		{
			DisableTacticalTeamPanelButtons( TRUE );
		}
		else
		{
			DisableTacticalTeamPanelButtons( FALSE );
		}

		fInterfacePanelDirty = DIRTYLEVEL2;
	}


	if ( gsCurInterfacePanel == SM_PANEL )
	{
		if ( fFromItemPickup )
		{
			// If we have the item pointer up...
			if ( gpItemPointer != NULL )
			{
				ReevaluateItemHatches( gpSMCurrentMerc, fEnable );

				// Turn it all false first....
				InternalHandleCompatibleAmmoUI( gpSMCurrentMerc, gpItemPointer, FALSE );
				InternalHandleCompatibleAmmoUI( gpSMCurrentMerc, gpItemPointer, TRUE );

				gfCheckForMouseOverItem = FALSE;

				// Highlight guys ....
				HandleAnyMercInSquadHasCompatibleStuff( (INT8)CurrentSquad( ), gpItemPointer, FALSE );

			}
			else
			{
				//InternalHandleCompatibleAmmoUI( gpSMCurrentMerc, gpItemPointer, FALSE );
				gfCheckForMouseOverItem = FALSE;

				HandleAnyMercInSquadHasCompatibleStuff( (INT8) CurrentSquad( ), NULL, TRUE );
			}

			if ( fEnable )
			{
				ReevaluateItemHatches( gpSMCurrentMerc, fEnable );
			}

			fInterfacePanelDirty = DIRTYLEVEL2;
		}

		if ( fEnable )
		{
			// only enable the following if NOT in shopkeeper's interface
			if ( !(guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE ) )
			{
				EnableButton( iSMPanelButtons[ CLIMB_BUTTON ] );
				EnableButton( iSMPanelButtons[ BURSTMODE_BUTTON ] );
				EnableButton( iSMPanelButtons[ STANCEUP_BUTTON ] );
				EnableButton( iSMPanelButtons[ STANCEDOWN_BUTTON ] );
				EnableButton( iSMPanelButtons[ LOOK_BUTTON ] );
				EnableButton( iSMPanelButtons[ UPDOWN_BUTTON ] );
				EnableButton( iSMPanelButtons[ HANDCURSOR_BUTTON ] );
				if ( giSMStealthButton != -1 )
				{
					EnableButton( giSMStealthButton );
				}
				RenderBackpackButtons(ENABLE_BUTTON);	/* CHRISL: Needed for new inventory backpack buttons */

				if ( gfDisableTacticalPanelButtons )
				{
					DisableButton( iSMPanelButtons[ OPTIONS_BUTTON ] );
					DisableButton( iSMPanelButtons[ SM_DONE_BUTTON ] );
					DisableButton( iSMPanelButtons[ SM_MAP_SCREEN_BUTTON ] );

					if (UsingNewInventorySystem())
					{
						DisableButton(iSMPanelButtons[SM_SKILL_MENU_BUTTON]);
						DisableButton(iSMPanelButtons[SM_SECTOR_MENU_BUTTON]);
					}					
				}
				else
				{
					EnableButton( iSMPanelButtons[ OPTIONS_BUTTON ] );
					EnableButton( iSMPanelButtons[ SM_DONE_BUTTON ] );
					EnableButton( iSMPanelButtons[ SM_MAP_SCREEN_BUTTON ] );

					if (UsingNewInventorySystem())
					{
						EnableButton(iSMPanelButtons[SM_SKILL_MENU_BUTTON]);
						EnableButton(iSMPanelButtons[SM_SECTOR_MENU_BUTTON]);
					}					
				}

				//enable the radar map region
				MSYS_EnableRegion(&gRadarRegion);

				gfSMDisableForItems = FALSE;

			DisableInvRegions( gfSMDisableForItems );
			}

			if ( !fFromItemPickup )
			{
				EnableButton( iSMPanelButtons[ NEXTMERC_BUTTON ] );
				EnableButton( iSMPanelButtons[ PREVMERC_BUTTON ] );
			}
		}
		else
		{
			DisableButton( iSMPanelButtons[ CLIMB_BUTTON ] );
			DisableButton( iSMPanelButtons[ BURSTMODE_BUTTON ] );
			DisableButton( iSMPanelButtons[ STANCEUP_BUTTON ] );
			DisableButton( iSMPanelButtons[ STANCEDOWN_BUTTON ] );
			DisableButton( iSMPanelButtons[ LOOK_BUTTON ] );
			DisableButton( iSMPanelButtons[ UPDOWN_BUTTON ] );
			DisableButton( iSMPanelButtons[ HANDCURSOR_BUTTON ] );
			if ( giSMStealthButton != -1 )
			{
					DisableButton( giSMStealthButton );
			}
			RenderBackpackButtons(DISABLE_BUTTON);	/* CHRISL: Needed for new inventory backpack buttons */

			if ( !fFromItemPickup )
			{
				DisableButton( iSMPanelButtons[ NEXTMERC_BUTTON ] );
				DisableButton( iSMPanelButtons[ PREVMERC_BUTTON ] );
			}

			DisableButton( iSMPanelButtons[ OPTIONS_BUTTON ] );
			DisableButton( iSMPanelButtons[ SM_DONE_BUTTON ] );
			DisableButton( iSMPanelButtons[ SM_MAP_SCREEN_BUTTON ] );

			if (UsingNewInventorySystem())
			{
				DisableButton(iSMPanelButtons[SM_SKILL_MENU_BUTTON]);
				DisableButton(iSMPanelButtons[SM_SECTOR_MENU_BUTTON]);
			}
			
			//disable the radar map
			MSYS_DisableRegion(&gRadarRegion);
		}

		gfAllDisabled = !fEnable;

	}
}

UINT16 GetSMPanelCurrentMerc( )
{
	return( gusSMCurrentMerc );
}


// CHRISL: New functions to define initial settings for New Inventory system
void InitNewInventorySystem( )
{
	if (iResolution >= _640x480 && iResolution < _800x600)
		INTERFACE_WIDTH			= 640;
	else if (iResolution < _1024x768)
		INTERFACE_WIDTH			= 800;
	else
		INTERFACE_WIDTH			= 1024;

	INTERFACE_START_X		= (SCREEN_WIDTH - INTERFACE_WIDTH)/2;
	INTERFACE_HEIGHT		= 120;
	INV_INTERFACE_HEIGHT	= 200;
	INTERFACE_START_Y		= ( SCREEN_HEIGHT - INTERFACE_HEIGHT );
	INV_INTERFACE_START_Y	= ( SCREEN_HEIGHT - INV_INTERFACE_HEIGHT );
}

// CHRISL: New functions to define initial settings for Old Inventory system
void InitOldInventorySystem( )
{
	if (iResolution >= _640x480 && iResolution < _800x600)
		INTERFACE_WIDTH			= 640;
	else if (iResolution < _1024x768)
		INTERFACE_WIDTH			= 800;
	else
		INTERFACE_WIDTH			= 1024;

	INTERFACE_START_X		= (SCREEN_WIDTH - INTERFACE_WIDTH)/2;
	INTERFACE_HEIGHT		= 120;
	INV_INTERFACE_HEIGHT	= 140;
	INTERFACE_START_Y		= ( SCREEN_HEIGHT - INTERFACE_HEIGHT );
	INV_INTERFACE_START_Y	= ( SCREEN_HEIGHT - INV_INTERFACE_HEIGHT );
}

// CHRISL: These functions will setup inventory panel coords based on game options
BOOLEAN InitializeSMPanelCoordsOld()
{
	// int i; // unused (jonathanl)
	/*
	* now i use standard positions and i add to them starting coords of panel but
	* we can add here function that will read coord in panel from file
	* that will be phase2 of making GUI more customizable
	* any questions? joker
	*/
	// Inventory slots
	gSMInvPocketXY[HELMETPOS].sX = INTERFACE_START_X + 344;		gSMInvPocketXY[HELMETPOS].sY = INV_INTERFACE_START_Y + 6;	// HELMETPOS
	gSMInvPocketXY[VESTPOS].sX = INTERFACE_START_X + 344;		gSMInvPocketXY[VESTPOS].sY = INV_INTERFACE_START_Y + 35;	// VESTPOS
	gSMInvPocketXY[LEGPOS].sX = INTERFACE_START_X + 344;		gSMInvPocketXY[LEGPOS].sY = INV_INTERFACE_START_Y + 95;	// LEGPOS,
	gSMInvPocketXY[HEAD1POS].sX = INTERFACE_START_X + 226;		gSMInvPocketXY[HEAD1POS].sY = INV_INTERFACE_START_Y + 6;	// HEAD1POS
	gSMInvPocketXY[HEAD2POS].sX = INTERFACE_START_X + 226;		gSMInvPocketXY[HEAD2POS].sY = INV_INTERFACE_START_Y + 30;	// HEAD2POS
	gSMInvPocketXY[HANDPOS].sX = INTERFACE_START_X + 226;		gSMInvPocketXY[HANDPOS].sY = INV_INTERFACE_START_Y + 84;	// HANDPOS,
	gSMInvPocketXY[SECONDHANDPOS].sX = INTERFACE_START_X + 226;	gSMInvPocketXY[SECONDHANDPOS].sY = INV_INTERFACE_START_Y + 108;	// SECONDHANDPOS
	gSMInvPocketXY[BIGPOCK1POS].sX = INTERFACE_START_X + 468;	gSMInvPocketXY[BIGPOCK1POS].sY = INV_INTERFACE_START_Y + 5;	// BIGPOCK1
	gSMInvPocketXY[BIGPOCK2POS].sX = INTERFACE_START_X + 468;	gSMInvPocketXY[BIGPOCK2POS].sY = INV_INTERFACE_START_Y + 29;	// BIGPOCK2
	gSMInvPocketXY[BIGPOCK3POS].sX = INTERFACE_START_X + 468;	gSMInvPocketXY[BIGPOCK3POS].sY = INV_INTERFACE_START_Y + 53;	// BIGPOCK3
	gSMInvPocketXY[BIGPOCK4POS].sX = INTERFACE_START_X + 468;	gSMInvPocketXY[BIGPOCK4POS].sY = INV_INTERFACE_START_Y + 77;	// BIGPOCK4
	gSMInvPocketXY[SMALLPOCK1POS].sX = INTERFACE_START_X + 396;	gSMInvPocketXY[SMALLPOCK1POS].sY = INV_INTERFACE_START_Y + 5;	// SMALLPOCK1
	gSMInvPocketXY[SMALLPOCK2POS].sX = INTERFACE_START_X + 396;	gSMInvPocketXY[SMALLPOCK2POS].sY = INV_INTERFACE_START_Y + 29;	// SMALLPOCK2
	gSMInvPocketXY[SMALLPOCK3POS].sX = INTERFACE_START_X + 396;	gSMInvPocketXY[SMALLPOCK3POS].sY = INV_INTERFACE_START_Y + 53;	// SMALLPOCK3
	gSMInvPocketXY[SMALLPOCK4POS].sX = INTERFACE_START_X + 396;	gSMInvPocketXY[SMALLPOCK4POS].sY = INV_INTERFACE_START_Y + 77;	// SMALLPOCK4
	gSMInvPocketXY[SMALLPOCK5POS].sX = INTERFACE_START_X + 432;	gSMInvPocketXY[SMALLPOCK5POS].sY = INV_INTERFACE_START_Y + 5;	// SMALLPOCK5
	gSMInvPocketXY[SMALLPOCK6POS].sX = INTERFACE_START_X + 432;	gSMInvPocketXY[SMALLPOCK6POS].sY = INV_INTERFACE_START_Y + 29;	// SMALLPOCK6
	gSMInvPocketXY[SMALLPOCK7POS].sX = INTERFACE_START_X + 432;	gSMInvPocketXY[SMALLPOCK7POS].sY = INV_INTERFACE_START_Y + 53;	// SMALLPOCK7
	gSMInvPocketXY[SMALLPOCK8POS].sX = INTERFACE_START_X + 432;	gSMInvPocketXY[SMALLPOCK8POS].sY = INV_INTERFACE_START_Y + 77;	// SMALLPOCK8
	
	SM_ITEMDESC_START_X		= ( 214 + INTERFACE_START_X );
	SM_ITEMDESC_START_Y		= ( 1 +	INV_INTERFACE_START_Y );
	SM_ITEMDESC_HEIGHT		= 128;
	SM_ITEMDESC_WIDTH		= 358;

	SM_SELMERC_AP_X			= ( 70 + INTERFACE_START_X );
	SM_SELMERC_AP_Y			= ( 53 + INV_INTERFACE_START_Y );
	SM_SELMERC_AP_HEIGHT	= 12;
	SM_SELMERC_AP_WIDTH		= 11;

	SM_SELMERC_HEALTH_X		= ( 69 + INTERFACE_START_X );
	SM_SELMERC_HEALTH_Y		= ( 47 + INV_INTERFACE_START_Y );
	SM_SELMERC_HEALTH_WIDTH = 3;
	SM_SELMERC_HEALTH_HEIGHT = 42;

	SM_SELMERC_BREATH_X		= ( 75 + INTERFACE_START_X );
	SM_SELMERC_BREATH_Y		= ( 47 + INV_INTERFACE_START_Y );
	SM_SELMERC_BREATH_WIDTH = 3;
	SM_SELMERC_BREATH_HEIGHT = 42;

	SM_SELMERC_MORALE_X		= ( 81 + INTERFACE_START_X );
	SM_SELMERC_MORALE_Y		= ( 47 + INV_INTERFACE_START_Y );
	SM_SELMERC_MORALE_WIDTH = 3;
	SM_SELMERC_MORALE_HEIGHT = 42;

	SM_SELMERC_BARS_TIP_X	= (	SM_SELMERC_HEALTH_X - SM_SELMERC_HEALTH_WIDTH );
	SM_SELMERC_BARS_TIP_Y	= ( SM_SELMERC_HEALTH_Y - SM_SELMERC_HEALTH_HEIGHT );
	SM_SELMERC_BARS_TIP_WIDTH = ((SM_SELMERC_BREATH_WIDTH + 4) *3);
	SM_SELMERC_BARS_TIP_HEIGHT= ( SM_SELMERC_HEALTH_HEIGHT );

	SM_SELMERCNAME_X		= ( 7 + INTERFACE_START_X );
	SM_SELMERCNAME_Y		= ( 55 + INV_INTERFACE_START_Y );
	SM_SELMERCNAME_WIDTH	= 53;
	SM_SELMERCNAME_HEIGHT	= 9;

	SM_SELMERC_FACE_X		= ( 13 + INTERFACE_START_X );
	SM_SELMERC_FACE_Y		= ( 6 + INV_INTERFACE_START_Y );
	SM_SELMERC_FACE_HEIGHT	= 42;
	SM_SELMERC_FACE_WIDTH	= 48;

	SM_SELMERC_PLATE_X		= ( 4 + INTERFACE_START_X );
	SM_SELMERC_PLATE_Y		= ( 2 + INV_INTERFACE_START_Y );
	SM_SELMERC_PLATE_HEIGHT = 65;
	SM_SELMERC_PLATE_WIDTH	= 83;

	SM_BODYINV_X			= ( 244 + INTERFACE_START_X );
	SM_BODYINV_Y			= ( 6 + INV_INTERFACE_START_Y );

	// Cammo
	gSMCamoXY.sX = SM_BODYINV_X; gSMCamoXY.sY = SM_BODYINV_Y;

	SM_TALKB_X				= ( 155 + INTERFACE_START_X );
	SM_TALKB_Y				= ( 108 + INV_INTERFACE_START_Y );
	SM_MUTEB_X				= ( 91 + INTERFACE_START_X );
	SM_MUTEB_Y				= ( 108 + INV_INTERFACE_START_Y );
	SM_STANCEUPB_X			= ( 187 + INTERFACE_START_X );
	SM_STANCEUPB_Y			= ( 40 + INV_INTERFACE_START_Y );
	SM_UPDOWNB_X			= ( 91 + INTERFACE_START_X );
	SM_UPDOWNB_Y			= ( 73 + INV_INTERFACE_START_Y );
	SM_CLIMBB_X				= ( 187 + INTERFACE_START_X );
	SM_CLIMBB_Y				= ( 8 + INV_INTERFACE_START_Y );
	SM_STANCEDOWNB_X		= ( 187 + INTERFACE_START_X );
	SM_STANCEDOWNB_Y		= ( 108 + INV_INTERFACE_START_Y );
	SM_HANDCURSORB_X		= ( 123 + INTERFACE_START_X );
	SM_HANDCURSORB_Y		= ( 73 + INV_INTERFACE_START_Y );
	SM_PREVMERCB_X			= ( 9 + INTERFACE_START_X );
	SM_PREVMERCB_Y			= ( 70 + INV_INTERFACE_START_Y );
	SM_NEXTMERCB_X			= ( 51 + INTERFACE_START_X );
	SM_NEXTMERCB_Y			= ( 70 + INV_INTERFACE_START_Y );
	SM_OPTIONSB_X			= ( 9 + INTERFACE_START_X );
	SM_OPTIONSB_Y			= ( 105 + INV_INTERFACE_START_Y );
	SM_BURSTMODEB_X			= ( 155 + INTERFACE_START_X );
	SM_BURSTMODEB_Y			= ( 73 + INV_INTERFACE_START_Y );
	SM_LOOKB_X				= ( 123 + INTERFACE_START_X );
	SM_LOOKB_Y				= ( 108 + INV_INTERFACE_START_Y );
	SM_STEALTHMODE_X		= ( 187 + INTERFACE_START_X );
	SM_STEALTHMODE_Y		= ( 73 + INV_INTERFACE_START_Y );

	SM_DONE_X				=	xResOffset + (xResSize - 97);
	SM_MAPSCREEN_X			=	xResOffset + (xResSize - 51);

	//! sevenfm: nowhere to display in old inventory
	SM_SKILLMENU_X = 0;
	SM_SKILLMENU_Y = 0;
	SM_SECTORMENU_X = 0;
	SM_SECTORMENU_Y = 0;

//dnl	INTERFACE_CLOCK_X		=	xResOffset + (xResSize - 86);
//dnl	INTERFACE_CLOCK_Y		= ( 119	+ INV_INTERFACE_START_Y );
	INTERFACE_CLOCK_X		= xResOffset + (xResSize - 86);
	INTERFACE_CLOCK_Y		= SCREEN_HEIGHT - 24;
	LOCATION_NAME_X			=	xResOffset + (xResSize - 92);
	LOCATION_NAME_Y			= ( 65	+ INTERFACE_START_Y	);
	
	SM_DONE_Y				= ( 4 + INV_INTERFACE_START_Y );
	SM_MAPSCREEN_Y			= ( 4 + INV_INTERFACE_START_Y );

	SM_POSITIONB_X			= ( 106 + INTERFACE_START_X );
	SM_POSITIONB_Y			= ( 34 + INV_INTERFACE_START_Y );
	SM_POSITIONB_WIDTH		= 19;
	SM_POSITIONB_HEIGHT		= 24;

	SM_PERCENT_WIDTH		= 20;
	SM_PERCENT_HEIGHT		= 10;
	SM_ARMOR_X				= ( 347 + INTERFACE_START_X );
	SM_ARMOR_Y				= ( 79 + INV_INTERFACE_START_Y );
	SM_ARMOR_LABEL_X		= ( 363 + INTERFACE_START_X );
	SM_ARMOR_LABEL_Y		= ( 69 + INV_INTERFACE_START_Y );
	SM_ARMOR_PERCENT_X		= ( 368 + INTERFACE_START_X );
	SM_ARMOR_PERCENT_Y		= ( 79 + INV_INTERFACE_START_Y );

	SM_WEIGHT_LABEL_X		= ( 430 + INTERFACE_START_X );
	SM_WEIGHT_LABEL_Y		= ( 107 + INV_INTERFACE_START_Y );
	SM_WEIGHT_PERCENT_X		= ( 449 + INTERFACE_START_X );
	SM_WEIGHT_PERCENT_Y		= ( 107 + INV_INTERFACE_START_Y );
	SM_WEIGHT_X				= ( 428 + INTERFACE_START_X );
	SM_WEIGHT_Y				= ( 106 + INV_INTERFACE_START_Y );

	SM_CAMMO_LABEL_X		= ( 430 + INTERFACE_START_X );
	SM_CAMMO_LABEL_Y		= ( 122 + INV_INTERFACE_START_Y );
	SM_CAMMO_PERCENT_X		= ( 449 + INTERFACE_START_X );
	SM_CAMMO_PERCENT_Y		= ( 121 + INV_INTERFACE_START_Y );
	SM_CAMMO_X				= ( 428 + INTERFACE_START_X );
	SM_CAMMO_Y				= ( 121 + INV_INTERFACE_START_Y );

	SM_STATS_WIDTH			= 16;
	SM_STATS_HEIGHT			= 8 ;
	SM_AGI_X				= ( 115 + INTERFACE_START_X );
	SM_AGI_Y				= ( 7 + INV_INTERFACE_START_Y );
	SM_DEX_X				= ( 115 + INTERFACE_START_X );
	SM_DEX_Y				= ( 17 + INV_INTERFACE_START_Y );
	SM_STR_X				= ( 115 + INTERFACE_START_X );
	SM_STR_Y				= ( 27 + INV_INTERFACE_START_Y );
	SM_CHAR_X				= ( 115 + INTERFACE_START_X );
	SM_CHAR_Y				= ( 37 + INV_INTERFACE_START_Y );
	SM_WIS_X				= ( 115 + INTERFACE_START_X );
	SM_WIS_Y				= ( 47 + INV_INTERFACE_START_Y );
	SM_EXPLVL_X				= ( 163 + INTERFACE_START_X );
	SM_EXPLVL_Y				= ( 7 + INV_INTERFACE_START_Y );
	SM_MRKM_X				= ( 163 + INTERFACE_START_X );
	SM_MRKM_Y				= ( 17 + INV_INTERFACE_START_Y );
	SM_MECH_X				= ( 163 + INTERFACE_START_X );
	SM_MECH_Y				= ( 27 + INV_INTERFACE_START_Y );
	SM_EXPL_X				= ( 163 + INTERFACE_START_X );
	SM_EXPL_Y				= ( 37 + INV_INTERFACE_START_Y );
	SM_MED_X				= ( 163 + INTERFACE_START_X );
	SM_MED_Y				= ( 47 + INV_INTERFACE_START_Y );

	MONEY_X					= ( 460 + INTERFACE_START_X );
	MONEY_Y					= ( 105 + INV_INTERFACE_START_Y );
	MONEY_WIDTH				= 30;
	MONEY_HEIGHT			= 22;

	// ow ye font color they can also be customizable :P
	STATS_TITLE_FONT_COLOR	= 6;
	STATS_TEXT_FONT_COLOR	= 5;

	// Keyring	496/106 on Inventory_Bottom_Panel.sti
	KEYRING_X			= xResOffset + 496;
	KEYRING_Y			= (106 + INV_INTERFACE_START_Y);

	// so we got everything "dynamic" now we just return TRUE
	return ( TRUE );
}
BOOLEAN InitializeSMPanelCoordsNew()
{
	// int i; // unused (jonathanl)
	/* 
	 * now i use standard positions and i add to them starting coords of panel but 
	 * we can add here function that will read coord in panel from file
	 * that will be phase2 of making GUI more customizable
	 * any questions? joker
	 */
	/* CHRISL: Inventory slot information was completly altered to allow for new inventory pockets and
	to allow the new inventory interface to function properly. Also altered many of the interface coords
	so that the new interface would line up correctly.*/
	// Inventory slots
	if (iResolution >= _640x480 && iResolution < _800x600)
	{
		gSMInvPocketXY[0].sX = INTERFACE_START_X + 239;		gSMInvPocketXY[0].sY = INV_INTERFACE_START_Y + 68;		// HELMETPOS
		gSMInvPocketXY[1].sX = INTERFACE_START_X + 239;		gSMInvPocketXY[1].sY = INV_INTERFACE_START_Y + 96;		// VESTPOS
		gSMInvPocketXY[2].sX = INTERFACE_START_X + 239;		gSMInvPocketXY[2].sY = INV_INTERFACE_START_Y + 157;		// LEGPOS
		gSMInvPocketXY[3].sX = INTERFACE_START_X + 124;		gSMInvPocketXY[3].sY = INV_INTERFACE_START_Y + 68;		// HEAD1POS
		gSMInvPocketXY[4].sX = INTERFACE_START_X + 124;		gSMInvPocketXY[4].sY = INV_INTERFACE_START_Y + 92;		// HEAD2POS
		gSMInvPocketXY[5].sX = INTERFACE_START_X + 124;		gSMInvPocketXY[5].sY = INV_INTERFACE_START_Y + 146;		// HANDPOS
		gSMInvPocketXY[6].sX = INTERFACE_START_X + 124;		gSMInvPocketXY[6].sY = INV_INTERFACE_START_Y + 170;		// SECONDHANDPOS
		gSMInvPocketXY[7].sX = INTERFACE_START_X + 274;		gSMInvPocketXY[7].sY = INV_INTERFACE_START_Y + 13;		// VESTPOCK
		gSMInvPocketXY[8].sX = INTERFACE_START_X + 292;		gSMInvPocketXY[8].sY = INV_INTERFACE_START_Y + 79;		// LTHIGHPOCK
		gSMInvPocketXY[9].sX = INTERFACE_START_X + 340;		gSMInvPocketXY[9].sY = INV_INTERFACE_START_Y + 79;		// RTHIGHPOCK
		gSMInvPocketXY[10].sX = INTERFACE_START_X + 443;	gSMInvPocketXY[10].sY = INV_INTERFACE_START_Y + 79;		// CPACKPOCK
		gSMInvPocketXY[11].sX = INTERFACE_START_X + 486;	gSMInvPocketXY[11].sY = INV_INTERFACE_START_Y + 13;		// BPACKPOCK
		gSMInvPocketXY[12].sX = INTERFACE_START_X + 383;	gSMInvPocketXY[12].sY = INV_INTERFACE_START_Y + 79;		// GUNSLINGPOCKPOS
		gSMInvPocketXY[13].sX = INTERFACE_START_X + 499;	gSMInvPocketXY[13].sY = INV_INTERFACE_START_Y + 79;		// KNIFEPOCKPOS
		gSMInvPocketXY[14].sX = INTERFACE_START_X + 433;	gSMInvPocketXY[14].sY = INV_INTERFACE_START_Y + 116;	// BIGPOCK1
		gSMInvPocketXY[15].sX = INTERFACE_START_X + 433;	gSMInvPocketXY[15].sY = INV_INTERFACE_START_Y + 140;	// BIGPOCK2
		gSMInvPocketXY[16].sX = INTERFACE_START_X + 433;	gSMInvPocketXY[16].sY = INV_INTERFACE_START_Y + 164;	// BIGPOCK3
		gSMInvPocketXY[17].sX = INTERFACE_START_X + 582;	gSMInvPocketXY[17].sY = INV_INTERFACE_START_Y + 10;		// BIGPOCK4
		gSMInvPocketXY[18].sX = INTERFACE_START_X + 582;	gSMInvPocketXY[18].sY = INV_INTERFACE_START_Y + 34;		// BIGPOCK5
		gSMInvPocketXY[19].sX = INTERFACE_START_X + 582;	gSMInvPocketXY[19].sY = INV_INTERFACE_START_Y + 58;		// BIGPOCK6
		gSMInvPocketXY[20].sX = INTERFACE_START_X + 582;	gSMInvPocketXY[20].sY = INV_INTERFACE_START_Y + 82;		// BIGPOCK7
		gSMInvPocketXY[21].sX = INTERFACE_START_X + 439;	gSMInvPocketXY[21].sY = INV_INTERFACE_START_Y + 10;		// MEDPOCK1
		gSMInvPocketXY[22].sX = INTERFACE_START_X + 439;	gSMInvPocketXY[22].sY = INV_INTERFACE_START_Y + 34;		// MEDPOCK2
		gSMInvPocketXY[23].sX = INTERFACE_START_X + 295;	gSMInvPocketXY[23].sY = INV_INTERFACE_START_Y + 164;	// MEDPOCK3
		gSMInvPocketXY[24].sX = INTERFACE_START_X + 343;	gSMInvPocketXY[24].sY = INV_INTERFACE_START_Y + 164;	// MEDPOCK4
		gSMInvPocketXY[25].sX = INTERFACE_START_X + 324;	gSMInvPocketXY[25].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK1
		gSMInvPocketXY[26].sX = INTERFACE_START_X + 347;	gSMInvPocketXY[26].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK2
		gSMInvPocketXY[27].sX = INTERFACE_START_X + 370;	gSMInvPocketXY[27].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK3
		gSMInvPocketXY[28].sX = INTERFACE_START_X + 393;	gSMInvPocketXY[28].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK4
		gSMInvPocketXY[29].sX = INTERFACE_START_X + 416;	gSMInvPocketXY[29].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK5
		gSMInvPocketXY[30].sX = INTERFACE_START_X + 324;	gSMInvPocketXY[30].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK6
		gSMInvPocketXY[31].sX = INTERFACE_START_X + 347;	gSMInvPocketXY[31].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK7
		gSMInvPocketXY[32].sX = INTERFACE_START_X + 370;	gSMInvPocketXY[32].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK8	
		gSMInvPocketXY[33].sX = INTERFACE_START_X + 393;	gSMInvPocketXY[33].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK9
		gSMInvPocketXY[34].sX = INTERFACE_START_X + 416;	gSMInvPocketXY[34].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK10
		gSMInvPocketXY[35].sX = INTERFACE_START_X + 291;	gSMInvPocketXY[35].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK11
		gSMInvPocketXY[36].sX = INTERFACE_START_X + 314;	gSMInvPocketXY[36].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK12
		gSMInvPocketXY[37].sX = INTERFACE_START_X + 291;	gSMInvPocketXY[37].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK13
		gSMInvPocketXY[38].sX = INTERFACE_START_X + 314;	gSMInvPocketXY[38].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK14
		gSMInvPocketXY[39].sX = INTERFACE_START_X + 339;	gSMInvPocketXY[39].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK15
		gSMInvPocketXY[40].sX = INTERFACE_START_X + 362;	gSMInvPocketXY[40].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK16
		gSMInvPocketXY[41].sX = INTERFACE_START_X + 339;	gSMInvPocketXY[41].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK17
		gSMInvPocketXY[42].sX = INTERFACE_START_X + 362;	gSMInvPocketXY[42].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK18
		gSMInvPocketXY[43].sX = INTERFACE_START_X + 387;	gSMInvPocketXY[43].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK19
		gSMInvPocketXY[44].sX = INTERFACE_START_X + 410;	gSMInvPocketXY[44].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK20
		gSMInvPocketXY[45].sX = INTERFACE_START_X + 387;	gSMInvPocketXY[45].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK21
		gSMInvPocketXY[46].sX = INTERFACE_START_X + 410;	gSMInvPocketXY[46].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK22
		gSMInvPocketXY[47].sX = INTERFACE_START_X + 536;	gSMInvPocketXY[47].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK23
		gSMInvPocketXY[48].sX = INTERFACE_START_X + 536;	gSMInvPocketXY[48].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK24
		gSMInvPocketXY[49].sX = INTERFACE_START_X + 536;	gSMInvPocketXY[49].sY = INV_INTERFACE_START_Y + 58;		// SMALLPOCK25
		gSMInvPocketXY[50].sX = INTERFACE_START_X + 536;	gSMInvPocketXY[50].sY = INV_INTERFACE_START_Y + 82;		// SMALLPOCK26
		gSMInvPocketXY[51].sX = INTERFACE_START_X + 559;	gSMInvPocketXY[51].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK27
		gSMInvPocketXY[52].sX = INTERFACE_START_X + 559;	gSMInvPocketXY[52].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK28
		gSMInvPocketXY[53].sX = INTERFACE_START_X + 559;	gSMInvPocketXY[53].sY = INV_INTERFACE_START_Y + 58;		// SMALLPOCK29
		gSMInvPocketXY[54].sX = INTERFACE_START_X + 559;	gSMInvPocketXY[54].sY = INV_INTERFACE_START_Y + 82;		// SMALLPOCK30
	}
	else if (iResolution < _1024x768)
	{
		gSMInvPocketXY[0].sX = INTERFACE_START_X + 258;		gSMInvPocketXY[0].sY = INV_INTERFACE_START_Y + 68;		// HELMETPOS
		gSMInvPocketXY[1].sX = INTERFACE_START_X + 258;		gSMInvPocketXY[1].sY = INV_INTERFACE_START_Y + 97;		// VESTPOS
		gSMInvPocketXY[2].sX = INTERFACE_START_X + 258;		gSMInvPocketXY[2].sY = INV_INTERFACE_START_Y + 157;		// LEGPOS
		gSMInvPocketXY[3].sX = INTERFACE_START_X + 124;		gSMInvPocketXY[3].sY = INV_INTERFACE_START_Y + 68;		// HEAD1POS
		gSMInvPocketXY[4].sX = INTERFACE_START_X + 124;		gSMInvPocketXY[4].sY = INV_INTERFACE_START_Y + 92;		// HEAD2POS
		gSMInvPocketXY[5].sX = INTERFACE_START_X + 124;		gSMInvPocketXY[5].sY = INV_INTERFACE_START_Y + 146;		// HANDPOS
		gSMInvPocketXY[6].sX = INTERFACE_START_X + 124;		gSMInvPocketXY[6].sY = INV_INTERFACE_START_Y + 170;		// SECONDHANDPOS
		gSMInvPocketXY[7].sX = INTERFACE_START_X + 284;		gSMInvPocketXY[7].sY = INV_INTERFACE_START_Y + 13;		// VESTPOCK
		gSMInvPocketXY[8].sX = INTERFACE_START_X + 323;		gSMInvPocketXY[8].sY = INV_INTERFACE_START_Y + 79;		// LTHIGHPOCK
		gSMInvPocketXY[9].sX = INTERFACE_START_X + 439;		gSMInvPocketXY[9].sY = INV_INTERFACE_START_Y + 79;		// RTHIGHPOCK
		gSMInvPocketXY[10].sX = INTERFACE_START_X + 586;	gSMInvPocketXY[10].sY = INV_INTERFACE_START_Y + 79;		// CPACKPOCK
		gSMInvPocketXY[11].sX = INTERFACE_START_X + 594;	gSMInvPocketXY[11].sY = INV_INTERFACE_START_Y + 13;		// BPACKPOCK
		gSMInvPocketXY[12].sX = INTERFACE_START_X + 505;	gSMInvPocketXY[12].sY = INV_INTERFACE_START_Y + 79;		// GUNSLINGPOCKPOS
		gSMInvPocketXY[13].sX = INTERFACE_START_X + 388;	gSMInvPocketXY[13].sY = INV_INTERFACE_START_Y + 79;		// KNIFEPOCKPOS
		gSMInvPocketXY[14].sX = INTERFACE_START_X + 565;	gSMInvPocketXY[14].sY = INV_INTERFACE_START_Y + 116;	// BIGPOCK1
		gSMInvPocketXY[15].sX = INTERFACE_START_X + 565;	gSMInvPocketXY[15].sY = INV_INTERFACE_START_Y + 140;	// BIGPOCK2
		gSMInvPocketXY[16].sX = INTERFACE_START_X + 565;	gSMInvPocketXY[16].sY = INV_INTERFACE_START_Y + 164;	// BIGPOCK3
		gSMInvPocketXY[17].sX = INTERFACE_START_X + 729;	gSMInvPocketXY[17].sY = INV_INTERFACE_START_Y + 10;		// BIGPOCK4
		gSMInvPocketXY[18].sX = INTERFACE_START_X + 729;	gSMInvPocketXY[18].sY = INV_INTERFACE_START_Y + 34;		// BIGPOCK5
		gSMInvPocketXY[19].sX = INTERFACE_START_X + 729;	gSMInvPocketXY[19].sY = INV_INTERFACE_START_Y + 58;		// BIGPOCK6
		gSMInvPocketXY[20].sX = INTERFACE_START_X + 729;	gSMInvPocketXY[20].sY = INV_INTERFACE_START_Y + 82;		// BIGPOCK7
		gSMInvPocketXY[21].sX = INTERFACE_START_X + 527;	gSMInvPocketXY[21].sY = INV_INTERFACE_START_Y + 10;		// MEDPOCK1
		gSMInvPocketXY[22].sX = INTERFACE_START_X + 527;	gSMInvPocketXY[22].sY = INV_INTERFACE_START_Y + 34;		// MEDPOCK2
		gSMInvPocketXY[23].sX = INTERFACE_START_X + 333;	gSMInvPocketXY[23].sY = INV_INTERFACE_START_Y + 164;	// MEDPOCK3
		gSMInvPocketXY[24].sX = INTERFACE_START_X + 419;	gSMInvPocketXY[24].sY = INV_INTERFACE_START_Y + 164;	// MEDPOCK4
		gSMInvPocketXY[25].sX = INTERFACE_START_X + 347;	gSMInvPocketXY[25].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK1
		gSMInvPocketXY[26].sX = INTERFACE_START_X + 383;	gSMInvPocketXY[26].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK2
		gSMInvPocketXY[27].sX = INTERFACE_START_X + 419;	gSMInvPocketXY[27].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK3
		gSMInvPocketXY[28].sX = INTERFACE_START_X + 455;	gSMInvPocketXY[28].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK4
		gSMInvPocketXY[29].sX = INTERFACE_START_X + 491;	gSMInvPocketXY[29].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK5
		gSMInvPocketXY[30].sX = INTERFACE_START_X + 347;	gSMInvPocketXY[30].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK6
		gSMInvPocketXY[31].sX = INTERFACE_START_X + 383;	gSMInvPocketXY[31].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK7
		gSMInvPocketXY[32].sX = INTERFACE_START_X + 419;	gSMInvPocketXY[32].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK8	
		gSMInvPocketXY[33].sX = INTERFACE_START_X + 455;	gSMInvPocketXY[33].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK9
		gSMInvPocketXY[34].sX = INTERFACE_START_X + 491;	gSMInvPocketXY[34].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK10
		gSMInvPocketXY[35].sX = INTERFACE_START_X + 321;	gSMInvPocketXY[35].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK11
		gSMInvPocketXY[36].sX = INTERFACE_START_X + 357;	gSMInvPocketXY[36].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK12
		gSMInvPocketXY[37].sX = INTERFACE_START_X + 321;	gSMInvPocketXY[37].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK13
		gSMInvPocketXY[38].sX = INTERFACE_START_X + 357;	gSMInvPocketXY[38].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK14
		gSMInvPocketXY[39].sX = INTERFACE_START_X + 407;	gSMInvPocketXY[39].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK15
		gSMInvPocketXY[40].sX = INTERFACE_START_X + 443;	gSMInvPocketXY[40].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK16
		gSMInvPocketXY[41].sX = INTERFACE_START_X + 407;	gSMInvPocketXY[41].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK17
		gSMInvPocketXY[42].sX = INTERFACE_START_X + 443;	gSMInvPocketXY[42].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK18
		gSMInvPocketXY[43].sX = INTERFACE_START_X + 493;	gSMInvPocketXY[43].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK19
		gSMInvPocketXY[44].sX = INTERFACE_START_X + 529;	gSMInvPocketXY[44].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK20
		gSMInvPocketXY[45].sX = INTERFACE_START_X + 493;	gSMInvPocketXY[45].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK21
		gSMInvPocketXY[46].sX = INTERFACE_START_X + 529;	gSMInvPocketXY[46].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK22
		gSMInvPocketXY[47].sX = INTERFACE_START_X + 657;	gSMInvPocketXY[47].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK23
		gSMInvPocketXY[48].sX = INTERFACE_START_X + 657;	gSMInvPocketXY[48].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK24
		gSMInvPocketXY[49].sX = INTERFACE_START_X + 657;	gSMInvPocketXY[49].sY = INV_INTERFACE_START_Y + 58;		// SMALLPOCK25
		gSMInvPocketXY[50].sX = INTERFACE_START_X + 657;	gSMInvPocketXY[50].sY = INV_INTERFACE_START_Y + 82;		// SMALLPOCK26
		gSMInvPocketXY[51].sX = INTERFACE_START_X + 693;	gSMInvPocketXY[51].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK27
		gSMInvPocketXY[52].sX = INTERFACE_START_X + 693;	gSMInvPocketXY[52].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK28
		gSMInvPocketXY[53].sX = INTERFACE_START_X + 693;	gSMInvPocketXY[53].sY = INV_INTERFACE_START_Y + 58;		// SMALLPOCK29
		gSMInvPocketXY[54].sX = INTERFACE_START_X + 693;	gSMInvPocketXY[54].sY = INV_INTERFACE_START_Y + 82;		// SMALLPOCK30
	}
	else
	{
		//1024x768 resolution
		gSMInvPocketXY[0].sX = INTERFACE_START_X + 258;		gSMInvPocketXY[0].sY = INV_INTERFACE_START_Y + 68;		// HELMETPOS
		gSMInvPocketXY[1].sX = INTERFACE_START_X + 258;		gSMInvPocketXY[1].sY = INV_INTERFACE_START_Y + 97;		// VESTPOS
		gSMInvPocketXY[2].sX = INTERFACE_START_X + 258;		gSMInvPocketXY[2].sY = INV_INTERFACE_START_Y + 157;		// LEGPOS
		gSMInvPocketXY[3].sX = INTERFACE_START_X + 124;		gSMInvPocketXY[3].sY = INV_INTERFACE_START_Y + 68;		// HEAD1POS
		gSMInvPocketXY[4].sX = INTERFACE_START_X + 124;		gSMInvPocketXY[4].sY = INV_INTERFACE_START_Y + 92;		// HEAD2POS
		gSMInvPocketXY[5].sX = INTERFACE_START_X + 124;		gSMInvPocketXY[5].sY = INV_INTERFACE_START_Y + 146;		// HANDPOS
		gSMInvPocketXY[6].sX = INTERFACE_START_X + 124;		gSMInvPocketXY[6].sY = INV_INTERFACE_START_Y + 170;		// SECONDHANDPOS
		gSMInvPocketXY[7].sX = INTERFACE_START_X + 284;		gSMInvPocketXY[7].sY = INV_INTERFACE_START_Y + 13;		// VESTPOCK
		gSMInvPocketXY[8].sX = INTERFACE_START_X + 358;		gSMInvPocketXY[8].sY = INV_INTERFACE_START_Y + 79;		// LTHIGHPOCK
		gSMInvPocketXY[9].sX = INTERFACE_START_X + 516;		gSMInvPocketXY[9].sY = INV_INTERFACE_START_Y + 79;		// RTHIGHPOCK
		gSMInvPocketXY[10].sX = INTERFACE_START_X + 594;	gSMInvPocketXY[10].sY = INV_INTERFACE_START_Y + 128;	// CPACKPOCK
		gSMInvPocketXY[11].sX = INTERFACE_START_X + 594;	gSMInvPocketXY[11].sY = INV_INTERFACE_START_Y + 13;		// BPACKPOCK
		gSMInvPocketXY[12].sX = INTERFACE_START_X + 429;	gSMInvPocketXY[12].sY = INV_INTERFACE_START_Y + 79;		// GUNSLINGPOCKPOS
		gSMInvPocketXY[13].sX = INTERFACE_START_X + 594;	gSMInvPocketXY[13].sY = INV_INTERFACE_START_Y + 79;		// KNIFEPOCKPOS
		gSMInvPocketXY[14].sX = INTERFACE_START_X + 729;	gSMInvPocketXY[14].sY = INV_INTERFACE_START_Y + 116;	// BIGPOCK1
		gSMInvPocketXY[15].sX = INTERFACE_START_X + 729;	gSMInvPocketXY[15].sY = INV_INTERFACE_START_Y + 140;	// BIGPOCK2
		gSMInvPocketXY[16].sX = INTERFACE_START_X + 729;	gSMInvPocketXY[16].sY = INV_INTERFACE_START_Y + 164;	// BIGPOCK3
		gSMInvPocketXY[17].sX = INTERFACE_START_X + 729;	gSMInvPocketXY[17].sY = INV_INTERFACE_START_Y + 10;		// BIGPOCK4
		gSMInvPocketXY[18].sX = INTERFACE_START_X + 729;	gSMInvPocketXY[18].sY = INV_INTERFACE_START_Y + 34;		// BIGPOCK5
		gSMInvPocketXY[19].sX = INTERFACE_START_X + 729;	gSMInvPocketXY[19].sY = INV_INTERFACE_START_Y + 58;		// BIGPOCK6
		gSMInvPocketXY[20].sX = INTERFACE_START_X + 729;	gSMInvPocketXY[20].sY = INV_INTERFACE_START_Y + 82;		// BIGPOCK7
		gSMInvPocketXY[21].sX = INTERFACE_START_X + 527;	gSMInvPocketXY[21].sY = INV_INTERFACE_START_Y + 10;		// MEDPOCK1
		gSMInvPocketXY[22].sX = INTERFACE_START_X + 527;	gSMInvPocketXY[22].sY = INV_INTERFACE_START_Y + 34;		// MEDPOCK2
		gSMInvPocketXY[23].sX = INTERFACE_START_X + 359;	gSMInvPocketXY[23].sY = INV_INTERFACE_START_Y + 164;	// MEDPOCK3
		gSMInvPocketXY[24].sX = INTERFACE_START_X + 517;	gSMInvPocketXY[24].sY = INV_INTERFACE_START_Y + 164;	// MEDPOCK4
		gSMInvPocketXY[25].sX = INTERFACE_START_X + 347;	gSMInvPocketXY[25].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK1
		gSMInvPocketXY[26].sX = INTERFACE_START_X + 383;	gSMInvPocketXY[26].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK2
		gSMInvPocketXY[27].sX = INTERFACE_START_X + 419;	gSMInvPocketXY[27].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK3
		gSMInvPocketXY[28].sX = INTERFACE_START_X + 455;	gSMInvPocketXY[28].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK4
		gSMInvPocketXY[29].sX = INTERFACE_START_X + 491;	gSMInvPocketXY[29].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK5
		gSMInvPocketXY[30].sX = INTERFACE_START_X + 347;	gSMInvPocketXY[30].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK6
		gSMInvPocketXY[31].sX = INTERFACE_START_X + 383;	gSMInvPocketXY[31].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK7
		gSMInvPocketXY[32].sX = INTERFACE_START_X + 419;	gSMInvPocketXY[32].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK8	
		gSMInvPocketXY[33].sX = INTERFACE_START_X + 455;	gSMInvPocketXY[33].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK9
		gSMInvPocketXY[34].sX = INTERFACE_START_X + 491;	gSMInvPocketXY[34].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK10
		gSMInvPocketXY[35].sX = INTERFACE_START_X + 347;	gSMInvPocketXY[35].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK11
		gSMInvPocketXY[36].sX = INTERFACE_START_X + 383;	gSMInvPocketXY[36].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK12
		gSMInvPocketXY[37].sX = INTERFACE_START_X + 347;	gSMInvPocketXY[37].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK13
		gSMInvPocketXY[38].sX = INTERFACE_START_X + 383;	gSMInvPocketXY[38].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK14
		gSMInvPocketXY[39].sX = INTERFACE_START_X + 505;	gSMInvPocketXY[39].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK15
		gSMInvPocketXY[40].sX = INTERFACE_START_X + 541;	gSMInvPocketXY[40].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK16
		gSMInvPocketXY[41].sX = INTERFACE_START_X + 505;	gSMInvPocketXY[41].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK17
		gSMInvPocketXY[42].sX = INTERFACE_START_X + 541;	gSMInvPocketXY[42].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK18
		gSMInvPocketXY[43].sX = INTERFACE_START_X + 657;	gSMInvPocketXY[43].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK19
		gSMInvPocketXY[44].sX = INTERFACE_START_X + 693;	gSMInvPocketXY[44].sY = INV_INTERFACE_START_Y + 116;	// SMALLPOCK20
		gSMInvPocketXY[45].sX = INTERFACE_START_X + 657;	gSMInvPocketXY[45].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK21
		gSMInvPocketXY[46].sX = INTERFACE_START_X + 693;	gSMInvPocketXY[46].sY = INV_INTERFACE_START_Y + 140;	// SMALLPOCK22
		gSMInvPocketXY[47].sX = INTERFACE_START_X + 657;	gSMInvPocketXY[47].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK23
		gSMInvPocketXY[48].sX = INTERFACE_START_X + 657;	gSMInvPocketXY[48].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK24
		gSMInvPocketXY[49].sX = INTERFACE_START_X + 657;	gSMInvPocketXY[49].sY = INV_INTERFACE_START_Y + 58;		// SMALLPOCK25
		gSMInvPocketXY[50].sX = INTERFACE_START_X + 657;	gSMInvPocketXY[50].sY = INV_INTERFACE_START_Y + 82;		// SMALLPOCK26
		gSMInvPocketXY[51].sX = INTERFACE_START_X + 693;	gSMInvPocketXY[51].sY = INV_INTERFACE_START_Y + 10;		// SMALLPOCK27
		gSMInvPocketXY[52].sX = INTERFACE_START_X + 693;	gSMInvPocketXY[52].sY = INV_INTERFACE_START_Y + 34;		// SMALLPOCK28
		gSMInvPocketXY[53].sX = INTERFACE_START_X + 693;	gSMInvPocketXY[53].sY = INV_INTERFACE_START_Y + 58;		// SMALLPOCK29
		gSMInvPocketXY[54].sX = INTERFACE_START_X + 693;	gSMInvPocketXY[54].sY = INV_INTERFACE_START_Y + 82;		// SMALLPOCK30
	}

	SM_ITEMDESC_START_X		= ( 114 + INTERFACE_START_X );	//115
	SM_ITEMDESC_START_Y		= ( 1 +   INV_INTERFACE_START_Y );
	SM_ITEMDESC_HEIGHT		= 128;
	SM_ITEMDESC_WIDTH		= 358;

	SM_SELMERC_AP_X			= ( 71 + INTERFACE_START_X );
	SM_SELMERC_AP_Y			= ( 53 + INV_INTERFACE_START_Y );
	SM_SELMERC_AP_HEIGHT	= 12;
	SM_SELMERC_AP_WIDTH		= 11;
 
	SM_SELMERC_HEALTH_X		= ( 70 + INTERFACE_START_X );
	SM_SELMERC_HEALTH_Y		= ( 48 + INV_INTERFACE_START_Y );
	SM_SELMERC_HEALTH_WIDTH = 3;
	SM_SELMERC_HEALTH_HEIGHT = 42;

	SM_SELMERC_BREATH_X		= ( 76 + INTERFACE_START_X );
	SM_SELMERC_BREATH_Y		= ( 48 + INV_INTERFACE_START_Y );
	SM_SELMERC_BREATH_WIDTH = 3;
	SM_SELMERC_BREATH_HEIGHT = 42;

	SM_SELMERC_MORALE_X		= ( 82 + INTERFACE_START_X );
	SM_SELMERC_MORALE_Y		= ( 48 + INV_INTERFACE_START_Y );
	SM_SELMERC_MORALE_WIDTH = 3;
	SM_SELMERC_MORALE_HEIGHT = 42;

	SM_SELMERC_BARS_TIP_X	= (	SM_SELMERC_HEALTH_X - SM_SELMERC_HEALTH_WIDTH );
	SM_SELMERC_BARS_TIP_Y	= ( SM_SELMERC_HEALTH_Y - SM_SELMERC_HEALTH_HEIGHT );
	SM_SELMERC_BARS_TIP_WIDTH = ((SM_SELMERC_BREATH_WIDTH + 4) *3);
	SM_SELMERC_BARS_TIP_HEIGHT= ( SM_SELMERC_HEALTH_HEIGHT );

	SM_SELMERCNAME_X		= ( 12 + INTERFACE_START_X );
	SM_SELMERCNAME_Y		= ( 55 + INV_INTERFACE_START_Y );
	SM_SELMERCNAME_WIDTH	= 53;
	SM_SELMERCNAME_HEIGHT	= 11;

	SM_SELMERC_FACE_X		= ( 14 + INTERFACE_START_X );
	SM_SELMERC_FACE_Y		= ( 5 + INV_INTERFACE_START_Y );
	SM_SELMERC_FACE_HEIGHT	= 42;
	SM_SELMERC_FACE_WIDTH	= 48;

	SM_SELMERC_PLATE_X		= ( 5 + INTERFACE_START_X );
	SM_SELMERC_PLATE_Y		= ( 2 + INV_INTERFACE_START_Y );
	SM_SELMERC_PLATE_HEIGHT = 65;
	SM_SELMERC_PLATE_WIDTH	= 83;

	if (iResolution >= _640x480 && iResolution < _800x600)
	{
		SM_BODYINV_X			= ( 141 + INTERFACE_START_X );
		SM_BODYINV_Y			= ( 68 + INV_INTERFACE_START_Y );
	}
	else
	{
		SM_BODYINV_X			= ( 151 + INTERFACE_START_X );
		SM_BODYINV_Y			= ( 68 + INV_INTERFACE_START_Y );
	}

	// Cammo
	gSMCamoXY.sX = SM_BODYINV_X; gSMCamoXY.sY = SM_BODYINV_Y;

	SM_UPDOWNB_X			= ( 8 + INTERFACE_START_X );
	SM_UPDOWNB_Y			= ( 70 + INV_INTERFACE_START_Y );
	SM_HANDCURSORB_X		= ( 33 + INTERFACE_START_X );
	SM_HANDCURSORB_Y		= ( 70 + INV_INTERFACE_START_Y );
	SM_CLIMBB_X				= ( 58 + INTERFACE_START_X );
	SM_CLIMBB_Y				= ( 70 + INV_INTERFACE_START_Y );
	SM_MUTEB_X				= ( 8 + INTERFACE_START_X );
	SM_MUTEB_Y				= ( 98 + INV_INTERFACE_START_Y );
	SM_LOOKB_X				= ( 33 + INTERFACE_START_X );
	SM_LOOKB_Y				= ( 98 + INV_INTERFACE_START_Y );
	SM_TALKB_X				= ( 58 + INTERFACE_START_X );
	SM_TALKB_Y				= ( 98 + INV_INTERFACE_START_Y );
	SM_STANCEUPB_X			= ( 88 + INTERFACE_START_X );
	SM_STANCEUPB_Y			= ( 70 + INV_INTERFACE_START_Y );
	SM_STEALTHMODE_X		= ( 88 + INTERFACE_START_X );
	SM_STEALTHMODE_Y		= ( 98 + INV_INTERFACE_START_Y );
	SM_STANCEDOWNB_X		= ( 88 + INTERFACE_START_X );
	SM_STANCEDOWNB_Y		= ( 126 + INV_INTERFACE_START_Y );
	SM_BURSTMODEB_X			= ( 88 + INTERFACE_START_X );
	SM_BURSTMODEB_Y			= ( 167 + INV_INTERFACE_START_Y );
	SM_PREVMERCB_X			= ( 10 + INTERFACE_START_X );
	SM_PREVMERCB_Y			= ( 130 + INV_INTERFACE_START_Y );
	SM_NEXTMERCB_X			= ( 52 + INTERFACE_START_X );
	SM_NEXTMERCB_Y			= ( 130 + INV_INTERFACE_START_Y );
	SM_OPTIONSB_X			= ( 10 + INTERFACE_START_X );
	SM_OPTIONSB_Y			= ( 165 + INV_INTERFACE_START_Y );
	
	// CHRISL: Backpack buttons
	SM_DROPPACK_X			= ( 612 + INTERFACE_START_X );
	SM_DROPPACK_Y			= ( 39 + INV_INTERFACE_START_Y );
	SM_ZIPPER_X				= ( 590 + INTERFACE_START_X );
	SM_ZIPPER_Y				= ( 39 + INV_INTERFACE_START_Y );
	SM_MAPSCREEN_X			= xResOffset + (xResSize - 146);	// 152
	SM_DONE_X				= xResOffset + (xResSize - 146);	// 152

	SM_SKILLMENU_X			= SM_MAPSCREEN_X - 2;
	SM_SECTORMENU_X			= SM_MAPSCREEN_X + 23 - 2;

//dnl	INTERFACE_CLOCK_X		= xResOffset + (xResSize - 86);
//dnl	INTERFACE_CLOCK_Y		= ( 119	+ INV_INTERFACE_START_Y );
	INTERFACE_CLOCK_X		= xResOffset + (xResSize - 86);
	INTERFACE_CLOCK_Y		= SCREEN_HEIGHT - 24;
	LOCATION_NAME_X			= xResOffset + (xResSize - 92);
	LOCATION_NAME_Y			= ( 89	+ INTERFACE_START_Y	);

	SM_DONE_Y				= ( 118 + INV_INTERFACE_START_Y );
	SM_MAPSCREEN_Y			= ( 140 + INV_INTERFACE_START_Y );

	SM_SKILLMENU_Y			= SM_MAPSCREEN_Y + 24 + 3;
	SM_SECTORMENU_Y			= SM_MAPSCREEN_Y + 24 + 3;

	SM_POSITIONB_X			= ( 106 + INTERFACE_START_X );
	SM_POSITIONB_Y			= ( 34 + INV_INTERFACE_START_Y );
	SM_POSITIONB_WIDTH		= 19;
	SM_POSITIONB_HEIGHT		= 24;

	SM_PERCENT_WIDTH		= 20;
	SM_PERCENT_HEIGHT		= 10;

	if (iResolution >= _640x480 && iResolution < _800x600)
	{
		SM_ARMOR_X				= ( 245 + INTERFACE_START_X );
		SM_ARMOR_Y				= ( 141 + INV_INTERFACE_START_Y );
		SM_ARMOR_LABEL_X		= ( 260 + INTERFACE_START_X );
		SM_ARMOR_LABEL_Y		= ( 131 + INV_INTERFACE_START_Y );
		SM_ARMOR_PERCENT_X		= ( 265 + INTERFACE_START_X );
		SM_ARMOR_PERCENT_Y		= ( 141 + INV_INTERFACE_START_Y );
	}
	else
	{
		SM_ARMOR_X				= ( 264 + INTERFACE_START_X );
		SM_ARMOR_Y				= ( 141 + INV_INTERFACE_START_Y );
		SM_ARMOR_LABEL_X		= ( 279 + INTERFACE_START_X );
		SM_ARMOR_LABEL_Y		= ( 131 + INV_INTERFACE_START_Y );
		SM_ARMOR_PERCENT_X		= ( 284 + INTERFACE_START_X );
		SM_ARMOR_PERCENT_Y		= ( 141 + INV_INTERFACE_START_Y );
	}

	SM_WEIGHT_LABEL_X		= ( 220 + INTERFACE_START_X );
	SM_WEIGHT_LABEL_Y		= ( 34 + INV_INTERFACE_START_Y );
	SM_WEIGHT_PERCENT_X		= ( 239 + INTERFACE_START_X );
	SM_WEIGHT_PERCENT_Y		= ( 34 + INV_INTERFACE_START_Y );
	SM_WEIGHT_X				= ( 218 + INTERFACE_START_X );
	SM_WEIGHT_Y				= ( 33 + INV_INTERFACE_START_Y );

	SM_CAMMO_LABEL_X		= ( 220 + INTERFACE_START_X );
	SM_CAMMO_LABEL_Y		= ( 50 + INV_INTERFACE_START_Y );
	SM_CAMMO_PERCENT_X		= ( 239 + INTERFACE_START_X );
	SM_CAMMO_PERCENT_Y		= ( 49 + INV_INTERFACE_START_Y );
	SM_CAMMO_X				= ( 218 + INTERFACE_START_X );
	SM_CAMMO_Y				= ( 49 + INV_INTERFACE_START_Y );

	SM_STATS_WIDTH			= 16;
	SM_STATS_HEIGHT			= 8 ;
	SM_AGI_X				= ( 115 + INTERFACE_START_X );
	SM_AGI_Y				= ( 7 + INV_INTERFACE_START_Y );
	SM_DEX_X				= ( 115 + INTERFACE_START_X );
	SM_DEX_Y				= ( 17 + INV_INTERFACE_START_Y );
	SM_STR_X				= ( 115 + INTERFACE_START_X );
	SM_STR_Y				= ( 27 + INV_INTERFACE_START_Y );
	SM_CHAR_X				= ( 115 + INTERFACE_START_X );
	SM_CHAR_Y				= ( 37 + INV_INTERFACE_START_Y );
	SM_WIS_X				= ( 115 + INTERFACE_START_X );
	SM_WIS_Y				= ( 47 + INV_INTERFACE_START_Y );
	SM_EXPLVL_X				= ( 163 + INTERFACE_START_X );
	SM_EXPLVL_Y				= ( 7 + INV_INTERFACE_START_Y );
	SM_MRKM_X				= ( 163 + INTERFACE_START_X );
	SM_MRKM_Y				= ( 17 + INV_INTERFACE_START_Y );
	SM_MECH_X				= ( 163 + INTERFACE_START_X );
	SM_MECH_Y				= ( 27 + INV_INTERFACE_START_Y );
	SM_EXPL_X				= ( 163 + INTERFACE_START_X );
	SM_EXPL_Y				= ( 37 + INV_INTERFACE_START_Y );
	SM_MED_X				= ( 163 + INTERFACE_START_X );
	SM_MED_Y				= ( 47 + INV_INTERFACE_START_Y );

	MONEY_X					= ( 185 + INTERFACE_START_X );
	MONEY_Y					= ( 5 + INV_INTERFACE_START_Y );
	MONEY_WIDTH				= 30;
	MONEY_HEIGHT			= 22;
	
	// ow ye font color they can also be customizable :P
	STATS_TITLE_FONT_COLOR	= 6;
	STATS_TEXT_FONT_COLOR	= 5;

	//Keyring 218/5 on Inventory_Bottom_Panel.sti
	KEYRING_X			= xResOffset + 219; //209;
	KEYRING_Y			= (5 + INV_INTERFACE_START_Y);

	// so we got everything "dynamic" now we just return TRUE
	return ( TRUE );
}

BOOLEAN InitializeSMPanel(	)
{
	VOBJECT_DESC	VObjectDesc;

 /*	OK i need to initialize coords here
 *	Isnt it cool
 *	any questions? joker
 */
//	InitializeSMPanelCoords( );

	fDisplayOverheadMap = TRUE;

	// failing the CHECKF after this will cause you to lose your mouse
	VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;

	if (iResolution >= _640x480 && iResolution < _800x600)
	{
		strcpy( VObjectDesc.ImageFile, "INTERFACE\\inventory_bottom_panel.STI" );
	}
	else if (iResolution < _1024x768)
	{
		strcpy( VObjectDesc.ImageFile, "INTERFACE\\inventory_bottom_panel_800x600.STI" );
	}
	else
	{
		strcpy( VObjectDesc.ImageFile, "INTERFACE\\inventory_bottom_panel_1024x768.STI" );
	}

	CHECKF( AddVideoObject( &VObjectDesc, &guiSMPanel) );

	VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	FilenameForBPP("INTERFACE\\inventory_gold_front.sti", VObjectDesc.ImageFile);
	CHECKF( AddVideoObject( &VObjectDesc, &guiSMObjects ) );

	VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	FilenameForBPP("INTERFACE\\inv_frn.sti", VObjectDesc.ImageFile);
	CHECKF( AddVideoObject( &VObjectDesc, &guiSMObjects2 ) );


	VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	FilenameForBPP("INTERFACE\\secondary_gun_hidden.sti", VObjectDesc.ImageFile);
	CHECKF( AddVideoObject( &VObjectDesc, &guiSecItemHiddenVO ) );

	VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	FilenameForBPP("INTERFACE\\Bars.sti", VObjectDesc.ImageFile);
	CHECKF( AddVideoObject( &VObjectDesc, &guiBrownBackgroundForTeamPanel ) );	

	// Clear inv display stuff
	memset( gfSM_HandInvDispText, 0, sizeof( gfSM_HandInvDispText ) );

	// INIT viewport region
	// Set global mouse regions for SM panel
	// Define region for viewport
	MSYS_DefineRegion( &gViewportRegion, 0, 0 ,gsVIEWPORT_END_X, gsVIEWPORT_WINDOW_END_Y, MSYS_PRIORITY_NORMAL, VIDEO_NO_CURSOR, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK );
	MSYS_AddRegion( &gViewportRegion );

	// Create buttons
	CHECKF( CreateSMPanelButtons( ) );

	// Set viewports
	// Define region for panel
	MSYS_DefineRegion( &gSMPanelRegion, 0, INV_INTERFACE_START_Y ,SCREEN_WIDTH, SCREEN_HEIGHT, MSYS_PRIORITY_NORMAL,
						CURSOR_NORMAL, MSYS_NO_CALLBACK, InvPanelButtonClickCallback );
	// Add region
	MSYS_AddRegion( &gSMPanelRegion);

	//DEfine region for selected guy panel
	MSYS_DefineRegion( &gSM_SELMERCPanelRegion, SM_SELMERC_FACE_X, SM_SELMERC_FACE_Y, SM_SELMERC_FACE_X + SM_SELMERC_FACE_WIDTH, SM_SELMERC_FACE_Y + SM_SELMERC_FACE_HEIGHT, MSYS_PRIORITY_NORMAL,
						MSYS_NO_CURSOR, SelectedMercButtonMoveCallback, SelectedMercButtonCallback );
	// Add region
	MSYS_AddRegion( &gSM_SELMERCPanelRegion );

	//DEfine region for selected guy panel
	MSYS_DefineRegion( &gSM_SELMERCEnemyIndicatorRegion, SM_SELMERC_FACE_X + 1, SM_SELMERC_FACE_Y + 1, SM_SELMERC_FACE_X + INDICATOR_BOX_WIDTH, SM_SELMERC_FACE_Y + INDICATOR_BOX_HEIGHT, MSYS_PRIORITY_NORMAL,
						MSYS_NO_CURSOR, MSYS_NO_CALLBACK, SelectedMercEnemyIndicatorCallback );
	// Add region
	MSYS_AddRegion( &gSM_SELMERCEnemyIndicatorRegion );


	//DEfine region for money button
	MSYS_DefineRegion( &gSM_SELMERCMoneyRegion, MONEY_X, MONEY_Y, MONEY_X + MONEY_WIDTH, MONEY_Y + MONEY_HEIGHT, MSYS_PRIORITY_HIGH,
						MSYS_NO_CURSOR, MSYS_NO_CALLBACK, SMInvMoneyButtonCallback );
	// Add region
	MSYS_AddRegion( &gSM_SELMERCMoneyRegion );

	SetRegionFastHelpText( &(gSM_SELMERCMoneyRegion), TacticalStr[ MONEY_BUTTON_HELP_TEXT ] );

	// Check if mouse is in region and if so, adjust...
	if ( IsMouseInRegion( &gSM_SELMERCPanelRegion ) )
	{
		HandleMouseOverSoldierFaceForContMove( gpSMCurrentMerc, TRUE );
	}

	MSYS_DefineRegion(&gSM_SELMERCWeightRegion, (SM_WEIGHT_X + 2), (SM_WEIGHT_Y),
		(SM_WEIGHT_X + 28), (SM_WEIGHT_Y + 10), MSYS_PRIORITY_NORMAL,
		MSYS_NO_CURSOR, MSYS_NO_CALLBACK, SelectedMercButtonCallback);
	MSYS_AddRegion(&gSM_SELMERCWeightRegion);

	//DEfine region for selected guy panel
	// THIS IS A REGION OF BAR TIPS
	MSYS_DefineRegion( &gSM_SELMERCBarsRegion, SM_SELMERC_BARS_TIP_X, SM_SELMERC_BARS_TIP_Y,
					SM_SELMERC_BARS_TIP_X + SM_SELMERC_BARS_TIP_WIDTH, SM_SELMERC_BARS_TIP_Y + SM_SELMERC_BARS_TIP_HEIGHT, MSYS_PRIORITY_NORMAL,
						MSYS_NO_CURSOR, MSYS_NO_CALLBACK, SelectedMercButtonCallback );
	MSYS_AddRegion( &gSM_SELMERCBarsRegion );

	// Added by SANDRO - define region for camo tooltip window
	MSYS_DefineRegion( &gSM_SELMERCCamoRegion, ( SM_CAMMO_X + 2), ( SM_CAMMO_Y ),
					(SM_CAMMO_X + 28), (SM_CAMMO_Y + 10), MSYS_PRIORITY_NORMAL,
						MSYS_NO_CURSOR, MSYS_NO_CALLBACK, SelectedMercButtonCallback );
	MSYS_AddRegion( &gSM_SELMERCCamoRegion );

	// sevenfm: left panel
	MSYS_DefineRegion(&gSM_LeftPanelRegion, 0, INV_INTERFACE_START_Y, INTERFACE_START_X, INV_INTERFACE_START_Y + INV_INTERFACE_HEIGHT, MSYS_PRIORITY_NORMAL, CURSOR_NORMAL, MSYS_NO_CALLBACK, LeftPanelCallback);
	// Add region
	MSYS_AddRegion(&gSM_LeftPanelRegion);
	// Add tooltip
	/*if (gGameSettings.fOptions[TOPTION_TACTICAL_LEFT_PANEL])
	{
		SetRegionFastHelpText(&gSM_LeftPanelRegion, TacticalStr[TEXT_PANEL_TOOLTIP_STR]);
	}*/

	gsTacticalActionOffset = 0;
	// sevenfm: right panel
	//PrepareTacticalRightPanelData(2, 0);
	//MSYS_DefineRegion(&gSM_RightPanelRegion, gsPanelWideX, gsPanelWideY, gsPanelWideX + gsPanelWideWidth, gsPanelWideY + gsPanelWideHeight, MSYS_PRIORITY_NORMAL, CURSOR_NORMAL, MSYS_NO_CALLBACK, RightPanelCallback);
	MSYS_DefineRegion(&gSM_RightPanelRegion, INTERFACE_START_X + INTERFACE_WIDTH, INV_INTERFACE_START_Y, INTERFACE_START_X + INTERFACE_WIDTH + INTERFACE_START_X, INV_INTERFACE_START_Y + INV_INTERFACE_HEIGHT, MSYS_PRIORITY_NORMAL, CURSOR_NORMAL, MSYS_NO_CALLBACK, RightPanelCallback);
	// Add region
	MSYS_AddRegion(&gSM_RightPanelRegion);
	// Add tooltip
	SetRegionFastHelpText(&gSM_RightPanelRegion, TacticalStr[TEXT_PANEL_TOOLTIP_STR]);

	// Action panel
	if (gGameSettings.fOptions[TOPTION_TACTICAL_ACTION_PANEL])
	{		
		InitializeActionPanelData();
		CreateMouseRegionsActionPanel();
	}	

	// CHRISL: Setup default coords
	if((UsingNewInventorySystem() == true))
		InitInventoryNew();
	else
		InitInventoryOld();
	
	InitInvSlotInterface( gSMInvPocketXY, &gSMCamoXY, SMInvMoveCallback, SMInvClickCallback, SMInvMoveCammoCallback, SMInvClickCamoCallback, FALSE );

	InitKeyRingInterface( KeyRingItemPanelButtonCallback );


	// this is important! It will disable buttons like SM_MAP_SCREEN_BUTTON when they're supposed to be disabled - the previous
	// disabled state is lost everytime panel is reinitialized, because all the buttons are created from scratch!
	if ( gpItemPointer == NULL )
	{
		// empty cursor - enable, not from item pickup
		EnableSMPanelButtons( TRUE, FALSE );
	}
	else
	{
		// full cursor - disable, from item pickup
		EnableSMPanelButtons( FALSE, TRUE );
	}
	
	if( INTERFACE_START_X > 0 )
	{
		ColorFillVideoSurfaceArea( FRAME_BUFFER, 0, INV_INTERFACE_START_Y, SCREEN_WIDTH, SCREEN_HEIGHT, 0 );
		ColorFillVideoSurfaceArea( guiSAVEBUFFER, 0, INV_INTERFACE_START_Y, SCREEN_WIDTH, SCREEN_HEIGHT, 0 );
	}

	return( TRUE );
}


BOOLEAN CreateSMPanelButtons( )
{
	CHAR8 ubString[48];
  //CHAR8 ubString2[48]; //legion DisableButton
	//CHAR8 ubString3[48]; //legion

	// CHRISL: Reset backpack buttons as well
	giSMStealthImages = giSMZipperImages = giSMDropPackImages = -1;
	giSMStealthButton = giSMZipperButton = giSMDropPackButton = -1;
	gfUIStanceDifferent = TRUE;
	gfAllDisabled	= FALSE;

	FilenameForBPP("INTERFACE\\inventory_buttons.sti", ubString);
	
	//-----------------------
	// Load button Graphics
	iSMPanelImages[ STANCEUP_IMAGES	]			= LoadButtonImage(ubString,-1,0,-1,10,-1 );

	iSMPanelImages[ UPDOWN_IMAGES	]				= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	] ,-1,9,-1,19,-1 );
	iSMPanelImages[ CLIMB_IMAGES	]					= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	] ,-1,3,-1,13,-1 );
	iSMPanelImages[ STANCEDOWN_IMAGES	]		= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	] ,-1,8,-1,18,-1 );
	iSMPanelImages[ HANDCURSOR_IMAGES	]		= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	] ,-1,1,-1,11,-1 );
	iSMPanelImages[ PREVMERC_IMAGES	]			= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	] ,-1,20,-1,22,-1 );
	iSMPanelImages[ NEXTMERC_IMAGES	]			= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	] ,-1,21,-1,23,-1 );
	//iSMPanelImages[ BURSTMODE_IMAGES	]			= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	] ,-1,7,-1,17,-1 );
	iSMPanelImages[ LOOK_IMAGES	]					= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	] ,-1,2,-1,12,-1 );
	iSMPanelImages[ TALK_IMAGES	]					= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	] ,-1,6,-1,16,-1 );
	iSMPanelImages[ MUTE_IMAGES	]					= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	] ,-1,5,-1,15,-1 );
	
	iSMPanelImages[ OPTIONS_IMAGES	]				= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	] ,-1,24,-1,25,-1 );
	
	iBurstButtonImages[ WM_NORMAL ]					= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	], -1,  7, -1, -1, -1 );
	iBurstButtonImages[ WM_BURST ]					= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	], -1, 17, -1, -1, -1 );
	iBurstButtonImages[ WM_AUTOFIRE ]				= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	], -1, 17, -1, -1, -1 );
	iBurstButtonImages[ WM_ATTACHED_GL ]			= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	], -1, 26, -1, -1, -1 );
	iBurstButtonImages[ WM_ATTACHED_GL_BURST ]		= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	], -1, 17, -1, -1, -1 );
	iBurstButtonImages[ WM_ATTACHED_GL_AUTO ]		= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	], -1, 17, -1, -1, -1 );
	iBurstButtonImages[ WM_ATTACHED_UB ]			= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	], -1,  7, -1, -1, -1 );
	iBurstButtonImages[ WM_ATTACHED_UB_BURST ]		= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	], -1, 17, -1, -1, -1 );
	iBurstButtonImages[ WM_ATTACHED_UB_AUTO ]		= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	], -1, 17, -1, -1, -1 );
	iBurstButtonImages[ WM_ATTACHED_BAYONET ]		= UseLoadedButtonImage( iSMPanelImages[ STANCEUP_IMAGES	], -1, 17, -1, -1, -1 );

	FilenameForBPP("INTERFACE\\invadd-ons.sti", ubString);
	// Load button Graphics
	iSMPanelImages[ STANCE_IMAGES	]				= LoadButtonImage(ubString,0,0,-1,2,-1 );

	FilenameForBPP("INTERFACE\\inventory_buttons_2.sti", ubString);
	// Load button Graphics
	iSMPanelImages[ DONE_IMAGES	]					= LoadButtonImage(ubString,-1,1,-1,3,-1 );
	iSMPanelImages[ MAPSCREEN_IMAGES	]			= UseLoadedButtonImage( iSMPanelImages[ DONE_IMAGES	] ,-1,0,-1,2,-1 );

	// sevenfm
	iSMPanelImages[SKILL_MENU_IMAGES] = UseLoadedButtonImage(iSMPanelImages[STANCEUP_IMAGES], -1, 6, -1, 16, -1);
	iSMPanelImages[SECTOR_MENU_IMAGES] = UseLoadedButtonImage(iSMPanelImages[STANCEUP_IMAGES], -1, 1, -1, 11, -1);

	// CHRISL: Needed for new inventory backpack buttons
	FilenameForBPP("INTERFACE\\backpack_buttons.sti", ubString);
	iSMPanelImages[ BACKPACK_IMAGES  ]				= LoadButtonImage(ubString,0,0,-1,2,-1 );

	// Create buttons

	// SET BUTTONS TO -1
	memset( iSMPanelButtons, -1, sizeof( iSMPanelButtons ) );
									
	iSMPanelButtons[ SM_MAP_SCREEN_BUTTON ] = QuickCreateButton( iSMPanelImages[ MAPSCREEN_IMAGES ], SM_MAPSCREEN_X, SM_MAPSCREEN_Y,
										BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
										DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnMapScreenCallback );
	SetButtonFastHelpText( iSMPanelButtons[ SM_MAP_SCREEN_BUTTON ], TacticalStr[ MAPSCREEN_POPUPTEXT ]);
	SetBtnHelpEndCallback( iSMPanelButtons[ SM_MAP_SCREEN_BUTTON ], HelpTextDoneCallback );

	// sevenfm: no place for additional buttons when using old system
	if(UsingNewInventorySystem())
	{
		// sevenfm: skill menu
		iSMPanelButtons[SM_SKILL_MENU_BUTTON] = QuickCreateButton(iSMPanelImages[SKILL_MENU_IMAGES], SM_SKILLMENU_X, SM_SKILLMENU_Y,
			BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
			DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnSkillMenuCallback);
		SetButtonFastHelpText(iSMPanelButtons[SM_SKILL_MENU_BUTTON], TacticalStr[SKILLMENU_POPUPTEXT]);
		SetBtnHelpEndCallback(iSMPanelButtons[SM_SKILL_MENU_BUTTON], HelpTextDoneCallback);

		// sevenfm: sector menu
		iSMPanelButtons[SM_SECTOR_MENU_BUTTON] = QuickCreateButton(iSMPanelImages[SECTOR_MENU_IMAGES], SM_SECTORMENU_X, SM_SECTORMENU_Y,
			BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
			DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnSectorMenuCallback);
		SetButtonFastHelpText(iSMPanelButtons[SM_SECTOR_MENU_BUTTON], TacticalStr[SECTORMENU_POPUPTEXT]);
		SetBtnHelpEndCallback(iSMPanelButtons[SM_SECTOR_MENU_BUTTON], HelpTextDoneCallback);
	}	

	iSMPanelButtons[ SM_DONE_BUTTON ] = CreateIconAndTextButton( iSMPanelImages[ DONE_IMAGES ], L"", FONT12ARIAL,
													FONT_MCOLOR_WHITE, DEFAULT_SHADOW ,
													FONT_MCOLOR_WHITE, DEFAULT_SHADOW ,
													TEXT_CJUSTIFIED,
													SM_DONE_X, SM_DONE_Y, BUTTON_TOGGLE ,MSYS_PRIORITY_HIGH-1,
													DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnSMDoneCallback );
	SetButtonFastHelpText( iSMPanelButtons[ SM_DONE_BUTTON ], TacticalStr[ END_TURN_POPUPTEXT ] );
	SetBtnHelpEndCallback( iSMPanelButtons[ SM_DONE_BUTTON ], HelpTextDoneCallback );


	iSMPanelButtons[ TALK_BUTTON ] = QuickCreateButton( iSMPanelImages[ TALK_IMAGES ], SM_TALKB_X, SM_TALKB_Y,
										BUTTON_NEWTOGGLE, MSYS_PRIORITY_HIGH - 1,
										MSYS_NO_CALLBACK, (GUI_CALLBACK)BtnTalkCallback );
	SetButtonFastHelpText( iSMPanelButtons[ TALK_BUTTON ], TacticalStr[ TALK_CURSOR_POPUPTEXT ] );
	SetBtnHelpEndCallback( iSMPanelButtons[ TALK_BUTTON ], HelpTextDoneCallback );



	iSMPanelButtons[ MUTE_BUTTON ] = QuickCreateButton( iSMPanelImages[ MUTE_IMAGES ], SM_MUTEB_X, SM_MUTEB_Y,
										BUTTON_NEWTOGGLE, MSYS_PRIORITY_HIGH - 1,
										DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnMuteCallback );
	SetButtonFastHelpText( iSMPanelButtons[ MUTE_BUTTON ], TacticalStr[ TOGGLE_MUTE_POPUPTEXT ] );
	SetBtnHelpEndCallback( iSMPanelButtons[ MUTE_BUTTON ], HelpTextDoneCallback );


	iSMPanelButtons[ STANCEUP_BUTTON ] = QuickCreateButton( iSMPanelImages[ STANCEUP_IMAGES ], SM_STANCEUPB_X, SM_STANCEUPB_Y,
										BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
										DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnStanceUpCallback );
	if ( iSMPanelButtons[ STANCEUP_BUTTON ] == -1 )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Cannot create Interface button" );
		return( FALSE );
	}
	SetButtonFastHelpText( iSMPanelButtons[ STANCEUP_BUTTON ], TacticalStr[ CHANGE_STANCE_UP_POPUPTEXT ] );
	SetBtnHelpEndCallback( iSMPanelButtons[ STANCEUP_BUTTON ], HelpTextDoneCallback );

	//SetButtonFastHelpText( iSMPanelButtons[ STANCEUP_BUTTON ],L"Change Stance Up");


	iSMPanelButtons[ UPDOWN_BUTTON ] = QuickCreateButton( iSMPanelImages[ UPDOWN_IMAGES ], SM_UPDOWNB_X, SM_UPDOWNB_Y,
										BUTTON_NEWTOGGLE, MSYS_PRIORITY_HIGH - 1,
										MSYS_NO_CALLBACK, (GUI_CALLBACK)BtnUpdownCallback );
	if ( iSMPanelButtons[ UPDOWN_BUTTON ] == -1 )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Cannot create Interface button" );
		return( FALSE );
	}
	//SetButtonFastHelpText( iSMPanelButtons[ UPDOWN_BUTTON ],L"Whatever");
	SetButtonFastHelpText( iSMPanelButtons[ UPDOWN_BUTTON ], TacticalStr[ CURSOR_LEVEL_POPUPTEXT ] );
	SetBtnHelpEndCallback( iSMPanelButtons[ UPDOWN_BUTTON ], HelpTextDoneCallback );


	iSMPanelButtons[ CLIMB_BUTTON ] = QuickCreateButton( iSMPanelImages[ CLIMB_IMAGES ], SM_CLIMBB_X, SM_CLIMBB_Y,
										BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
										DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnClimbCallback );
	if ( iSMPanelButtons[ CLIMB_BUTTON ] == -1 )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Cannot create Interface button" );
		return( FALSE );
	}
	//SetButtonFastHelpText( iSMPanelButtons[ CLIMB_BUTTON ],L"Climb");
	SetButtonFastHelpText( iSMPanelButtons[ CLIMB_BUTTON ], TacticalStr[ JUMPCLIMB_POPUPTEXT ] );
	SetBtnHelpEndCallback( iSMPanelButtons[ CLIMB_BUTTON ], HelpTextDoneCallback );


	iSMPanelButtons[ STANCEDOWN_BUTTON ] = QuickCreateButton( iSMPanelImages[ STANCEDOWN_IMAGES ], SM_STANCEDOWNB_X, SM_STANCEDOWNB_Y,
										BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
										DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnStanceDownCallback );
	if ( iSMPanelButtons[ STANCEDOWN_BUTTON ] == -1 )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Cannot create Interface button" );
		return( FALSE );
	}
	//SetButtonFastHelpText( iSMPanelButtons[ STANCEDOWN_BUTTON ],L"Change Stance Down");
	SetButtonFastHelpText( iSMPanelButtons[ STANCEDOWN_BUTTON ], TacticalStr[ CHANGE_STANCE_DOWN_POPUPTEXT ] );
	SetBtnHelpEndCallback( iSMPanelButtons[ STANCEDOWN_BUTTON ], HelpTextDoneCallback );


	iSMPanelButtons[ HANDCURSOR_BUTTON ] = QuickCreateButton( iSMPanelImages[ HANDCURSOR_IMAGES ], SM_HANDCURSORB_X, SM_HANDCURSORB_Y,
										BUTTON_NEWTOGGLE, MSYS_PRIORITY_HIGH - 1,
										MSYS_NO_CALLBACK, (GUI_CALLBACK)BtnHandCursorCallback );
	if ( iSMPanelButtons[ HANDCURSOR_BUTTON ] == -1 )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Cannot create Interface button" );
		return( FALSE );
	}
	//SetButtonFastHelpText( iSMPanelButtons[ HANDCURSOR_BUTTON ],L"Change Stance Down");
	SetButtonFastHelpText( iSMPanelButtons[ HANDCURSOR_BUTTON ], TacticalStr[ EXAMINE_CURSOR_POPUPTEXT ] );
	SetBtnHelpEndCallback( iSMPanelButtons[ HANDCURSOR_BUTTON ], HelpTextDoneCallback );


	iSMPanelButtons[ PREVMERC_BUTTON ] = QuickCreateButton( iSMPanelImages[ PREVMERC_IMAGES ], SM_PREVMERCB_X, SM_PREVMERCB_Y,
										BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
										DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnPrevMercCallback );
	if ( iSMPanelButtons[ PREVMERC_BUTTON ] == -1 )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Cannot create Interface button" );
		return( FALSE );
	}
	//SetButtonFastHelpText( iSMPanelButtons[ PREVMERC_BUTTON ],L"Change Stance Down");
	SetButtonFastHelpText( iSMPanelButtons[ PREVMERC_BUTTON ], TacticalStr[ PREV_MERC_POPUPTEXT ] );
	SetBtnHelpEndCallback( iSMPanelButtons[ PREVMERC_BUTTON ], HelpTextDoneCallback );


	iSMPanelButtons[ NEXTMERC_BUTTON ] = QuickCreateButton( iSMPanelImages[ NEXTMERC_IMAGES ], SM_NEXTMERCB_X, SM_NEXTMERCB_Y,
										BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
										DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnNextMercCallback );
	if ( iSMPanelButtons[ NEXTMERC_BUTTON ] == -1 )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Cannot create Interface button" );
		return( FALSE );
	}
	SetButtonFastHelpText( iSMPanelButtons[ NEXTMERC_BUTTON ], TacticalStr[ NEXT_MERC_POPUPTEXT ] );
	//SetButtonFastHelpText( iSMPanelButtons[ NEXTMERC_BUTTON ],L"Change Stance Down");
	SetBtnHelpEndCallback( iSMPanelButtons[ NEXTMERC_BUTTON ], HelpTextDoneCallback );


	iSMPanelButtons[ OPTIONS_BUTTON ] = QuickCreateButton( iSMPanelImages[ OPTIONS_IMAGES ], SM_OPTIONSB_X, SM_OPTIONSB_Y,
										BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
										DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnOptionsCallback );
	if ( iSMPanelButtons[ OPTIONS_BUTTON ] == -1 )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Cannot create Interface button" );
		return( FALSE );
	}
	//SetButtonFastHelpText( iSMPanelButtons[ OPTIONS_BUTTON ],L"Change Stance Down");
	SetButtonFastHelpText( iSMPanelButtons[ OPTIONS_BUTTON ], TacticalStr[ CHANGE_OPTIONS_POPUPTEXT ] );
	SetBtnHelpEndCallback( iSMPanelButtons[ OPTIONS_BUTTON ], HelpTextDoneCallback );


	/*iSMPanelButtons[ BURSTMODE_BUTTON ] = QuickCreateButton( iSMPanelImages[ BURSTMODE_IMAGES ], SM_BURSTMODEB_X, SM_BURSTMODEB_Y,
										BUTTON_NEWTOGGLE, MSYS_PRIORITY_HIGH - 1,
										MSYS_NO_CALLBACK, (GUI_CALLBACK)BtnBurstModeCallback );*/
	iSMPanelButtons[ BURSTMODE_BUTTON ] = QuickCreateButton( iBurstButtonImages[ WM_NORMAL ], SM_BURSTMODEB_X, SM_BURSTMODEB_Y, BUTTON_NO_TOGGLE, MSYS_PRIORITY_HIGH, MSYS_NO_CALLBACK, (GUI_CALLBACK)BtnBurstModeCallback );

	if ( iSMPanelButtons[ BURSTMODE_BUTTON ] == -1 )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Cannot create Interface button" );
		return( FALSE );
	}
	SetButtonFastHelpText( iSMPanelButtons[ BURSTMODE_BUTTON ], TacticalStr[ TOGGLE_BURSTMODE_POPUPTEXT ] );
	//SetButtonFastHelpText( iSMPanelButtons[ BURSTMODE_BUTTON ],L"Change Stance Down");
	SetBtnHelpEndCallback( iSMPanelButtons[ BURSTMODE_BUTTON ], HelpTextDoneCallback );

	iSMPanelButtons[ LOOK_BUTTON ] = QuickCreateButton( iSMPanelImages[ LOOK_IMAGES ], SM_LOOKB_X, SM_LOOKB_Y,
										BUTTON_NEWTOGGLE, MSYS_PRIORITY_HIGH - 1,
										MSYS_NO_CALLBACK, (GUI_CALLBACK)BtnLookCallback );
	if ( iSMPanelButtons[ LOOK_BUTTON ] == -1 )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Cannot create Interface button" );
		return( FALSE );
	}
	//SetButtonFastHelpText( iSMPanelButtons[ LOOK_BUTTON ],L"Change Stance Down");
	SetButtonFastHelpText( iSMPanelButtons[ LOOK_BUTTON ], TacticalStr[ LOOK_CURSOR_POPUPTEXT ] );
	SetBtnHelpEndCallback( iSMPanelButtons[ LOOK_BUTTON ], HelpTextDoneCallback );

	return( TRUE );
}

void	RemoveSMPanelButtons( )
{
	UINT32 cnt;

	for ( cnt = 0; cnt < NUM_SM_BUTTONS; cnt++ )
	{
		if ( iSMPanelButtons[ cnt ] != -1 )
		{
			RemoveButton( iSMPanelButtons[ cnt ] );
		}
	}

	for ( cnt = 0; cnt < NUM_SM_BUTTON_IMAGES; cnt++ )
	{
		if(iSMPanelImages[ cnt ] != -1)
		{
			UnloadButtonImage( iSMPanelImages[ cnt ] );
		}
	}

	if ( giSMStealthButton != -1 )
	{
		RemoveButton( giSMStealthButton );
	}

	RenderBackpackButtons(DEACTIVATE_BUTTON);	/* CHRISL: Needed for new inventory backpack buttons */

	if ( giSMStealthImages != -1 )
	{
		UnloadButtonImage( giSMStealthImages );
	}
	RenderBackpackButtons(UNLOAD_BUTTON);	/* CHRISL: Needed for new inventory backpack buttons */
	
	UnloadButtonImage( iBurstButtonImages[ WM_NORMAL ] );
	UnloadButtonImage( iBurstButtonImages[ WM_BURST ] );
	UnloadButtonImage( iBurstButtonImages[ WM_AUTOFIRE ] );
	UnloadButtonImage( iBurstButtonImages[ WM_ATTACHED_GL ] );
	UnloadButtonImage( iBurstButtonImages[ WM_ATTACHED_GL_BURST ] );
	UnloadButtonImage( iBurstButtonImages[ WM_ATTACHED_GL_AUTO ] );
	UnloadButtonImage( iBurstButtonImages[ WM_ATTACHED_UB ] );
	UnloadButtonImage( iBurstButtonImages[ WM_ATTACHED_UB_BURST ] );
	UnloadButtonImage( iBurstButtonImages[ WM_ATTACHED_UB_AUTO ] );
	UnloadButtonImage( iBurstButtonImages[ WM_ATTACHED_BAYONET ] );
}




BOOLEAN ShutdownSMPanel( )
{

	// All buttons and regions and video objects and video surfaces will be deleted at shutddown of SGM
	// We may want to delete them at the interm as well, to free up room for other panels
	DeleteVideoObjectFromIndex( guiSMPanel );
	DeleteVideoObjectFromIndex( guiSMObjects );
	DeleteVideoObjectFromIndex( guiSMObjects2 );
	DeleteVideoObjectFromIndex( guiSecItemHiddenVO );
	DeleteVideoObjectFromIndex( guiBrownBackgroundForTeamPanel );

	gubSelectSMPanelToMerc = NOBODY;

	// CJC: delete key ring if open
	DeleteKeyRingPopup(); // function will abort if key ring is not up

	// ATE: Delete desc panel if it was open....
	if ( gfInItemDescBox )
	{
		DeleteItemDescriptionBox( );
	}

	// Shutdown item slot interface
	ShutdownInvSlotInterface( );

	// shutdown keyring interface
	ShutdownKeyRingInterface( );

	ShutdownInventoryInterface();

	MSYS_RemoveRegion( &gSMPanelRegion );
	MSYS_RemoveRegion( &gSM_SELMERCPanelRegion );
	MSYS_RemoveRegion( &gSM_SELMERCBarsRegion );
	MSYS_RemoveRegion( &gSM_SELMERCMoneyRegion );
	MSYS_RemoveRegion( &gSM_SELMERCEnemyIndicatorRegion );
	MSYS_RemoveRegion(&gSM_SELMERCWeightRegion);
	MSYS_RemoveRegion( &gSM_SELMERCCamoRegion ); // added - SANDRO

	// sevenfm
	MSYS_RemoveRegion(&gSM_LeftPanelRegion);
	MSYS_RemoveRegion(&gSM_RightPanelRegion);
	DestroyMouseRegionsActionPanel();

	HandleMouseOverSoldierFaceForContMove( gpSMCurrentMerc, FALSE );

	MSYS_RemoveRegion( &gViewportRegion );

	RemoveSMPanelButtons( );

	//CHRISL: Following line is to make sure that team panel buttons activate
	DisableTacticalTeamPanelButtons( FALSE );

	// sevenfm: by default try to use big radar map
	if (fAllowBigRadarMap && INTERFACE_START_X >= 240)
	{
		gfRenderSquadList = TRUE;
	}

	return( TRUE );
}


void RenderSMPanel( BOOLEAN *pfDirty )
{
	INT16 sFontX, sFontY;
	INT16	usX, usY;
	CHAR16 sString[64];
	UINT32	cnt;
	static CHAR16 pStr[1024], pMoraleStr[64], sTemp[64];

	if ( gubSelectSMPanelToMerc != NOBODY )
	{
		// Give him the panel!
		SetSMPanelCurrentMerc( gubSelectSMPanelToMerc );
	}

	// ATE: Don't do anything if we are in stack popup and are refreshing stuff....
	if ( ( InItemStackPopup( ) || ( InKeyRingPopup( ) ) ) && (*pfDirty) == DIRTYLEVEL1 )
	{
		return;
	}

	if ( gfCheckForMouseOverItem )
	{
		if ( ( GetJA2Clock( ) - guiMouseOverItemTime ) > 100 )
		{
			if ( HandleCompatibleAmmoUI( gpSMCurrentMerc, (INT8)gbCheckForMouseOverItemPos, TRUE ) )
			{
				(*pfDirty) = DIRTYLEVEL2;
			}

			gfCheckForMouseOverItem = FALSE;
		}
	}

	HandleNewlyAddedItems( gpSMCurrentMerc, pfDirty );

	if ( InItemDescriptionBox( ) )
	{
		HandleItemDescriptionBox( pfDirty );
	}

	if ( *pfDirty == DIRTYLEVEL2 )
	{
		ColorFillVideoSurfaceArea(guiSAVEBUFFER, 0, INV_INTERFACE_START_Y, INTERFACE_START_X, INTERFACE_START_Y + INV_INTERFACE_HEIGHT, Get16BPPColor(FROMRGB(16, 16, 16)));
		RestoreExternBackgroundRect(0, INV_INTERFACE_START_Y, INTERFACE_START_X, INV_INTERFACE_HEIGHT);
		ColorFillVideoSurfaceArea(guiSAVEBUFFER, INTERFACE_START_X + INTERFACE_WIDTH, INV_INTERFACE_START_Y, INTERFACE_START_X * 2 + INTERFACE_WIDTH, INTERFACE_START_Y + INV_INTERFACE_HEIGHT, Get16BPPColor(FROMRGB(16, 16, 16)));
		RestoreExternBackgroundRect(INTERFACE_START_X + INTERFACE_WIDTH, INV_INTERFACE_START_Y, INTERFACE_START_X, INV_INTERFACE_HEIGHT);

		// sevenfm: render left text panel
		if (gGameSettings.fOptions[TOPTION_TACTICAL_LEFT_PANEL])
		{
			PrepareTacticalLeftPanelData(2, 2);
			DrawTacticalLeftPanel();
		}

		// sevenfm: render right action panel
		if (gGameSettings.fOptions[TOPTION_TACTICAL_ACTION_PANEL])
		{
			RenderActionPanel();
		}		

		if ( InItemDescriptionBox( ) )
		{
			// CHRISL: Changed 3rd parameter so we can display graphic based on inventory system used
			BltVideoObjectFromIndex( guiSAVEBUFFER, guiSMPanel, UsingNewInventorySystem(), INTERFACE_START_X, INV_INTERFACE_START_Y, VO_BLT_SRCTRANSPARENCY, NULL );
			RenderSoldierFace( gpSMCurrentMerc, SM_SELMERC_FACE_X, SM_SELMERC_FACE_Y, TRUE );


			// ATE: Need these lines here to fix flash bug with face selection box
			if ( gfSMDisableForItems )
			{
				BltVideoObjectFromIndex( guiSAVEBUFFER, guiSMObjects2, 0, SM_SELMERC_PLATE_X, SM_SELMERC_PLATE_Y, VO_BLT_SRCTRANSPARENCY, NULL );
				RestoreExternBackgroundRect( SM_SELMERC_PLATE_X, SM_SELMERC_PLATE_Y, SM_SELMERC_PLATE_WIDTH , SM_SELMERC_PLATE_HEIGHT );
			}
			else
			{
				if ( gusSelectedSoldier == gpSMCurrentMerc->ubID && gTacticalStatus.ubCurrentTeam == OUR_TEAM && OK_INTERRUPT_MERC(	gpSMCurrentMerc ) )
				{
					BltVideoObjectFromIndex( guiSAVEBUFFER, guiSMObjects, 0, SM_SELMERC_PLATE_X, SM_SELMERC_PLATE_Y, VO_BLT_SRCTRANSPARENCY, NULL );
					RestoreExternBackgroundRect( SM_SELMERC_PLATE_X, SM_SELMERC_PLATE_Y, SM_SELMERC_PLATE_WIDTH , SM_SELMERC_PLATE_HEIGHT );
				}
			}

			// HEADROCK HAM 5: Test for active Transform Popup
			if (gfItemDescTransformPopupVisible)
			{
				gItemDescTransformPopup->show();
			}
			else
			{
				RenderItemDescriptionBox( );
			}
		}
		else
		{
			// CHRISL: Changed 3rd parameter so we can display graphic based on inventory system used
			BltVideoObjectFromIndex( guiSAVEBUFFER, guiSMPanel, UsingNewInventorySystem(), INTERFACE_START_X, INV_INTERFACE_START_Y, VO_BLT_SRCTRANSPARENCY, NULL );

			RenderInvBodyPanel( gpSMCurrentMerc, SM_BODYINV_X, SM_BODYINV_Y );

			// CHECK FOR PANEL STUFF / DEATHS / CLOSURES
			CheckForFacePanelStartAnims( gpSMCurrentMerc, SM_SELMERC_FACE_X, SM_SELMERC_FACE_Y );

			// Hitlight

			if ( gfSMDisableForItems )
			{
				BltVideoObjectFromIndex( guiSAVEBUFFER, guiSMObjects2, 0, SM_SELMERC_PLATE_X, SM_SELMERC_PLATE_Y, VO_BLT_SRCTRANSPARENCY, NULL );
				RestoreExternBackgroundRect( SM_SELMERC_PLATE_X, SM_SELMERC_PLATE_Y, SM_SELMERC_PLATE_WIDTH , SM_SELMERC_PLATE_HEIGHT );
			}
			else
			{
				if ( gusSelectedSoldier == gpSMCurrentMerc->ubID && gTacticalStatus.ubCurrentTeam == OUR_TEAM && OK_INTERRUPT_MERC(	gpSMCurrentMerc ) )
				{
					BltVideoObjectFromIndex( guiSAVEBUFFER, guiSMObjects, 0, SM_SELMERC_PLATE_X, SM_SELMERC_PLATE_Y, VO_BLT_SRCTRANSPARENCY, NULL );
					RestoreExternBackgroundRect( SM_SELMERC_PLATE_X, SM_SELMERC_PLATE_Y, SM_SELMERC_PLATE_WIDTH , SM_SELMERC_PLATE_HEIGHT );
				}
			}

			// HEADROCK HAM 3.6: "progress" bars showing how near the character is to "leveling up" in any stat. The bar
			// is displayed behind the current stat value, as see on the character's info panel.
			// This section draws TACTICAL info pages. Another section is in mapscreen.cpp and draws STRATEGIC info pages.
			// The feature is toggled by Options-Menu switch, and its color is determined in the INI files.
			if ( gGameSettings.fOptions[TOPTION_STAT_PROGRESS_BARS] )
			{
				UINT8	*pDestBuf;
				UINT32 uiDestPitchBYTES = 0;
				SGPRect ClipRect;
				UINT8 ubBarWidth;
				UINT16 usColor = Get16BPPColor( FROMRGB( gGameExternalOptions.ubStatProgressBarsRed, gGameExternalOptions.ubStatProgressBarsGreen, gGameExternalOptions.ubStatProgressBarsBlue ) );
				//pDestBuf = LockVideoSurface( FRAME_BUFFER, &uiDestPitchBYTES );
				pDestBuf = LockVideoSurface( guiSAVEBUFFER, &uiDestPitchBYTES );

				// AGI
				if (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sAgilityGain)
				{
					ubBarWidth = (SM_STATS_WIDTH * (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sAgilityGain+1)) / SubpointsPerPoint(AGILAMT,0);
					ubBarWidth = __max(0, __min(ubBarWidth, SM_STATS_WIDTH));
					ClipRect.iTop = (SM_AGI_Y-1);
					ClipRect.iBottom = (SM_AGI_Y-1) + SM_STATS_HEIGHT;
					ClipRect.iLeft = SM_AGI_X;
					ClipRect.iRight = SM_AGI_X + ubBarWidth;
					Blt16BPPBufferHatchRectWithColor( (UINT16*)pDestBuf, uiDestPitchBYTES, &ClipRect, usColor );
				}

				// DEX
				if (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sDexterityGain)
				{
					ubBarWidth = (SM_STATS_WIDTH * (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sDexterityGain+1)) / SubpointsPerPoint(DEXTAMT,0);
					ubBarWidth = __max(0, __min(ubBarWidth, SM_STATS_WIDTH));
					ClipRect.iTop = (SM_DEX_Y-1);
					ClipRect.iBottom = (SM_DEX_Y-1) + SM_STATS_HEIGHT;
					ClipRect.iLeft = SM_DEX_X;
					ClipRect.iRight = SM_DEX_X + ubBarWidth;
					Blt16BPPBufferHatchRectWithColor( (UINT16*)pDestBuf, uiDestPitchBYTES, &ClipRect, usColor );
				}

				// STR
				if (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sStrengthGain)
				{
					ubBarWidth = (SM_STATS_WIDTH * (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sStrengthGain+1)) / SubpointsPerPoint(STRAMT,0);
					ubBarWidth = __max(0, __min(ubBarWidth, SM_STATS_WIDTH));
					ClipRect.iTop = (SM_STR_Y-1);
					ClipRect.iBottom = (SM_STR_Y-1) + SM_STATS_HEIGHT;
					ClipRect.iLeft = SM_STR_X;
					ClipRect.iRight = SM_STR_X + ubBarWidth;
					Blt16BPPBufferHatchRectWithColor( (UINT16*)pDestBuf, uiDestPitchBYTES, &ClipRect, usColor );
				}

				// WIS
				if (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sWisdomGain)
				{
					ubBarWidth = (SM_STATS_WIDTH * (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sWisdomGain+1)) / SubpointsPerPoint(WISDOMAMT,0);
					ubBarWidth = __max(0, __min(ubBarWidth, SM_STATS_WIDTH));
					ClipRect.iTop = (SM_WIS_Y-1);
					ClipRect.iBottom = (SM_WIS_Y-1) + SM_STATS_HEIGHT;
					ClipRect.iLeft = SM_WIS_X;
					ClipRect.iRight = SM_WIS_X + ubBarWidth;
					Blt16BPPBufferHatchRectWithColor( (UINT16*)pDestBuf, uiDestPitchBYTES, &ClipRect, usColor );
				}

				// MRK
				if (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sMarksmanshipGain)
				{
					ubBarWidth = (SM_STATS_WIDTH * (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sMarksmanshipGain+1)) / SubpointsPerPoint(MARKAMT,0);
					ubBarWidth = __max(0, __min(ubBarWidth, SM_STATS_WIDTH));
					ClipRect.iTop = (SM_MRKM_Y-1);
					ClipRect.iBottom = (SM_MRKM_Y-1) + SM_STATS_HEIGHT;
					ClipRect.iLeft = SM_MRKM_X;
					ClipRect.iRight = SM_MRKM_X + ubBarWidth;
					Blt16BPPBufferHatchRectWithColor( (UINT16*)pDestBuf, uiDestPitchBYTES, &ClipRect, usColor );
				}

				// LDR
				if (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sLeadershipGain)
				{
					ubBarWidth = (SM_STATS_WIDTH * (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sLeadershipGain+1)) / SubpointsPerPoint(LDRAMT,0);
					ubBarWidth = __max(0, __min(ubBarWidth, SM_STATS_WIDTH));
					ClipRect.iTop = (SM_CHAR_Y-1);
					ClipRect.iBottom = (SM_CHAR_Y-1) + SM_STATS_HEIGHT;
					ClipRect.iLeft = SM_CHAR_X;
					ClipRect.iRight = SM_CHAR_X + ubBarWidth;
					Blt16BPPBufferHatchRectWithColor( (UINT16*)pDestBuf, uiDestPitchBYTES, &ClipRect, usColor );
				}

				// MECH
				if (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sMechanicGain)
				{
					ubBarWidth = (SM_STATS_WIDTH * (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sMechanicGain+1)) / SubpointsPerPoint(MECHANAMT,0);
					ubBarWidth = __max(0, __min(ubBarWidth, SM_STATS_WIDTH));
					ClipRect.iTop = (SM_MECH_Y-1);
					ClipRect.iBottom = (SM_MECH_Y-1) + SM_STATS_HEIGHT;
					ClipRect.iLeft = SM_MECH_X;
					ClipRect.iRight = SM_MECH_X + ubBarWidth;
					Blt16BPPBufferHatchRectWithColor( (UINT16*)pDestBuf, uiDestPitchBYTES, &ClipRect, usColor );
				}

				// EXPLO
				if (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sExplosivesGain)
				{
					ubBarWidth = (SM_STATS_WIDTH * (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sExplosivesGain+1)) / SubpointsPerPoint(EXPLODEAMT,0);
					ubBarWidth = __max(0, __min(ubBarWidth, SM_STATS_WIDTH));
					ClipRect.iTop = (SM_EXPL_Y-1);
					ClipRect.iBottom = (SM_EXPL_Y-1) + SM_STATS_HEIGHT;
					ClipRect.iLeft = SM_EXPL_X;
					ClipRect.iRight = SM_EXPL_X + ubBarWidth;
					Blt16BPPBufferHatchRectWithColor( (UINT16*)pDestBuf, uiDestPitchBYTES, &ClipRect, usColor );
				}

				// MED
				if (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sMedicalGain)
				{
					ubBarWidth = (SM_STATS_WIDTH * (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sMedicalGain+1)) / SubpointsPerPoint(MEDICALAMT,0);
					ubBarWidth = __max(0, __min(ubBarWidth, SM_STATS_WIDTH));
					ClipRect.iTop = (SM_MED_Y-1);
					ClipRect.iBottom = (SM_MED_Y-1) + SM_STATS_HEIGHT;
					ClipRect.iLeft = SM_MED_X;
					ClipRect.iRight = SM_MED_X + ubBarWidth;
					Blt16BPPBufferHatchRectWithColor( (UINT16*)pDestBuf, uiDestPitchBYTES, &ClipRect, usColor );
				}

				// EXPLEVEL
				if (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sExpLevelGain)
				{
					ubBarWidth = (SM_STATS_WIDTH * (gMercProfiles[ gpSMCurrentMerc->ubProfile ].sExpLevelGain+1)) / SubpointsPerPoint(EXPERAMT, gpSMCurrentMerc->stats.bExpLevel);
					ubBarWidth = __max(0, __min(ubBarWidth, SM_STATS_WIDTH));
					ClipRect.iTop = (SM_EXPLVL_Y-1);
					ClipRect.iBottom = (SM_EXPLVL_Y-1) + SM_STATS_HEIGHT;
					ClipRect.iLeft = SM_EXPLVL_X;
					ClipRect.iRight = SM_EXPLVL_X + ubBarWidth;
					Blt16BPPBufferHatchRectWithColor( (UINT16*)pDestBuf, uiDestPitchBYTES, &ClipRect, usColor );
				}

				UnLockVideoSurface( guiSAVEBUFFER );
			}

			// Render faceplate
			//BltVideoObjectFromIndex( guiSAVEBUFFER, guiSMObjects2, 1, SM_SELMERC_NAMEPLATE_X, SM_SELMERC_NAMEPLATE_Y, VO_BLT_SRCTRANSPARENCY, NULL );
			//RestoreExternBackgroundRect( SM_SELMERC_NAMEPLATE_X, SM_SELMERC_NAMEPLATE_Y, SM_SELMERC_NAMEPLATE_WIDTH, SM_SELMERC_NAMEPLATE_HEIGHT );

			// Blit position
			//if(gbPixelDepth==16)
			//{
				//BltVideoObjectFromIndex( guiSAVEBUFFER, guiSMObjects, gbSMCurStanceObj, SM_POSITIONB_X, SM_POSITIONB_Y, VO_BLT_SRCTRANSPARENCY, NULL );
			//}
			//RestoreExternBackgroundRect( SM_POSITIONB_X, SM_POSITIONB_Y, SM_POSITIONB_WIDTH , SM_POSITIONB_HEIGHT );

			SetFont( BLOCKFONT2 );

			// Render Values for stats!
			// Set font drawing to saved buffer
			SetFontDestBuffer( guiSAVEBUFFER, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, FALSE );

			SetFontBackground( FONT_MCOLOR_BLACK );
			SetFontForeground( STATS_TITLE_FONT_COLOR );
			for( cnt = 0; cnt < 5; cnt++ )
			{
				mprintf( INTERFACE_START_X + 92, ( INT16 )( INV_INTERFACE_START_Y + 7 + cnt * 10 ), pShortAttributeStrings[ cnt	] );
				mprintf( INTERFACE_START_X + 137, ( INT16 )( INV_INTERFACE_START_Y + 7 + cnt * 10 ), pShortAttributeStrings[ cnt + 5 ] );
			}

			mprintf( SM_ARMOR_LABEL_X - StringPixLength( pInvPanelTitleStrings[0], BLOCKFONT2 ) / 2, SM_ARMOR_LABEL_Y, pInvPanelTitleStrings[ 0 ] );

			#ifdef CHINESE
				mprintf( SM_ARMOR_PERCENT_X, SM_ARMOR_PERCENT_Y, ChineseSpecString1 );
			#else
				mprintf( SM_ARMOR_PERCENT_X, SM_ARMOR_PERCENT_Y, L"%%" );
			#endif

			mprintf( SM_WEIGHT_LABEL_X - StringPixLength( pInvPanelTitleStrings[1], BLOCKFONT2 ), SM_WEIGHT_LABEL_Y, pInvPanelTitleStrings[ 1 ] );

			#ifdef CHINESE
				mprintf( SM_WEIGHT_PERCENT_X, SM_WEIGHT_PERCENT_Y, ChineseSpecString1 );
			#else
				mprintf( SM_WEIGHT_PERCENT_X, SM_WEIGHT_PERCENT_Y, L"%%" );
			#endif
			
			mprintf( SM_CAMMO_LABEL_X - StringPixLength( pInvPanelTitleStrings[2], BLOCKFONT2 ), SM_CAMMO_LABEL_Y, pInvPanelTitleStrings[ 2 ] );

			#ifdef CHINESE
				mprintf( SM_CAMMO_PERCENT_X, SM_CAMMO_PERCENT_Y, ChineseSpecString1 );
			#else
				mprintf( SM_CAMMO_PERCENT_X, SM_CAMMO_PERCENT_Y, L"%%" );
			#endif

			/*UpdateStatColor( gpSMCurrentMerc->timeChanges.uiChangeAgilityTime, ( BOOLEAN ) ( gpSMCurrentMerc->usValueGoneUp & AGIL_INCREASE? TRUE: FALSE ), ( BOOLEAN ) ( ( gGameOptions.fNewTraitSystem && ( gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_AGILITY] > 0 )) ? TRUE : FALSE), MercUnderTheInfluence(gpSMCurrentMerc, DRUG_TYPE_AGILITY)); // SANDRO
			swprintf( sString, L"%2d", min(100, gpSMCurrentMerc->stats.bAgility + gpSMCurrentMerc->bExtraAgility) );*/

			// Agility
			UpdateStatColor(gpSMCurrentMerc->timeChanges.uiChangeAgilityTime, gpSMCurrentMerc->usValueGoneUp & AGIL_INCREASE, 
				gGameOptions.fNewTraitSystem && gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_AGILITY], 
				gpSMCurrentMerc->bExtraAgility != 0 || GetDrunkLevel(gpSMCurrentMerc) != SOBER, FALSE, FALSE, FALSE, gpSMCurrentMerc->sWeightCarriedAtTurnStart > 100);
			swprintf(sString, L"%2d", min(100, EffectiveAgility(gpSMCurrentMerc, FALSE)));
			FindFontRightCoordinates(SM_AGI_X, SM_AGI_Y, SM_STATS_WIDTH, SM_STATS_HEIGHT, sString, BLOCKFONT2, &usX, &usY);
			mprintf(usX, usY, sString);
			
			/*UpdateStatColor( gpSMCurrentMerc->timeChanges.uiChangeDexterityTime,( BOOLEAN ) ( gpSMCurrentMerc->usValueGoneUp & DEX_INCREASE? TRUE: FALSE ), ( BOOLEAN ) ( ( gGameOptions.fNewTraitSystem && ( gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_DEXTERITY] > 0 )) ? TRUE : FALSE), MercUnderTheInfluence(gpSMCurrentMerc, DRUG_TYPE_DEXTERITY)); // SANDRO
			swprintf( sString, L"%2d", min(100, gpSMCurrentMerc->stats.bDexterity + gpSMCurrentMerc->bExtraDexterity) );*/

			// Dexterity
			UpdateStatColor(gpSMCurrentMerc->timeChanges.uiChangeDexterityTime, gpSMCurrentMerc->usValueGoneUp & DEX_INCREASE, 
				gGameOptions.fNewTraitSystem && gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_DEXTERITY] > 0, 
				gpSMCurrentMerc->bExtraDexterity != 0 || GetDrunkLevel(gpSMCurrentMerc) != SOBER, gpSMCurrentMerc->InShock(), gpSMCurrentMerc->IsInspired(), FALSE, FALSE);
			swprintf(sString, L"%2d", min(100, EffectiveDexterity(gpSMCurrentMerc, FALSE)));
			FindFontRightCoordinates(SM_DEX_X, SM_DEX_Y ,SM_STATS_WIDTH ,SM_STATS_HEIGHT ,sString, BLOCKFONT2, &usX, &usY);
			mprintf( usX, usY , sString );

			/*UpdateStatColor( gpSMCurrentMerc->timeChanges.uiChangeStrengthTime, ( BOOLEAN )( gpSMCurrentMerc->usValueGoneUp & STRENGTH_INCREASE?TRUE: FALSE ), ( BOOLEAN ) ( (( gGameOptions.fNewTraitSystem && ( gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_STRENGTH] > 0 )) || (gGameOptions.fFoodSystem && gpSMCurrentMerc->usStarveDamageStrength > 0) ) ? TRUE : FALSE), MercUnderTheInfluence(gpSMCurrentMerc, DRUG_TYPE_STRENGTH)); // SANDRO
			swprintf( sString, L"%2d", min(100, gpSMCurrentMerc->stats.bStrength + gpSMCurrentMerc->bExtraStrength) );*/

			// Strength
			UpdateStatColor(gpSMCurrentMerc->timeChanges.uiChangeStrengthTime, gpSMCurrentMerc->usValueGoneUp & STRENGTH_INCREASE, 
				gGameOptions.fNewTraitSystem && gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_STRENGTH] > 0 || gGameOptions.fFoodSystem && gpSMCurrentMerc->usStarveDamageStrength > 0, 
				gpSMCurrentMerc->bExtraStrength !=0, FALSE, FALSE, FALSE, FALSE);
			swprintf(sString, L"%2d", min(100, EffectiveStrength(gpSMCurrentMerc, FALSE)));
			FindFontRightCoordinates(SM_STR_X, SM_STR_Y ,SM_STATS_WIDTH ,SM_STATS_HEIGHT ,sString, BLOCKFONT2, &usX, &usY);
			mprintf( usX, usY , sString );

			/*UpdateStatColor( gpSMCurrentMerc->timeChanges.uiChangeLeadershipTime, ( BOOLEAN )( gpSMCurrentMerc->usValueGoneUp & LDR_INCREASE? TRUE: FALSE ), ( BOOLEAN ) ( ( gGameOptions.fNewTraitSystem && ( gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_LEADERSHIP] > 0 )) ? TRUE : FALSE), FALSE); // SANDRO
			swprintf( sString, L"%2d", gpSMCurrentMerc->stats.bLeadership );*/

			// Leadership
			UpdateStatColor(gpSMCurrentMerc->timeChanges.uiChangeLeadershipTime, gpSMCurrentMerc->usValueGoneUp & LDR_INCREASE, 
				gGameOptions.fNewTraitSystem && gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_LEADERSHIP] > 0,
				GetDrunkLevel(gpSMCurrentMerc) != SOBER, FALSE, FALSE, FALSE, FALSE);
			swprintf(sString, L"%2d", EffectiveLeadership(gpSMCurrentMerc));
			FindFontRightCoordinates(SM_CHAR_X, SM_CHAR_Y ,SM_STATS_WIDTH ,SM_STATS_HEIGHT ,sString, BLOCKFONT2, &usX, &usY);
			mprintf( usX, usY , sString );

			/*UpdateStatColor( gpSMCurrentMerc->timeChanges.uiChangeWisdomTime,( BOOLEAN ) ( gpSMCurrentMerc->usValueGoneUp & WIS_INCREASE? TRUE: FALSE ), ( BOOLEAN ) ( ( gGameOptions.fNewTraitSystem && ( gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_WISDOM] > 0 )) ? TRUE : FALSE), MercUnderTheInfluence(gpSMCurrentMerc, DRUG_TYPE_WISDOM)); // SANDRO
			swprintf( sString, L"%2d", min(100, gpSMCurrentMerc->stats.bWisdom + gpSMCurrentMerc->bExtraWisdom ));*/

			// Wisdom
			UpdateStatColor(gpSMCurrentMerc->timeChanges.uiChangeWisdomTime, gpSMCurrentMerc->usValueGoneUp & WIS_INCREASE, 
				gGameOptions.fNewTraitSystem && gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_WISDOM], 
				gpSMCurrentMerc->bExtraWisdom != 0 || GetDrunkLevel(gpSMCurrentMerc) != SOBER, FALSE, gpSMCurrentMerc->IsInspired(), FALSE, FALSE);
			swprintf(sString, L"%2d", min(100, EffectiveWisdom(gpSMCurrentMerc)));
			FindFontRightCoordinates(SM_WIS_X, SM_WIS_Y ,SM_STATS_WIDTH ,SM_STATS_HEIGHT ,sString, BLOCKFONT2, &usX, &usY);
			mprintf( usX, usY , sString );

			/*UpdateStatColor( gpSMCurrentMerc->timeChanges.uiChangeLevelTime, ( BOOLEAN ) ( gpSMCurrentMerc->usValueGoneUp & LVL_INCREASE? TRUE: FALSE ),  FALSE , FALSE);
			swprintf( sString, L"%2d", min(10, gpSMCurrentMerc->stats.bExpLevel + gpSMCurrentMerc->bExtraExpLevel) );*/

			// Experience level
			UpdateStatColor(gpSMCurrentMerc->timeChanges.uiChangeLevelTime, gpSMCurrentMerc->usValueGoneUp & LVL_INCREASE, 
				FALSE, 
				gpSMCurrentMerc->bExtraExpLevel != 0 || GetDrunkLevel(gpSMCurrentMerc) != SOBER, FALSE, gpSMCurrentMerc->IsInspired(), gGameOptions.fNewTraitSystem && IS_MERC_BODY_TYPE(gpSMCurrentMerc) && GetSquadleadersCountInVicinity(gpSMCurrentMerc, TRUE, FALSE) > 0, FALSE);
			swprintf(sString, L"%2d", min(10, EffectiveExpLevel(gpSMCurrentMerc, TRUE)));
			FindFontRightCoordinates(SM_EXPLVL_X, SM_EXPLVL_Y ,SM_STATS_WIDTH ,SM_STATS_HEIGHT ,sString, BLOCKFONT2, &usX, &usY);
			mprintf( usX, usY , sString );

			/*UpdateStatColor( gpSMCurrentMerc->timeChanges.uiChangeMarksmanshipTime, ( BOOLEAN ) ( gpSMCurrentMerc->usValueGoneUp & MRK_INCREASE? TRUE: FALSE ), ( BOOLEAN ) ( ( gGameOptions.fNewTraitSystem && ( gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_MARKSMANSHIP] > 0 )) ? TRUE : FALSE), FALSE); // SANDRO
			swprintf( sString, L"%2d", gpSMCurrentMerc->stats.bMarksmanship );*/

			// Marksmanship
			UpdateStatColor(gpSMCurrentMerc->timeChanges.uiChangeMarksmanshipTime, gpSMCurrentMerc->usValueGoneUp & MRK_INCREASE, 
				gGameOptions.fNewTraitSystem && gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_MARKSMANSHIP] > 0, 
				GetDrunkLevel(gpSMCurrentMerc) != SOBER || MercUnderTheInfluence(gpSMCurrentMerc, DRUG_TYPE_ADRENALINE), gpSMCurrentMerc->InShock(), gpSMCurrentMerc->IsInspired(), FALSE, FALSE);
			swprintf(sString, L"%2d", EffectiveMarksmanship(gpSMCurrentMerc));
			FindFontRightCoordinates(SM_MRKM_X, SM_MRKM_Y ,SM_STATS_WIDTH ,SM_STATS_HEIGHT ,sString, BLOCKFONT2, &usX, &usY);
			mprintf( usX, usY , sString );

			/*UpdateStatColor( gpSMCurrentMerc->timeChanges.uiChangeMechanicalTime, ( BOOLEAN ) ( gpSMCurrentMerc->usValueGoneUp & MECH_INCREASE ? TRUE: FALSE ), ( BOOLEAN ) ( ( gGameOptions.fNewTraitSystem && ( gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_MECHANICAL] > 0 )) ? TRUE : FALSE), FALSE); // SANDRO
			swprintf( sString, L"%2d", gpSMCurrentMerc->stats.bMechanical );*/

			// Mechanical
			UpdateStatColor(gpSMCurrentMerc->timeChanges.uiChangeMechanicalTime, gpSMCurrentMerc->usValueGoneUp & MECH_INCREASE, 
				gGameOptions.fNewTraitSystem && gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_MECHANICAL] > 0, 
				GetDrunkLevel(gpSMCurrentMerc) != SOBER, FALSE, FALSE, FALSE, FALSE);
			swprintf(sString, L"%2d", EffectiveMechanical(gpSMCurrentMerc));
			FindFontRightCoordinates(SM_MECH_X, SM_MECH_Y ,SM_STATS_WIDTH ,SM_STATS_HEIGHT ,sString, BLOCKFONT2, &usX, &usY);
			mprintf( usX, usY , sString );

			/*UpdateStatColor( gpSMCurrentMerc->timeChanges.uiChangeExplosivesTime, ( BOOLEAN ) ( gpSMCurrentMerc->usValueGoneUp & EXP_INCREASE? TRUE: FALSE ), ( BOOLEAN ) ( ( gGameOptions.fNewTraitSystem && ( gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_EXPLOSIVES] > 0 )) ? TRUE : FALSE), FALSE); // SANDRO
			swprintf( sString, L"%2d", gpSMCurrentMerc->stats.bExplosive );*/

			// Explosives
			UpdateStatColor(gpSMCurrentMerc->timeChanges.uiChangeExplosivesTime, gpSMCurrentMerc->usValueGoneUp & EXP_INCREASE, 
				gGameOptions.fNewTraitSystem && gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_EXPLOSIVES], 
				GetDrunkLevel(gpSMCurrentMerc) != SOBER, FALSE, FALSE, FALSE, FALSE);
			swprintf(sString, L"%2d", EffectiveExplosive(gpSMCurrentMerc));
			FindFontRightCoordinates(SM_EXPL_X, SM_EXPL_Y ,SM_STATS_WIDTH ,SM_STATS_HEIGHT ,sString, BLOCKFONT2, &usX, &usY);
			mprintf( usX, usY , sString );

			/*UpdateStatColor( gpSMCurrentMerc->timeChanges.uiChangeMedicalTime, ( BOOLEAN ) ( gpSMCurrentMerc->usValueGoneUp & MED_INCREASE? TRUE: FALSE ), ( BOOLEAN ) ( ( gGameOptions.fNewTraitSystem && ( gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_MEDICAL] > 0 )) ? TRUE : FALSE), FALSE); // SANDRO
			swprintf( sString, L"%2d", gpSMCurrentMerc->stats.bMedical );*/

			// Medical
			UpdateStatColor(gpSMCurrentMerc->timeChanges.uiChangeMedicalTime, gpSMCurrentMerc->usValueGoneUp & MED_INCREASE, 
				gGameOptions.fNewTraitSystem && gpSMCurrentMerc->ubCriticalStatDamage[DAMAGED_STAT_MEDICAL] > 0, 
				GetDrunkLevel(gpSMCurrentMerc) != SOBER, FALSE, FALSE, FALSE, FALSE);
			swprintf(sString, L"%2d", EffectiveMedical(gpSMCurrentMerc));
			FindFontRightCoordinates(SM_MED_X, SM_MED_Y ,SM_STATS_WIDTH ,SM_STATS_HEIGHT ,sString, BLOCKFONT2, &usX, &usY);
			mprintf( usX, usY , sString );

			if ( gpSMCurrentMerc->stats.bLife >= OKLIFE )
			{
				SetFontBackground( FONT_MCOLOR_BLACK );
				SetFontForeground( STATS_TEXT_FONT_COLOR );
			}
			else
			{
				SetFontBackground( FONT_MCOLOR_BLACK );
				SetFontForeground( FONT_MCOLOR_DKGRAY );
			}

			// Display armour value!
			swprintf( sString, L"%3d", ArmourPercent( gpSMCurrentMerc ) );
			FindFontRightCoordinates(SM_ARMOR_X, SM_ARMOR_Y ,SM_PERCENT_WIDTH ,SM_PERCENT_HEIGHT ,sString, BLOCKFONT2, &usX, &usY);
			mprintf( usX, usY , sString );

			// Display weight value!
			swprintf( sString, L"%3d", CalculateCarriedWeight( gpSMCurrentMerc ) );
			FindFontRightCoordinates(SM_WEIGHT_X, SM_WEIGHT_Y ,SM_PERCENT_WIDTH ,SM_PERCENT_HEIGHT ,sString, BLOCKFONT2, &usX, &usY);
			mprintf( usX, usY , sString );

			ApplyEquipmentBonuses(gpSMCurrentMerc);

			// Display camo value!
			// sevenfm: show max value instead of sum
			//swprintf( sString, L"%3d", max(0, min( (gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo+gpSMCurrentMerc->urbanCamo+gpSMCurrentMerc->wornUrbanCamo+gpSMCurrentMerc->desertCamo+gpSMCurrentMerc->wornDesertCamo+gpSMCurrentMerc->snowCamo+gpSMCurrentMerc->wornSnowCamo),100 )) );
			INT8 bMaxCamo = max(0, min((max(gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo, max(gpSMCurrentMerc->urbanCamo + gpSMCurrentMerc->wornUrbanCamo, max(gpSMCurrentMerc->desertCamo + gpSMCurrentMerc->wornDesertCamo, gpSMCurrentMerc->snowCamo + gpSMCurrentMerc->wornSnowCamo)))), 100));
			swprintf(sString, L"%3d", bMaxCamo);

			// sevenfm: choose camo color
			if (bMaxCamo == 0)
				;
			else if (gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo >= gpSMCurrentMerc->urbanCamo + gpSMCurrentMerc->wornUrbanCamo &&
				gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo >= gpSMCurrentMerc->desertCamo + gpSMCurrentMerc->wornDesertCamo &&
				gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo >= gpSMCurrentMerc->snowCamo + gpSMCurrentMerc->wornSnowCamo &&
				gpSMCurrentMerc->desertCamo + gpSMCurrentMerc->wornDesertCamo >= gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo / 2)
				SetFontForeground(FONT_KHAKI);
			else if (gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo >= gpSMCurrentMerc->urbanCamo + gpSMCurrentMerc->wornUrbanCamo &&
				gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo >= gpSMCurrentMerc->desertCamo + gpSMCurrentMerc->wornDesertCamo &&
				gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo >= gpSMCurrentMerc->snowCamo + gpSMCurrentMerc->wornSnowCamo)
				SetFontForeground(FONT_GREEN);
			else if (gpSMCurrentMerc->desertCamo + gpSMCurrentMerc->wornDesertCamo >= gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo &&
				gpSMCurrentMerc->desertCamo + gpSMCurrentMerc->wornDesertCamo >= gpSMCurrentMerc->urbanCamo + gpSMCurrentMerc->wornUrbanCamo &&
				gpSMCurrentMerc->desertCamo + gpSMCurrentMerc->wornDesertCamo >= gpSMCurrentMerc->snowCamo + gpSMCurrentMerc->wornSnowCamo &&
				gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo >= gpSMCurrentMerc->desertCamo + gpSMCurrentMerc->wornDesertCamo / 2)
				SetFontForeground(FONT_KHAKI);
			else if (gpSMCurrentMerc->desertCamo + gpSMCurrentMerc->wornDesertCamo >= gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo &&
				gpSMCurrentMerc->desertCamo + gpSMCurrentMerc->wornDesertCamo >= gpSMCurrentMerc->urbanCamo + gpSMCurrentMerc->wornUrbanCamo &&
				gpSMCurrentMerc->desertCamo + gpSMCurrentMerc->wornDesertCamo >= gpSMCurrentMerc->snowCamo + gpSMCurrentMerc->wornSnowCamo)
				SetFontForeground(FONT_DKYELLOW);
			else if (gpSMCurrentMerc->urbanCamo + gpSMCurrentMerc->wornUrbanCamo >= gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo &&
				gpSMCurrentMerc->urbanCamo + gpSMCurrentMerc->wornUrbanCamo >= gpSMCurrentMerc->desertCamo + gpSMCurrentMerc->wornDesertCamo &&
				gpSMCurrentMerc->urbanCamo + gpSMCurrentMerc->wornUrbanCamo >= gpSMCurrentMerc->snowCamo + gpSMCurrentMerc->wornSnowCamo)
				SetFontForeground(FONT_MCOLOR_DKGRAY);
			else if (gpSMCurrentMerc->snowCamo + gpSMCurrentMerc->wornSnowCamo >= gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo &&
				gpSMCurrentMerc->snowCamo + gpSMCurrentMerc->wornSnowCamo >= gpSMCurrentMerc->desertCamo + gpSMCurrentMerc->wornDesertCamo &&
				gpSMCurrentMerc->snowCamo + gpSMCurrentMerc->wornSnowCamo >= gpSMCurrentMerc->urbanCamo + gpSMCurrentMerc->wornUrbanCamo)
				SetFontForeground(FONT_MCOLOR_WHITE);

			FindFontRightCoordinates(SM_CAMMO_X, SM_CAMMO_Y ,SM_PERCENT_WIDTH ,SM_PERCENT_HEIGHT ,sString, BLOCKFONT2, &usX, &usY);
			mprintf( usX, usY , sString );

			// restore color
			if (gpSMCurrentMerc->stats.bLife >= OKLIFE)
			{
				SetFontBackground(FONT_MCOLOR_BLACK);
				SetFontForeground(STATS_TEXT_FONT_COLOR);
			}
			else
			{
				SetFontBackground(FONT_MCOLOR_BLACK);
				SetFontForeground(FONT_MCOLOR_DKGRAY);
			}

			// reset to frame buffer!
			SetFontDestBuffer( FRAME_BUFFER, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, FALSE );
			RestoreExternBackgroundRect( INTERFACE_START_X, INV_INTERFACE_START_Y, SCREEN_WIDTH - INTERFACE_START_X , INV_INTERFACE_HEIGHT );
			RenderSoldierFace( gpSMCurrentMerc, SM_SELMERC_FACE_X, SM_SELMERC_FACE_Y, TRUE );
		}

		// Render Name!
		SetFont( BLOCKFONT2 );

		if ( gpSMCurrentMerc->bStealthMode )
		{
			SetFontBackground( FONT_MCOLOR_BLACK );
			SetFontForeground( FONT_MCOLOR_LTYELLOW );
		}
		else
		{
			SetFontBackground( FONT_MCOLOR_BLACK );
			SetFontForeground( FONT_MCOLOR_LTGRAY );
		}

		RestoreExternBackgroundRect( SM_SELMERCNAME_X, SM_SELMERCNAME_Y, SM_SELMERCNAME_WIDTH, SM_SELMERCNAME_HEIGHT );
		VarFindFontCenterCoordinates( SM_SELMERCNAME_X, SM_SELMERCNAME_Y, SM_SELMERCNAME_WIDTH, SM_SELMERCNAME_HEIGHT, SMALLFONT1, &sFontX, &sFontY, L"%s", gpSMCurrentMerc->name );
		mprintf( sFontX + 5, sFontY, L"%s", gpSMCurrentMerc->name );

	}

	if ( *pfDirty != DIRTYLEVEL0 )
	{
		///////////////////////////////////////////////////////////////////////
		// SANDRO - added a tooltip window showing exact camo percentage for every type
		// *******************************************************************
		CHAR16 pStrCamo[1024];
		swprintf( pStrCamo, L"" );
		swprintf( pStr, L"");
		if ((gpSMCurrentMerc->bCamo + gpSMCurrentMerc->wornCamo) > 0 )
		{
			swprintf( pStrCamo, L"\n%d/%d%s %s", gpSMCurrentMerc->bCamo, gpSMCurrentMerc->wornCamo, L"%", gzMiscItemStatsFasthelp[ 21 ]);
			wcscat( pStr, pStrCamo);
			swprintf( pStrCamo, L"" );
		}
		if ((gpSMCurrentMerc->urbanCamo + gpSMCurrentMerc->wornUrbanCamo) > 0 )
		{
			swprintf( pStrCamo, L"\n%d/%d%s %s", gpSMCurrentMerc->urbanCamo, gpSMCurrentMerc->wornUrbanCamo, L"%", gzMiscItemStatsFasthelp[ 22 ]);
			wcscat( pStr, pStrCamo);
			swprintf( pStrCamo, L"" );
		}
		if ((gpSMCurrentMerc->desertCamo + gpSMCurrentMerc->wornDesertCamo) > 0 )
		{
			swprintf( pStrCamo, L"\n%d/%d%s %s", gpSMCurrentMerc->desertCamo, gpSMCurrentMerc->wornDesertCamo, L"%", gzMiscItemStatsFasthelp[ 23 ]);
			wcscat( pStr, pStrCamo);
			swprintf( pStrCamo, L"" );
		}
		if ((gpSMCurrentMerc->snowCamo + gpSMCurrentMerc->wornSnowCamo) > 0 )
		{
			swprintf( pStrCamo, L"\n%d/%d%s %s", gpSMCurrentMerc->snowCamo, gpSMCurrentMerc->wornSnowCamo, L"%", gzMiscItemStatsFasthelp[ 24 ] );
			wcscat( pStr, pStrCamo);
			swprintf( pStrCamo, L"" );
		}
		
		// anv: display stealth together with camo
		INT16 wornstealth = GetWornStealth(gpSMCurrentMerc);
		//INT16 bonusstealth = gpSMCurrentMerc->GetBackgroundValue(BG_PERC_STEALTH);
		INT16 bonusstealth = GetStealth(gpSMCurrentMerc) - GetWornStealth(gpSMCurrentMerc);
		/*if ( gpSMCurrentMerc->ubBodyType == BLOODCAT )
		{
			bonusstealth += 50;
		}
		// SANDRO - new/old traits
		else if ( gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( gpSMCurrentMerc, STEALTHY_NT ))
		{
			bonusstealth += gSkillTraitValues.ubSTBonusToMoveQuietly;
		}
		else if ( !gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( gpSMCurrentMerc, STEALTHY_OT ))
		{
			bonusstealth += 25 * NUM_SKILL_TRAITS( gpSMCurrentMerc, STEALTHY_OT );
		}*/

		if ( bonusstealth != 0 || wornstealth!= 0 )
		{
			CHAR16 pStrBonusStealth[1024];
			CHAR16 pStrWornStealth[1024];
			if( bonusstealth < 0 )
				swprintf( pStrBonusStealth, L"%d", bonusstealth );
			else
				swprintf( pStrBonusStealth, L"+%d", bonusstealth );
			if( wornstealth < 0 )
				swprintf( pStrWornStealth, L"%d", wornstealth );
			else
				swprintf( pStrWornStealth, L"+%d", wornstealth );
			swprintf( pStrCamo, L"\n%s/%s %s", pStrBonusStealth, pStrWornStealth, gzMiscItemStatsFasthelp[ 25 ] );
			wcscat( pStr, pStrCamo);
			swprintf( pStrCamo, L"" );
		}

		SetRegionFastHelpText( &(gSM_SELMERCCamoRegion), pStr );
		SetRegionHelpEndCallback( &gSM_SELMERCCamoRegion, SkiHelpTextDoneCallBack );

		// Flugente: weight help text
		FLOAT totalweight = (FLOAT)GetTotalWeight(gpSMCurrentMerc);
		swprintf(pStr, gzMiscItemStatsFasthelp[35], totalweight / 10.0);

		SetRegionFastHelpText(&(gSM_SELMERCWeightRegion), pStr);

		/////////////////////////////////////////////////////////////////////////////////////////

		// UPdate stats!
		if ( gpSMCurrentMerc->stats.bLife != 0 )
		{
			if ( gpSMCurrentMerc->flags.uiStatusFlags & SOLDIER_VEHICLE )
			{
				swprintf( pStr, TacticalStr[ VEHICLE_VITAL_STATS_POPUPTEXT ], gpSMCurrentMerc->stats.bLife, gpSMCurrentMerc->stats.bLifeMax, gpSMCurrentMerc->bBreath, gpSMCurrentMerc->bBreathMax );
				SetRegionFastHelpText( &(gSM_SELMERCBarsRegion), pStr );
			}
			else if ( gpSMCurrentMerc->flags.uiStatusFlags & SOLDIER_ROBOT )
			{
				swprintf( pStr, gzLateLocalizedString[ 16 ], gpSMCurrentMerc->stats.bLife, gpSMCurrentMerc->stats.bLifeMax );
				SetRegionFastHelpText( &(gTEAM_BarsRegions[ cnt ]), pStr );
			}
			else
			{
				GetMoraleString( gpSMCurrentMerc, pMoraleStr );
				// Flugente: food info if food system is active
				if ( gGameOptions.fFoodSystem && gpSMCurrentMerc->ubProfile != ROBOT && !IsVehicle(gpSMCurrentMerc) )
				{
					// Flugente: added a display for poison, only show text if actually poisoned
					if ( gpSMCurrentMerc->bPoisonSum > 0 )
					{
						swprintf( pStr, TacticalStr[ MERC_VITAL_STATS_WITH_POISON_AND_FOOD_POPUPTEXT ], gpSMCurrentMerc->stats.bLife, gpSMCurrentMerc->stats.bLifeMax, gpSMCurrentMerc->bPoisonSum, gpSMCurrentMerc->stats.bLifeMax, gpSMCurrentMerc->bBreath, gpSMCurrentMerc->bBreathMax, pMoraleStr, (INT32)(100*(gpSMCurrentMerc->bDrinkLevel - FOOD_MIN) / FOOD_HALF_RANGE), L"%", (INT32)(100*(gpSMCurrentMerc->bFoodLevel - FOOD_MIN) / FOOD_HALF_RANGE), L"%" );
					}
					else
					{
						swprintf( pStr, TacticalStr[ MERC_VITAL_STATS_WITH_FOOD_POPUPTEXT ], gpSMCurrentMerc->stats.bLife, gpSMCurrentMerc->stats.bLifeMax, gpSMCurrentMerc->bBreath, gpSMCurrentMerc->bBreathMax, pMoraleStr, (INT32)(100*(gpSMCurrentMerc->bDrinkLevel - FOOD_MIN) / FOOD_HALF_RANGE), L"%", (INT32)(100*(gpSMCurrentMerc->bFoodLevel - FOOD_MIN) / FOOD_HALF_RANGE), L"%" );
					}
				}
				else
				{
					// Flugente: added a display for poison, only show text if actually poisoned
					if ( gpSMCurrentMerc->bPoisonSum > 0 )
					{
						swprintf( pStr, TacticalStr[ MERC_VITAL_STATS_WITH_POISON_POPUPTEXT ], gpSMCurrentMerc->stats.bLife, gpSMCurrentMerc->stats.bLifeMax, gpSMCurrentMerc->bPoisonSum, gpSMCurrentMerc->stats.bLifeMax, gpSMCurrentMerc->bBreath, gpSMCurrentMerc->bBreathMax, pMoraleStr );
					}
					else
					{
						swprintf( pStr, TacticalStr[ MERC_VITAL_STATS_POPUPTEXT ], gpSMCurrentMerc->stats.bLife, gpSMCurrentMerc->stats.bLifeMax, gpSMCurrentMerc->bBreath, gpSMCurrentMerc->bBreathMax, pMoraleStr );
					}
				}

				// sevenfm: also print shock
				if (gpSMCurrentMerc->ShockLevelPercent() > 0)
				{
					swprintf(pStr, L"%s\n%s: %d%%", pStr, TacticalStr[MERC_VITAL_STATS_SHOCK], gpSMCurrentMerc->ShockLevelPercent());
				}

				SetRegionFastHelpText( &(gSM_SELMERCBarsRegion), pStr );

				// Buggler: skills/traits tooltip on merc portrait
				if (gGameSettings.fOptions[TOPTION_SHOW_SKILLS_ON_PORTRAIT])
				{
					// clear pStr value
					swprintf( pStr, L"");

					if (gGameOptions.fNewTraitSystem) // SANDRO - old/new traits check
					{
						UINT8 ubTempSkillArray[30];
						INT8 bNumSkillTraits = 0;

						// lets rearrange our skills to a temp array
						// we also get the number of lines (skills) to be displayed 
						for ( UINT8 ubCnt = 1; ubCnt < NUM_SKILLTRAITS_NT; ubCnt++ )
						{
							if ( ProfileHasSkillTrait( gpSMCurrentMerc->ubProfile, ubCnt ) == 2 )
							{
								ubTempSkillArray[bNumSkillTraits] = (ubCnt + NEWTRAIT_MERCSKILL_EXPERTOFFSET);
								bNumSkillTraits++;
							}
							else if ( ProfileHasSkillTrait( gpSMCurrentMerc->ubProfile, ubCnt ) == 1 )
							{
								ubTempSkillArray[bNumSkillTraits] = ubCnt;
								bNumSkillTraits++;
							}
						}

						if ( bNumSkillTraits == 0 )
						{
							swprintf( pStr, L"%s", pPersonnelScreenStrings[ PRSNL_TXT_NOSKILLS ] );
						}
						else
						{
							for ( UINT8 ubCnt = 0; ubCnt < bNumSkillTraits; ubCnt++ )
							{
								swprintf( sTemp, L"%s\n", gzMercSkillTextNew[ ubTempSkillArray[ubCnt] ] );
								wcscat( pStr, sTemp );
							}
						}
					}
					else
					{
						INT8 bSkill1 = 0, bSkill2 = 0; 	
						bSkill1 = gMercProfiles[ gpSMCurrentMerc->ubProfile ].bSkillTraits[0];
						bSkill2 = gMercProfiles[ gpSMCurrentMerc->ubProfile ].bSkillTraits[1];

						if ( bSkill1 == 0 && bSkill2 == 0 )
						{
							swprintf( pStr, L"%s", pPersonnelScreenStrings[ PRSNL_TXT_NOSKILLS ] );
						}
						else
						{
							//if the 2 skills are the same, add the '(expert)' at the end
							if( bSkill1 == bSkill2 )
							{
								swprintf( pStr, L"%s %s", gzMercSkillText[bSkill1], gzMercSkillText[EXPERT] );
							}
							else
							{
								//Display the first skill
								if( bSkill1 != 0 )
								{
									swprintf( pStr, L"%s\n", gzMercSkillText[bSkill1] );
								}
								if( bSkill2 != 0 )
								{
									swprintf( sTemp, L"%s", gzMercSkillText[bSkill2] );
									wcscat( pStr, sTemp );
								}
							}
						}
					}
					SetRegionFastHelpText( &gSM_SELMERCPanelRegion, pStr );
				}				
			}
		}
		else
		{
			SetRegionFastHelpText( &(gSM_SELMERCBarsRegion), L"" );
			SetRegionFastHelpText( &gSM_SELMERCPanelRegion, L"" );
		}

		//if we are in the shop keeper interface
		if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
			SetRegionHelpEndCallback( &gSM_SELMERCBarsRegion, SkiHelpTextDoneCallBack );

		// display AP
		if ( !( gpSMCurrentMerc->flags.uiStatusFlags & SOLDIER_DEAD ) )
		{
			if ( gTacticalStatus.uiFlags & TURNBASED && (gTacticalStatus.uiFlags & INCOMBAT ) && gpSMCurrentMerc->stats.bLife >= OKLIFE )
			{
				SetFont( TINYFONT1 );
				//if ( gpSMCurrentMerc->sLastTarget != NOWHERE && !EnoughPoints( gpSMCurrentMerc, MinAPsToAttack( gpSMCurrentMerc, gpSMCurrentMerc->sLastTarget, FALSE ), 0, FALSE ) || GetUIApsToDisplay( gpSMCurrentMerc ) < 0 )
				if ( !EnoughPoints( gpSMCurrentMerc, MinAPsToAttack( gpSMCurrentMerc, gpSMCurrentMerc->sLastTarget, FALSE, 0 ), 0, FALSE ) || GetUIApsToDisplay( gpSMCurrentMerc ) < 0 )
				{
					if(MercUnderTheInfluence(gpSMCurrentMerc, DRUG_TYPE_ADRENALINE))
					{
						SetFontBackground(FONT_MCOLOR_BLACK);
						SetRGBFontForeground(200, 5, 100);
					}
					else
					{
						SetFontBackground(FONT_MCOLOR_BLACK);
						SetFontForeground(FONT_MCOLOR_DKRED);
					}					
				}
				else
				{
					if (MercUnderTheInfluence(gpSMCurrentMerc, DRUG_TYPE_ADRENALINE))
					{
						SetFontBackground( FONT_MCOLOR_BLACK );
						//SetFontForeground( FONT_MCOLOR_LTBLUE );

						// Flugente: new colour for being drugged, as blue on black was hard to see
						SetRGBFontForeground( 250, 5, 250 );
					}
					else if ( gpSMCurrentMerc->bStealthMode )
					{
						SetFontBackground( FONT_MCOLOR_BLACK );
						SetFontForeground( FONT_MCOLOR_LTYELLOW );
					}
					else
					{
						SetFontBackground( FONT_MCOLOR_BLACK );
						SetFontForeground( FONT_MCOLOR_LTGRAY );
					}
				}

				gprintfRestore(SM_SELMERC_AP_X, SM_SELMERC_AP_Y, L"%3d", GetUIApsToDisplay(gpSMCurrentMerc));
				VarFindFontCenterCoordinates( SM_SELMERC_AP_X, SM_SELMERC_AP_Y, SM_SELMERC_AP_WIDTH, SM_SELMERC_AP_HEIGHT, TINYFONT1, &sFontX, &sFontY, L"%d", GetUIApsToDisplay( gpSMCurrentMerc ) );
				mprintf( sFontX, SM_SELMERC_AP_Y, L"%d", GetUIApsToDisplay( gpSMCurrentMerc ) );
			}

			// Display bars
			DrawLifeUIBarEx( gpSMCurrentMerc, SM_SELMERC_HEALTH_X, SM_SELMERC_HEALTH_Y, SM_SELMERC_HEALTH_WIDTH, SM_SELMERC_HEALTH_HEIGHT, TRUE , FRAME_BUFFER );

			if (!(gpSMCurrentMerc->flags.uiStatusFlags & SOLDIER_ROBOT))
			{
				DrawBreathUIBarEx(gpSMCurrentMerc, SM_SELMERC_BREATH_X, SM_SELMERC_BREATH_Y, SM_SELMERC_HEALTH_WIDTH, SM_SELMERC_HEALTH_HEIGHT, TRUE, FRAME_BUFFER);
				DrawMoraleUIBarEx(gpSMCurrentMerc, SM_SELMERC_MORALE_X, SM_SELMERC_MORALE_Y, SM_SELMERC_MORALE_WIDTH, SM_SELMERC_MORALE_HEIGHT, TRUE, FRAME_BUFFER);
			}
		}

	}

/*	I added this
 *	any questions? joker
 */
	/*if( gfRenderRadarScreen == TRUE )
	//if(gfRenderSquadList == FALSE)
	{*/
		// Render clock
		// CHRISL: If we're in NIV mode, don't render the clock on the SM panel
		if(UsingNewInventorySystem() == false)
		{
			RenderClock( INTERFACE_CLOCK_X, INTERFACE_CLOCK_Y );
			CreateMouseRegionForPauseOfClock( INTERFACE_CLOCK_X, INTERFACE_CLOCK_Y );
		}
		else
		{
			RemoveMouseRegionForPauseOfClock();
		}

		// CHRISL: Change function call to include X,Y coordinates.
		RenderTownIDString( LOCATION_NAME_X, LOCATION_NAME_Y );

	/*}//XXXj in tactical bug is: NIV pushes town name +Y and boom
	else
	{
		RemoveMouseRegionForPauseOfClock( );
	}*/

	UpdateSMPanel( );


	//HandlePanelFaceAnimations( gpSMCurrentMerc );

	HandleSoldierFaceFlash( gpSMCurrentMerc, SM_SELMERC_FACE_X, SM_SELMERC_FACE_Y );

	// Render items in guy's hand!
	HandleRenderInvSlots( gpSMCurrentMerc, *pfDirty );

	if ( gfSMDisableForItems && (*pfDirty) != DIRTYLEVEL0 )
	{
		UINT8	*pDestBuf;
		UINT32 uiDestPitchBYTES;
		SGPRect ClipRect;

		// CHRISL: Change hatching area based on inventory system
		ClipRect.iLeft = ((UsingNewInventorySystem() == true)) ? 0 : 87;
		ClipRect.iRight = ((UsingNewInventorySystem() == true)) ? SCREEN_WIDTH : 536;
		//ClipRect.iLeft	 = 87;
		//ClipRect.iRight  = 536;
		ClipRect.iTop		= INV_INTERFACE_START_Y;
		ClipRect.iBottom = SCREEN_HEIGHT;
		pDestBuf = LockVideoSurface( FRAME_BUFFER, &uiDestPitchBYTES );
		Blt16BPPBufferHatchRect( (UINT16*)pDestBuf, uiDestPitchBYTES, &ClipRect );
		UnLockVideoSurface( FRAME_BUFFER );
	}

}

void UpdateStatColor(UINT32 uiTimer, BOOLEAN fIncrease, BOOLEAN fDamaged, BOOLEAN fAugmented, BOOLEAN fInShock, BOOLEAN fInspired, BOOLEAN fLeader, BOOLEAN fCarryWeight)
{
	if ( gpSMCurrentMerc->stats.bLife >= OKLIFE )
	{
		// sevenfm: apply short term color changes first
		if (fInShock)
		{
			SetFontForeground(FONT_ORANGE);
		}
		else if (fInspired)
		{
			SetFontForeground(FONT_LTBLUE);
		}
		else if (fLeader)
		{
			SetFontForeground(FONT_BLUE);
		}
		else if (fAugmented)
		{
			SetRGBFontForeground(250, 5, 250);
		}
		else if (fCarryWeight)
		{
			SetFontForeground(FONT_METALGRAY);
		}
		else if ((GetJA2Clock() < CHANGE_STAT_RECENTLY_DURATION + uiTimer) && (uiTimer != 0))
		{
			if (fIncrease)
			{
				SetFontForeground(FONT_LTGREEN);
			}
			else
			{
				SetFontForeground(FONT_RED);
			}
		}
		else if (fDamaged)
		{
			SetFontForeground(FONT_RED);
		}
		else
		{
			SetFontBackground(FONT_MCOLOR_BLACK);
			SetFontForeground(STATS_TEXT_FONT_COLOR);
		}

		// SANDRO - if damaged stat we could regain, show in red until repaired
		/*if( fDamaged )
		{
			SetFontForeground( FONT_RED );
		}
		else if( ( GetJA2Clock()	< CHANGE_STAT_RECENTLY_DURATION + uiTimer) && ( uiTimer != 0 ) )
		{
			if( fIncrease )
			{
				SetFontForeground( FONT_LTGREEN );
			}
			else
			{
				SetFontForeground( FONT_RED );
			}
		}
		else if ( fAugmented )
		{
			SetRGBFontForeground( 250, 5, 250 );
		}
		else
		{
			SetFontBackground( FONT_MCOLOR_BLACK );
			SetFontForeground( STATS_TEXT_FONT_COLOR );
		}*/
	}
	else
	{
		SetFontBackground( FONT_MCOLOR_BLACK );
		SetFontForeground( FONT_MCOLOR_DKGRAY );
	}
}



void SMInvMoveCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}

	UINT32 uiHandPos = MSYS_GetRegionUserData(pRegion, 0);

	if ( gpSMCurrentMerc->inv[ uiHandPos ].exists() == false )
		return;

	if (iReason == MSYS_CALLBACK_REASON_MOVE)
	{
	}
	else if (iReason == MSYS_CALLBACK_REASON_GAIN_MOUSE )
	{
		if ( gpItemPointer == NULL )
		{
			// Setup a timer....
			guiMouseOverItemTime = GetJA2Clock( );
			gfCheckForMouseOverItem = TRUE;
			gbCheckForMouseOverItemPos = (INT8)uiHandPos;
		}
	}
	if (iReason == MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		//gfSM_HandInvDispText[ uiHandPos ] = 1;
		if ( gpItemPointer == NULL )
		{
			HandleCompatibleAmmoUI( gpSMCurrentMerc, (INT8)uiHandPos, FALSE );
			gfCheckForMouseOverItem = FALSE;
			fInterfacePanelDirty = DIRTYLEVEL2;
			gbCheckForMouseOverItemPos = NO_SLOT;
		}
	}
}

void InvPanelButtonClickCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}
	if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN)
	{
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	{

	}
}


void SMInvMoveCammoCallback(MOUSE_REGION * pRegion, INT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}
	else if (iReason == MSYS_CALLBACK_REASON_GAIN_MOUSE)
	{
		if (gpItemPointer == NULL)
		{
			// Setup a timer....
			guiMouseOverItemTime = GetJA2Clock();
			gfCheckForMouseOverItem = TRUE;
			gbCheckForMouseOverItemPos = NO_SLOT;
		}
	}
	if (iReason == MSYS_CALLBACK_REASON_LOST_MOUSE)
	{
		if (gpItemPointer == NULL)
		{
			//gfSM_HandInvDispText[ uiHandPos ] = 1;
			HandleCompatibleAmmoUI(gpSMCurrentMerc, (INT8)NO_SLOT, FALSE);
			gfCheckForMouseOverItem = FALSE;
		}
	}
}


void SMInvClickCamoCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	//UINT16 usNewItemIndex;
	UINT8	ubSrcID, ubDestID;
	BOOLEAN fGoodAPs;

	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}

	if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		// Apply camo ( if we have something in cursor... )
		// If we do not have an item in hand, start moving it

		if ( gpItemPointer != NULL )
		{
			// ATE: OK, get source, dest guy if different... check for and then charge appropriate APs
			ubSrcID	= gpSMCurrentMerc->ubID;
			ubDestID = gpItemPointerSoldier->ubID;

			//if ( ubSrcID == ubDestID )
			{
				// We are doing this ourselve, continue
				if ( gpSMCurrentMerc->stats.bLife >= CONSCIOUSNESS )
				{
					//usNewItemIndex = gpItemPointer->usItem;

					// Try to apply camo....
					if ( ApplyCammo( gpSMCurrentMerc, gpItemPointer, &fGoodAPs ) )
					{
						if ( fGoodAPs )
						{
							// WANNE: We should only delete the face, if there was a camo we applied.
							// This should fix the bug and crashes with missing faces
							if (gGameExternalOptions.fShowCamouflageFaces == TRUE )
							{
								// Flugente: refresh face regardless of result of SetCamoFace(), otherwise applying a rag will not clean the picture
								SetCamoFace( gpSMCurrentMerc );
								DeleteSoldierFace( gpSMCurrentMerc );// remove face
								gpSMCurrentMerc->iFaceIndex = InitSoldierFace( gpSMCurrentMerc );// create new face
							}
							
							// Dirty
							fInterfacePanelDirty = DIRTYLEVEL2;

							// Check if it's the same now!
							if ( gpItemPointer->exists() == false )
							{
								gbCompatibleApplyItem = FALSE;
								EndItemPointer( );
							}

							// Say OK acknowledge....
							gpSMCurrentMerc->DoMercBattleSound( BATTLE_SOUND_COOL1 );
						}
					}
					// sevenfm: halve AP when using hydration bladder from water slot in backpack
					else if (!gGameOptions.fFoodSystem && 
						ApplyCanteen(gpSMCurrentMerc, gpItemPointer, &fGoodAPs, TRUE, Item[gpItemPointer->usItem].HydrationBladder && LBEPocketType[GetPocketIndex(gpSMCurrentMerc, gbItemPointerSrcSlot)].pHydrationBladder))
					{
						// Dirty
						if ( fGoodAPs )
						{
							fInterfacePanelDirty = DIRTYLEVEL2;

							// Check if it's the same now!
							if ( gpItemPointer->exists() == false )
							{
								gbCompatibleApplyItem = FALSE;
								EndItemPointer( );
							}
						}
					}
					else if ( ApplyElixir( gpSMCurrentMerc, gpItemPointer, &fGoodAPs ) )
					{
						if ( fGoodAPs )
						{
							// Dirty
							fInterfacePanelDirty = DIRTYLEVEL2;

							// Check if it's the same now!
							if ( gpItemPointer->exists() == false )
							{
								gbCompatibleApplyItem = FALSE;
								EndItemPointer( );
							}

							// Say OK acknowledge....
							gpSMCurrentMerc->DoMercBattleSound( BATTLE_SOUND_COOL1 );
						}
					}
					else if ( ApplyDrugs( gpSMCurrentMerc, gpItemPointer, TRUE ) )
					{
						// Dirty
						fInterfacePanelDirty = DIRTYLEVEL2;

						// Check if it's the same now!
						if ( gpItemPointer->exists() == false )
						{
							gbCompatibleApplyItem = FALSE;
							EndItemPointer( );
						}

						/*
						// COMMENTED OUT DUE TO POTENTIAL SERIOUS PROBLEMS WITH STRATEGIC EVENT SYSTEM

						if ( gpSMCurrentMerc->ubProfile == LARRY_NORMAL )
						{
							// Larry's taken something!
							gpSMCurrentMerc = SwapLarrysProfiles( gpSMCurrentMerc );
						}
						*/


						// Say OK acknowledge....
						gpSMCurrentMerc->DoMercBattleSound( BATTLE_SOUND_COOL1 );

					}
					else if ( gGameOptions.fFoodSystem && ApplyFood( gpSMCurrentMerc, gpItemPointer, FALSE, FALSE ) )
					{
						// Dirty
						fInterfacePanelDirty = DIRTYLEVEL2;

						// Check if it's the same now!
						if ( gpItemPointer->exists() == false )
						{
							gbCompatibleApplyItem = FALSE;
							EndItemPointer( );
						}

						// Say OK acknowledge....
						gpSMCurrentMerc->DoMercBattleSound( BATTLE_SOUND_COOL1 );
					}
					else if ( ApplyClothes( gpSMCurrentMerc, gpItemPointer ) )
					{
						// Dirty
						fInterfacePanelDirty = DIRTYLEVEL2;

						// Check if it's the same now!
						if ( gpItemPointer->exists() == false )
						{
							gbCompatibleApplyItem = FALSE;
							EndItemPointer( );
						}

						// Say OK acknowledge....
						gpSMCurrentMerc->DoMercBattleSound( BATTLE_SOUND_COOL1 );
					}
					else
					{
						// Send message
						//Heinz: 23.02.09 BUGFIX: Don't send message when SKI is on
						if( !( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE ) ) ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[ CANNOT_DO_INV_STUFF_STR ] );
					}
				}
			}
		}
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	{

	}
}


BOOLEAN HandleNailsVestFetish( SOLDIERTYPE *pSoldier, UINT32 uiHandPos, UINT16 usReplaceItem )
{
	BOOLEAN fRefuse = FALSE;

	// OK are we nails?
	if ( pSoldier->ubProfile == NAILS )
	{
		// if this the VEST POS?
		if ( uiHandPos == VESTPOS )
		{
			// Are we trying to pick it up?
			if ( usReplaceItem == NOTHING )
			{
				fRefuse = TRUE;
			}
			else
			{
				// Do we have nothing or the leather vest or kevlar leather vest?
				if ( Item[usReplaceItem].leatherjacket ||
						usReplaceItem == COMPOUND18 ||
						usReplaceItem == JAR_QUEEN_CREATURE_BLOOD )
				{
					// This is good....
					fRefuse = FALSE;
				}
				else
				{
					fRefuse = TRUE;
				}
			}

			if ( fRefuse )
			{
				// Say quote!
				TacticalCharacterDialogue( pSoldier, 61 );
				return( TRUE );
			}
		}
	}

	if (!fRefuse &&
		(uiHandPos == VESTPOS || uiHandPos == HELMETPOS || uiHandPos == LEGPOS) &&
		pSoldier->inv[uiHandPos].exists() && 
		pSoldier->inv[uiHandPos].usItem == pSoldier->GetBackgroundValue(BG_FETISH_ITEM))
	{
		// Say quote!
		TacticalCharacterDialogue(pSoldier, 36);
		//pSoldier->DoMercBattleSound(BATTLE_SOUND_LOWMARALE_ATTN1);
		return(TRUE);
	}

	return( FALSE );
}

BOOLEAN UIHandleItemPlacement( UINT8 ubHandPos, UINT16 usOldItemIndex, UINT16 usNewItemIndex, BOOLEAN fDeductPoints )
{
	// Try to place here
	if ( PlaceObject( gpSMCurrentMerc, ubHandPos, gpItemPointer ) )
	{
		if ( fDeductPoints )
		{
			// Deduct points
			if ( gpItemPointerSoldier->stats.bLife >= CONSCIOUSNESS )
			{
				DeductPoints( gpItemPointerSoldier,	2, 0, UNTRIGGERED_INTERRUPT );
			}
			if ( gpSMCurrentMerc->stats.bLife >= CONSCIOUSNESS )
			{
				DeductPoints( gpSMCurrentMerc,	2, 0, UNTRIGGERED_INTERRUPT );
			}
		}

		HandleTacticalEffectsOfEquipmentChange( gpSMCurrentMerc, ubHandPos, usOldItemIndex, usNewItemIndex );

		// Dirty
		fInterfacePanelDirty = DIRTYLEVEL2;

		// Check if cursor is empty now
		// sevenfm: check gpItemPointer before using it
		if (gpItemPointer && !gpItemPointer->exists())
		{
			EndItemPointer( );
		}

		if ( gpItemPointerSoldier != gpSMCurrentMerc )
		{
			ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[ MSG_ITEM_PASSED_TO_MERC ], ShortItemNames[ usNewItemIndex ], gpSMCurrentMerc->name );
		}

		// UPDATE ITEM POINTER.....
		gpItemPointerSoldier = gpSMCurrentMerc;

		if ( gpItemPointer != NULL )
		{
			ReevaluateItemHatches( gpSMCurrentMerc, FALSE );
		}

		// Set cursor back to normal mode...
		guiPendingOverrideEvent = A_CHANGE_TO_MOVE;

		return( TRUE );

	}
	else
	{
		return( FALSE );
	}

}

//Jenilee
INT32 iLastHandPos = -1;

#define SLOT_NONE		0
#define SLOT_HANDS		1
#define SLOT_EQUIPMENT	2
#define SLOT_VEST		3
#define SLOT_RIG		4
#define SLOT_CPACK		5
#define SLOT_BPACK		6
#define SLOT_SLING		7
#define SLOT_KNIFE		8
#define SLOT_FACE		9

STR16 uiSlotTypeName[] = 
{
	L"",				//0
	L"Hands",			//1
	L"Equipment",		//2
	L"Vest",			//3
	L"Leg Rig",			//4
	L"Combat Pack",		//5
	L"Backpack",		//6
	L"Sling",			//7
	L"Knife",			//8
	L"Face",			//9
	L"Large Pocket",	//10
	L"Small Pocket"		//11
};

INT16 uiNIVSlotType[NUM_INV_SLOTS] = { 
						2, 2, 2, //0-2
						9, 9, //3-4
						1, 1, //5-6
						2, 2, 2, 2, 2,  //7-11
						7, //12
						8, //13
						5, 5, 5, //14-16
						6, 6, 6, 6, //17-20
						3, 3, //21-22
						4, 4, //23-24
						3, 3, 3, 3, 3, //25-29
						3, 3, 3, 3, 3, //30-34
						4, 4, 4, 4, //35-38
						4, 4, 4, 4, //39-42
						5, 5, 5, 5, //43-46
						6, 6, 6, 6, //47-50
						6, 6, 6, 6  //51-54
					};

UINT16 uiOIVSlotType[NUM_INV_SLOTS] = {
						2, 2, 2, //0-2
						9, 9, //3-4
						1, 1, //5-6
						0, 0, 0, 0, 0, 0, 0, //7-13
						10, 10, 10, 10,	//14-17
						0, 0, 0, 0, 0, 0, 0, //18-24
						11, 11, 11, 11, 11, 11, 11, 11, //25-32
						0, 0, 0, 0, 0, 0, 0, 0,	//33-40
						0, 0, 0, 0, 0, 0, 0, 0,	//41-48
						0, 0, 0, 0, 0, 0	//49-54
};

UINT16 GetInvMovementCost(OBJECTTYPE* pObj, INT16 old_pos, INT16 new_pos)
{
	if (!(gTacticalStatus.uiFlags & INCOMBAT) || //Not in combat
		(old_pos == -1 || new_pos == -1)||	//Either position is invalid
		(old_pos == new_pos))				//Old position same as new position
		return 0;
	INT16 src_type;
	INT16 dst_type;
	if (UsingNewInventorySystem() == TRUE) 
	{
		src_type = uiNIVSlotType[old_pos];
		dst_type = uiNIVSlotType[new_pos];
	}
	else
	{
		src_type = uiOIVSlotType[old_pos];
		dst_type = uiOIVSlotType[new_pos];
	}
	if (src_type == dst_type)	//Moving to same lbe type
		return 0;

	//If these 2 arrays are initiated outside the function, APBPConstants will not have been initialized, and all values will be 0
	INT16 uiAPCostFromSlot[12] =
	{
		APBPConstants[AP_INV_FROM_NONE],
		APBPConstants[AP_INV_FROM_HANDS],
		APBPConstants[AP_INV_FROM_EQUIPMENT],
		APBPConstants[AP_INV_FROM_VEST],
		APBPConstants[AP_INV_FROM_RIG],
		APBPConstants[AP_INV_FROM_CPACK],
		APBPConstants[AP_INV_FROM_BPACK],
		APBPConstants[AP_INV_FROM_SLING],
		APBPConstants[AP_INV_FROM_KNIFE],
		APBPConstants[AP_INV_FROM_FACE],
		APBPConstants[AP_INV_FROM_BIG_POCKET],
		APBPConstants[AP_INV_FROM_SMALL_POCKET]
	};
	INT16 uiAPCostToSlot[12] =
	{
		APBPConstants[AP_INV_TO_NONE],
		APBPConstants[AP_INV_TO_HANDS],
		APBPConstants[AP_INV_TO_EQUIPMENT],
		APBPConstants[AP_INV_TO_VEST],
		APBPConstants[AP_INV_TO_RIG],
		APBPConstants[AP_INV_TO_CPACK],
		APBPConstants[AP_INV_TO_BPACK],
		APBPConstants[AP_INV_TO_SLING],
		APBPConstants[AP_INV_TO_KNIFE],
		APBPConstants[AP_INV_TO_FACE],
		APBPConstants[AP_INV_TO_BIG_POCKET],
		APBPConstants[AP_INV_TO_SMALL_POCKET]
	};
	FLOAT Weight_Divisor = gGameExternalOptions.uWeightDivisor;
	UINT16 weight_modifier;
	if (Weight_Divisor > 0)
		weight_modifier = DynamicAdjustAPConstants((int)((Item[pObj->usItem].ubWeight) / Weight_Divisor));
	else
		weight_modifier = 0;

	INT32 cost = 0; 

	cost += uiAPCostFromSlot[src_type];
	cost += uiAPCostToSlot[dst_type];
	cost += weight_modifier;

	// Flugente if we move an item from our hands to the sling, and item has a weapon sling, don't charge any APs
	if ( 1 == src_type && 7 == dst_type )
	{
		// if item has a weapon sling attached
		if ( HasAttachmentOfClass(pObj, AC_SLING) )
			// we simply let go of the item, we can do that because the sling will catch it
			cost = 0;
	}

	if (cost > APBPConstants[AP_INV_MAX_COST]) 
		cost = APBPConstants[AP_INV_MAX_COST];

#ifdef _DEBUG
	CHAR16 szTemp[255];
	swprintf(szTemp, L"Moving item from %s to %s (costs: %d/%d/%d/%d)", uiSlotTypeName[src_type], uiSlotTypeName[dst_type], uiAPCostFromSlot[src_type], uiAPCostToSlot[dst_type], weight_modifier, cost);
	ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_CHAT, szTemp);
#endif
	return cost;
}

void SMInvClickCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	UINT32 uiHandPos;
	// Copies of values
	UINT16 usOldItemIndex, usNewItemIndex;
	UINT16 usItemPrevInItemPointer;
	UINT16 usCostToMoveItem = 0; //Jenilee
	BOOLEAN fNewItem = FALSE;
	static BOOLEAN	fRightDown = FALSE;
	static BOOLEAN	fLeftDown = FALSE;

	uiHandPos = MSYS_GetRegionUserData( pRegion, 0 );

	if ( (guiTacticalInterfaceFlags & INTERFACE_MAPSCREEN ) )
	{
		return;
	}

	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}

	//if we are in the shop keeper interface
	if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
	{
		// and this inventory slot is hatched out
		if( ShouldSoldierDisplayHatchOnItem( gpSMCurrentMerc->ubProfile, (INT16)uiHandPos ) )
		{
			// it means that item is a copy of one in the player's offer area, so we treat it as if the slot was empty (ignore)
			// if the cursor has an item in it, we still ignore the click, because handling swaps in this situation would be
			// ugly, we'd have to the the swap, then make the bOwnerSlot of the item just picked up a -1 in its offer area spot.
			return;
		}
	}


	//if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	//{
	//	fLeftDown = TRUE;
	//}
	//else if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP && fLeftDown )
	// CHRISL: Are we in combat, wearing a backpack with the zipper closed?  Don't allow access to backpack items
	if ((UsingNewInventorySystem() == true))
	{
		if (icLBE[uiHandPos] == BPACKPOCKPOS && 
			(!(gpSMCurrentMerc->flags.ZipperFlag) || (gpSMCurrentMerc->flags.ZipperFlag && gAnimControl[gpSMCurrentMerc->usAnimState].ubEndHeight == ANIM_STAND)) && 
			(gTacticalStatus.uiFlags & INCOMBAT) && 
			(iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN))
		{
			iReason = MSYS_CALLBACK_REASON_NONE;
		}
	}

	if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		fLeftDown = FALSE;

		// If we do not have an item in hand, start moving it
		if ( gpItemPointer == NULL )
		{
			// Return if empty
			if ( gpSMCurrentMerc->inv[ uiHandPos ].exists() == false )
				return;

			if ( gpSMCurrentMerc->ubID != gusSelectedSoldier )
			{
				SelectSoldier( gpSMCurrentMerc->ubID, FALSE, FALSE );
			}

			// OK, check if this is Nails, and we're in the vest position , don't allow it to come off....
			if ( HandleNailsVestFetish( gpSMCurrentMerc, uiHandPos, NOTHING ) )
			{
				return;
			}

			// sevenfm: don't allow with negative AP
			/*if (gGameOptions.fInventoryCostsAP &&
				gpSMCurrentMerc->bActionPoints < 0)
			{
				ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[CANNOT_DO_INV_STUFF_STR]);
				return;
			}*/

			if ( _KeyDown(CTRL) )
			{				
				CleanUpStack( &( gpSMCurrentMerc->inv[ uiHandPos ] ), NULL );
				return;
			}
			
			if((UsingNewInventorySystem() == true))
			{
				/*if we pick up a backpack without reactivating the drop pack button, and we have a dropkey, reactivate the button*/
				if(uiHandPos == BPACKPOCKPOS)
				{
					// Deal with the zipper before we do anything
					if(gpSMCurrentMerc->flags.ZipperFlag)
						if(!ChangeZipperStatus(gpSMCurrentMerc, FALSE))
							return;
					// Do we still have a linked backpack?  If so, reset droppackflag
					for(unsigned int wi = 0; wi < guiNumWorldItems; wi++)
					{
						if(gWorldItems[wi].soldierID == gpSMCurrentMerc->ubID && gWorldItems[wi].object.exists() == true)
						{
							gpSMCurrentMerc->flags.DropPackFlag = TRUE;
							break;
						}
					}
					RenderBackpackButtons(ACTIVATE_BUTTON);	/* CHRISL: Needed for new inventory backpack buttons */
				}
			}

			// Turn off new item glow!
			gpSMCurrentMerc->inv.bNewItemCount[ uiHandPos ] = 0;

			usOldItemIndex = gpSMCurrentMerc->inv[ uiHandPos ].usItem;

			//Jenilee: remember our last selected slot
			iLastHandPos = uiHandPos;


			// move item into the mouse cursor
			BeginItemPointer( gpSMCurrentMerc, (UINT8)uiHandPos );

			//if we are in the shopkeeper interface
			if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
			{
				// pick up item from regular inventory slot into cursor OR try to sell it ( unless CTRL is held down )
				BeginSkiItemPointer( PLAYERS_INVENTORY, ( INT8 ) uiHandPos, ( BOOLEAN )!gfKeyState[ CTRL ] );
			}

			HandleTacticalEffectsOfEquipmentChange( gpSMCurrentMerc, uiHandPos, usOldItemIndex, NOTHING );

			// HandleCompatibleAmmoUI( gpSMCurrentMerc, (INT8)uiHandPos, FALSE );
		}
		else	// item in cursor
		{
			UINT8			ubSrcID, ubDestID;
			BOOLEAN		fOKToGo = FALSE;
			BOOLEAN		fDeductPoints = FALSE;

			// ATE: OK, get source, dest guy if different... check for and then charge appropriate APs
			ubSrcID	= gpSMCurrentMerc->ubID;
			ubDestID = gpItemPointerSoldier->ubID;

			if ( ubSrcID == ubDestID )
			{
				if(!CanItemFitInPosition(gpSMCurrentMerc, gpItemPointer, (INT8)uiHandPos, FALSE))//dnl ch66 070913
					return;

				if (gGameOptions.fInventoryCostsAP)
				{
					//Jenilee: determine the cost of moving this item around in our inventory
					usCostToMoveItem = GetInvMovementCost(gpItemPointer, iLastHandPos, uiHandPos);
					// Flugente: backgrounds
					usCostToMoveItem = (usCostToMoveItem * (100 + gpSMCurrentMerc->GetBackgroundValue(BG_INVENTORY))) / 100;
				}
				
				// sevenfm: less restrictions, allow to put item into inventory if resulting AP >= -MAX_AP_CARRIED
				if (usCostToMoveItem == 0 || 
					gpSMCurrentMerc->bActionPoints >= usCostToMoveItem - APBPConstants[MAX_AP_CARRIED] && gpSMCurrentMerc->bActionPoints >= usCostToMoveItem + APBPConstants[AP_MIN_LIMIT] ||
					iLastHandPos == uiHandPos)
				{
					fOKToGo = TRUE;
				}
				else
				{
					ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[NOT_ENOUGH_APS_STR]);
					fOKToGo = FALSE;
				}

				/*if ( ( usCostToMoveItem == 0 ) || ( gpSMCurrentMerc->bActionPoints >= usCostToMoveItem ) )
				{
					fOKToGo = TRUE;
					//iLastHandPos = uiHandPos;//dnl ch66 070913 this should be set after we move item
				}
				else //we dont have enough APs to move it to this slot, show a warning message
				{
					// silversurfer: What if our old slot is occupied now (could happen when we swap items)?
					// We will be stuck with an item at the hand cursor and nowhere to put it -> bad. :-(
					// So let's check if our old slot is empty and if it is not allow item placement anyway.
					if ( gpSMCurrentMerc->inv[ iLastHandPos ].usItem == NULL )
					{
						ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[NOT_ENOUGH_APS_STR]);
						fOKToGo = FALSE;
					}
					else
						fOKToGo = TRUE;
				}*/

				// We are doing this ourselve, continue
//				fOKToGo = TRUE;
			}
			else
			{
				// These guys are different....
				fDeductPoints = TRUE;

				// First check points for src guy
				if ( gpItemPointerSoldier->stats.bLife >= CONSCIOUSNESS )
				{
					if ( EnoughPoints( gpItemPointerSoldier, 3, 0, TRUE ) )
					{
						fOKToGo = TRUE;
					}
				}
				else
				{
					fOKToGo = TRUE;
				}

				// Should we go on?
				if ( fOKToGo )
				{
					if ( gpSMCurrentMerc->stats.bLife >= CONSCIOUSNESS )
					{
						if ( EnoughPoints( gpSMCurrentMerc, 3, 0, TRUE ) )
						{
							fOKToGo = TRUE;
						}
						else
						{
							fOKToGo = FALSE;
						}
					}
				}
			}

			if ( fOKToGo )
			{
				usOldItemIndex = gpSMCurrentMerc->inv[ uiHandPos ].usItem;
				usNewItemIndex = gpItemPointer->usItem;

				// OK, check if this is Nails, and we're in the vest position , don't allow it to come off....
				if ( HandleNailsVestFetish( gpSMCurrentMerc, uiHandPos, usNewItemIndex ) )
				{
					return;
				}

				if ( _KeyDown(CTRL) )
				{
					if (gpItemPointer->exists() && gpSMCurrentMerc->inv[uiHandPos].exists())
					{
						if (gpItemPointer->usItem == gpSMCurrentMerc->inv[uiHandPos].usItem)
							CleanUpStack(&(gpSMCurrentMerc->inv[uiHandPos]), gpItemPointer);
						else // Madd: attach / merge object, merge only works on single objects for now
						{
							UINT8 cnt = gpSMCurrentMerc->inv[uiHandPos].ubNumberOfObjects;
							if (gpItemPointer->ubNumberOfObjects < cnt)
								cnt = gpItemPointer->ubNumberOfObjects;

							for (UINT8 i = 0; i < cnt; i++)
							{
								usCostToMoveItem = 0;
								if ((gTacticalStatus.uiFlags & INCOMBAT))
								{
									// silversurfer: This didn't cost any AP. Why? CTRL + LeftClick should deduct the same AP as manual attachment in the EDB.
									usCostToMoveItem = AttachmentAPCost(gpItemPointer->usItem, gpSMCurrentMerc->inv[uiHandPos].usItem, gpSMCurrentMerc);
									// Flugente: backgrounds
									usCostToMoveItem = (usCostToMoveItem * (100 + gpSMCurrentMerc->GetBackgroundValue(BG_INVENTORY))) / 100;
									// do we have enough AP?
									if (!EnoughPoints(gpSMCurrentMerc, usCostToMoveItem, 0, TRUE))
										return;
									//gpSMCurrentMerc->bActionPoints -= usCostToMoveItem;
								}
								if (gpSMCurrentMerc->inv[uiHandPos].AttachObject(gpSMCurrentMerc, gpItemPointer, TRUE, i))
								{
									gpSMCurrentMerc->bActionPoints -= usCostToMoveItem;
								}
							}
						}
					}

					if ( gpItemPointer->exists() == false )
					{
						EndItemPointer( );
					}
					return;
				}

				// hold ALT key to swap valid attachment item instead
				if ( _KeyDown(ALT) )
				{
					// do nothing
				}
				// we allow attaching on items in any slot
				else if ( ValidAttachment( usNewItemIndex, &(gpSMCurrentMerc->inv[uiHandPos]) ) )
				{
					// it's an attempt to attach; bring up the inventory panel
					if ( !InItemDescriptionBox( ) )
					{
						InitItemDescriptionBox( gpSMCurrentMerc, (UINT8)uiHandPos, ITEMDESC_START_X, ITEMDESC_START_Y, 0 );
					}
					return;
				}

				if ( uiHandPos == HANDPOS || uiHandPos == SECONDHANDPOS || uiHandPos == HELMETPOS || uiHandPos == VESTPOS || uiHandPos == LEGPOS )
				{
					if ( ValidMerge( usNewItemIndex, usOldItemIndex ) && _KeyDown(SHIFT) )
					{
						// bring up merge requestor
						gubHandPos = (UINT8) uiHandPos;
						gusOldItemIndex = usOldItemIndex;
						gusNewItemIndex = usNewItemIndex;
						gfDeductPoints = fDeductPoints;

						if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
						{
							//the only way to merge items is to pick them up.	In SKI when you pick up an item, the cursor is
							//locked in a region, free it up.
							FreeMouseCursor( TRUE );

							DoMessageBox( MSG_BOX_BASIC_STYLE, Message[ STR_MERGE_ITEMS ], SHOPKEEPER_SCREEN, ( UINT8 )MSG_BOX_FLAG_YESNO, MergeMessageBoxCallBack, NULL );
						}
						else
							DoMessageBox( MSG_BOX_BASIC_STYLE, Message[ STR_MERGE_ITEMS ], GAME_SCREEN, ( UINT8 )MSG_BOX_FLAG_YESNO, MergeMessageBoxCallBack, NULL );
						return;
					}
					// else handle normally
				}


				/* CHRISL: For New Inventory system.  Are we swapping LBE Items?  We'll need to move any
				items in the associated IC group pockets into a new LBENODE.  We'll have to check
				whether we already have an LBENODE for this type of LBE item and, if so, remove
				the items from it, place them in the LBE Items current sector, and delete the existing
				LBENODE.  Then we need to know if the LBE Item in the cursor is an LBENODE
				or just a normal OBJECTTYPE.  If it's an LBENODE, we need to move it's items into
				the appropriate pockets for the soldier and then delete the LBENODE.*/
				if((UsingNewInventorySystem() == true))
				{
					//If we put a new pack in the backpack pocket, turn off the droppack button
					if(uiHandPos == BPACKPOCKPOS && CanItemFitInPosition(gpSMCurrentMerc, gpItemPointer, uiHandPos, FALSE))
					{
						// First, deal with the zipper
						if(gpSMCurrentMerc->flags.ZipperFlag)
							if(!ChangeZipperStatus(gpSMCurrentMerc, FALSE))
								return;
						if(gpSMCurrentMerc->flags.DropPackFlag)
							gpSMCurrentMerc->flags.DropPackFlag = FALSE;
						RenderBackpackButtons(ACTIVATE_BUTTON);	/* CHRISL: Needed for new inventory backpack buttons */
					}
				}

				// remember the item type currently in the item pointer
				usItemPrevInItemPointer = gpItemPointer->usItem;

				if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
				{
					// If it's just been purchased or repaired, mark it as a "new item"
					fNewItem = ( BOOLEAN ) ( gMoveingItem.uiFlags & ( ARMS_INV_JUST_PURCHASED | ARMS_INV_ITEM_REPAIRED ) );
				}

				// try to place the item in the cursor into this inventory slot
				if ( UIHandleItemPlacement( (UINT8) uiHandPos, usOldItemIndex, usNewItemIndex, fDeductPoints ) )
				{
					iLastHandPos = uiHandPos;//dnl ch66 070913
					//Jenilee: pay the price
					//just make sure to handle that if we are putting it back in the SAME slot, the cost should be 0!!!
					gpSMCurrentMerc->bActionPoints -= usCostToMoveItem;

					RenderBackpackButtons(ACTIVATE_BUTTON);	/* CHRISL: Needed for new inventory backpack buttons */
					// it worked!	if we're in the SKI...
					if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
					{
						SetNewItem( gpSMCurrentMerc, ( UINT8 ) uiHandPos, fNewItem );

						// and the cursor is now empty
						if( gpItemPointer == NULL )
						{
							// clean up
							gMoveingItem.initialize();
							SetSkiCursor( CURSOR_NORMAL );
						}
						else
						{
							// if we're holding something else in the pointer now
							if ( usItemPrevInItemPointer != gpItemPointer->usItem )
							{
								// pick up item swapped out of inventory slot into cursor (don't try to sell)
								BeginSkiItemPointer( PLAYERS_INVENTORY, -1, FALSE );
							}
							else
							{
								// otherwise, leave the cursor as is, means more items were picked up at once than can be placed in this slot
								// we deal with this by leaving the remainder in the cursor, to be put down elsewhere using subsequent clicks
							}
						}
					}

					// Setup a timer....
					//guiMouseOverItemTime = GetJA2Clock( );
					//gfCheckForMouseOverItem = TRUE;
					//gbCheckForMouseOverItemPos = (INT8)uiHandPos;
				}
			}
		}

		// Flugente: we have to recheck our flashlights, as we changed items
		//gpSMCurrentMerc->usSoldierFlagMask |= SOLDIER_REDOFLASHLIGHT;
		gpSMCurrentMerc->HandleFlashLights();

		// sevenfm: update sight
		HandleSight(gpSMCurrentMerc, SIGHT_LOOK);

		// sevenfm: update morale, as we could add/remove walkman
		RefreshSoldierMorale(gpSMCurrentMerc);
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	{
		fRightDown = TRUE;
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP && fRightDown )
	{
		fRightDown = FALSE;

		// Return if empty
		if ( gpSMCurrentMerc->inv[ uiHandPos ].exists() == false )
			return;

		// CJC: OK, get source, dest guy if different, don't allow panels to be brought up
		/*
		if ( gpItemPointer && gpSMCurrentMerc->ubID != gpItemPointerSoldier->ubID )
		{
			return;
		}
		*/

		// Turn off new item glow!
		gpSMCurrentMerc->inv.bNewItemCount[ uiHandPos ] = 0;

		// Some global stuff here - for esc, etc
		// Check for # of slots in item
		// CHRISL: Use new ItemSlotLimit function if we're using the new inventory system
		UINT8 isLimit = ItemSlotLimit( &gpSMCurrentMerc->inv[ uiHandPos ], uiHandPos, gpSMCurrentMerc );

		// access description box directly if CTRL is pressed for stack items
		if( !( ( gpSMCurrentMerc->inv[ uiHandPos ].ubNumberOfObjects > 1 && isLimit > 0 ) && ( guiCurrentScreen != MAP_SCREEN ) ) || _KeyDown( CTRL ) )
		{
			if ( !InItemDescriptionBox( ) )
			{
				if ( _KeyDown(SHIFT) && gpItemPointer == NULL && Item[gpSMCurrentMerc->inv[ uiHandPos ].usItem].usItemClass == IC_GUN && (gpSMCurrentMerc->inv[ uiHandPos ])[uiHandPos]->data.gun.ubGunShotsLeft > 0 && !(Item[gpSMCurrentMerc->inv[ uiHandPos ].usItem].singleshotrocketlauncher) && !( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE ) )
				{
					EmptyWeaponMagazine( &(gpSMCurrentMerc->inv[ uiHandPos ]), &gItemPointer, uiHandPos );
					gpItemPointer = &gItemPointer;
					gpItemPointerSoldier = gpSMCurrentMerc;
				}
				else
					InitItemDescriptionBox( gpSMCurrentMerc, (UINT8)uiHandPos, ITEMDESC_START_X, ITEMDESC_START_Y, 0 );
			}
		}
		else
		{
			if ( !InItemStackPopup( )	)
			{
				//CHRISL: Set popup width based on pocket size
				INT16	invWidth;
				if(uiHandPos >= (UINT32)BIGPOCKSTART && uiHandPos < (UINT32)BIGPOCKFINAL)
					invWidth = 531;
				else if(uiHandPos >= (UINT32)MEDPOCKSTART && uiHandPos < (UINT32)MEDPOCKFINAL)
					invWidth = 412;
				else
					invWidth = 314;

				//InitItemStackPopup( gpSMCurrentMerc, (UINT8)uiHandPos, SM_ITEMDESC_START_X, SM_ITEMDESC_START_Y, SM_ITEMDESC_WIDTH, SM_ITEMDESC_HEIGHT );
				//CHRISL: In OIV mode, we don't want to offset the keyring popup.
				UINT8 inv_interface_start_x;
				if(UsingNewInventorySystem() == false)
					inv_interface_start_x = 213;
				else
					inv_interface_start_x = 248;
				InitItemStackPopup( gpSMCurrentMerc, (UINT8)uiHandPos, inv_interface_start_x + xResOffset, INV_INTERFACE_START_Y, invWidth, ( SCREEN_HEIGHT - INV_INTERFACE_START_Y ) );

			}
		}
	}
	else if (iReason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		fRightDown = FALSE;
		fLeftDown = FALSE;
	}

}
// CHRISL: Function to adjust zipper backpack button status
BOOLEAN  ChangeZipperStatus(SOLDIERTYPE *pSoldier, BOOLEAN newStatus)
{
	INT16	sAPCost;
	INT32	iBPCost = APBPConstants[BP_WORK_ZIPPER];
	INT8	bNewStance;

	//Set AP cost based on what we're doing
	sAPCost = (newStatus) ? APBPConstants[AP_CLOSE_ZIPPER] : APBPConstants[AP_OPEN_ZIPPER];
		// SANDRO - ambidextrous bonus to handle things faster
	if ( gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( pSoldier, AMBIDEXTROUS_NT ) )
		sAPCost = (INT16)(((FLOAT)sAPCost * (FLOAT)(100 - gSkillTraitValues.ubAMWorkBackpackAPsReduction) / 100.0f) + 0.5f);

	//Are we currently in combat?
	if(gTacticalStatus.uiFlags & INCOMBAT)
	{
		// If we have fewer base points then the default cost to unzip a pack, use all remaining points instead
		if(pSoldier->bInitialActionPoints < sAPCost)
			sAPCost = pSoldier->bInitialActionPoints;
		if(EnoughPoints(pSoldier, sAPCost, iBPCost, TRUE))
			DeductPoints(pSoldier, sAPCost, iBPCost);
		else
			return FALSE;
	}

	//Opening a pack?
	if(newStatus)
	{
		// Are we standing?
		bNewStance = gAnimControl[ pSoldier->usAnimState ].ubEndHeight;
		if(bNewStance == ANIM_STAND)
		{
			bNewStance = ANIM_CROUCH;
			UIHandleSoldierStanceChange( pSoldier->ubID, bNewStance );
		}
		pSoldier->flags.ZipperFlag = newStatus;
		gfUIStanceDifferent = TRUE;
	}
	// Closing a pack?
	else
	{
		pSoldier->flags.ZipperFlag = newStatus;
		gfUIStanceDifferent = TRUE;
	}
	fCharacterInfoPanelDirty = TRUE;
	fInterfacePanelDirty = DIRTYLEVEL2;

	return TRUE;
}

// CHRISL: Function to adjust droppack packpack button status
BOOLEAN ChangeDropPackStatus(SOLDIERTYPE *pSoldier, BOOLEAN newStatus)
{
	INT16	sAPCost = APBPConstants[AP_BACK_PACK];
	INT32	iBPCost = APBPConstants[BP_BACK_PACK];
	INT32	worldKey=1, iRange=0;

	// Are we dropping a pack that has the zipper open?
	if(newStatus && pSoldier->flags.ZipperFlag)
	{
		sAPCost = 0;
		if(!ChangeZipperStatus(pSoldier, FALSE))
			return FALSE;
	}
	
	// Are we currently in combat?
	if((gTacticalStatus.uiFlags & INCOMBAT) || (gTacticalStatus.fEnemyInSector))
	{
		// If we're standing over the backpack that we're trying to pick up, reset the ap cost to 0
		if(!newStatus) {
			for(unsigned int wi = 0; wi < guiNumWorldItems; wi++)
			{
				if(gWorldItems[wi].soldierID == pSoldier->ubID && gWorldItems[wi].object.exists() == true)
				{
					for (unsigned int x = 0; x < gWorldItems[wi].object.ubNumberOfObjects; ++x) {
						// Failsafe in case our LBEArray data ever goes missing.
						if(x >= LBEArray.size()){
							break;
						}
						if(gWorldItems[wi].object.IsActiveLBE(x)) {
							// sevenfm: r8399
							//if(gWorldItems[wi].object.GetLBEPointer(x)->lbeIndex != NONE) {
							LBENODE* pLBE = gWorldItems[wi].object.GetLBEPointer(x);
							if(pLBE && pLBE->lbeIndex != NONE) {
								// Found an associated backpack so figure out how far that pack is from us
								iRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, gWorldItems[wi].sGridNo );
								if(gWorldItems[wi].sGridNo == pSoldier->sGridNo)	// standing on pack - pickup regardless
								{
									// Buggler: pickup should cost the same as manually picking it up
									//sAPCost = 0;
									break;
								}
								else if(iRange < 26 && !(gTacticalStatus.uiFlags & INCOMBAT))	// should mean anything within 2 diagonal tiles while not in combat
								{
									// just break because we want to allow this with no changes
									break;
								}
								// If not, we can't pick up the item
								else
								{
									ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, NewInvMessage[NIV_CAN_NOT_PICKUP] );
									return FALSE;
								}
							}
						}
					}
					break;
				}
			}
		}
		// Do we have enough APs to complete this action?
		if(EnoughPoints(pSoldier, sAPCost, iBPCost, TRUE))
			DeductPoints(pSoldier, sAPCost, iBPCost);
		else
			return FALSE;
	}

	// Dropping a pack?
	if(newStatus)
	{
		// We need to clear any existing WorldItems that are linked with this soldier
		for(unsigned int wi = 0; wi < guiNumWorldItems; wi++)
		{
			if(gWorldItems[wi].soldierID == pSoldier->ubID)
			{
				gWorldItems[wi].soldierID = -1;
			}
		}
		MoveItemToLBEItem( pSoldier, BPACKPOCKPOS );
		AddItemToPoolAndGetIndex(pSoldier->sGridNo, &pSoldier->inv[BPACKPOCKPOS], 1, pSoldier->pathing.bLevel, 0 , -1, pSoldier->ubID, &worldKey );
		// Item successfully added to world
		if(worldKey != ITEM_NOT_FOUND)
		{
			NotifySoldiersToLookforItems( );
			DeleteObj(&pSoldier->inv[BPACKPOCKPOS]);
			pSoldier->flags.DropPackFlag = newStatus;
			gfUIStanceDifferent = TRUE;
		}
	}
	// Picking up a pack?
	else
	{
		for(unsigned int wi = 0; wi < guiNumWorldItems; wi++)
		{
			//CHRISL: There's the remote chance that a non-LBE item might get associated with a merc.  If that happens, we can
			//	have a CTD, so lets resolve that here.
			if(Item[gWorldItems[wi].object.usItem].usItemClass != IC_LBEGEAR)
				gWorldItems[wi].soldierID = -1;
			if(gWorldItems[wi].soldierID == pSoldier->ubID && gWorldItems[wi].fExists == TRUE && Item[gWorldItems[wi].object.usItem].usItemClass == IC_LBEGEAR && LoadBearingEquipment[Item[gWorldItems[wi].object.usItem].ubClassIndex].lbeClass == BACKPACK)
			{
				for (int x = 0; x < gWorldItems[wi].object.ubNumberOfObjects; ++x) {
					// Is the item we dropped in this sector and does it have an active LBENODE flag?
					//if(gWorldItems[wi].object.IsActiveLBE(x)) {
						// Is the LBENODE we're trying to pick up actually in use?
						//if(gWorldItems[wi].object.GetLBEPointer(x)->lbeIndex != NONE)
						{
							// Try to pickup the LBENODE
							if(AutoPlaceObject(pSoldier, &(gWorldItems[wi].object ), TRUE ))
							{
								RemoveItemFromPool(gWorldItems[wi].sGridNo, wi, gWorldItems[wi].ubLevel);
								pSoldier->flags.DropPackFlag = newStatus;
								gfUIStanceDifferent = TRUE;
								return TRUE;
							}
							else
							{
								ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, NewInvMessage[NIV_NO_DROP] );
								return TRUE;
							}
						}
					//}
				}
				ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, NewInvMessage[NIV_NO_PACK] );
				break;
			}
		}
		//ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, NewInvMessage[NIV_NO_PACK] );
	}

	return TRUE;
}


void MergeMessageBoxCallBack( UINT8 ubExitValue )
{
	if ( ubExitValue == MSG_BOX_RETURN_YES )
	{
		//ADB see what happens here
		DebugBreakpoint();
		gpSMCurrentMerc->inv[ gubHandPos ].AttachObject( gpItemPointerSoldier, gpItemPointer, FALSE );

		// re-evaluate repairs
		gfReEvaluateEveryonesNothingToDo = TRUE;

		if (gpItemPointer->exists() == false)
		{
			// merge item consumed
			EndItemPointer();
			fInterfacePanelDirty = DIRTYLEVEL2;
		}
	}
	else
	{
		UIHandleItemPlacement( gubHandPos, gusOldItemIndex, gusNewItemIndex, gfDeductPoints );
	}
}

BOOLEAN gfFaceContOldStatus = FALSE;

void HandleMouseOverSoldierFaceForContMove( SOLDIERTYPE *pSoldier, BOOLEAN fOn )
{
	FACETYPE *pFace;
	INT32 sGridNo = NOWHERE;

	if ( pSoldier == NULL )
	{
		return;
	}

	if ( fOn )
	{
		// Check if we are waiting to continue move...
		if ( CheckForMercContMove( pSoldier ) )
		{
			// Display 'cont' on face....
			// Get face
			pFace = &gFacesData[ pSoldier->iFaceIndex ];

			pFace->fDisplayTextOver = FACE_DRAW_TEXT_OVER;
			wcscpy( pFace->zDisplayText, TacticalStr[ CONTINUE_OVER_FACE_STR ] );

			sGridNo = pSoldier->pathing.sFinalDestination;

			if ( pSoldier->bGoodContPath )
			{
				sGridNo = pSoldier->sContPathLocation;
			}

			// While our mouse is here, draw a path!
			PlotPath( pSoldier, sGridNo, NO_COPYROUTE, PLOT, TEMPORARY, (UINT16)pSoldier->usMovementMode, NOT_STEALTH, FORWARD, pSoldier->bActionPoints );
		}
	}
	else
	{
		// Remove 'cont' on face....
		// Get face
		pFace = &gFacesData[ pSoldier->iFaceIndex ];

		pFace->fDisplayTextOver = FACE_ERASE_TEXT_OVER;

		// Erase path!
		ErasePath( TRUE );
	}

	if (fOn != gfFaceContOldStatus)
	{
		fInterfacePanelDirty = DIRTYLEVEL2;
		gfFaceContOldStatus = fOn;
	}
}

void SelectedMercButtonMoveCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	if ( gpSMCurrentMerc == NULL )
	{
		return;
	}

	if (iReason & MSYS_CALLBACK_REASON_MOVE )
	{
		HandleMouseOverSoldierFaceForContMove( gpSMCurrentMerc, TRUE );
	}
	else if ( iReason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		HandleMouseOverSoldierFaceForContMove( gpSMCurrentMerc, FALSE );
	}

}




void SelectedMercButtonCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	//SOLDIERTYPE *pVehicle;

	if ( gpSMCurrentMerc == NULL )
	{
		return;
	}

	//if we are in the shop keeper interface
	if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
		return;

	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}
	if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		// ATE: Don't if this guy can't....
		if (!gfSMDisableForItems)
		{
			if (CheckForMercContMove(gpSMCurrentMerc))
			{
				// Continue
				ContinueMercMovement(gpSMCurrentMerc);
				ErasePath(TRUE);
			}
			else
			{
				HandleLocateSelectMerc(gpSMCurrentMerc->ubID, 0);
			}
		}
	}
	//CHRISL: Change this to UP so that we don't inadvertently switch to action mode when closing Inv panel
	//else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP)
	{
		// ATE: Cannot get out by right clicking...
		//if ( gpItemPointer == NULL )
		{
			//if ( !gfSMDisableForItems || ( gfSMDisableForItems && gpItemPointer == NULL ) )
			{
				// Delete desc
				if ( InItemDescriptionBox( ) )
				{
					DeleteItemDescriptionBox( );
				}

				gfSwitchPanel = TRUE;
				gbNewPanel = TEAM_PANEL;
			}
		}
	}
}


void SelectedMercEnemyIndicatorCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	if ( gpSMCurrentMerc == NULL )
	{
		return;
	}

	//if we are in the shop keeper interface
	if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
		return;

	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}
	if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		// ATE: Don't if this guy can't....
		if ( !gfSMDisableForItems )
		{
			//if ( gpSMCurrentMerc->flags.uiStatusFlags & ( SOLDIER_DRIVER | SOLDIER_PASSENGER ) )
			//{
			//}
			//else
			{
				if (gpSMCurrentMerc->aiData.bOppCnt > 0)
				{
					CycleVisibleEnemies(gpSMCurrentMerc);
				}
				else
				{
					SelectedMercButtonCallback(pRegion, iReason);
				}
			}
		}
	}
}



void BtnStanceUpCallback(GUI_BUTTON *btn,INT32 reason)
{
	INT8 bNewStance;

	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );

		bNewStance = gAnimControl[ gpSMCurrentMerc->usAnimState ].ubEndHeight;

		if ( bNewStance == ANIM_CROUCH )
		{
			bNewStance = ANIM_STAND;
		}
		else if ( bNewStance == ANIM_PRONE )
		{
			bNewStance = ANIM_CROUCH;
		}

		UIHandleSoldierStanceChange( gpSMCurrentMerc->ubID, bNewStance );

	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{

		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}
	
}


// CHRISL: Callback functions for backpack buttons
void BtnDropPackCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
		//CHRISL: Added "SHIFT-LMB" option to drop packs for all members of current squad
		if ( _KeyDown( SHIFT ) )
		{
			INT8 bAssignment = gpSMCurrentMerc->bAssignment;
			for( int x = gTacticalStatus.Team[ OUR_TEAM ].bFirstID; x <= gTacticalStatus.Team[ OUR_TEAM ].bLastID; x++ )
			{
				/* Is DropPackFlag currently false and is there something in the backpack pocket?  If so, we haven't
				dropped a pack yet and apparently want to*/
				if(MercPtrs[x]->bAssignment == bAssignment && MercPtrs[x]->inv[BPACKPOCKPOS].exists() == true && !MercPtrs[x]->flags.DropPackFlag)
				{
					ChangeDropPackStatus(MercPtrs[x], TRUE);
				}
				/* Is DropPackFlag currently true, is nothing in the backpack pocket and have we dropped a pack?  If so, we
				must want to retreive a backpack we previously dropped.*/
				else if(MercPtrs[x]->bAssignment == bAssignment && MercPtrs[x]->inv[BPACKPOCKPOS].exists() == false && MercPtrs[x]->flags.DropPackFlag)
				{
					ChangeDropPackStatus(MercPtrs[x], FALSE);
				}
			}
		}
		else
		{
			/* Is DropPackFlag currently false and is there something in the backpack pocket?  If so, we haven't
			dropped a pack yet and apparently want to*/
			if(gpSMCurrentMerc->inv[BPACKPOCKPOS].exists() == true && !gpSMCurrentMerc->flags.DropPackFlag)
			{
				ChangeDropPackStatus(gpSMCurrentMerc, TRUE);
			}
			/* Is DropPackFlag currently true, is nothing in the backpack pocket and have we dropped a pack?  If so, we
			must want to retreive a backpack we previously dropped.*/
			else if(gpSMCurrentMerc->inv[BPACKPOCKPOS].exists() == false && gpSMCurrentMerc->flags.DropPackFlag)
			{
				ChangeDropPackStatus(gpSMCurrentMerc, FALSE);
			}
		}
	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}
}

void BtnZipperCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
		//Are we in combat, do we have a backpack on and is the pack closed? Open it
		if((gTacticalStatus.uiFlags & INCOMBAT) && gpSMCurrentMerc->inv[BPACKPOCKPOS].exists() == true && !gpSMCurrentMerc->flags.ZipperFlag)
		{
			ChangeZipperStatus(gpSMCurrentMerc, TRUE);
		}
		//Is the pack open?
		else if(gpSMCurrentMerc->flags.ZipperFlag)
		{
			ChangeZipperStatus(gpSMCurrentMerc, FALSE);
		}
		//Are we not in combat?
		else if(!(gTacticalStatus.uiFlags & INCOMBAT))
		{
			ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, NewInvMessage[NIV_ZIPPER_COMBAT] );
		}
	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}
}

void BtnUpdownCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if( reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );

		//gsInterfaceLevel = gpSMCurrentMerc->bUIInterfaceLevel;

		// Change interface level via HandleUI handler
		UIHandleChangeLevel( NULL );

		// Remember soldier's new value
		gpSMCurrentMerc->bUIInterfaceLevel = (INT8)gsInterfaceLevel;
	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}

}

void BtnClimbCallback(GUI_BUTTON *btn,INT32 reason)
{
	BOOLEAN						fNearHeigherLevel;
	BOOLEAN						fNearLowerLevel;
	INT8							bDirection;

	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );

		if (!IsValidStance(gpSMCurrentMerc, ANIM_CROUCH))
		{
			if (gpSMCurrentMerc->IsUnconscious())
			{
				ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, gzLateLocalizedString[4], gpSMCurrentMerc->GetName());
			}
			return;
		}

		GetMercClimbDirection( gpSMCurrentMerc->ubID, &fNearLowerLevel, &fNearHeigherLevel );

		if ( fNearLowerLevel )
		{
			gpSMCurrentMerc->BeginSoldierClimbDownRoof();
		}

		if ( fNearHeigherLevel )
		{
			gpSMCurrentMerc->BeginSoldierClimbUpRoof();
		}
		
		//---------------Legion by JAzz-----------
		
		if (gGameExternalOptions.fCanClimbOnWalls)
		{ 
			if ( FindWallJumpDirection( gpSMCurrentMerc, gpSMCurrentMerc->sGridNo, gpSMCurrentMerc->ubDirection, &bDirection ) )
			{
				gpSMCurrentMerc->BeginSoldierClimbWall();
			}
		}
		
		//------------------------------------------
		// sevenfm: disabled
		/*if ( FindFenceJumpDirection( gpSMCurrentMerc, gpSMCurrentMerc->sGridNo, gpSMCurrentMerc->ubDirection, &bDirection ) )
		{
			gpSMCurrentMerc->BeginSoldierClimbFence();
		}*/
	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}

}

void BtnStanceDownCallback(GUI_BUTTON *btn,INT32 reason)
{
	INT8 bNewStance;

	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );

		bNewStance = gAnimControl[ gpSMCurrentMerc->usAnimState ].ubEndHeight;

		if ( bNewStance == ANIM_STAND )
		{
			bNewStance = ANIM_CROUCH;
		}
		else if ( bNewStance == ANIM_CROUCH )
		{
			bNewStance = ANIM_PRONE;
		}

		UIHandleSoldierStanceChange( gpSMCurrentMerc->ubID, bNewStance );

	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}

}

void BtnStealthModeCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );

		gpSMCurrentMerc->bStealthMode = !	gpSMCurrentMerc->bStealthMode;
		gfUIStanceDifferent		= TRUE;
		gfPlotNewMovement			= TRUE;
		fInterfacePanelDirty	= DIRTYLEVEL2;

		// stop shadow hiding
		//gpSMCurrentMerc->UpdateShadowHiding();
	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}

}


void BtnHandCursorCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		ToggleHandCursorMode( &guiCurrentEvent );
	}

}

void BtnTalkCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
			ToggleTalkCursorMode( &guiCurrentEvent );
	}

}

void BtnMuteCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		if ( gpSMCurrentMerc->flags.uiStatusFlags & SOLDIER_MUTE )
		{
			ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, TacticalStr[ MUTE_OFF_STR ], gpSMCurrentMerc->name );
			gpSMCurrentMerc->flags.uiStatusFlags &= ( ~SOLDIER_MUTE );
		}
		else
		{
			ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, TacticalStr[ MUTE_ON_STR ], gpSMCurrentMerc->name );
			gpSMCurrentMerc->flags.uiStatusFlags |= ( SOLDIER_MUTE );
		}
	}

}


void BtnPrevMercCallback(GUI_BUTTON *btn,INT32 reason)
{
	INT16 sID;

	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );

		sID = FindPrevActiveAndAliveMerc( gpSMCurrentMerc, TRUE, TRUE );

		gubSelectSMPanelToMerc = (UINT8)sID;

		if ( !gfInItemPickupMenu )
		{
			//if we are in the shop keeper interface
			if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
			{
				LocateSoldier( sID, DONTSETLOCATOR );
				// refresh background for player slots (in case item values change due to Flo's discount)
				gubSkiDirtyLevel = SKI_DIRTY_LEVEL2;
			}
			else
				LocateSoldier( sID, SETLOCATOR );
		}

		//if the user is in the shop keeper interface and is in the item desc
		if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
		{
			if ( InItemDescriptionBox( ) )
			{
				DeleteItemDescriptionBox( );
			}
		}
	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}

}

void BtnNextMercCallback(GUI_BUTTON *btn,INT32 reason)
{
	INT16 sID;

	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );

		sID = FindNextActiveAndAliveMerc( gpSMCurrentMerc, TRUE, TRUE );

		// Give him the panel!
		gubSelectSMPanelToMerc = (UINT8)sID;

		if ( !gfInItemPickupMenu )
		{
			//if we are in the shop keeper interface
			if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
			{
				LocateSoldier( sID, DONTSETLOCATOR );
				// refresh background for player slots (in case item values change due to Flo's discount)
				gubSkiDirtyLevel = SKI_DIRTY_LEVEL2;
			}
			else
				LocateSoldier( sID, SETLOCATOR );
		}

		//if the user is in the shop keeper interface and is in the item desc
		if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
		{
			if ( InItemDescriptionBox( ) )
			{
				DeleteItemDescriptionBox( );
			}
		}
	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}

}


void BtnOptionsCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );

		guiPreviousOptionScreen = guiCurrentScreen;
		LeaveTacticalScreen( OPTIONS_SCREEN );

	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}

}

void BtnSMDoneCallback(GUI_BUTTON *btn,INT32 reason)
{

	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );

		gfBeginEndTurn = TRUE;
	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}

}


void BtnMapScreenCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );

		// Enter mapscreen...
		//gfEnteringMapScreen = TRUE;
		GoToMapScreenFromTactical();
	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}
}

void BtnSkillMenuCallback(GUI_BUTTON *btn, INT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN)
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON);

		if (gusSelectedSoldier != NOBODY && MercPtrs[gusSelectedSoldier] && !TileIsOutOfBounds(MercPtrs[gusSelectedSoldier]->sGridNo))
		{
			LocateGridNo(MercPtrs[gusSelectedSoldier]->sGridNo);
			TraitsMenu(MercPtrs[gusSelectedSoldier]->sGridNo);
		}
	}
	else if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE)
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON);
	}
}

extern void HandleTBSectorMenu(void);

void BtnSectorMenuCallback(GUI_BUTTON *btn, INT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN)
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON);

		if (gusSelectedSoldier != NOBODY)
		{
			//HandleTacticalFunctionSelection(MercPtrs[gusSelectedSoldier]);
			HandleTBSectorMenu();
		}
	}
	else if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE)
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON);
	}
}

/*
uiTempVObject[0] = LoadButtonImage( "Interface\\InventoryButtons.sti", -1, 7, -1, -1, -1 );
uiTempVObject[1] = UseLoadedButtonImage( uiTempVObject[0], -1, 17, -1, -1, -1 );
uiTempVObject[2] = UseLoadedButtonImage( uiTempVObject[0], -1, 26, -1, -1, -1 );
iButtonID = QuickCreateButton( uiTempVObject[0], 0, 0, BUTTON_NO_TOGGLE, MSYS_PRIORITY_HIGH, MSYS_NO_CALLBACK, CycleCallback );

void CycleCallback( GUI_BUTTON *btn, INT32 reason )
{
	static INT32 buttonValue = 0;
	if( reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		buttonValue += buttonValue < 2 ? 1 : -2;
		btn->ImageNum = uiTempVObject[ buttonValue ];
		btn->uiFlags |= BUTTON_DIRTY;
	}
}
*/

void BtnBurstModeCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		ChangeWeaponMode( gpSMCurrentMerc );
//		btn->ImageNum = iBurstButtonImages[ gpSMCurrentMerc->bWeaponMode ];
//		btn->uiFlags |= BUTTON_DIRTY;

	}

}

void BtnLookCallback(GUI_BUTTON *btn,INT32 reason)
{

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );

		ToggleLookCursorMode( NULL );

	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}

}

void BtnPositionShowCallback(GUI_BUTTON *btn,INT32 reason)
{

}

BOOLEAN InitializeTEAMPanelCoords( )
{
	TM_FACE_WIDTH		= 48;
	TM_FACE_HEIGHT		= 43;
	TM_APPANEL_HEIGHT	= 56;
	TM_APPANEL_WIDTH	= 16;

	TM_ENDTURN_X				=	xResOffset + (xResSize - 131);
	TM_ROSTERMODE_X				=	xResOffset + (xResSize - 131);
	TM_DISK_X					=	xResOffset + (xResSize - 131);
	// CHRISL: New definitions for the team panel clock and location coordinates
	INTERFACE_CLOCK_TM_X	= xResOffset + (xResSize - 86 );
	INTERFACE_CLOCK_TM_Y	= ( 96	+ INTERFACE_START_Y );
	LOCATION_NAME_TM_X		= xResOffset + (xResSize - 92 );
	LOCATION_NAME_TM_Y		= ( 65	+ INTERFACE_START_Y	);
		
	TM_ENDTURN_Y		= ( 9 + INTERFACE_START_Y );
	TM_ROSTERMODE_Y	= ( 45 + INTERFACE_START_Y );
	TM_DISK_Y			= ( 81 + INTERFACE_START_Y );

	TM_NAME_WIDTH		= 60;
	TM_NAME_HEIGHT		= 9;
	TM_LIFEBAR_WIDTH	= 3;
	TM_LIFEBAR_HEIGHT	= 42;
	TM_FACEHIGHTL_WIDTH = 84;
	TM_FACEHIGHTL_HEIGHT = 114;
	TM_AP_HEIGHT		= 10;
	TM_AP_WIDTH		= 15;

	TM_INV_WIDTH		= 58;
	TM_INV_HEIGHT		= 23;
	TM_INV_HAND1STARTX = ( 8 + INTERFACE_START_X );
	TM_INV_HAND1STARTY = ( 67 + INTERFACE_START_Y );
	TM_INV_HAND2STARTX = ( 8 + INTERFACE_START_X );
	TM_INV_HAND2STARTY = ( 93 + INTERFACE_START_Y );
	TM_INV_HAND_SEP	= 83;
	TM_INV_HAND_SEPY	= 24;

	TM_BARS_REGION_HEIGHT	= 47;
	TM_BARS_REGION_WIDTH	= 26;

	INDICATOR_BOX_WIDTH	= 12;
	INDICATOR_BOX_HEIGHT = 10;

	for (int idx = 0; idx < TeamInterfaceMax/2; idx+=1) {
		const int evenIdx = idx * 2;
		Assert(evenIdx < TeamInterfaceMax);
		const int oddIdx = idx*2+1;
		Assert(oddIdx < TeamInterfaceMax);
		sTEAMAPPanelXY[evenIdx] = ( 69+idx*83	+ INTERFACE_START_X );		sTEAMAPPanelXY[oddIdx] = ( 6 + INTERFACE_START_Y );
		sTEAMFacesXY[evenIdx] = ( 13+idx*83+(idx>0) + INTERFACE_START_X );	sTEAMFacesXY[oddIdx] = ( 6 + INTERFACE_START_Y );
		sTEAMNamesXY[evenIdx] = ( 7+idx*83 + INTERFACE_START_X );			sTEAMNamesXY[oddIdx] = ( 55 + INTERFACE_START_Y );
		sTEAMFaceHighlXY[evenIdx] = ( 4+idx*83 + INTERFACE_START_X );		sTEAMFaceHighlXY[oddIdx] = ( 2 + INTERFACE_START_Y );
		sTEAMLifeXY[evenIdx] = ( 69+idx*83 + INTERFACE_START_X );			sTEAMLifeXY[oddIdx] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
		sTEAMBreathXY[evenIdx] = ( 75+idx*83 + INTERFACE_START_X );			sTEAMBreathXY[oddIdx] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
		sTEAMMoraleXY[evenIdx] = ( 81+idx*83 + INTERFACE_START_X );			sTEAMMoraleXY[oddIdx] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
		sTEAMApXY[evenIdx] = ( 68+idx*83 + INTERFACE_START_X );				sTEAMApXY[oddIdx] = ( 52 + INTERFACE_START_Y );
		sTEAMBarsXY[evenIdx] = ( 61+idx*83+(idx>0) + INTERFACE_START_X );	sTEAMBarsXY[oddIdx] = ( 3 + INTERFACE_START_Y );
		sTEAMHandInvXY[evenIdx] = ( TM_INV_HAND1STARTX + ( idx * TM_INV_HAND_SEP ));	sTEAMHandInvXY[oddIdx] = TM_INV_HAND1STARTY;
	}

/*
	sTEAMAPPanelXY[0] = ( 69	+ INTERFACE_START_X );	sTEAMAPPanelXY[1] = ( 6 + INTERFACE_START_Y );
	sTEAMAPPanelXY[2] = ( 151 + INTERFACE_START_X );	sTEAMAPPanelXY[3] = ( 6 + INTERFACE_START_Y );
	sTEAMAPPanelXY[4] = ( 234 + INTERFACE_START_X );	sTEAMAPPanelXY[5] = ( 6 + INTERFACE_START_Y );
	sTEAMAPPanelXY[6] = ( 317 + INTERFACE_START_X );	sTEAMAPPanelXY[7] = ( 6 + INTERFACE_START_Y );
	sTEAMAPPanelXY[8] = ( 401 + INTERFACE_START_X );	sTEAMAPPanelXY[9] = ( 6 + INTERFACE_START_Y );
	sTEAMAPPanelXY[10] = ( 484 + INTERFACE_START_X );	sTEAMAPPanelXY[11] = ( 6 + INTERFACE_START_Y );

	sTEAMFacesXY[0] = ( 13 + INTERFACE_START_X );	sTEAMFacesXY[1] = ( 6 + INTERFACE_START_Y );
	sTEAMFacesXY[2] = ( 97 + INTERFACE_START_X );	sTEAMFacesXY[3] = ( 6 + INTERFACE_START_Y );
	sTEAMFacesXY[4] = ( 180 + INTERFACE_START_X );	sTEAMFacesXY[5] = ( 6 + INTERFACE_START_Y );
	sTEAMFacesXY[6] = ( 263 + INTERFACE_START_X );	sTEAMFacesXY[7] = ( 6 + INTERFACE_START_Y );
	sTEAMFacesXY[8] = ( 346 + INTERFACE_START_X );	sTEAMFacesXY[9] = ( 6 + INTERFACE_START_Y );
	sTEAMFacesXY[10] = ( 429 + INTERFACE_START_X );	sTEAMFacesXY[11] = ( 6 + INTERFACE_START_Y );

	sTEAMNamesXY[0] = ( 7 + INTERFACE_START_X );	sTEAMNamesXY[1] = ( 55 + INTERFACE_START_Y );
	sTEAMNamesXY[2] = ( 90 + INTERFACE_START_X );	sTEAMNamesXY[3] = ( 55 + INTERFACE_START_Y );
	sTEAMNamesXY[4] = ( 173 + INTERFACE_START_X );	sTEAMNamesXY[5] = ( 55 + INTERFACE_START_Y );
	sTEAMNamesXY[6] = ( 256 + INTERFACE_START_X );	sTEAMNamesXY[7] = ( 55 + INTERFACE_START_Y );
	sTEAMNamesXY[8] = ( 339 + INTERFACE_START_X );	sTEAMNamesXY[9] = ( 55 + INTERFACE_START_Y );
	sTEAMNamesXY[10] = ( 422 + INTERFACE_START_X );	sTEAMNamesXY[11] = ( 55 + INTERFACE_START_Y );

	sTEAMFaceHighlXY[0] = ( 4 + INTERFACE_START_X );	sTEAMFaceHighlXY[1] = ( 2 + INTERFACE_START_Y );
	sTEAMFaceHighlXY[2] = ( 87 + INTERFACE_START_X );	sTEAMFaceHighlXY[3] = ( 2 + INTERFACE_START_Y );
	sTEAMFaceHighlXY[4] = ( 170 + INTERFACE_START_X );	sTEAMFaceHighlXY[5] = ( 2 + INTERFACE_START_Y );
	sTEAMFaceHighlXY[6] = ( 253 + INTERFACE_START_X );	sTEAMFaceHighlXY[7] = ( 2 + INTERFACE_START_Y );
	sTEAMFaceHighlXY[8] = ( 336 + INTERFACE_START_X );	sTEAMFaceHighlXY[9] = ( 2 + INTERFACE_START_Y );
	sTEAMFaceHighlXY[10] = ( 419 + INTERFACE_START_X );	sTEAMFaceHighlXY[11] = ( 2 + INTERFACE_START_Y );

	sTEAMLifeXY[0] = ( 69 + INTERFACE_START_X );	sTEAMLifeXY[1] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMLifeXY[2] = ( 152 + INTERFACE_START_X );	sTEAMLifeXY[3] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMLifeXY[4] = ( 235 + INTERFACE_START_X );	sTEAMLifeXY[5] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMLifeXY[6] = ( 318 + INTERFACE_START_X );	sTEAMLifeXY[7] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMLifeXY[8] = ( 401 + INTERFACE_START_X );	sTEAMLifeXY[9] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMLifeXY[10] = ( 484 + INTERFACE_START_X );	sTEAMLifeXY[11] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );

	sTEAMBreathXY[0] = ( 75 + INTERFACE_START_X );	sTEAMBreathXY[1] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMBreathXY[2] = ( 158 + INTERFACE_START_X );	sTEAMBreathXY[3] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMBreathXY[4] = ( 241 + INTERFACE_START_X );	sTEAMBreathXY[5] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMBreathXY[6] = ( 324 + INTERFACE_START_X );	sTEAMBreathXY[7] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMBreathXY[8] = ( 407 + INTERFACE_START_X );	sTEAMBreathXY[9] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMBreathXY[10] = ( 490 + INTERFACE_START_X );sTEAMBreathXY[11] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );

	sTEAMMoraleXY[0] = ( 81 + INTERFACE_START_X );	sTEAMMoraleXY[1] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMMoraleXY[2] = ( 164 + INTERFACE_START_X );	sTEAMMoraleXY[3] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMMoraleXY[4] = ( 247 + INTERFACE_START_X );	sTEAMMoraleXY[5] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMMoraleXY[6] = ( 330 + INTERFACE_START_X );	sTEAMMoraleXY[7] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMMoraleXY[8] = ( 413 + INTERFACE_START_X );	sTEAMMoraleXY[9] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );
	sTEAMMoraleXY[10] = ( 496 + INTERFACE_START_X );sTEAMMoraleXY[11] = ( ( 5 + INTERFACE_START_Y ) + TM_LIFEBAR_HEIGHT );

	sTEAMApXY[0] = ( 68 + INTERFACE_START_X );	sTEAMApXY[1] = ( 52 + INTERFACE_START_Y );
	sTEAMApXY[2] = ( 152 + INTERFACE_START_X );	sTEAMApXY[3] = ( 52 + INTERFACE_START_Y );
	sTEAMApXY[4] = ( 233 + INTERFACE_START_X );	sTEAMApXY[5] = ( 52 + INTERFACE_START_Y );
	sTEAMApXY[6] = ( 317 + INTERFACE_START_X );	sTEAMApXY[7] = ( 52 + INTERFACE_START_Y );
	sTEAMApXY[8] = ( 400 + INTERFACE_START_X );	sTEAMApXY[9] = ( 52 + INTERFACE_START_Y );
	sTEAMApXY[10] = ( 483 + INTERFACE_START_X );sTEAMApXY[11] = ( 52 + INTERFACE_START_Y );

	sTEAMBarsXY[0] = ( 61 + INTERFACE_START_X );	sTEAMBarsXY[1] = ( 3 + INTERFACE_START_Y );
	sTEAMBarsXY[2] = ( 145 + INTERFACE_START_X );	sTEAMBarsXY[3] = ( 3 + INTERFACE_START_Y );
	sTEAMBarsXY[4] = ( 228 + INTERFACE_START_X );	sTEAMBarsXY[5] = ( 3 + INTERFACE_START_Y );
	sTEAMBarsXY[6] = ( 311 + INTERFACE_START_X );	sTEAMBarsXY[7] = ( 3 + INTERFACE_START_Y );
	sTEAMBarsXY[8] = ( 394 + INTERFACE_START_X );	sTEAMBarsXY[9] = ( 3 + INTERFACE_START_Y );
	sTEAMBarsXY[10] = ( 477 + INTERFACE_START_X );	sTEAMBarsXY[11] = ( 3 + INTERFACE_START_Y );

	sTEAMHandInvXY[0] = ( TM_INV_HAND1STARTX + ( 0 * TM_INV_HAND_SEP ));	sTEAMHandInvXY[1] = TM_INV_HAND1STARTY;
	sTEAMHandInvXY[2] = ( TM_INV_HAND1STARTX + ( 1 * TM_INV_HAND_SEP ));	sTEAMHandInvXY[3] = TM_INV_HAND1STARTY;
	sTEAMHandInvXY[4] = ( TM_INV_HAND1STARTX + ( 2 * TM_INV_HAND_SEP ));	sTEAMHandInvXY[5] = TM_INV_HAND1STARTY;
	sTEAMHandInvXY[6] = ( TM_INV_HAND1STARTX + ( 3 * TM_INV_HAND_SEP ));	sTEAMHandInvXY[7] = TM_INV_HAND1STARTY;
	sTEAMHandInvXY[8] = ( TM_INV_HAND1STARTX + ( 4 * TM_INV_HAND_SEP ));	sTEAMHandInvXY[9] = TM_INV_HAND1STARTY;
	sTEAMHandInvXY[10] = ( TM_INV_HAND1STARTX + ( 5 * TM_INV_HAND_SEP ));	sTEAMHandInvXY[11] = TM_INV_HAND1STARTY;
	// ufff to much copy&paste :D
*/

	return ( TRUE );
}

extern void InitRadarScreenCoords();

// TEAM PANEL!!!!!!!!!!!!!!
BOOLEAN InitializeTEAMPanel(	)
{
	VSURFACE_DESC		vs_desc;
	VOBJECT_DESC	VObjectDesc;
	UINT32					cnt, posIndex;
	static BOOLEAN	fFirstTime = TRUE;

	/* CHRISL: Add something here to reset the cursor to a pointer so we don't inadvertantly activate
	context sensitive cursor when closing the inventory panel*/
	fDisplayOverheadMap = TRUE;

	// WANNE: Make a black background color for the whole screen
	ColorFillVideoSurfaceArea( FRAME_BUFFER, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Get16BPPColor( FROMRGB( 0, 0, 0 ) ) );

//	InitializeTEAMPanelCoords( );
	// INit viewport region
	// Set global mouse regions
	// Define region for viewport
	MSYS_DefineRegion( &gViewportRegion, 0, 0 ,gsVIEWPORT_END_X, gsVIEWPORT_WINDOW_END_Y, MSYS_PRIORITY_NORMAL, VIDEO_NO_CURSOR, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK );
	MSYS_AddRegion( &gViewportRegion );

	// Load interface panels
	vs_desc.fCreateFlags = VSURFACE_CREATE_FROMFILE | VSURFACE_SYSTEM_MEM_USAGE;

	VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;

	//SQUAD10 FIX:  Use panels with more slots if SquadSize at current resolution is > 6

	if (iResolution >= _640x480 && iResolution < _800x600)
	{
		//if( FileExists("INTERFACE\\bottom_bar.png") )
			//FilenameForBPP("INTERFACE\\bottom_bar.png", VObjectDesc.ImageFile);
		//else
			FilenameForBPP("INTERFACE\\bottom_bar.sti", VObjectDesc.ImageFile);
	}
	else if (iResolution < _1024x768)
	{
		if (gGameOptions.ubSquadSize > 6)
		{
			//if( FileExists("INTERFACE\\bottom_bar_8_800x600.png") )
				//FilenameForBPP("INTERFACE\\bottom_bar_8_800x600.png", VObjectDesc.ImageFile);
			//else
				FilenameForBPP("INTERFACE\\bottom_bar_8_800x600.sti", VObjectDesc.ImageFile);
		}
		else
		{
			//if( FileExists("INTERFACE\\bottom_bar_800x600.png") )
				//FilenameForBPP("INTERFACE\\bottom_bar_800x600.png", VObjectDesc.ImageFile);
			//else
				FilenameForBPP("INTERFACE\\bottom_bar_800x600.sti", VObjectDesc.ImageFile);
		}
	}
	else
	{
		if (gGameOptions.ubSquadSize > 8)
		{
			//if( FileExists("INTERFACE\\bottom_bar_10_1024x768.png") )
				//FilenameForBPP("INTERFACE\\bottom_bar_10_1024x768.png", VObjectDesc.ImageFile);
			//else
				FilenameForBPP("INTERFACE\\bottom_bar_10_1024x768.sti", VObjectDesc.ImageFile);
		}
		else if (gGameOptions.ubSquadSize > 6)
		{
			//if( FileExists("INTERFACE\\bottom_bar_8_1024x768.png") )
				//FilenameForBPP("INTERFACE\\bottom_bar_8_1024x768.png", VObjectDesc.ImageFile);
			//else
				FilenameForBPP("INTERFACE\\bottom_bar_8_1024x768.sti", VObjectDesc.ImageFile);
		}
		else
		{
			//if( FileExists("INTERFACE\\bottom_bar_1024x768.png") )
				//FilenameForBPP("INTERFACE\\bottom_bar_1024x768.png", VObjectDesc.ImageFile);
			//else
				FilenameForBPP("INTERFACE\\bottom_bar_1024x768.sti", VObjectDesc.ImageFile);
		}
	}

	CHECKF( AddVideoObject( &VObjectDesc, &guiTEAMPanel ) );

	VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	FilenameForBPP("INTERFACE\\gold_front.sti", VObjectDesc.ImageFile);
	CHECKF( AddVideoObject( &VObjectDesc, &guiTEAMObjects ) );

	VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	FilenameForBPP("INTERFACE\\Bars.sti", VObjectDesc.ImageFile);
	CHECKF( AddVideoObject( &VObjectDesc, &guiBrownBackgroundForTeamPanel ) );

	// sevenfm: tactical panel background
	gfBackgroundPanel = FALSE;
	sprintf(VObjectDesc.ImageFile, "INTERFACE\\panel_%d.png", INTERFACE_WIDTH - 4 - INTERFACE_PANEL_RIGHT - gGameOptions.ubSquadSize * 83);
	VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	if (FileExists(VObjectDesc.ImageFile))
	{
		CHECKF(AddVideoObject(&VObjectDesc, &guiBackgroundPanel));
		gfBackgroundPanel = TRUE;
	}

	gfBackgroundLeft = FALSE;
	sprintf(VObjectDesc.ImageFile, "INTERFACE\\panel_%d_left.png", INTERFACE_START_X);
	VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	if (FileExists(VObjectDesc.ImageFile))
	{
		CHECKF(AddVideoObject(&VObjectDesc, &guiBackgroundLeft));
		gfBackgroundLeft = TRUE;
	}

	gfBackgroundRight = FALSE;
	sprintf(VObjectDesc.ImageFile, "INTERFACE\\panel_%d_right.png", INTERFACE_START_X);
	VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
	if (FileExists(VObjectDesc.ImageFile))
	{
		CHECKF(AddVideoObject(&VObjectDesc, &guiBackgroundRight));
		gfBackgroundRight = TRUE;
	}

	// Clear inv display stuff
	memset( gfTEAM_HandInvDispText, 0, sizeof( gfTEAM_HandInvDispText ) );

	// Create buttons
	CHECKF( CreateTEAMPanelButtons( ) );

	// Set viewports
	// Define region for panel
	MSYS_DefineRegion( &gTEAM_PanelRegion, 0, gsVIEWPORT_END_Y ,SCREEN_WIDTH, SCREEN_HEIGHT, MSYS_PRIORITY_NORMAL,
						CURSOR_NORMAL, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK );
	// Add region
	MSYS_AddRegion( &gTEAM_PanelRegion);

	// sevenfm: extended tactical panel
	MSYS_DefineRegion( &gTEAM_ExtTacticsRegion, INTERFACE_START_X + 4 + gGameOptions.ubSquadSize * 83, INTERFACE_START_Y, INTERFACE_START_X + INTERFACE_WIDTH - 138, INTERFACE_START_Y + INTERFACE_HEIGHT, MSYS_PRIORITY_NORMAL,
						CURSOR_NORMAL, MSYS_NO_CALLBACK, TacticalPanelCallback );
	// Add region
	MSYS_AddRegion( &gTEAM_ExtTacticsRegion);

	// sevenfm: scroll area
	/*MSYS_DefineRegion(&gTEAM_ExtScrollRegion, INTERFACE_START_X, INTERFACE_START_Y, INTERFACE_START_X + 4 + gGameOptions.ubSquadSize * 83, INTERFACE_START_Y + INTERFACE_HEIGHT, MSYS_PRIORITY_NORMAL,
		CURSOR_NORMAL, MSYS_NO_CALLBACK, ScrollPanelCallback);
	// Add region
	MSYS_AddRegion(&gTEAM_ExtScrollRegion);*/

	// sevenfm: left panel
	MSYS_DefineRegion(&gTEAM_LeftPanelRegion, 0, INTERFACE_START_Y, INTERFACE_START_X, INTERFACE_START_Y + INTERFACE_HEIGHT, MSYS_PRIORITY_NORMAL,
		CURSOR_NORMAL, MSYS_NO_CALLBACK, LeftPanelCallback);
	// Add region
	MSYS_AddRegion(&gTEAM_LeftPanelRegion);
	// Add tooltip
	/*if (gGameSettings.fOptions[TOPTION_TACTICAL_LEFT_PANEL])
	{
		SetRegionFastHelpText(&gTEAM_LeftPanelRegion, TacticalStr[TEXT_PANEL_TOOLTIP_STR]);
	}*/

	// sevenfm: right panel
	PrepareTacticalRightPanelData(2, 0);
	MSYS_DefineRegion(&gTEAM_RightPanelRegion, gsPanelWideX, gsPanelWideY, gsPanelWideX + gsPanelWideWidth, gsPanelWideY + gsPanelWideHeight, MSYS_PRIORITY_NORMAL, CURSOR_NORMAL, MSYS_NO_CALLBACK, RightPanelCallback);
	//MSYS_DefineRegion(&gTEAM_RightPanelRegion, INTERFACE_START_X + INTERFACE_WIDTH, INTERFACE_START_Y, INTERFACE_START_X + INTERFACE_WIDTH + INTERFACE_START_X, INTERFACE_START_Y + INTERFACE_HEIGHT, MSYS_PRIORITY_NORMAL, CURSOR_NORMAL, MSYS_NO_CALLBACK, RightPanelCallback);
	// Add region
	MSYS_AddRegion(&gTEAM_RightPanelRegion);
	// Add tooltip
	SetRegionFastHelpText(&gTEAM_RightPanelRegion, TacticalStr[TEXT_PANEL_TOOLTIP_STR]);

	// sevenfm: action panel
	gsTacticalActionOffset = 0;
	// Action panel
	if (gGameSettings.fOptions[TOPTION_TACTICAL_ACTION_PANEL] && !(fAllowBigRadarMap && INTERFACE_START_X >= 240))
	{
		InitializeActionPanelData();
		CreateMouseRegionsActionPanel();
	}

	for ( posIndex = 0, cnt = 0; cnt < gGameOptions.ubSquadSize; cnt++, posIndex +=2 )
	{
		MSYS_DefineRegion( &gTEAM_FaceRegions[ cnt ], sTEAMFacesXY[ posIndex ], sTEAMFacesXY[ posIndex + 1 ] ,(INT16)(sTEAMFacesXY[ posIndex ] + TM_FACE_WIDTH ), (INT16)(sTEAMFacesXY[ posIndex + 1 ] + TM_FACE_HEIGHT), MSYS_PRIORITY_NORMAL,
							MSYS_NO_CURSOR, MercFacePanelMoveCallback, MercFacePanelCallback );
		// Add region
		MSYS_AddRegion( &gTEAM_FaceRegions[ cnt ] );
		MSYS_SetRegionUserData( &gTEAM_FaceRegions[ cnt ], 0, cnt );


		MSYS_DefineRegion( &gTEAM_EnemyIndicator[ cnt ], (INT16)(sTEAMFacesXY[ posIndex ] + 1), (INT16)(sTEAMFacesXY[ posIndex + 1 ] + 1 ) ,(INT16)(sTEAMFacesXY[ posIndex ] + INDICATOR_BOX_WIDTH ), (INT16)(sTEAMFacesXY[ posIndex + 1 ] + INDICATOR_BOX_HEIGHT), MSYS_PRIORITY_NORMAL,
							MSYS_NO_CURSOR, MSYS_NO_CALLBACK, EnemyIndicatorClickCallback );
		// Add region
		MSYS_AddRegion( &gTEAM_EnemyIndicator[ cnt ] );
		MSYS_SetRegionUserData( &gTEAM_EnemyIndicator[ cnt ], 0, cnt );

		if ( IsMouseInRegion( &gTEAM_FaceRegions[ cnt ] ) )
		{
			if ( gTeamPanel[ cnt ].ubID != NOBODY )
			{
				HandleMouseOverSoldierFaceForContMove( MercPtrs[ gTeamPanel[ cnt ].ubID ], TRUE );
			}
		}

		MSYS_DefineRegion( &gTEAM_BarsRegions[ cnt ], sTEAMBarsXY[ posIndex ], sTEAMBarsXY[ posIndex + 1 ] ,(INT16)(sTEAMBarsXY[ posIndex ] + TM_BARS_REGION_WIDTH ), (INT16)(sTEAMBarsXY[ posIndex + 1 ] + TM_BARS_REGION_HEIGHT ), MSYS_PRIORITY_NORMAL,
							MSYS_NO_CURSOR, MSYS_NO_CALLBACK, MercFacePanelCallback );
		// Add region
		MSYS_AddRegion( &gTEAM_BarsRegions[ cnt ] );
		MSYS_SetRegionUserData( &gTEAM_BarsRegions[ cnt ], 0, cnt );

		MSYS_DefineRegion( &gTEAM_LeftBarsRegions[ cnt ], (INT16)(sTEAMFacesXY[ posIndex ] - 8 ), sTEAMFacesXY[ posIndex + 1 ] ,(INT16)(sTEAMFacesXY[ posIndex ] ), (INT16)(sTEAMFacesXY[ posIndex + 1 ] + TM_BARS_REGION_HEIGHT ), MSYS_PRIORITY_NORMAL,
							MSYS_NO_CURSOR, MSYS_NO_CALLBACK, MercFacePanelCallback );
		// Add region
		MSYS_AddRegion( &gTEAM_LeftBarsRegions[ cnt ] );
		MSYS_SetRegionUserData( &gTEAM_LeftBarsRegions[ cnt ], 0, cnt );

		MSYS_DefineRegion( &gTEAM_FirstHandInv[ cnt ], sTEAMHandInvXY[ posIndex ], sTEAMHandInvXY[ posIndex + 1 ], (INT16)(sTEAMHandInvXY[ posIndex ] + TM_INV_WIDTH ) ,(INT16)(sTEAMHandInvXY[ posIndex + 1 ] + TM_INV_HEIGHT ), MSYS_PRIORITY_NORMAL,
							MSYS_NO_CURSOR, MSYS_NO_CALLBACK, TMClickFirstHandInvCallback );
		// Add region
		MSYS_AddRegion( &gTEAM_FirstHandInv[ cnt ] );
		MSYS_SetRegionUserData( &gTEAM_FirstHandInv[ cnt ], 0, cnt );

		MSYS_DefineRegion( &gTEAM_SecondHandInv[ cnt ], sTEAMHandInvXY[ posIndex ], (INT16)( sTEAMHandInvXY[ posIndex + 1 ] + 24 ), (INT16)(sTEAMHandInvXY[ posIndex ] + TM_INV_WIDTH) ,(INT16)(sTEAMHandInvXY[ posIndex + 1 ] + TM_INV_HEIGHT + 24 ), MSYS_PRIORITY_NORMAL,
							MSYS_NO_CURSOR, MSYS_NO_CALLBACK, TMClickSecondHandInvCallback );
		// Add region
		MSYS_AddRegion( &gTEAM_SecondHandInv[ cnt ] );
		MSYS_SetRegionUserData( &gTEAM_SecondHandInv[ cnt ], 0, cnt );
	}

	// sevenfm: log info
	/*PrepareTacticalPanelData(FALSE, FALSE, 0, 0);
	CHAR16 buf[1024];
	swprintf(buf, 1024, L"Init panel: x=%d y=%d width=%d height=%d extra=%d", gsPanelWideX, gsPanelWideY, gsPanelWideWidth, gsPanelWideHeight, INTERFACE_START_X);
	AddTacticalDebugMessage(buf);*/

	//DEfine region for selected guy panel
	//MSYS_DefineRegion( &gSM_SELMERCPanalRegion, SM_SELMERC_FACE_X, SM_SELMERC_FACE_Y, SM_SELMERC_FACE_X + SM_SELMERC_FACE_WIDTH, SM_SELMERC_FACE_Y + SM_SELMERC_FACE_HEIGHT, MSYS_PRIORITY_NORMAL,
	//					CURSOR_NORMAL, MSYS_NO_CALLBACK, SelectedMercButtonCallback );
	// Add region
	//MSYS_AddRegion( &gSM_SELMERCPanalRegion );

	// sevenfm: try to enable big radar screen
	if (fAllowBigRadarMap && INTERFACE_START_X >= 240)
		gfRenderSquadList = TRUE;
	else
		gfRenderSquadList = FALSE;

	InitRadarScreenCoords();
	MoveRadarScreen();

	// sevenfm: clear left/right widescreen panels
	if (INTERFACE_START_X > 0)
	{
		ColorFillVideoSurfaceArea(FRAME_BUFFER, 0, INV_INTERFACE_START_Y, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
		ColorFillVideoSurfaceArea(guiSAVEBUFFER, 0, INV_INTERFACE_START_Y, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	}
	
	return( TRUE );
}

BOOLEAN ShutdownTEAMPanel( )
{
	UINT32					cnt;

	// All buttons and regions and video objects and video surfaces will be deleted at shutdown of SGM
	// We may want to delete them at the interm as well, to free up room for other panels
	// Delete video objects!
	DeleteVideoObjectFromIndex( guiTEAMPanel );
	DeleteVideoObjectFromIndex( guiTEAMObjects );
	DeleteVideoObjectFromIndex( guiBrownBackgroundForTeamPanel );

	// sevenfm: tactical panel background
	if (gfBackgroundPanel)
		DeleteVideoObjectFromIndex(guiBackgroundPanel);
	if (gfBackgroundLeft)
		DeleteVideoObjectFromIndex(guiBackgroundLeft);
	if (gfBackgroundRight)
		DeleteVideoObjectFromIndex(guiBackgroundRight);

	MSYS_RemoveRegion(&gTEAM_PanelRegion);
	
	MSYS_RemoveRegion(&gTEAM_ExtTacticsRegion);
	//MSYS_RemoveRegion(&gTEAM_ExtScrollRegion);
	MSYS_RemoveRegion(&gTEAM_LeftPanelRegion);
	MSYS_RemoveRegion(&gTEAM_RightPanelRegion);
	
	DestroyMouseRegionsActionPanel();

	// Remove viewport
	MSYS_RemoveRegion( &gViewportRegion );

	for ( cnt = 0; cnt < NUMBER_OF_SOLDIERS_PER_SQUAD; cnt++ )
	{
		MSYS_RemoveRegion( &gTEAM_EnemyIndicator[ cnt ] );
		MSYS_RemoveRegion( &gTEAM_FaceRegions[ cnt ] );
		MSYS_RemoveRegion( &gTEAM_BarsRegions[ cnt ] );
		MSYS_RemoveRegion( &gTEAM_LeftBarsRegions[ cnt ] );
		MSYS_RemoveRegion( &gTEAM_FirstHandInv[ cnt ] );
		MSYS_RemoveRegion( &gTEAM_SecondHandInv[ cnt ] );

		if ( gTeamPanel[ cnt ].ubID != NOBODY )
		{
			HandleMouseOverSoldierFaceForContMove( MercPtrs[ gTeamPanel[ cnt ].ubID ], FALSE );
		}
	}

	// Remove buttons
	RemoveTEAMPanelButtons( );

	gfRenderSquadList = FALSE;

	// turn off the display of the squad list
	if( gfRenderRadarScreen == FALSE )
	{
		// start rendering radar region again,
		gfRenderRadarScreen = TRUE;

		// remove squad panel
		CreateDestroyMouseRegionsForSquadList( );
	}
	return( TRUE );
}


void RenderTEAMPanel( BOOLEAN fDirty )
{
	INT16	sFontX, sFontY;
	UINT32	cnt, posIndex;
	SOLDIERTYPE	*pSoldier;
	static CHAR16	pStr[1024], pMoraleStr[64], sTemp[64];

	if ( fDirty == DIRTYLEVEL2 )
	{
		MarkAButtonDirty( iTEAMPanelButtons[ TEAM_DONE_BUTTON ] );
		MarkAButtonDirty( iTEAMPanelButtons[ TEAM_MAP_SCREEN_BUTTON ] );
		MarkAButtonDirty( iTEAMPanelButtons[ CHANGE_SQUAD_BUTTON ] );

		// Blit video surface
		BltVideoObjectFromIndex( guiSAVEBUFFER, guiTEAMPanel, 0, INTERFACE_START_X, INTERFACE_START_Y, VO_BLT_SRCTRANSPARENCY, NULL );
		RestoreExternBackgroundRect( INTERFACE_START_X, INTERFACE_START_Y, SCREEN_WIDTH - INTERFACE_START_X , INTERFACE_HEIGHT );

		// sevenfm: always update squad list background if squad list active
		if(gfRenderSquadList)
		{
			VOBJECT_DESC	VObjectDesc;
			HVOBJECT hHandle;
			UINT32 uiHandle;

			VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
			FilenameForBPP("INTERFACE\\squadpanel.sti", VObjectDesc.ImageFile);
			if (AddVideoObject(&VObjectDesc, &uiHandle))
			{
				GetVideoObject(&hHandle, uiHandle);

				BltVideoObject(guiSAVEBUFFER, hHandle, 0, (xResOffset + xResSize - 102 - 1) + 2, gsVIEWPORT_END_Y, VO_BLT_SRCTRANSPARENCY, NULL);
				RestoreExternBackgroundRect((xResOffset + xResSize - 102 - 1) + 2, gsVIEWPORT_END_Y, 102, (INT16)(SCREEN_HEIGHT - gsVIEWPORT_END_Y));
				DeleteVideoObjectFromIndex(uiHandle);
			}
		}

		// sevenfm: left/right widescreen panels
		// left panel
		ColorFillVideoSurfaceArea(guiSAVEBUFFER, 0, INTERFACE_START_Y, INTERFACE_START_X, INTERFACE_START_Y + INTERFACE_HEIGHT, Get16BPPColor(FROMRGB(16, 16, 16)));
		if (gfBackgroundLeft && (!gGameSettings.fOptions[TOPTION_TACTICAL_LEFT_PANEL] || UseLeftActionPanel()))
		{
			BltVideoObjectFromIndex(guiSAVEBUFFER, guiBackgroundLeft, 0, 0, INTERFACE_START_Y, VO_BLT_SRCTRANSPARENCY, NULL);
		}
		RestoreExternBackgroundRect(0, INTERFACE_START_Y, INTERFACE_START_X, INTERFACE_HEIGHT);

		// right panel
		ColorFillVideoSurfaceArea(guiSAVEBUFFER, INTERFACE_START_X + INTERFACE_WIDTH, INTERFACE_START_Y, INTERFACE_START_X + INTERFACE_WIDTH * 2, INTERFACE_START_Y + INTERFACE_HEIGHT, Get16BPPColor(FROMRGB(16, 16, 16)));
		if (gfBackgroundRight && !gGameSettings.fOptions[TOPTION_TACTICAL_ACTION_PANEL])
		{
			BltVideoObjectFromIndex(guiSAVEBUFFER, guiBackgroundRight, 0, INTERFACE_START_X + INTERFACE_WIDTH, INTERFACE_START_Y, VO_BLT_SRCTRANSPARENCY, NULL);
		}
		RestoreExternBackgroundRect(INTERFACE_START_X + INTERFACE_WIDTH, INTERFACE_START_Y, INTERFACE_START_X, INTERFACE_HEIGHT);

		// LOOP THROUGH ALL MERCS ON TEAM PANEL
		for ( cnt = 0, posIndex = 0; cnt < gGameOptions.ubSquadSize; cnt++, posIndex+= 2 )
		{
			// GET SOLDIER
			if ( !gTeamPanel[ cnt ].fOccupied )
			{
				//BLIT CLOSE PANEL
				BltVideoObjectFromIndex( guiSAVEBUFFER, guiCLOSE, 5, sTEAMFacesXY[ posIndex ], sTEAMFacesXY[ posIndex + 1 ],	VO_BLT_SRCTRANSPARENCY, NULL );
				RestoreExternBackgroundRect( sTEAMFacesXY[ posIndex ], sTEAMFacesXY[ posIndex + 1 ], TM_FACE_WIDTH, TM_FACE_HEIGHT );

				// BLIT AP CLOSE PANEL
				//BltVideoObjectFromIndex( FRAME_BUFFER, guiTEAMObjects, 3, sTEAMAPPanelXY[ posIndex ], sTEAMAPPanelXY[ posIndex + 1 ], VO_BLT_SRCTRANSPARENCY, NULL );
				//RestoreExternBackgroundRect( (INT16)(sTEAMAPPanelXY[ posIndex ]), (INT16)(sTEAMAPPanelXY[ posIndex + 1 ]), TM_APPANEL_WIDTH, TM_APPANEL_HEIGHT );
				//InvalidateRegion( (INT16)(sTEAMAPPanelXY[ posIndex ]), (INT16)(sTEAMAPPanelXY[ posIndex + 1 ]), (INT16)(sTEAMAPPanelXY[ posIndex ] + TM_APPANEL_WIDTH), (INT16)(sTEAMAPPanelXY[ posIndex + 1 ] + TM_APPANEL_HEIGHT ) );

				if ( gTacticalStatus.ubCurrentTeam != OUR_TEAM || INTERRUPT_QUEUED )
				{
					// Hatch out...
					BltVideoObjectFromIndex( guiSAVEBUFFER, guiTEAMObjects, 1, sTEAMFaceHighlXY[ posIndex ], sTEAMFaceHighlXY[ posIndex + 1 ], VO_BLT_SRCTRANSPARENCY, NULL );
					RestoreExternBackgroundRect( sTEAMFaceHighlXY[ posIndex ], sTEAMFaceHighlXY[ posIndex + 1 ], TM_FACEHIGHTL_WIDTH, TM_FACEHIGHTL_HEIGHT );
				}

			}
			else
			{
				pSoldier = MercPtrs[ gTeamPanel[ cnt ].ubID ];

				if ( pSoldier->flags.uiStatusFlags & ( SOLDIER_DRIVER ) )
				{
					// Get soldier pointer for vehicle.....
					SOLDIERTYPE *pVehicle = GetSoldierStructureForVehicle( pSoldier->iVehicleId );

					// WDS 07/02/3008 - Fix bug of getting into vehicle you don't control
					Assert (pVehicle != 0);

					//OK, for each item, set dirty text if applicable!
					swprintf( pStr, TacticalStr[ DRIVER_POPUPTEXT ], pVehicle->stats.bLife, pVehicle->stats.bLifeMax, pVehicle->bBreath, pVehicle->bBreathMax );
					SetRegionFastHelpText( &(gTEAM_FirstHandInv[ cnt ]), pStr );
				}
				// Add text for first hand popup
				else
				{
					GetHelpTextForItem( pStr, &( pSoldier->inv[ HANDPOS ] ), pSoldier );

					//OK, for each item, set dirty text if applicable!
					SetRegionFastHelpText( &(gTEAM_FirstHandInv[ cnt ]), pStr );
				}

				// Add text for second hand popup
				if ( pSoldier->flags.uiStatusFlags & ( SOLDIER_PASSENGER | SOLDIER_DRIVER ) )
				{
					//OK, for each item, set dirty text if applicable!
					SetRegionFastHelpText( &(gTEAM_SecondHandInv[ cnt ]), TacticalStr[ EXIT_VEHICLE_POPUPTEXT ] );
				}
				else
				{
					GetHelpTextForItem( pStr, &( pSoldier->inv[ SECONDHANDPOS ]	), pSoldier );

					//OK, for each item, set dirty text if applicable!
					SetRegionFastHelpText( &(gTEAM_SecondHandInv[ cnt ]), pStr );
				}

				// Render Selected guy if selected
				if ( gusSelectedSoldier == pSoldier->ubID && gTacticalStatus.ubCurrentTeam == OUR_TEAM && OK_INTERRUPT_MERC( pSoldier ) )
				{
					//if(gbPixelDepth==16)
					//{
						BltVideoObjectFromIndex( guiSAVEBUFFER, guiTEAMObjects, 0, sTEAMFaceHighlXY[ posIndex ], sTEAMFaceHighlXY[ posIndex + 1 ], VO_BLT_SRCTRANSPARENCY, NULL );
						RestoreExternBackgroundRect( sTEAMFaceHighlXY[ posIndex ], sTEAMFaceHighlXY[ posIndex + 1 ], TM_FACEHIGHTL_WIDTH, TM_FACEHIGHTL_HEIGHT );

						// Render faceplate
						//BltVideoObjectFromIndex( guiSAVEBUFFER, guiTEAMObjects, 1, sTEAMFacesXY[ posIndex ] + TM_FACEPLATE_OFFSET_X, sTEAMFacesXY[ posIndex + 1 ] + TM_FACEPLATE_OFFSET_Y, VO_BLT_SRCTRANSPARENCY, NULL );
						//RestoreExternBackgroundRect( (INT16)(sTEAMFacesXY[ posIndex ] + TM_FACEPLATE_OFFSET_X), (INT16)(sTEAMFacesXY[ posIndex + 1 ] + TM_FACEPLATE_OFFSET_Y ), TM_FACEPLATE_WIDTH, TM_FACEPLATE_HEIGHT );
					//}
				}
				else if ( gTacticalStatus.ubCurrentTeam != OUR_TEAM || !OK_INTERRUPT_MERC( pSoldier ) )
				{
					BltVideoObjectFromIndex( guiSAVEBUFFER, guiTEAMObjects, 1, sTEAMFaceHighlXY[ posIndex ], sTEAMFaceHighlXY[ posIndex + 1 ], VO_BLT_SRCTRANSPARENCY, NULL );
					RestoreExternBackgroundRect( sTEAMFaceHighlXY[ posIndex ], sTEAMFaceHighlXY[ posIndex + 1 ], TM_FACEHIGHTL_WIDTH, TM_FACEHIGHTL_HEIGHT );
						// Render non-selection
						//BltVideoObjectFromIndex( guiSAVEBUFFER, guiTEAMObjects, 4, sTEAMFaceHighlXY[ posIndex ], sTEAMFaceHighlXY[ posIndex + 1 ], VO_BLT_SRCTRANSPARENCY, NULL );
						//RestoreExternBackgroundRect( sTEAMFaceHighlXY[ posIndex ], sTEAMFaceHighlXY[ posIndex + 1 ], TM_FACEHIGHTL_WIDTH, TM_FACEHIGHTL_HEIGHT );

						// Render faceplate
						//BltVideoObjectFromIndex( guiSAVEBUFFER, guiTEAMObjects, 1, sTEAMFacesXY[ posIndex ] + TM_FACEPLATE_OFFSET_X, sTEAMFacesXY[ posIndex + 1 ] + TM_FACEPLATE_OFFSET_Y, VO_BLT_SRCTRANSPARENCY, NULL );
						//RestoreExternBackgroundRect( (INT16)(sTEAMFacesXY[ posIndex ] + TM_FACEPLATE_OFFSET_X), (INT16)(sTEAMFacesXY[ posIndex + 1 ] + TM_FACEPLATE_OFFSET_Y ), TM_FACEPLATE_WIDTH, TM_FACEPLATE_HEIGHT );


				}

				CheckForFacePanelStartAnims( pSoldier, sTEAMFacesXY[ posIndex ], sTEAMFacesXY[ posIndex + 1 ] );

				RenderSoldierFace( pSoldier, sTEAMFacesXY[ posIndex ], sTEAMFacesXY[ posIndex + 1 ], TRUE );

				// Restore AP/LIFE POSIITONS

				// Render name!
				SetFont( BLOCKFONT2 );

				if ( pSoldier->bStealthMode )
				{
					SetFontBackground( FONT_MCOLOR_BLACK );
					SetFontForeground( FONT_MCOLOR_LTYELLOW );
				}
				else
				{
					SetFontBackground( FONT_MCOLOR_BLACK );
					SetFontForeground( FONT_MCOLOR_LTGRAY );
				}

				// RENDER ON SAVE BUFFER!
				SetFontDestBuffer( guiSAVEBUFFER, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, FALSE );
				VarFindFontCenterCoordinates( (INT16)(sTEAMNamesXY[ posIndex ] + 2 ), (INT16)(sTEAMNamesXY[ posIndex + 1 ] ), TM_NAME_WIDTH, TM_NAME_HEIGHT, BLOCKFONT2, &sFontX, &sFontY, L"%s", pSoldier->name );
				mprintf( sFontX, sFontY, L"%s", pSoldier->name );
				gprintfRestore( sFontX, sFontY, L"%s", pSoldier->name );

				/*anv - Render selected guy's extended panel*/
				if ( gusSelectedSoldier == pSoldier->ubID && 
					gTacticalStatus.ubCurrentTeam == OUR_TEAM && 
					OK_INTERRUPT_MERC( pSoldier ) )
				{
					if (gGameSettings.fOptions[TOPTION_TACTICAL_PANEL])
					{
						// tactical panel
						PrepareTacticalPanelData(FALSE, FALSE, 0, 0);

						if (gfBackgroundPanel)
						{
							BltVideoObjectFromIndex(guiSAVEBUFFER, guiBackgroundPanel, 0, gsPanelWideX, gsPanelWideY, VO_BLT_SRCTRANSPARENCY, NULL);
							RestoreExternBackgroundRect(gsPanelWideX, gsPanelWideY, gsPanelTotalWidth, gsPanelWideHeight);
						}

						// sevenfm: render tactical panel
						if (pSoldier && pSoldier->bActive && pSoldier->bInSector)
						{
							DrawTacticalPanel(pSoldier);
						}
					}					
				}				
				
				// reset to frame buffer!
				SetFontDestBuffer( FRAME_BUFFER, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, FALSE );
			}
		}

		// sevenfm: render left text panel
		if (gGameSettings.fOptions[TOPTION_TACTICAL_LEFT_PANEL] && !UseLeftActionPanel())
		{
			PrepareTacticalLeftPanelData(2, 2);
			DrawTacticalLeftPanel();
		}

		// sevenfm: render right action panel
		if (gGameSettings.fOptions[TOPTION_TACTICAL_ACTION_PANEL] && !(fAllowBigRadarMap && INTERFACE_START_X >= 240))
		{
			RenderActionPanel();
		}
	}

	// Loop through all mercs and make go
	for ( cnt = 0, posIndex = 0; cnt < gGameOptions.ubSquadSize; cnt++, posIndex+= 2 )
	{
		// GET SOLDIER
		if ( gTeamPanel[ cnt ].fOccupied )
		{
			pSoldier = MercPtrs[ gTeamPanel[ cnt ].ubID ];

			// Update animations....
			if ( pSoldier->flags.fClosePanel || pSoldier->flags.fClosePanelToDie )
			{
				pSoldier->sPanelFaceX = gFacesData[ pSoldier->iFaceIndex ].usFaceX;
				pSoldier->sPanelFaceY = gFacesData[ pSoldier->iFaceIndex ].usFaceY;
			}


			if ( fDirty != DIRTYLEVEL0 )
			{
				// UPdate stats!
				if ( fDirty == DIRTYLEVEL2 )
				{
					if ( pSoldier->stats.bLife != 0 )
					{
						if ( pSoldier->flags.uiStatusFlags & SOLDIER_VEHICLE )
						{
							 swprintf( pStr, TacticalStr[ VEHICLE_VITAL_STATS_POPUPTEXT ], pSoldier->stats.bLife, pSoldier->stats.bLifeMax, pSoldier->bBreath, pSoldier->bBreathMax );
							 SetRegionFastHelpText( &(gTEAM_BarsRegions[ cnt ]), pStr );
						}
						else if ( pSoldier->flags.uiStatusFlags & SOLDIER_ROBOT )
						{
							 swprintf( pStr, gzLateLocalizedString[ 16 ], pSoldier->stats.bLife, pSoldier->stats.bLifeMax );
							 SetRegionFastHelpText( &(gTEAM_BarsRegions[ cnt ]), pStr );
						}
						else
						{
							GetMoraleString( pSoldier, pMoraleStr );
			
							if ( gGameOptions.fFoodSystem && pSoldier->ubProfile != ROBOT && !IsVehicle(pSoldier) )
							{
								// Flugente: added a display for poison, only show text if actually poisoned
								if ( pSoldier->bPoisonSum > 0 )
								{
									swprintf( pStr, TacticalStr[ MERC_VITAL_STATS_WITH_POISON_AND_FOOD_POPUPTEXT ], pSoldier->stats.bLife, pSoldier->stats.bLifeMax, pSoldier->bPoisonSum, pSoldier->stats.bLifeMax, pSoldier->bBreath, pSoldier->bBreathMax, pMoraleStr, (INT32)(100*(pSoldier->bDrinkLevel - FOOD_MIN) / FOOD_HALF_RANGE), L"%", (INT32)(100*(pSoldier->bFoodLevel - FOOD_MIN) / FOOD_HALF_RANGE), L"%" );
								}
								else
								{
									swprintf( pStr, TacticalStr[ MERC_VITAL_STATS_WITH_FOOD_POPUPTEXT ], pSoldier->stats.bLife, pSoldier->stats.bLifeMax, pSoldier->bBreath, pSoldier->bBreathMax, pMoraleStr, (INT32)(100*(pSoldier->bDrinkLevel - FOOD_MIN) / FOOD_HALF_RANGE), L"%", (INT32)(100*(pSoldier->bFoodLevel - FOOD_MIN) / FOOD_HALF_RANGE), L"%" );
								}
							}
							else
							{
								// Flugente: added a display for poison, only show text if actually poisoned
								if ( pSoldier->bPoisonSum > 0 )
								{
									swprintf( pStr, TacticalStr[ MERC_VITAL_STATS_WITH_POISON_POPUPTEXT ], pSoldier->stats.bLife, pSoldier->stats.bLifeMax, pSoldier->bPoisonSum, pSoldier->stats.bLifeMax, pSoldier->bBreath, pSoldier->bBreathMax, pMoraleStr );
								}
								else
								{
									swprintf( pStr, TacticalStr[ MERC_VITAL_STATS_POPUPTEXT ], pSoldier->stats.bLife, pSoldier->stats.bLifeMax, pSoldier->bBreath, pSoldier->bBreathMax, pMoraleStr );
								}
							}

							// sevenfm: also print shock
							if (pSoldier->ShockLevelPercent() > 0)
							{
								swprintf(pStr, L"%s\n%s: %d%%", pStr, TacticalStr[MERC_VITAL_STATS_SHOCK], pSoldier->ShockLevelPercent());
							}

							SetRegionFastHelpText( &(gTEAM_BarsRegions[ cnt ]), pStr );
			
							// Buggler: skills/traits tooltip on merc portrait
							if (gGameSettings.fOptions[TOPTION_SHOW_SKILLS_ON_PORTRAIT])
							{
								// clear pStr value
								swprintf( pStr, L"");

								if (gGameOptions.fNewTraitSystem) // SANDRO - old/new traits check
								{
									UINT8 ubTempSkillArray[30];
									INT8 bNumSkillTraits = 0;

									// lets rearrange our skills to a temp array
									// we also get the number of lines (skills) to be displayed 
									for ( UINT8 ubCnt = 1; ubCnt < NUM_SKILLTRAITS_NT; ubCnt++ )
									{
										if ( ProfileHasSkillTrait( pSoldier->ubProfile, ubCnt ) == 2 )
										{
											ubTempSkillArray[bNumSkillTraits] = (ubCnt + NEWTRAIT_MERCSKILL_EXPERTOFFSET);
											bNumSkillTraits++;
										}
										else if ( ProfileHasSkillTrait( pSoldier->ubProfile, ubCnt ) == 1 )
										{
											ubTempSkillArray[bNumSkillTraits] = ubCnt;
											bNumSkillTraits++;
										}
									}

									if ( bNumSkillTraits == 0 )
									{
										swprintf( pStr, L"%s", pPersonnelScreenStrings[ PRSNL_TXT_NOSKILLS ] );
									}
									else
									{
										for ( UINT8 ubCnt = 0; ubCnt < bNumSkillTraits; ubCnt++ )
										{
											swprintf( sTemp, L"%s\n", gzMercSkillTextNew[ ubTempSkillArray[ubCnt] ] );
											wcscat( pStr, sTemp );
										}
									}
								}
								else
								{
									INT8 bSkill1 = 0, bSkill2 = 0; 	
									bSkill1 = gMercProfiles[ pSoldier->ubProfile ].bSkillTraits[0];
									bSkill2 = gMercProfiles[ pSoldier->ubProfile ].bSkillTraits[1];

									if ( bSkill1 == 0 && bSkill2 == 0 )
									{
										swprintf( pStr, L"%s", pPersonnelScreenStrings[ PRSNL_TXT_NOSKILLS ] );
									}
									else
									{
										//if the 2 skills are the same, add the '(expert)' at the end
										if( bSkill1 == bSkill2 )
										{
											swprintf( pStr, L"%s %s", gzMercSkillText[bSkill1], gzMercSkillText[EXPERT] );
										}
										else
										{
											//Display the first skill
											if( bSkill1 != 0 )
											{
												swprintf( pStr, L"%s\n", gzMercSkillText[bSkill1] );
											}
											if( bSkill2 != 0 )
											{
												swprintf( sTemp, L"%s", gzMercSkillText[bSkill2] );
												wcscat( pStr, sTemp );
											}
										}
									}
								}
								SetRegionFastHelpText( &(gTEAM_FaceRegions[ cnt ]), pStr );
							}							
						}
					}
					else
					{
						SetRegionFastHelpText( &(gTEAM_BarsRegions[ cnt ]), L"" );
						SetRegionFastHelpText( &(gTEAM_FaceRegions[ cnt ]), L"" );
					}
				}

				if ( !( pSoldier->flags.uiStatusFlags & SOLDIER_DEAD ) )
				{
					DrawLifeUIBarEx( pSoldier, sTEAMLifeXY[ posIndex ], sTEAMLifeXY[ posIndex + 1 ], TM_LIFEBAR_WIDTH, TM_LIFEBAR_HEIGHT, TRUE, FRAME_BUFFER );

					if ( !( pSoldier->flags.uiStatusFlags & SOLDIER_ROBOT ) )
					{
							DrawBreathUIBarEx( pSoldier, sTEAMBreathXY[ posIndex ], sTEAMBreathXY[ posIndex + 1 ], TM_LIFEBAR_WIDTH, TM_LIFEBAR_HEIGHT, TRUE, FRAME_BUFFER );
							DrawMoraleUIBarEx( pSoldier, sTEAMMoraleXY[ posIndex ], sTEAMMoraleXY[ posIndex + 1 ], TM_LIFEBAR_WIDTH, TM_LIFEBAR_HEIGHT, TRUE, FRAME_BUFFER );
					}

					if ( gTacticalStatus.uiFlags & TURNBASED && pSoldier->stats.bLife >= OKLIFE )
					{
						// Render APs
						SetFont( TINYFONT1 );

						//if ( pSoldier->sLastTarget != NOWHERE && !EnoughPoints( pSoldier, MinAPsToAttack( pSoldier, pSoldier->sLastTarget, TRUE ), 0, FALSE ) || GetUIApsToDisplay( pSoldier ) < 0 )
						if ( !EnoughPoints( pSoldier, MinAPsToAttack( pSoldier, pSoldier->sLastTarget, TRUE, 0 ), 0, FALSE ) || GetUIApsToDisplay( pSoldier ) < 0 )
						{
							if (MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE))
							{
								SetFontBackground(FONT_MCOLOR_BLACK);
								SetRGBFontForeground(200, 5, 100);
							}
							else
							{
								SetFontBackground(FONT_MCOLOR_BLACK);
								SetFontForeground(FONT_MCOLOR_DKRED);
							}							
						}
						else
						{
							if (MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE))
							{
								SetFontBackground( FONT_MCOLOR_BLACK );
								//SetFontForeground( FONT_MCOLOR_LTBLUE );

								// Flugente: new colour for being drugged, as blue on black was hard to see
								SetRGBFontForeground( 250, 5, 250 );
							}
							else if ( pSoldier->bStealthMode )
							{
								SetFontBackground( FONT_MCOLOR_BLACK );
								SetFontForeground( FONT_MCOLOR_LTYELLOW );
							}
							else
							{
								SetFontBackground( FONT_MCOLOR_BLACK );
								SetFontForeground( FONT_MCOLOR_LTGRAY );
							}
						}
						RestoreExternBackgroundRect(sTEAMApXY[posIndex], sTEAMApXY[posIndex + 1], TM_AP_WIDTH + 1, TM_AP_HEIGHT);

						if (gTacticalStatus.uiFlags & INCOMBAT )
						{
							VarFindFontCenterCoordinates(sTEAMApXY[posIndex], sTEAMApXY[posIndex + 1], TM_AP_WIDTH, TM_AP_HEIGHT, TINYFONT1, &sFontX, &sFontY, L"%d", GetUIApsToDisplay(pSoldier));
							mprintf( sFontX, sTEAMApXY[ posIndex + 1], L"%d", GetUIApsToDisplay( pSoldier ) );
						}
					}
				}
				else
				{
					//Erase!
					//DrawUIBar( pSoldier->bBreath, sTEAMBreathXY[ posIndex ], sTEAMBreathXY[ posIndex + 1 ], TM_LIFEBAR_WIDTH, TM_LIFEBAR_HEIGHT, DRAW_ERASE_BAR );
					//DrawUIBar( pSoldier->stats.bLife, sTEAMLifeXY[ posIndex ], sTEAMLifeXY[ posIndex + 1 ], TM_LIFEBAR_WIDTH, TM_LIFEBAR_HEIGHT, DRAW_ERASE_BAR );

					// Erase APs
					RestoreExternBackgroundRect(sTEAMApXY[posIndex], sTEAMApXY[posIndex + 1], TM_AP_WIDTH + 1, TM_AP_HEIGHT);
				}
			}

			RenderSoldierTeamInv( pSoldier, sTEAMHandInvXY[ posIndex ], sTEAMHandInvXY[ posIndex + 1 ], (UINT8)cnt, fDirty );

			HandleSoldierFaceFlash( pSoldier, sTEAMFacesXY[ posIndex ], sTEAMFacesXY[ posIndex + 1 ] );


			if ( !(guiTacticalInterfaceFlags & INTERFACE_MAPSCREEN ) )
			{
				//HandlePanelFaceAnimations( pSoldier );
			}

		}
		else
		{
			// anv: for unoccupied slots clear tooltips
			SetRegionFastHelpText( &(gTEAM_FaceRegions[ cnt ]), L"" );
			SetRegionFastHelpText( &(gTEAM_BarsRegions[ cnt ]), L"" );
		}
	}

	//if (gfRenderRadarScreen == TRUE)
	//if(gfRenderSquadList == FALSE)
	RenderClock(INTERFACE_CLOCK_TM_X, INTERFACE_CLOCK_TM_Y);
	CreateMouseRegionForPauseOfClock(INTERFACE_CLOCK_TM_X, INTERFACE_CLOCK_TM_Y);
	RenderTownIDString(LOCATION_NAME_TM_X, LOCATION_NAME_TM_Y);

	UpdateTEAMPanel( );

	//if( gfRenderRadarScreen == TRUE )
	/*if(gfRenderSquadList == FALSE)
	{*/
		// Render clock
		// CHRISL: Change coords to base off of right side of screen and use new paramters in RenderTownIDString
		/*RenderClock( INTERFACE_CLOCK_TM_X, INTERFACE_CLOCK_TM_Y );
		CreateMouseRegionForPauseOfClock( INTERFACE_CLOCK_TM_X, INTERFACE_CLOCK_TM_Y );
		RenderTownIDString( LOCATION_NAME_TM_X, LOCATION_NAME_TM_Y);*/
	/*}
	else
	{
		RemoveMouseRegionForPauseOfClock( );
	}*/
}

BOOLEAN CreateTEAMPanelButtons( )
{
CHAR8	ubString[48];

	FilenameForBPP("INTERFACE\\bottom_bar_buttons.sti", ubString);

	// Load button Graphics
	iTEAMPanelImages[ ENDTURN_IMAGES	]			= LoadButtonImage(ubString,-1,0,-1, 3,-1 );
	iTEAMPanelImages[ ROSTERMODE_IMAGES	]	= UseLoadedButtonImage( iTEAMPanelImages[ ENDTURN_IMAGES	] ,-1, 1,-1, 4,-1 );
	iTEAMPanelImages[ DISK_IMAGES	]				= UseLoadedButtonImage( iTEAMPanelImages[ ENDTURN_IMAGES	] ,-1, 2,-1, 5, -1 );

	// Create buttons
	iTEAMPanelButtons[ TEAM_DONE_BUTTON ] = QuickCreateButton( iTEAMPanelImages[ ENDTURN_IMAGES ], TM_ENDTURN_X, TM_ENDTURN_Y,
										BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
										DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnEndTurnCallback );
	if ( iTEAMPanelButtons[ TEAM_DONE_BUTTON ] == -1 )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Cannot create Interface button" );
		return( FALSE );
	}
	SetButtonFastHelpText( iTEAMPanelButtons[ TEAM_DONE_BUTTON ], TacticalStr[ END_TURN_POPUPTEXT ] );



	iTEAMPanelButtons[ TEAM_MAP_SCREEN_BUTTON ] = QuickCreateButton( iTEAMPanelImages[ ROSTERMODE_IMAGES ], TM_ROSTERMODE_X, TM_ROSTERMODE_Y,
										BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
										DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnRostermodeCallback );
	if ( iTEAMPanelButtons[ TEAM_MAP_SCREEN_BUTTON ] == -1 )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Cannot create Interface button" );
		return( FALSE );
	}
	SetButtonFastHelpText( iTEAMPanelButtons[ TEAM_MAP_SCREEN_BUTTON ], TacticalStr[ MAPSCREEN_POPUPTEXT ] );

	iTEAMPanelButtons[ CHANGE_SQUAD_BUTTON ] = QuickCreateButton( iTEAMPanelImages[ DISK_IMAGES ], TM_DISK_X, TM_DISK_Y,
										BUTTON_TOGGLE, MSYS_PRIORITY_HIGH - 1,
										DEFAULT_MOVE_CALLBACK, (GUI_CALLBACK)BtnSquadCallback );
	if ( iTEAMPanelButtons[ CHANGE_SQUAD_BUTTON ] == -1 )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Cannot create Interface button" );
		return( FALSE );
	}
	SetButtonFastHelpText( iTEAMPanelButtons[ CHANGE_SQUAD_BUTTON ], TacticalStr[ CHANGE_SQUAD_POPUPTEXT ] );

	return( TRUE );
}

void	RemoveTEAMPanelButtons( )
{
	UINT32 cnt;

	for ( cnt = 0; cnt < NUM_TEAM_BUTTONS; cnt++ )
	{
		RemoveButton( iTEAMPanelButtons[ cnt ] );
	}

	for ( cnt = 0; cnt < NUM_TEAM_BUTTON_IMAGES; cnt++ )
	{
		UnloadButtonImage( iTEAMPanelImages[ cnt ] );
	}

}

void BtnEndTurnCallback(GUI_BUTTON *btn,INT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );

		// END TURN
		UIHandleEndTurn( NULL );

	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}

}

void BtnRostermodeCallback(GUI_BUTTON *btn,INT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );

		if ( guiCurrentScreen == GAME_SCREEN )
		{
			GoToMapScreenFromTactical();
			EnableRadarScreenRender( );
		}
	}
}


// callback to handle squad switching callback
void BtnSquadCallback(GUI_BUTTON *btn,INT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );

		ToggleSquadRender( );
		if (gGameSettings.fOptions[TOPTION_TACTICAL_ACTION_PANEL])
		{
			DestroyMouseRegionsActionPanel();
			if (!fAllowBigRadarMap)
			{
				CreateMouseRegionsActionPanel();
			}
		}		
		fInterfacePanelDirty = DIRTYLEVEL2;
	}
	else if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}
}

void SetTEAMPanelCurrentMerc( UINT8 ubNewID )
{
	// Ignore ther ID given!
	// ALWYAS USE CURRENT MERC!

	fInterfacePanelDirty = DIRTYLEVEL2;

	UpdateTEAMPanel( );
}

void UpdateTEAMPanel( )
{
	INT32		cnt;

// WANNE: In editor mode, always disable the Done button, because we dont have any merc!
#ifdef JA2EDITOR
	DisableButton( iTEAMPanelButtons[ TEAM_DONE_BUTTON ] );
#else
	if ( (gTacticalStatus.ubCurrentTeam != gbPlayerNum) || (gTacticalStatus.uiFlags & REALTIME ) || !(gTacticalStatus.uiFlags & INCOMBAT ) )
	{
		DisableButton( iTEAMPanelButtons[ TEAM_DONE_BUTTON ] );
	}
	else
	{
		EnableButton( iTEAMPanelButtons[ TEAM_DONE_BUTTON ] );
	}
#endif

// WANNE: In editor mode, always disable the strategy map button, because we dont have any merc!
#ifdef JA2EDITOR
	DisableButton( iTEAMPanelButtons[ TEAM_MAP_SCREEN_BUTTON ] );
#else

	if ( gTacticalStatus.uiFlags & ENGAGED_IN_CONV )
	{
		DisableButton( iTEAMPanelButtons[ TEAM_MAP_SCREEN_BUTTON ] );
	}
	else
	{
		EnableButton( iTEAMPanelButtons[ TEAM_MAP_SCREEN_BUTTON ] );
	}
#endif


	if ( gfDisableTacticalPanelButtons )
	{
		DisableButton( iTEAMPanelButtons[ TEAM_DONE_BUTTON ] );
		DisableButton( iTEAMPanelButtons[ TEAM_MAP_SCREEN_BUTTON ] );
		DisableButton( iTEAMPanelButtons[ CHANGE_SQUAD_BUTTON ] );

		// OK, disable item regions.......
		for ( cnt = 0; cnt < NUMBER_OF_SOLDIERS_PER_SQUAD; cnt++ )
		{
			MSYS_DisableRegion( &gTEAM_EnemyIndicator[ cnt ] );

			MSYS_DisableRegion( &gTEAM_FirstHandInv[ cnt ] );

			MSYS_DisableRegion( &gTEAM_SecondHandInv[ cnt ] );
		}

		//disable the radar map region
		// If NOT in overhead map
		if ( !InOverheadMap( ) )
		{
			MSYS_DisableRegion(&gRadarRegion);
		}

	}
	else
	{

// WANNE: In editor mode, always disable the squad button, because we dont have any merc!
#ifdef JA2EDITOR
		DisableButton( iTEAMPanelButtons[ CHANGE_SQUAD_BUTTON ] );
#else
		EnableButton( iTEAMPanelButtons[ CHANGE_SQUAD_BUTTON ] );
#endif

		for ( cnt = 0; cnt < NUMBER_OF_SOLDIERS_PER_SQUAD; cnt++ )
		{
			MSYS_EnableRegion( &gTEAM_EnemyIndicator[ cnt ] );

			MSYS_EnableRegion( &gTEAM_FirstHandInv[ cnt ] );

			MSYS_EnableRegion( &gTEAM_SecondHandInv[ cnt ] );
		}

		//Enable the radar map region
		MSYS_EnableRegion(&gRadarRegion);
	}
}


void HandleMouseOverTeamFaceForContMove( BOOLEAN fOn )
{
	FACETYPE *pFace;

	if ( fOn )
	{
		// Check if we are waiting to continue move...
		if ( CheckForMercContMove( gpSMCurrentMerc ) )
		{
			// Display 'cont' on face....
			// Get face
			pFace = &gFacesData[ gpSMCurrentMerc->iFaceIndex ];

			pFace->fDisplayTextOver = FACE_DRAW_TEXT_OVER;
			wcscpy( pFace->zDisplayText, TacticalStr[ CONTINUE_OVER_FACE_STR ] );

			// While our mouse is here, draw a path!
			PlotPath( gpSMCurrentMerc, gpSMCurrentMerc->pathing.sFinalDestination, NO_COPYROUTE, PLOT, TEMPORARY, (UINT16)gpSMCurrentMerc->usMovementMode, NOT_STEALTH, FORWARD, gpSMCurrentMerc->bActionPoints );
		}
	}
	else
	{
		// Remove 'cont' on face....
		// Get face
		pFace = &gFacesData[ gpSMCurrentMerc->iFaceIndex ];

		pFace->fDisplayTextOver = FACE_ERASE_TEXT_OVER;

		// Erase path!
		ErasePath( TRUE );
	}
}


void MercFacePanelMoveCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	UINT8 ubID, ubSoldierID;
	SOLDIERTYPE	*pSoldier;

	ubID = (UINT8) MSYS_GetRegionUserData( pRegion, 0 );

	// If our flags are set to do this, gofoit!
	if ( (guiTacticalInterfaceFlags & INTERFACE_MAPSCREEN ) )
	{
		return;
	}

	if ( ubID == NOBODY )
	{
		return;
	}

	// Now use soldier ID values
	ubSoldierID = gTeamPanel[ ubID ].ubID;

	if ( ubSoldierID == NOBODY )
	{
		return;
	}

	pSoldier = MercPtrs[ ubSoldierID ];

	if ( !pSoldier->bActive )
	{
		return;
	}


	if (iReason & MSYS_CALLBACK_REASON_MOVE )
	{
		HandleMouseOverSoldierFaceForContMove( pSoldier, TRUE );
	}
	else if ( iReason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		HandleMouseOverSoldierFaceForContMove( pSoldier, FALSE );
	}

}


void EnemyIndicatorClickCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	UINT8 ubID, ubSoldierID;

	// sevenfm: scroll squads
	/*if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP || iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		ScrollPanelCallback(pRegion, iReason);
		return;
	}*/

	ubID = (UINT8) MSYS_GetRegionUserData( pRegion, 0 );

	if ( ubID == NOBODY )
	{
		return;
	}

	// Now use soldier ID values
	ubSoldierID = gTeamPanel[ ubID ].ubID;

	if ( ubSoldierID == NOBODY )
	{
		return;
	}


	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}

	if ( !MercPtrs[ ubSoldierID ]->bActive )
	{
		return;
	}


	if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN)
	{
		//if ( MercPtrs[ ubSoldierID ]->flags.uiStatusFlags & ( SOLDIER_DRIVER | SOLDIER_PASSENGER ) )
		//{
		//}
		//else		
		{
			SOLDIERTYPE *pSoldier;

			pSoldier = MercPtrs[ ubSoldierID ];

			if ( pSoldier->aiData.bOppCnt > 0 )
			{	// Cycle....
				CycleVisibleEnemies( pSoldier );
			}
			else
			{
				MercFacePanelCallback( pRegion, iReason );
			}
		}
	}
}

void ScrollPanelCallback(MOUSE_REGION * pRegion, INT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}

	SOLDIERTYPE *pNewSoldier;
	INT32		iCurrentSquad;

	if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
	{
		if (gusSelectedSoldier != NOBODY)
		{
			//Select next squad
			iCurrentSquad = CurrentSquad();
			pNewSoldier = FindPrevActiveSquad(MercPtrs[gusSelectedSoldier]);
			if (pNewSoldier->bAssignment != iCurrentSquad)
			{
				HandleLocateSelectMerc(pNewSoldier->ubID, LOCATEANDSELECT_MERC);

				if (gGameExternalOptions.fUseXMLSquadNames && pNewSoldier->bAssignment < ON_DUTY)
					ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[MSG_SQUAD_ACTIVE], SquadNames[pNewSoldier->bAssignment].squadname);
				else
					ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[MSG_SQUAD_ACTIVE], pNewSoldier->bAssignment + 1);

				LocateSoldier(pNewSoldier->ubID, SETLOCATOR);
			}
		}

		fInterfacePanelDirty = DIRTYLEVEL2;
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		//ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"next squad");
		if (gusSelectedSoldier != NOBODY)
		{
			//Select next squad
			iCurrentSquad = CurrentSquad();
			pNewSoldier = FindNextActiveSquad(MercPtrs[gusSelectedSoldier]);
			if (pNewSoldier->bAssignment != iCurrentSquad)
			{
				HandleLocateSelectMerc(pNewSoldier->ubID, LOCATEANDSELECT_MERC);

				if (gGameExternalOptions.fUseXMLSquadNames && pNewSoldier->bAssignment < ON_DUTY)
					ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[MSG_SQUAD_ACTIVE], SquadNames[pNewSoldier->bAssignment].squadname);
				else
					ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[MSG_SQUAD_ACTIVE], pNewSoldier->bAssignment + 1);

				LocateSoldier(pNewSoldier->ubID, SETLOCATOR);
			}
		}

		fInterfacePanelDirty = DIRTYLEVEL2;
	}
}

void SwitchTextPanelMode(void)
{
	if (ubTacticalMessagePanelMode == LEFT_PANEL_MODE_STATISTICS)
	{
		ClearDisplayedListOfTacticalStrings();
		ubTacticalMessagePanelMode = LEFT_PANEL_MODE_MESSAGE;
		gsTacticalMessagesOffset = 0;
	}
	else if (ubTacticalMessagePanelMode == LEFT_PANEL_MODE_MESSAGE)
	{
		ubTacticalMessagePanelMode = LEFT_PANEL_MODE_HELP;
		gsTacticalMessagesOffset = 0;
	}
	else if (ubTacticalMessagePanelMode == LEFT_PANEL_MODE_HELP)
	{
		if (CHEATER_CHEAT_LEVEL())
		{
			ubTacticalMessagePanelMode = LEFT_PANEL_MODE_AI;
		}
		else
		{
			ubTacticalMessagePanelMode = LEFT_PANEL_MODE_STATISTICS;			
		}
		gsTacticalMessagesOffset = 0;
	}
	else if (ubTacticalMessagePanelMode == LEFT_PANEL_MODE_AI)
	{
		if (CHEATER_CHEAT_LEVEL())
		{
			ubTacticalMessagePanelMode = LEFT_PANEL_MODE_DEBUG;
		}
		else
		{
			ubTacticalMessagePanelMode = LEFT_PANEL_MODE_STATISTICS;
		}
		gsTacticalMessagesOffset = 0;
	}
	else if (ubTacticalMessagePanelMode == LEFT_PANEL_MODE_DEBUG)
	{		
		ubTacticalMessagePanelMode = LEFT_PANEL_MODE_STATISTICS;
		gsTacticalMessagesOffset = 0;
	}
	fInterfacePanelDirty = DIRTYLEVEL2;
}

void LeftPanelCallback(MOUSE_REGION * pRegion, INT32 iReason)
{
	if (UseLeftActionPanel())
		return;

	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}

	if (iReason & MSYS_CALLBACK_REASON_MBUTTON_DWN)
	{
		SwitchTextPanelMode();
		DisableLeftPanelHelpTooltip();
	}
	else if (iReason & MSYS_CALLBACK_REASON_MBUTTON_UP)
	{
	}
	if (iReason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	{
		SwitchTextPanelMode();
		DisableLeftPanelHelpTooltip();
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP)
	{
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
	{
		if (ubTacticalMessagePanelMode == LEFT_PANEL_MODE_DEBUG)
		{
			if (gsTacticalMessagesOffset < (INT32)gTacticalDebugMessageVector.size() - 1)
				gsTacticalMessagesOffset++;

			//gsTacticalMessagesOffset = min(gsTacticalMessagesOffset + 1, gTacticalDebugMessageVector.size());
		}
		else if (ubTacticalMessagePanelMode == LEFT_PANEL_MODE_AI)
		{
			if (gsTacticalMessagesOffset < (INT32)gTacticalAIMessageVector.size() - 1)
				gsTacticalMessagesOffset++;

			//gsTacticalMessagesOffset = min(gsTacticalMessagesOffset + 1, gTacticalAIMessageVector.size());
		}
		else if (ubTacticalMessagePanelMode == LEFT_PANEL_MODE_MESSAGE)
		{
			if (gsTacticalMessagesOffset < (INT32)gTacticalMessageVector.size() - 1)
				gsTacticalMessagesOffset++;

			//gsTacticalMessagesOffset = min(gsTacticalMessagesOffset + 1, gTacticalMessageVector.size());
		}
		else if (ubTacticalMessagePanelMode == LEFT_PANEL_MODE_HELP)
		{
			if (gsTacticalMessagesOffset < (INT32)gTacticalHelpMessageVector.size() - 1)
				gsTacticalMessagesOffset++;

			//gsTacticalMessagesOffset = min(gsTacticalMessagesOffset + 1, gTacticalMessageVector.size());
		}
		DisableLeftPanelHelpTooltip();
		fInterfacePanelDirty = DIRTYLEVEL2;
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		DisableLeftPanelHelpTooltip();
		gsTacticalMessagesOffset = max(0, gsTacticalMessagesOffset - 1);
		fInterfacePanelDirty = DIRTYLEVEL2;
	}
	else if (iReason & MSYS_CALLBACK_REASON_X1BUTTON_DWN)
	{
	}
	else if (iReason & MSYS_CALLBACK_REASON_X2BUTTON_DWN)
	{
	}
}

void RightPanelCallback(MOUSE_REGION * pRegion, INT32 iReason)
{
	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}

	if (iReason & MSYS_CALLBACK_REASON_MBUTTON_DWN)
	{
		//SwitchTextPanelMode();
		DisableRightPanelHelpTooltip();
	}
	else if (iReason & MSYS_CALLBACK_REASON_MBUTTON_UP)
	{
	}
	if (iReason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	{
		//SwitchTextPanelMode();
		DisableRightPanelHelpTooltip();
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP)
	{
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
	{
		PrepareTacticalRightPanelData(2, 0);

		if (gsTacticalActionOffset > 0)
			gsTacticalActionOffset--;
		
		DestroyMouseRegionsActionPanel();
		CreateMouseRegionsActionPanel();

		//ScreenMsg(FONT_LTGREEN, MSG_INTERFACE, L"offset %d", gsTacticalActionOffset);
		DisableRightPanelHelpTooltip();

		fInterfacePanelDirty = DIRTYLEVEL2;
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		PrepareTacticalRightPanelData(2, 0);

		if (gsTacticalActionOffset < (INT32)gActionDataVector.size() - gsPanelWindowHeight / GetFontHeight(TINYFONT1) ||
			(gsPanelTotalWidth >= INTERFACE_PANEL_MEDIUM && gsTacticalActionOffset < (INT32)gActionDataVector2.size() - gsPanelWindowHeight / GetFontHeight(TINYFONT1)))
			gsTacticalActionOffset++;

		DestroyMouseRegionsActionPanel();
		CreateMouseRegionsActionPanel();

		//ScreenMsg(FONT_LTBLUE, MSG_INTERFACE, L"offset %d", gsTacticalActionOffset);
		DisableRightPanelHelpTooltip();

		fInterfacePanelDirty = DIRTYLEVEL2;
	}
	else if (iReason & MSYS_CALLBACK_REASON_X1BUTTON_DWN)
	{
	}
	else if (iReason & MSYS_CALLBACK_REASON_X2BUTTON_DWN)
	{
	}
}

void TacticalPanelCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}
	
	if (iReason & MSYS_CALLBACK_REASON_MBUTTON_DWN )
	{
		SwitchTacticalPanelMode();
	}
	else if (iReason & MSYS_CALLBACK_REASON_MBUTTON_UP )
	{
	}
	if (iReason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	{
		SwitchTacticalPanelMode();
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP)
	{
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP )
	{
	}
	else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN )
	{
	}
	else if ( iReason & MSYS_CALLBACK_REASON_X1BUTTON_DWN )
	{
	}
	else if ( iReason & MSYS_CALLBACK_REASON_X2BUTTON_DWN )
	{
	}
}

void MercFacePanelCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	UINT8 ubID, ubSoldierID;
	//SOLDIERTYPE *pVehicle;

	// sevenfm: scroll squads
	/*if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP || iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		ScrollPanelCallback(pRegion, iReason);
		return;
	}*/

	ubID = (UINT8) MSYS_GetRegionUserData( pRegion, 0 );

	// If our flags are set to do this, gofoit!
	if ( (guiTacticalInterfaceFlags & INTERFACE_MAPSCREEN ) )
	{
		if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN)
		{
			SetInfoChar( ubID );
		}
		return;
	}

	if ( ubID == NOBODY )
	{
		return;
	}

	// anv: did we click on empty panel?
	if ( !gTeamPanel[ ubID ].fOccupied )
	{
		return;
	}

	// Now use soldier ID values
	ubSoldierID = gTeamPanel[ ubID ].ubID;

	if ( ubSoldierID == NOBODY )
	{
		return;
	}

	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}

	if ( !MercPtrs[ ubSoldierID ]->bActive )
	{
		return;
	}

	// sevenfm: disable click if not accessible during interrupt
	if (!OK_INTERRUPT_MERC(MercPtrs[ubSoldierID]))
	{
		return;
	}

	if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN)
	{
		if ( !gfInItemPickupMenu && gpItemPointer == NULL )
		{
			//if ( MercPtrs[ ubSoldierID ]->flags.uiStatusFlags & ( SOLDIER_DRIVER | SOLDIER_PASSENGER ) )
			//if ( MercPtrs[ ubSoldierID ]->flags.uiStatusFlags & ( SOLDIER_DRIVER ) )
			//{
			//	pVehicle = GetSoldierStructureForVehicle( MercPtrs[ ubSoldierID ]->iVehicleId );

			//	HandleLocateSelectMerc( pVehicle->ubID, 0 );
			//}
			//else
			{
				if ( !InOverheadMap( ) )
				{
					// If we can continue a move, do so!
					if ( CheckForMercContMove( MercPtrs[ ubSoldierID ] ) )
					{
						// Continue
						ContinueMercMovement( MercPtrs[ ubSoldierID ] );
						ErasePath( TRUE );
					}
					else
					{
						// HEADROCK HAM 3.5: Shift-Click a merc's face will add him to the current selection.
						if (!(gTacticalStatus.uiFlags & INCOMBAT) && _KeyDown( SHIFT ) )
						{
							if ( ! (MercPtrs[ ubSoldierID ]->flags.uiStatusFlags & SOLDIER_MULTI_SELECTED ) )
							{
								if ( OK_CONTROLLABLE_MERC( MercPtrs[ ubSoldierID ] ) && !( MercPtrs[ ubSoldierID ]->flags.uiStatusFlags & ( SOLDIER_VEHICLE | SOLDIER_PASSENGER | SOLDIER_DRIVER ) ) )
								{
									//ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"%s added", MercPtrs[ ubSoldierID ]->name );
									MercPtrs[ gusSelectedSoldier ]->flags.uiStatusFlags |= SOLDIER_MULTI_SELECTED;
									MercPtrs[ ubSoldierID ]->flags.uiStatusFlags |= SOLDIER_MULTI_SELECTED;
									EndMultiSoldierSelection( TRUE );
								}
							}
							// A shift-click on a selected character will remove that character from the current selection.
							else
							{
								//ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"%s removed", MercPtrs[ ubSoldierID ]->name );
								MercPtrs[ ubSoldierID ]->flags.uiStatusFlags &= (~SOLDIER_MULTI_SELECTED );
								if (ubSoldierID != gusSelectedSoldier)
								{
									MercPtrs[ gusSelectedSoldier ]->flags.uiStatusFlags |= SOLDIER_MULTI_SELECTED;
								}
								EndMultiSoldierSelection( TRUE );
							}
						}
						else
						{
							//ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"%s selected", MercPtrs[ ubSoldierID ]->name );
							HandleLocateSelectMerc( ubSoldierID, 0 );
						}
					}
				}
				else
				{
					SelectSoldier( ubSoldierID, TRUE, FALSE );
				}
			}
		}
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP)
	{
		if ( !InOverheadMap( ) )
		{
			// Only if guy is not dead!
			//if ( !( MercPtrs[ ubSoldierID ]->flags.uiStatusFlags & SOLDIER_DEAD ) && !AM_AN_EPC( MercPtrs[ ubSoldierID ] ) && !( MercPtrs[ ubSoldierID ]->flags.uiStatusFlags & ( SOLDIER_DRIVER | SOLDIER_PASSENGER ) ) )
			if (!(MercPtrs[ubSoldierID]->flags.uiStatusFlags & SOLDIER_DEAD) && !AM_AN_EPC(MercPtrs[ubSoldierID]))
			{
				gfSwitchPanel = TRUE;
				gbNewPanel = SM_PANEL;
				gubNewPanelParam = (UINT8)ubSoldierID;
				// silversurfer: This needs to be reset to TRUE because inventory view only allows radar screen to be displayed and not the squad list.
				gfRenderRadarScreen = TRUE;
				gfRenderSquadList = FALSE;
			}
		}
	}
}

extern void InternalSelectSoldier(UINT16 usSoldierID, BOOLEAN fAcknowledge, BOOLEAN fForceReselect, BOOLEAN fFromUI);

void HandleLocateSelectMerc(UINT8 ubID, INT8 bFlag, BOOLEAN fClearConsidered)
{
	BOOLEAN fSelect = FALSE;

	if (fClearConsidered)
	{
		// mark all soldiers as not considered yet
		for (UINT8 cnt = gTacticalStatus.Team[gbPlayerNum].bFirstID; cnt <= gTacticalStatus.Team[gbPlayerNum].bLastID; cnt++)
		{
			if (MercPtrs[cnt] && MercPtrs[cnt]->bActive)
			{
				MercPtrs[cnt]->usSoldierFlagMask2 &= ~SOLDIER_CONSIDERED;
			}
		}
	}	

	if( !MercPtrs[ ubID ]->bActive )
	{
		return;
	}	

	if ( gpItemPointer != NULL )
	{
		// ATE: Disable for feel purposes....
		//return;
	}

	// ATE: No matter what we do... if below OKLIFE, just locate....
	if ( MercPtrs[ ubID ]->stats.bLife < OKLIFE )
	{
		LocateSoldier( ubID, SETLOCATOR );
		return;
	}	

	/*if ( _KeyDown( ALT ) )
	{
		if (gGameSettings.fOptions[TOPTION_OLD_SELECTION_METHOD])
		{
			// Select merc
			InternalSelectSoldier(ubID, TRUE, FALSE, TRUE);
			MercPtrs[ubID]->flags.fFlashLocator = FALSE;
			ResetMultiSelection();
		}
		else
		{
			// Just locate....
			LocateSoldier(ubID, SETLOCATOR);
		}
	}
	else
	{*/
		if ( MercPtrs[ ubID ]->flags.fFlashLocator == FALSE )
		{
			/*if ( gGameSettings.fOptions[ TOPTION_OLD_SELECTION_METHOD ] )
			{
				// If we are currently selected, slide to location
				if ( ubID == gusSelectedSoldier )
				{
					SlideTo( NOWHERE, gusSelectedSoldier, NOBODY ,SETLOCATOR);
				}
				else
				{
					LocateSoldier( ubID, SETLOCATOR );
				}
			}
			else
			{*/
				if ( ubID == gusSelectedSoldier )
				{
					SlideTo( NOWHERE, gusSelectedSoldier, NOBODY ,SETLOCATOR );
				}
				else
				{
					fSelect = TRUE;
					//ShowRadioLocator( ubID, SHOW_LOCATOR_NORMAL );
				}
			//}
		}
		else
		{
			/*if ( gGameSettings.fOptions[ TOPTION_OLD_SELECTION_METHOD ] )
			{
				// If we are currently selected, slide to location
				if ( ubID == gusSelectedSoldier )
				{
					SlideTo( NOWHERE, gusSelectedSoldier, NOBODY ,DONTSETLOCATOR);
				}
				else
				{
					LocateSoldier( ubID, DONTSETLOCATOR );
				}

				fSelect = TRUE;
			}
			else
			{*/
				if ( ubID == gusSelectedSoldier )
				{
					LocateSoldier( ubID, DONTSETLOCATOR );
				}
				else
				{
					fSelect = TRUE;
				}
			//}
		}

		if ( bFlag == LOCATEANDSELECT_MERC )
		{
			fSelect = TRUE;
		}

		if ( fSelect )
		{
			// Select merc, only if alive!
			if ( !( MercPtrs[ ubID ]->flags.uiStatusFlags & SOLDIER_DEAD ) )
			{
				InternalSelectSoldier(ubID, TRUE, FALSE, TRUE);
			}
		}
	//}

	ResetMultiSelection( );

	// Handle locate select merc....
	HandleMouseOverSoldierFaceForContMove( MercPtrs[ ubID ], TRUE );
}

void ShowRadioLocator( UINT8 ubID, UINT8 ubLocatorSpeed )
{
	RESETTIMECOUNTER( MercPtrs[ ubID ]->timeCounters.FlashSelCounter, FLASH_SELECTOR_DELAY );

	MercPtrs[ ubID ]->flags.fFlashLocator = TRUE;
	MercPtrs[ ubID ]->sLocatorFrame = 0;

	if ( ubLocatorSpeed == SHOW_LOCATOR_NORMAL )
	{
		MercPtrs[ ubID ]->ubNumLocateCycles = 3;
	}
	else if ( ubLocatorSpeed == SHOW_LOCATOR_NOISE )
	{
		MercPtrs[ ubID ]->ubNumLocateCycles = 1;
	}
	else
	{
		MercPtrs[ ubID ]->ubNumLocateCycles = 2;
	}
}

void EndRadioLocator( UINT8 ubID )
{
	MercPtrs[ ubID ]->flags.fFlashLocator = FALSE;
	MercPtrs[ ubID ]->flags.fShowLocator = FALSE;
}



void CheckForFacePanelStartAnims( SOLDIERTYPE *pSoldier, INT16 sPanelX, INT16 sPanelY )
{

	if ( !pSoldier->bActive )
	{
		return;
	}


	if ( pSoldier->flags.fUIdeadMerc	)
	{
//		pSoldier->sPanelFaceX	= sPanelX;
//		pSoldier->sPanelFaceY	= sPanelY;
	}


}


void FinishAnySkullPanelAnimations( )
{
	SOLDIERTYPE *pTeamSoldier;
	INT32		cnt2;

	cnt2 = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;

	// run through list
	for ( pTeamSoldier = MercPtrs[ cnt2 ]; cnt2 <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; cnt2++,pTeamSoldier++ )
	{
		if ( pTeamSoldier->bActive && pTeamSoldier->stats.bLife == 0 )
		{
			if ( pTeamSoldier->flags.fUIdeadMerc || pTeamSoldier->flags.fClosePanelToDie )
			{
				HandlePlayerTeamMemberDeathAfterSkullAnimation( pTeamSoldier );

				pTeamSoldier->flags.fUIdeadMerc = FALSE;
				pTeamSoldier->flags.fClosePanelToDie = FALSE;
			}
		}
	}

}

void HandlePanelFaceAnimations( SOLDIERTYPE *pSoldier )
{
	if ( pSoldier->bTeam != gbPlayerNum )
	{
		return;
	}


	if ( !pSoldier->bActive )
	{
		return;
	}

	if ( pSoldier->flags.uiStatusFlags & SOLDIER_VEHICLE )
	{
		// Don't do this for a vehice.
		return;
	}

	// If this is the first time we are active, open panel!
	if ( pSoldier->flags.fUInewMerc )
	{
		pSoldier->flags.fUInewMerc = FALSE;
	}

	if ( pSoldier->flags.fUICloseMerc )
	{
		pSoldier->flags.fUICloseMerc = FALSE;
	}

	if ( pSoldier->flags.fUIdeadMerc	)
	{
		pSoldier->sPanelFaceX = gFacesData[ pSoldier->iFaceIndex ].usFaceX;
		pSoldier->sPanelFaceY = gFacesData[ pSoldier->iFaceIndex ].usFaceY;

		pSoldier->flags.fUIdeadMerc = FALSE;
		pSoldier->flags.fClosePanel		= TRUE;
		pSoldier->flags.fClosePanelToDie = TRUE;
		pSoldier->ubClosePanelFrame = 0;
		pSoldier->ubDeadPanelFrame = 0;
		RESETTIMECOUNTER( pSoldier->timeCounters.PanelAnimateCounter, 160 );
	}

	if ( pSoldier->flags.fClosePanel )
	{
		if ( TIMECOUNTERDONE( pSoldier->timeCounters.PanelAnimateCounter, 160 ) )
		{
				pSoldier->ubClosePanelFrame++;

				if ( pSoldier->ubClosePanelFrame > 5 )
				{
					pSoldier->flags.fClosePanel = FALSE;
					pSoldier->ubClosePanelFrame = 5;

					if ( pSoldier->flags.fClosePanelToDie )
					{
						pSoldier->flags.fDeadPanel = TRUE;
						//PlayJA2Sample( (UINT8)HEADCR_1, RATE_11025, HIGHVOLUME, 1, MIDDLEPAN );
					}
					else
					{
						if ( !gFacesData[ pSoldier->iFaceIndex ].fDisabled )
						{
							RestoreExternBackgroundRect( pSoldier->sPanelFaceX, pSoldier->sPanelFaceY, TM_FACE_WIDTH, TM_FACE_HEIGHT );
						}
					}
				}
				RESETTIMECOUNTER( pSoldier->timeCounters.PanelAnimateCounter, 160 );
		}
	}

	if ( pSoldier->flags.fClosePanel )
	{
		// Render panel!
		//if(gbPixelDepth==16)
		//{
			if ( !gFacesData[ pSoldier->iFaceIndex ].fDisabled )
			{
				RestoreExternBackgroundRect( pSoldier->sPanelFaceX, pSoldier->sPanelFaceY, TM_FACE_WIDTH, TM_FACE_HEIGHT );
				BltVideoObjectFromIndex( FRAME_BUFFER, guiCLOSE, pSoldier->ubClosePanelFrame, pSoldier->sPanelFaceX, pSoldier->sPanelFaceY, VO_BLT_SRCTRANSPARENCY, NULL );
				InvalidateRegion( pSoldier->sPanelFaceX, pSoldier->sPanelFaceY, pSoldier->sPanelFaceX + TM_FACE_WIDTH, pSoldier->sPanelFaceY + TM_FACE_HEIGHT );
			}
		//}
	}


	if ( pSoldier->flags.fDeadPanel )
	{
		if ( TIMECOUNTERDONE(	pSoldier->timeCounters.PanelAnimateCounter, 160 ) )
		{
				pSoldier->ubDeadPanelFrame++;

				if ( pSoldier->ubDeadPanelFrame == 4 )
				{
					ScreenMsg( FONT_RED, MSG_SKULL_UI_FEEDBACK, pMercDeadString[ 0 ], pSoldier->name );

					PlayJA2Sample( (UINT8)DOORCR_1, RATE_11025, HIGHVOLUME, 1, MIDDLEPAN );
					PlayJA2Sample( (UINT8)HEADCR_1, RATE_11025, HIGHVOLUME, 1, MIDDLEPAN );
				}

				if ( pSoldier->ubDeadPanelFrame > 5 )
				{
					pSoldier->flags.fDeadPanel = FALSE;
					pSoldier->ubDeadPanelFrame = 5;
					pSoldier->flags.fClosePanelToDie = FALSE;

					// Finish!
					if ( !gFacesData[ pSoldier->iFaceIndex ].fDisabled )
					{
						BltVideoObjectFromIndex( guiSAVEBUFFER, guiDEAD, pSoldier->ubDeadPanelFrame, pSoldier->sPanelFaceX, pSoldier->sPanelFaceY, VO_BLT_SRCTRANSPARENCY, NULL );

						// Blit hatch!
						BltVideoObjectFromIndex( guiSAVEBUFFER, guiHATCH, 0, pSoldier->sPanelFaceX, pSoldier->sPanelFaceY, VO_BLT_SRCTRANSPARENCY, NULL );

						RestoreExternBackgroundRect( pSoldier->sPanelFaceX, pSoldier->sPanelFaceY, TM_FACE_WIDTH, TM_FACE_HEIGHT );
					}
					HandlePlayerTeamMemberDeathAfterSkullAnimation( pSoldier );

				}
				RESETTIMECOUNTER( pSoldier->timeCounters.PanelAnimateCounter, 160 );
		}
	}

	if ( pSoldier->flags.fDeadPanel )
	{
		// Render panel!
		//if(gbPixelDepth==16)
		//{
				if ( !gFacesData[ pSoldier->iFaceIndex ].fDisabled )
				{
					BltVideoObjectFromIndex( FRAME_BUFFER, guiDEAD, pSoldier->ubDeadPanelFrame, pSoldier->sPanelFaceX, pSoldier->sPanelFaceY, VO_BLT_SRCTRANSPARENCY, NULL );

					// Blit hatch!
					BltVideoObjectFromIndex( guiSAVEBUFFER, guiHATCH, 0, pSoldier->sPanelFaceX, pSoldier->sPanelFaceY, VO_BLT_SRCTRANSPARENCY, NULL );

					InvalidateRegion( pSoldier->sPanelFaceX, pSoldier->sPanelFaceY, pSoldier->sPanelFaceX + TM_FACE_WIDTH, pSoldier->sPanelFaceY + TM_FACE_HEIGHT );
				}
		//}
	}


	if ( pSoldier->flags.fOpenPanel )
	{
		if ( TIMECOUNTERDONE( pSoldier->timeCounters.PanelAnimateCounter, 160 ) )
		{
				pSoldier->bOpenPanelFrame--;

				if ( pSoldier->bOpenPanelFrame < 0 )
				{
					pSoldier->flags.fOpenPanel = FALSE;
					pSoldier->bOpenPanelFrame = 0;

					if ( !gFacesData[ pSoldier->iFaceIndex ].fDisabled )
					{
						RestoreExternBackgroundRect( pSoldier->sPanelFaceX, pSoldier->sPanelFaceY, TM_FACE_WIDTH, TM_FACE_HEIGHT );
					}
				}
				RESETTIMECOUNTER( pSoldier->timeCounters.PanelAnimateCounter, 160 );
		}
	}

	if ( pSoldier->flags.fOpenPanel )
	{
		// Render panel!
		//if(gbPixelDepth==16)
		//{
				if ( !gFacesData[ pSoldier->iFaceIndex ].fDisabled )
				{
					RestoreExternBackgroundRect( pSoldier->sPanelFaceX, pSoldier->sPanelFaceY, TM_FACE_WIDTH, TM_FACE_HEIGHT );
					BltVideoObjectFromIndex( FRAME_BUFFER, guiCLOSE, pSoldier->bOpenPanelFrame, pSoldier->sPanelFaceX, pSoldier->sPanelFaceY, VO_BLT_SRCTRANSPARENCY, NULL );
					//InvalidateRegion( sTEAMFacesXY[ ubOpenPanelID ], sTEAMFacesXY[ ubOpenPanelID + 1 ], sTEAMFacesXY[ ubOpenPanelID ] + TM_FACE_WIDTH, sTEAMFacesXY[ ubOpenPanelID + 1 ] + TM_FACE_HEIGHT );
				}
		//}
	}

}



void HandleSoldierFaceFlash( SOLDIERTYPE *pSoldier, INT16 sFaceX, INT16 sFaceY )
{

}



void RenderSoldierTeamInv( SOLDIERTYPE *pSoldier, INT16 sX, INT16 sY, UINT8 ubPanelNum, BOOLEAN fDirty )
{
	if ( pSoldier->bActive && !(pSoldier->flags.uiStatusFlags & SOLDIER_DEAD ) )
	{
		if (pSoldier->flags.uiStatusFlags & SOLDIER_DRIVER)
		{
			INVRenderSteeringWheel(guiSAVEBUFFER, guiVEHINV, pSoldier, sX, sY, TM_INV_WIDTH, TM_INV_HEIGHT, fDirty);
		}
		else
		{
			// Look in primary hand
			INVRenderItem( guiSAVEBUFFER, pSoldier, &(pSoldier->inv[ HANDPOS ]), sX, sY, TM_INV_WIDTH, TM_INV_HEIGHT, fDirty, &(gfTEAM_HandInvDispText[ ubPanelNum ][ HANDPOS ] ), 0 , FALSE, 0);
		}

		if ( pSoldier->flags.uiStatusFlags & ( SOLDIER_PASSENGER | SOLDIER_DRIVER ) )
		{
			BltVideoObjectFromIndex( guiSAVEBUFFER, guiVEHINV, 1, sX, (INT16)(sY + TM_INV_HAND_SEPY), VO_BLT_SRCTRANSPARENCY, NULL );
			RestoreExternBackgroundRect( sX, (INT16)(sY + TM_INV_HAND_SEPY), (INT16)(TM_INV_WIDTH ) , (INT16)( TM_INV_HEIGHT ) );
		}
		else
		{
			// Do secondary hand
			INVRenderItem( guiSAVEBUFFER, pSoldier, &(pSoldier->inv[ SECONDHANDPOS ]), sX, (INT16)(sY + TM_INV_HAND_SEPY), TM_INV_WIDTH, TM_INV_HEIGHT, fDirty, &(gfTEAM_HandInvDispText[ ubPanelNum ][ SECONDHANDPOS ] ), 0 , FALSE, 0);
		}

	}
}


void TMFirstHandInvCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	UINT8 ubID, ubSoldierID;

	ubID = (UINT8) MSYS_GetRegionUserData( pRegion, 0 );

	// If our flags are set to do this, gofoit!
	if ( (guiTacticalInterfaceFlags & INTERFACE_MAPSCREEN ) )
	{
		return;
	}

	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}

	if ( ubID == NOBODY )
		return;


	// Now use soldier ID values
	ubSoldierID = gTeamPanel[ ubID ].ubID;


	if ( !MercPtrs[ ubSoldierID ]->bActive )
	{
		return;
	}


	if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN)
	{
		//OK FOR NOW, DROP ITEM WHERE BUDDY IS
		{
		//	SOLDIERTYPE *pSoldier;

		//	if(	GetSoldier( &pSoldier, ubSoldierID ) )
		//	{
		//	}
		}
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	{

	}

}


void TMClickFirstHandInvCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	UINT8	ubID, ubSoldierID;
	UINT16	usOldHandItem;

	ubID = (UINT8) MSYS_GetRegionUserData( pRegion, 0 );

	if ( ubID == NOBODY )
		return;

	// Now use soldier ID values
	ubSoldierID = gTeamPanel[ ubID ].ubID;

	if ( ubSoldierID == NOBODY )
		return;

	//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"ubID %d ubSoldierID %d", ubID, ubSoldierID);

	// sevenfm: scroll squads
	if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP || iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		if (gGameExternalOptions.fScopeModes)
		{
			if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
				ChangeScopeMode(MercPtrs[ubSoldierID], NOWHERE, TRUE);
			else
				ChangeScopeMode(MercPtrs[ubSoldierID], NOWHERE);
		}
	}

	if (iReason == MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		// anv: select vehicle by clicking on the steering wheel
		//if ( MercPtrs[ ubSoldierID ]->flags.uiStatusFlags & SOLDIER_DRIVER )
		//{
		//	SOLDIERTYPE *pVehicle = GetSoldierStructureForVehicle( MercPtrs[ ubSoldierID ]->iVehicleId );
		//	HandleLocateSelectMerc( pVehicle->ubID, 0 );
		//}
		//else
		ChangeWeaponMode(MercPtrs[ubSoldierID]);
		// Change to use cursor mode...
		//guiPendingOverrideEvent = A_ON_TERRAIN;
	}

	if (iReason == MSYS_CALLBACK_REASON_RBUTTON_UP )
	{
		if (gGameExternalOptions.fScopeModes && MercPtrs[ubSoldierID]->inv[HANDPOS].exists() && Item[MercPtrs[ubSoldierID]->inv[HANDPOS].usItem].usItemClass & IC_GUN)
		{
			if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
				ChangeScopeMode(MercPtrs[ubSoldierID], NOWHERE, TRUE);
			else
				ChangeScopeMode(MercPtrs[ubSoldierID], NOWHERE);
		}
		else if ( !AM_A_ROBOT( MercPtrs[ ubSoldierID ] ) )
		{
			usOldHandItem = MercPtrs[ ubSoldierID ]->inv[HANDPOS].usItem;
			SwapHandItems( MercPtrs[ ubSoldierID ] );
			MercPtrs[ ubSoldierID ]->ReLoadSoldierAnimationDueToHandItemChange( usOldHandItem, MercPtrs[ ubSoldierID ]->inv[HANDPOS].usItem );
			fInterfacePanelDirty = DIRTYLEVEL2;
		}
	}
}


void TMClickSecondHandInvCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	UINT8	ubID, ubSoldierID;
	UINT16	usOldHandItem;

	ubID = (UINT8) MSYS_GetRegionUserData( pRegion, 0 );

	if ( ubID == NOBODY )
		return;

	// Now use soldier ID values
	ubSoldierID = gTeamPanel[ ubID ].ubID;

	if ( ubSoldierID == NOBODY )
		return;

	// sevenfm: scroll squads
	/*if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP || iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN)
	{
		if (gGameExternalOptions.fScopeModes)
		{
			if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP)
				ChangeScopeMode(MercPtrs[ubSoldierID], NOWHERE, TRUE);
			else
				ChangeScopeMode(MercPtrs[ubSoldierID], NOWHERE);

			return;
		}
	}*/

	if (iReason == MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		if ( MercPtrs[ ubSoldierID ]->flags.uiStatusFlags & ( SOLDIER_PASSENGER | SOLDIER_DRIVER ) )
		{
			ExitVehicle( MercPtrs[ ubSoldierID ] );
		}
		else if (UsingNewInventorySystem() && !AM_A_ROBOT(MercPtrs[ubSoldierID]))
		{
			MercPtrs[ubSoldierID]->SwitchWeapons();
		}
	}

	if (iReason == MSYS_CALLBACK_REASON_RBUTTON_UP )
	{
		if (!(MercPtrs[ubSoldierID]->flags.uiStatusFlags & (SOLDIER_PASSENGER | SOLDIER_DRIVER)) &&
			!AM_A_ROBOT(MercPtrs[ubSoldierID]))
		{
			usOldHandItem = MercPtrs[ ubSoldierID ]->inv[HANDPOS].usItem;
			SwapHandItems( MercPtrs[ ubSoldierID ] );
			MercPtrs[ ubSoldierID ]->ReLoadSoldierAnimationDueToHandItemChange( usOldHandItem, MercPtrs[ ubSoldierID ]->inv[HANDPOS].usItem );
			fInterfacePanelDirty = DIRTYLEVEL2;
		}
	}
}


BOOLEAN PlayerExistsInSlot( UINT8 ubID )
{
	INT32 cnt;

	for ( cnt = 0; cnt < gGameOptions.ubSquadSize; cnt++ )
	{
		if ( gTeamPanel[ cnt ].fOccupied	)
		{
			if ( gTeamPanel[ cnt ].ubID == ubID )
			{
				return( TRUE );
			}
		}
	}

	return( FALSE );
}


INT8 GetTeamSlotFromPlayerID( UINT8 ubID )
{
	INT8 cnt;

	for ( cnt = 0; cnt < gGameOptions.ubSquadSize; cnt++ )
	{
		if ( gTeamPanel[ cnt ].fOccupied	)
		{
			if ( gTeamPanel[ cnt ].ubID == ubID )
			{
				return( cnt );
			}
		}
	}

	return( -1 );
}


BOOLEAN RemovePlayerFromTeamSlotGivenMercID( UINT8 ubMercID )
{
	INT32 cnt;

	for ( cnt = 0; cnt < gGameOptions.ubSquadSize; cnt++ )
	{
		if ( gTeamPanel[ cnt ].fOccupied	)
		{
			if ( gTeamPanel[ cnt ].ubID == ubMercID )
			{
				RemovePlayerFromInterfaceTeamSlot( (UINT8)cnt );
				return( TRUE );
			}
		}
	}

	return( FALSE );
}


void AddPlayerToInterfaceTeamSlot(UINT8 ubID)
{
	INT32	cnt;

	// If we are a vehicle don't ever add.....
	if (MercPtrs[ubID]->flags.uiStatusFlags & SOLDIER_VEHICLE)
	{
		return;
	}

	//if ( !( MercPtrs[ ubID ]->flags.uiStatusFlags & ( SOLDIER_DRIVER | SOLDIER_PASSENGER ) ) )
	//{
	if (!PlayerExistsInSlot(ubID))
	{
		// Find a free slot
		for (cnt = 0; cnt < gGameOptions.ubSquadSize; cnt++)
		{
			if (!gTeamPanel[cnt].fOccupied)
			{
				gTeamPanel[cnt].fOccupied = TRUE;
				gTeamPanel[cnt].ubID = ubID;

				MSYS_SetRegionUserData(&gTEAM_FirstHandInv[cnt], 0, cnt);
				MSYS_SetRegionUserData(&gTEAM_FaceRegions[cnt], 0, cnt);

				// DIRTY INTERFACE
				fInterfacePanelDirty = DIRTYLEVEL2;

				// Set ID to do open anim
				MercPtrs[ubID]->flags.fUInewMerc = TRUE;

				break;
			}
		}
	}
}

BOOLEAN InitTEAMSlots( )
{
	INT32 cnt;

	for ( cnt = 0; cnt < gGameOptions.ubSquadSize; cnt++ )
	{
		gTeamPanel[ cnt ].fOccupied = FALSE;
		gTeamPanel[ cnt ].ubID			= NOBODY;

	}

	return( TRUE );
}


BOOLEAN GetPlayerIDFromInterfaceTeamSlot( UINT8 ubPanelSlot, UINT8 *pubID )
{
	if ( ubPanelSlot >= gGameOptions.ubSquadSize )
	{
		return( FALSE );
	}

	if ( gTeamPanel[ ubPanelSlot ].fOccupied )
	{
		*pubID = gTeamPanel[ ubPanelSlot ].ubID;
		return( TRUE );
	}
	else
	{
		return( FALSE );
	}
}

UINT8 GetInterfaceTeamSlotFromPlayerID(UINT8 ubID)
{
	for (UINT8 ubPanelSlot = 0; ubPanelSlot < gGameOptions.ubSquadSize; ubPanelSlot++)
	{
		if (gTeamPanel[ubPanelSlot].fOccupied && gTeamPanel[ubPanelSlot].ubID == ubID)
		{
			return ubPanelSlot;
		}
	}

	return 255;
}

void RemoveAllPlayersFromSlot( )
{
	int cnt;

	for ( cnt = 0; cnt < gGameOptions.ubSquadSize; cnt++ )
	{
		RemovePlayerFromInterfaceTeamSlot( (UINT8)cnt );
	}
}


BOOLEAN RemovePlayerFromInterfaceTeamSlot( UINT8 ubPanelSlot )
{
	if ( ubPanelSlot >= gGameOptions.ubSquadSize )
	{
		return( FALSE );
	}

	if ( gTeamPanel[ ubPanelSlot ].fOccupied )
	{
		if ( !( MercPtrs[ gTeamPanel[ ubPanelSlot ].ubID ]->flags.uiStatusFlags & SOLDIER_DEAD ) )
	{
		// Set Id to close
		MercPtrs[ gTeamPanel[ ubPanelSlot ].ubID ]->flags.fUICloseMerc		= TRUE;
	}

		// Set face to inactive...
		SetAutoFaceInActive( MercPtrs[ gTeamPanel[ ubPanelSlot ].ubID ]->iFaceIndex );


		gTeamPanel[ ubPanelSlot ].fOccupied = FALSE;
		gTeamPanel[ ubPanelSlot ].ubID = NOBODY;

		MSYS_SetRegionUserData( &gTEAM_FirstHandInv[ ubPanelSlot ], 0, NOBODY );
		MSYS_SetRegionUserData( &gTEAM_FaceRegions[ ubPanelSlot ],	0, NOBODY );

		// DIRTY INTERFACE
		fInterfacePanelDirty = DIRTYLEVEL2;

		return( TRUE );
	}
	else
	{
		return( FALSE );
	}
}


/* CHRISL: Adjust function to allow X,Y coordinates to be passed so that we can render the TownID string
seperately between the team and inventory windows. */
void RenderTownIDString( INT16 sX, INT16 sY )
{
	CHAR16	zTownIDString[80];
	INT16 sFontX, sFontY;

	// when NIV is open in tactical
	if (sY+GetFontHeight(COMPFONT) >= SCREEN_HEIGHT) return;

	// Render town, position
	SetFont( COMPFONT );
	SetFontForeground( 183 );
	SetFontBackground( FONT_BLACK );

	GetSectorIDString( gWorldSectorX, gWorldSectorY, gbWorldSectorZ, zTownIDString, TRUE );
	ReduceStringLength( zTownIDString, 80, COMPFONT );
	VarFindFontCenterCoordinates( sX , sY, 80, 16, COMPFONT, &sFontX, &sFontY, zTownIDString );
	mprintf( sFontX, sFontY, L"%s", zTownIDString );
}


void CheckForAndAddMercToTeamPanel( SOLDIERTYPE *pSoldier )
{

	if ( pSoldier->bActive	)
	{
		// Add to interface if the are ours
		if ( pSoldier->bTeam == gbPlayerNum )
		{
			// Are we in the loaded sector?
			if ( pSoldier->sSectorX == gWorldSectorX && pSoldier->sSectorY == gWorldSectorY && pSoldier->bSectorZ == gbWorldSectorZ && !pSoldier->flags.fBetweenSectors && pSoldier->bInSector )
			{
				// IF on duty....
				if( ( pSoldier->bAssignment ==	CurrentSquad( ) )|| ( SoldierIsDeadAndWasOnSquad( pSoldier, ( INT8 )( CurrentSquad( ) ) ) ) )
				{

					if( pSoldier->bAssignment == ASSIGNMENT_DEAD )
					{
						pSoldier->flags.fUICloseMerc = FALSE;
					}
					// ATE: ALrighty, if we have the insertion code of helicopter..... don't add just yet!
					/// ( will add in heli code )
					if ( pSoldier->ubStrategicInsertionCode != INSERTION_CODE_CHOPPER )
					{
						AddPlayerToInterfaceTeamSlot( pSoldier->ubID );
					}

					// ARE WE A VEHICLE.....
					if ( pSoldier->flags.uiStatusFlags & SOLDIER_VEHICLE )
					{
						AddPassangersToTeamPanel( pSoldier->bVehicleID );
					}
				}
			}
			else
			{
				// Make sure we are NOT in this world!
				// Set gridno to one that's not visib;l
				pSoldier->RemoveSoldierFromGridNo( );

				// Remove slot from tactical
				RemoveMercSlot( pSoldier );

			}
		}
	}
}

void CleanUpStack( OBJECTTYPE * pObj, OBJECTTYPE * pCursorObj )
{
	INT16	bMaxPoints;

	if ( !(Item[ pObj->usItem ].usItemClass & IC_AMMO || Item[ pObj->usItem ].usItemClass & IC_KIT || Item[ pObj->usItem ].usItemClass & IC_MEDKIT  || Item[pObj->usItem].canteen || Item[pObj->usItem].gascan || Item[pObj->usItem].alcohol ) )
	{
		return;
	}

	if ( Item[ pObj->usItem ].usItemClass & IC_AMMO )
	{
		bMaxPoints = Magazine[ Item[ pObj->usItem ].ubClassIndex ].ubMagSize;
	}
	else
	{
		bMaxPoints = 100;
	}

	if ( pCursorObj && pCursorObj->usItem == pObj->usItem )
	{
		DistributeStatus(pCursorObj, pObj, bMaxPoints);
	}
	DistributeStatus(pObj, pObj, bMaxPoints);

	// r8463
	// Flugente: one of the items on the stack might not be full. Make sure it is the first one, so the player can see what is missing at a glance
	if ( pObj->ubNumberOfObjects > 1 )
	{
		if ( Item[pObj->usItem].usItemClass & IC_AMMO )
		{
			UINT16 shots_first	= (*pObj)[0]->data.ubShotsLeft;
			(*pObj)[0]->data.ubShotsLeft = (*pObj)[pObj->ubNumberOfObjects - 1]->data.ubShotsLeft;
			(*pObj)[pObj->ubNumberOfObjects - 1]->data.ubShotsLeft = shots_first;
		}
		else if ( Item[pObj->usItem].usItemClass & IC_MAPFILTER_KIT )
		{
			INT16 status_first = (*pObj)[0]->data.objectStatus;
			(*pObj)[0]->data.objectStatus = (*pObj)[pObj->ubNumberOfObjects - 1]->data.objectStatus;
			(*pObj)[pObj->ubNumberOfObjects - 1]->data.objectStatus = status_first;
		}
	}
}

UINT8 FindNextMercInTeamPanel( SOLDIERTYPE *pSoldier, BOOLEAN fGoodForLessOKLife, BOOLEAN fOnlyRegularMercs )
{
	INT32 cnt;
	INT32 bFirstID;
	SOLDIERTYPE *pTeamSoldier;

	bFirstID = GetTeamSlotFromPlayerID( pSoldier->ubID );

	if ( bFirstID == -1 )
	{
		return( pSoldier->ubID );
	}

	for ( cnt = ( bFirstID + 1 ); cnt < gGameOptions.ubSquadSize; cnt++ )
	{
		if ( gTeamPanel[ cnt ].fOccupied )
		{
			// Set Id to close
			pTeamSoldier = MercPtrs[ gTeamPanel[ cnt ].ubID ];

			if ( fOnlyRegularMercs )
			{
				if ( pTeamSoldier->bActive && ( AM_AN_EPC( pTeamSoldier ) || AM_A_ROBOT( pTeamSoldier ) ) )
				{
					continue;
				}
			}

			if ( fGoodForLessOKLife )
			{
				if ( pTeamSoldier->stats.bLife > 0 && pTeamSoldier->bActive && pTeamSoldier->bInSector && pTeamSoldier->bTeam == gbPlayerNum && pTeamSoldier->bAssignment < ON_DUTY	&& OK_INTERRUPT_MERC( pTeamSoldier ) && pSoldier->bAssignment == pTeamSoldier->bAssignment )
				{
					return( (UINT8)gTeamPanel[ cnt ].ubID );
				}
			}
			else
			{
				if ( OK_CONTROLLABLE_MERC( pTeamSoldier) && OK_INTERRUPT_MERC( pTeamSoldier ) && pSoldier->bAssignment == pTeamSoldier->bAssignment )
				{
					return( (UINT8)gTeamPanel[ cnt ].ubID );
				}
			}
		}
	}

	if ( ( gusSelectedSoldier != NOBODY ) && ( gGameSettings.fOptions[ TOPTION_SPACE_SELECTS_NEXT_SQUAD ] ) )
	{
		// only allow if nothing in hand and if in SM panel, the Change Squad button must be enabled
		if ( ( ( gsCurInterfacePanel != TEAM_PANEL ) || ( ButtonList[ iTEAMPanelButtons[ CHANGE_SQUAD_BUTTON ] ]->uiFlags & BUTTON_ENABLED ) ) )
		{
			SOLDIERTYPE *pNewSoldier;
			INT32		iCurrentSquad;

			//Select next squad
			iCurrentSquad = CurrentSquad( );

			pNewSoldier = FindNextActiveSquad( MercPtrs[ gusSelectedSoldier ] );

			if ( pNewSoldier->bAssignment != iCurrentSquad )
			{
				//ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[ MSG_SQUAD_ACTIVE ], ( CurrentSquad( ) + 1 ) );
				ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[ MSG_SQUAD_ACTIVE ], ( pNewSoldier->bAssignment + 1 ) );

				return( pNewSoldier->ubID );
			}
		}
	}

	// none found,
	// Now loop back
	for ( cnt = 0; cnt < bFirstID; cnt++ )
	{
		if ( gTeamPanel[ cnt ].fOccupied )
		{
			pTeamSoldier = MercPtrs[ gTeamPanel[ cnt ].ubID ];

			if ( fOnlyRegularMercs )
			{
				if ( pTeamSoldier->bActive && ( AM_AN_EPC( pTeamSoldier ) || AM_A_ROBOT( pTeamSoldier ) ) )
				{
					continue;
				}
			}

			if ( fGoodForLessOKLife )
			{
				if ( pTeamSoldier->stats.bLife > 0 && pTeamSoldier->bActive && pTeamSoldier->bInSector && pTeamSoldier->bTeam == gbPlayerNum && pTeamSoldier->bAssignment < ON_DUTY	&& OK_INTERRUPT_MERC( pTeamSoldier ) && pSoldier->bAssignment == pTeamSoldier->bAssignment )
				{
					return( (UINT8)gTeamPanel[ cnt ].ubID );
				}
			}
			else
			{
				if ( OK_CONTROLLABLE_MERC( pTeamSoldier) && OK_INTERRUPT_MERC( pTeamSoldier ) && pSoldier->bAssignment == pTeamSoldier->bAssignment )
				{
					return( (UINT8)gTeamPanel[ cnt ].ubID );
				}
			}
		}
	}

	// IF we are here, keep as we always were!
	return( pSoldier->ubID );
}


void HelpTextDoneCallback( void )
{
	gfRerenderInterfaceFromHelpText = TRUE;

	//To redraw the Store Keeper Interface
	gubSkiDirtyLevel = SKI_DIRTY_LEVEL2;
}


void DisableTacticalTeamPanelButtons( BOOLEAN fDisable )
{
	gfDisableTacticalPanelButtons = fDisable;
}


void BeginKeyPanelFromKeyShortcut( )
{
	SOLDIERTYPE *pSoldier = NULL;
	INT16 sStartYPosition = 0;
	INT16 sWidth = 0, sHeight = 0;

	if ( gsCurInterfacePanel != SM_PANEL )
	{
		return;
	}

	if ( gpSMCurrentMerc == NULL )
	{
		return;
	}

	if ( gfInKeyRingPopup )
	{
		return;
	}


	sStartYPosition = INV_INTERFACE_START_Y;
	sWidth = SCREEN_WIDTH;
	sHeight = ( SCREEN_HEIGHT - ( INV_INTERFACE_START_Y ) );
	pSoldier = gpSMCurrentMerc;

	//if we are in the shop keeper interface
	if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
		return;

	//CHRISL: In OIV mode, we don't want to offset the keyring popup.
	if(UsingNewInventorySystem() == false)
		InitKeyRingPopup( pSoldier, 0, sStartYPosition, sWidth, sHeight );
	else
		InitKeyRingPopup( pSoldier, 35, sStartYPosition, sWidth, sHeight );
}


void KeyRingItemPanelButtonCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	SOLDIERTYPE *pSoldier = NULL;
	INT16 sStartYPosition = 0;
	INT16 sWidth = 0, sHeight = 0;

	if( guiCurrentScreen == MAP_SCREEN )
	{
		if( bSelectedInfoChar == -1 )
		{
			return;
		}

		if( gCharactersList[ bSelectedInfoChar ].fValid == FALSE )
		{
			return;
		}




		pSoldier = &( Menptr[ gCharactersList[ bSelectedInfoChar ].usSolID ] );
		sStartYPosition = MAP_START_KEYRING_Y;
		sWidth = 261;
		sHeight = ( 359 - 107 );
	}
	else
	{
		if ( gpSMCurrentMerc == NULL )
		{
			return;
		}

		sStartYPosition = INV_INTERFACE_START_Y;
		sWidth = SCREEN_WIDTH;
		sHeight = ( SCREEN_HEIGHT - ( INV_INTERFACE_START_Y ) );
		pSoldier = gpSMCurrentMerc;
	}

	//if we are in the shop keeper interface
	if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
		return;

	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}

	if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		if( guiCurrentScreen == MAP_SCREEN )
		{
			// want the inv done button shutdown and the region behind the keyring shaded
			//ForceButtonUnDirty( giMapInvDoneButton );
			// shade the background
			ShadowVideoSurfaceRect( FRAME_BUFFER , 0, 107, 261, 359 );
			InvalidateRegion( 0, 107, 261, 359 );
		}

		//CHRISL: In OIV mode, we don't want to offset the keyring popup.
		if(UsingNewInventorySystem() == false)
			InitKeyRingPopup( pSoldier, 0, sStartYPosition, sWidth, sHeight );
		else
			InitKeyRingPopup( pSoldier, 35, sStartYPosition, sWidth, sHeight );
	}
}

void KeyRingSlotInvClickCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	UINT32 uiKeyRing;
	// Copyies of values
	UINT16	usOldItemIndex;
	static BOOLEAN	fRightDown = FALSE;
	static BOOLEAN	fLeftDown = FALSE;
	INT32 iNumberOfKeysTaken = 0;

	uiKeyRing = MSYS_GetRegionUserData( pRegion, 0 );


	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}

	//if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	//{
	//	fLeftDown = TRUE;
	//}
	//else if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP && fLeftDown )
	if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		fLeftDown = FALSE;

		//if we are in the shop keeper interface
		if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
		{
			INVENTORY_IN_SLOT InvSlot;

			if( gMoveingItem.sItemIndex == 0 )
			{
				//Delete the contents of the item cursor
				gMoveingItem.initialize();
			}
			else
			{
				// Return if empty
				//if ( gpSMCurrentMerc->inv[ uiHandPos ].exists() == false )
				//	return;


				// Fill out the inv slot for the item
				//InvSlot.sItemIndex = gpSMCurrentMerc->inv[ uiHandPos ].usItem;
//			InvSlot.ubNumberOfItems = gpSMCurrentMerc->inv[ uiHandPos ].ubNumberOfObjects;
//			InvSlot.ubItemQuality = gpSMCurrentMerc->inv[ uiHandPos ].gun.bGunStatus;
				//InvSlot.ItemObject = gpSMCurrentMerc->inv[ uiHandPos ];
				//InvSlot.ubLocationOfObject = PLAYERS_INVENTORY;

				//InvSlot.ubIdOfMercWhoOwnsTheItem = gpSMCurrentMerc->ubProfile;


				//Add the item to the Players Offer Area
				//AddItemToPlayersOfferArea( gpSMCurrentMerc->ubProfile, &InvSlot, (UINT8)uiHandPos );

				// Dirty
				fInterfacePanelDirty = DIRTYLEVEL2;
			}
			return;
		}

		// If we do not have an item in hand, start moving it
		if ( gpItemPointer == NULL )
		{
			// Return if empty
			if( ( gpItemPopupSoldier->pKeyRing[ uiKeyRing ].ubKeyID == INVALID_KEY_NUMBER ) || ( gpItemPopupSoldier->pKeyRing[ uiKeyRing ].ubNumber == 0 ) )
				return;

			// If our flags are set to do this, gofoit!
			if ( (guiTacticalInterfaceFlags & INTERFACE_MAPSCREEN ) )
			{
			}
			else
			{
				if ( gpItemPopupSoldier->ubID != gusSelectedSoldier )
				{
					SelectSoldier( gpItemPopupSoldier->ubID, FALSE, FALSE );
				}
			}

			usOldItemIndex =	( UINT16 )uiKeyRing ;

			BeginKeyRingItemPointer( gpItemPopupSoldier, (UINT8)usOldItemIndex );
			//BeginItemPointer( gpSMCurrentMerc, (UINT8)uiHandPos );

		}
		else
		{
			UINT8			ubSrcID, ubDestID;
			BOOLEAN		fOKToGo = FALSE;
			BOOLEAN		fDeductPoints = FALSE;

			// WANNE: Instead of checking the item number for a key, check the itemclass!!!
			if (Item[gpItemPointer->usItem].usItemClass != IC_KEY)
			{
				return;
			}

			// ATE: OK, get source, dest guy if different... check for and then charge appropriate APs
			ubSrcID	= ( UINT8 )gCharactersList[ bSelectedInfoChar ].usSolID;
			if ( gpItemPointerSoldier )
			{
				ubDestID = gpItemPointerSoldier->ubID;
			}
			else
			{
				ubDestID = ubSrcID;
			}

			if ( ubSrcID == ubDestID )
			{
				// We are doing this ourselve, continue
				fOKToGo = TRUE;
			}
			else
			{
				// These guys are different....
				fDeductPoints = TRUE;

				// First check points for src guy
				if ( gpItemPointerSoldier->stats.bLife >= CONSCIOUSNESS )
				{
					if ( EnoughPoints( gpItemPointerSoldier, 2, 0, TRUE ) )
					{
						fOKToGo = TRUE;
					}
				}
				else
				{
					fOKToGo = TRUE;
				}

				// Should we go on?
				if ( fOKToGo )
				{
					if ( gpSMCurrentMerc->stats.bLife >= CONSCIOUSNESS )
					{
						if ( EnoughPoints( gpSMCurrentMerc, 2, 0, TRUE ) )
						{
							fOKToGo = TRUE;
						}
						else
						{
							fOKToGo = FALSE;
						}
					}
				}
			}

			if ( fOKToGo )
			{
				//usOldItemIndex = gpSMCurrentMerc->inv[ uiHandPos ].usItem;
				//usNewItemIndex = gpItemPointer->usItem;

				if ( gpItemPopupSoldier->pKeyRing[ uiKeyRing ].ubKeyID == INVALID_KEY_NUMBER || gpItemPopupSoldier->pKeyRing[ uiKeyRing ].ubKeyID == (*gpItemPointer)[0]->data.key.ubKeyID)
				{
					// Try to place here
					if ( ( iNumberOfKeysTaken = AddKeysToSlot( gpItemPopupSoldier, ( INT8 )uiKeyRing, gpItemPointer ) ) )
					{

						if ( fDeductPoints )
						{
							// Deduct points
							if ( gpItemPointerSoldier->stats.bLife >= CONSCIOUSNESS )
							{
								DeductPoints( gpItemPointerSoldier,	2, 0, UNTRIGGERED_INTERRUPT );
							}
							if ( gpItemPopupSoldier->stats.bLife >= CONSCIOUSNESS )
							{
								DeductPoints( gpItemPopupSoldier,	2, 0, UNTRIGGERED_INTERRUPT );
							}
						}

						// Dirty
						fInterfacePanelDirty = DIRTYLEVEL2;

						gpItemPointer->ubNumberOfObjects -= ( UINT8 )iNumberOfKeysTaken;

						// Check if it's the same now!
						if ( gpItemPointer->exists() == false )
						{
							if ( guiTacticalInterfaceFlags & INTERFACE_MAPSCREEN )
							{
								MAPEndItemPointer();
							}
							else
							{
								EndItemPointer();
							}
						}

						// Setup a timer....
						//guiMouseOverItemTime = GetJA2Clock( );
						//gfCheckForMouseOverItem = TRUE;
						//gbCheckForMouseOverItemPos = (INT8)uiHandPos;

					}
				}
				else
				{
					// Swap!
					SwapKeysToSlot( gpItemPopupSoldier, (INT8) uiKeyRing, gpItemPointer );

					if ( fDeductPoints )
					{
						// Deduct points
						if ( gpItemPointerSoldier && gpItemPointerSoldier->stats.bLife >= CONSCIOUSNESS )
						{
							DeductPoints( gpItemPointerSoldier,	2, 0, UNTRIGGERED_INTERRUPT );
						}
						if ( gpSMCurrentMerc->stats.bLife >= CONSCIOUSNESS )
						{
							DeductPoints( gpSMCurrentMerc,	2, 0, UNTRIGGERED_INTERRUPT );
						}
					}

					// Dirty
					fInterfacePanelDirty = DIRTYLEVEL2;

				}
			}
		}

		UpdateItemHatches();
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	{
		fRightDown = TRUE;
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP && fRightDown )
	{
		fRightDown = FALSE;

		// Return if empty
		if( ( gpItemPopupSoldier->pKeyRing[ uiKeyRing ].ubKeyID == INVALID_KEY_NUMBER ) || ( gpItemPopupSoldier->pKeyRing[ uiKeyRing ].ubNumber == 0 ) )
		{
			DeleteKeyRingPopup( );
			fTeamPanelDirty = TRUE;
			return;
		}
		// Some global stuff here - for esc, etc
		// Check for # of slots in item
		if ( !InItemDescriptionBox( ) )
		{
			if ( guiTacticalInterfaceFlags & INTERFACE_MAPSCREEN )
			{
				//InitKeyItemDescriptionBox( gpItemPopupSoldier, (UINT8)uiKeyRing, MAP_ITEMDESC_START_X, MAP_ITEMDESC_START_Y, 0 );
		}
		else
		{
				InitKeyItemDescriptionBox( gpItemPopupSoldier, (UINT8)uiKeyRing, SM_ITEMDESC_START_X, SM_ITEMDESC_START_Y, 0 );
		}
		}
	}
	else if (iReason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		fRightDown = FALSE;
		fLeftDown = FALSE;
	}

}



void DisableSMPpanelButtonsWhenInShopKeeperInterface( BOOLEAN fDontDrawButtons )
{
	//Go through the buttons that will be under the ShopKeepers ATM panel and disable them
	DisableButton( iSMPanelButtons[ STANCEUP_BUTTON ] );
	DisableButton( iSMPanelButtons[ UPDOWN_BUTTON ] );
	DisableButton( iSMPanelButtons[ CLIMB_BUTTON ] );
	DisableButton( iSMPanelButtons[ STANCEDOWN_BUTTON ] );
	DisableButton( iSMPanelButtons[ HANDCURSOR_BUTTON ] );
	DisableButton( iSMPanelButtons[ BURSTMODE_BUTTON ] );
	DisableButton( iSMPanelButtons[ LOOK_BUTTON ] );
	DisableButton( iSMPanelButtons[ TALK_BUTTON ] );
	DisableButton( iSMPanelButtons[ MUTE_BUTTON ] );

	DisableButton( giSMStealthButton );

	//Make sure the options button is disabled
	DisableButton( iSMPanelButtons[ OPTIONS_BUTTON ] );

	//Make sure the mapscreen button is disabled
	DisableButton( iSMPanelButtons[ SM_MAP_SCREEN_BUTTON ] );

	//DisableButton
	DisableButton( iSMPanelButtons[ STANCEUP_BUTTON ] );
	DisableButton( iSMPanelButtons[ UPDOWN_BUTTON ] );
	DisableButton( iSMPanelButtons[ CLIMB_BUTTON ] );
	DisableButton( iSMPanelButtons[ STANCEDOWN_BUTTON ] );
	DisableButton( iSMPanelButtons[ HANDCURSOR_BUTTON ] );
	DisableButton( iSMPanelButtons[ BURSTMODE_BUTTON ] );
	DisableButton( iSMPanelButtons[ LOOK_BUTTON ] );
	DisableButton( iSMPanelButtons[ TALK_BUTTON ] );
	DisableButton( iSMPanelButtons[ MUTE_BUTTON ] );

	DisableButton( giSMStealthButton );
	RenderBackpackButtons(DISABLE_BUTTON);	/* CHRISL: Needed for new inventory backpack buttons */



	if( fDontDrawButtons )
	{
//ATM:
		//Go through the buttons that will be under the ShopKeepers ATM panel and disable them
		//DisableButton
		ButtonList[ iSMPanelButtons[ STANCEUP_BUTTON ] ]->uiFlags &= ~BUTTON_DIRTY;
		ButtonList[ iSMPanelButtons[ UPDOWN_BUTTON ] ]->uiFlags &= ~BUTTON_DIRTY;
		ButtonList[ iSMPanelButtons[ CLIMB_BUTTON ] ]->uiFlags &= ~BUTTON_DIRTY;
		ButtonList[ iSMPanelButtons[ STANCEDOWN_BUTTON ] ]->uiFlags &= ~BUTTON_DIRTY;
		ButtonList[ iSMPanelButtons[ HANDCURSOR_BUTTON ] ]->uiFlags &= ~BUTTON_DIRTY;
		ButtonList[ iSMPanelButtons[ BURSTMODE_BUTTON ] ]->uiFlags &= ~BUTTON_DIRTY;
		ButtonList[ iSMPanelButtons[ LOOK_BUTTON ] ]->uiFlags &= ~BUTTON_DIRTY;
		ButtonList[ iSMPanelButtons[ TALK_BUTTON ] ]->uiFlags &= ~BUTTON_DIRTY;
		ButtonList[ iSMPanelButtons[ MUTE_BUTTON ] ]->uiFlags &= ~BUTTON_DIRTY;

		ButtonList[ giSMStealthButton ]->uiFlags &= ~BUTTON_DIRTY;
		// CHRISL: Backpack buttons for new inventory system
		if((UsingNewInventorySystem() == true))
		{
			ButtonList[ giSMZipperButton ]->uiFlags &= ~BUTTON_DIRTY;
			ButtonList[ giSMDropPackButton ]->uiFlags &= ~BUTTON_DIRTY;
		}


		//Make sure the options button is disabled
		ButtonList[ iSMPanelButtons[ OPTIONS_BUTTON ] ]->uiFlags &= ~BUTTON_ENABLED;

		//Make sure the mapscreen button is disabled
		ButtonList[ iSMPanelButtons[ SM_MAP_SCREEN_BUTTON ] ]->uiFlags &= ~BUTTON_ENABLED;
	
		//DisableButton
		ButtonList[ iSMPanelButtons[ STANCEUP_BUTTON ] ]->uiFlags |= BUTTON_FORCE_UNDIRTY;
		ButtonList[ iSMPanelButtons[ UPDOWN_BUTTON ] ]->uiFlags |= BUTTON_FORCE_UNDIRTY;
		ButtonList[ iSMPanelButtons[ CLIMB_BUTTON ] ]->uiFlags |= BUTTON_FORCE_UNDIRTY;
		ButtonList[ iSMPanelButtons[ STANCEDOWN_BUTTON ] ]->uiFlags |= BUTTON_FORCE_UNDIRTY;
		ButtonList[ iSMPanelButtons[ HANDCURSOR_BUTTON ] ]->uiFlags |= BUTTON_FORCE_UNDIRTY;
		ButtonList[ iSMPanelButtons[ BURSTMODE_BUTTON ] ]->uiFlags |= BUTTON_FORCE_UNDIRTY;
		ButtonList[ iSMPanelButtons[ LOOK_BUTTON ] ]->uiFlags |= BUTTON_FORCE_UNDIRTY;
		ButtonList[ iSMPanelButtons[ TALK_BUTTON ] ]->uiFlags |= BUTTON_FORCE_UNDIRTY;
		ButtonList[ iSMPanelButtons[ MUTE_BUTTON ] ]->uiFlags |= BUTTON_FORCE_UNDIRTY;

		ButtonList[ giSMStealthButton ]->uiFlags |= BUTTON_FORCE_UNDIRTY;
		// CHRISL: Backpack buttons for new inventory system
		if((UsingNewInventorySystem() == true))
		{
			ButtonList[ giSMZipperButton ]->uiFlags |= BUTTON_FORCE_UNDIRTY;
			ButtonList[ giSMDropPackButton ]->uiFlags |= BUTTON_FORCE_UNDIRTY;
		}
	}
}


BOOLEAN IsMouseInRegion( MOUSE_REGION *pRegion )
{
	if ( (gusMouseXPos >= pRegion->RegionTopLeftX ) && (gusMouseXPos <= pRegion->RegionBottomRightX ) && (gusMouseYPos >= pRegion->RegionTopLeftY ) && (gusMouseYPos <= pRegion->RegionBottomRightY ) )
	{
		return( TRUE );
	}
	else
	{
		return( FALSE );
	}
}


void EnableButtonsForInItemBox( BOOLEAN fEnable )
{
	if ( !fEnable )
	{
		DisableButton( iSMPanelButtons[ NEXTMERC_BUTTON ] );
		DisableButton( iSMPanelButtons[ PREVMERC_BUTTON ] );
		DisableButton( iSMPanelButtons[ SM_MAP_SCREEN_BUTTON ] );
	}
	else
	{
		EnableButton( iSMPanelButtons[ NEXTMERC_BUTTON ] );
		EnableButton( iSMPanelButtons[ PREVMERC_BUTTON ] );

		if ( !gfDisableTacticalPanelButtons )
		{
			EnableButton( iSMPanelButtons[ SM_MAP_SCREEN_BUTTON ] );
		}
	}
}


void SMInvMoneyButtonCallback( MOUSE_REGION * pRegion, INT32 iReason )
{
	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
		return;
	}
	else if (iReason == MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		//If the current merc is to far away, dont allow anything to be done
		if( gfSMDisableForItems )
			return;

		//if the player has an item in his hand,
		if( gpItemPointer != NULL )
		{
			//and the item is money
			if( Item[ gpItemPointer->usItem ].usItemClass == IC_MONEY )
			{
				CHAR16	zText[512];
				CHAR16	zMoney[64];

				// Make sure we go back to movement mode...
				guiPendingOverrideEvent = A_CHANGE_TO_MOVE;
				HandleTacticalUI( );

				swprintf( zMoney, L"%d", (*gpItemPointer)[0]->data.money.uiMoneyAmount );

				InsertCommasForDollarFigure( zMoney );
				InsertDollarSignInToString( zMoney );

				//ask the user if they are sure they want to deposit the money
				swprintf( zText, gzMoneyWithdrawMessageText[ CONFIRMATION_TO_DEPOSIT_MONEY_TO_ACCOUNT ], zMoney );

				if( guiCurrentScreen == SHOPKEEPER_SCREEN )
				{
					//if we are in the shop keeper interface, free the cursor
					if( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE )
					{
						FreeMouseCursor( TRUE );
					}

					DoMessageBox( MSG_BOX_BASIC_STYLE, zText, SHOPKEEPER_SCREEN, ( UINT8 )MSG_BOX_FLAG_YESNO, ConfirmationToDepositMoneyToPlayersAccount, NULL );
				}
				else
					DoMessageBox( MSG_BOX_BASIC_STYLE, zText, GAME_SCREEN, ( UINT8 )MSG_BOX_FLAG_YESNO, ConfirmationToDepositMoneyToPlayersAccount, NULL );
			}
		}

		//else bring up the money item description box to remove money from the players account
		else
		{
			//set the flag indicating we are removing money from the players account
			gfAddingMoneyToMercFromPlayersAccount = TRUE;

			//create the temp object from the players account balance
//			if( LaptopSaveInfo.iCurrentBalance > MAX_MONEY_PER_SLOT )
//				CreateMoney( MAX_MONEY_PER_SLOT, &gItemPointer );
//			else
				CreateMoney( LaptopSaveInfo.iCurrentBalance, &gItemPointer );

			InternalInitItemDescriptionBox( &gItemPointer, ITEMDESC_START_X, ITEMDESC_START_Y, 0, gpSMCurrentMerc );
		}
	}
}

void ConfirmationToDepositMoneyToPlayersAccount( UINT8 ubExitValue )
{
	if ( ubExitValue == MSG_BOX_RETURN_YES )
	{
		//add the money to the players account
		AddTransactionToPlayersBook( MERC_DEPOSITED_MONEY_TO_PLAYER_ACCOUNT, gpSMCurrentMerc->ubProfile, GetWorldTotalMin(), (*gpItemPointer)[0]->data.money.uiMoneyAmount);

		// dirty shopkeeper
		gubSkiDirtyLevel = SKI_DIRTY_LEVEL2;

		EndItemPointer( );
	}
}


void ReEvaluateDisabledINVPanelButtons( )
{
	gfReEvaluateDisabledINVPanelButtons = TRUE;
}

void CheckForReEvaluateDisabledINVPanelButtons( )
{
	// OK, if we currently have an item pointer up....
	if ( gfReEvaluateDisabledINVPanelButtons )
	{
		if ( gpItemPointer || gfInItemPickupMenu )
		{
			EnableSMPanelButtons( FALSE, TRUE );
		}
		else
		{
			EnableSMPanelButtons( TRUE, TRUE );
		}

		gfReEvaluateDisabledINVPanelButtons = FALSE;
	}
}


void AbandonBoxingCallback( UINT8 ubExitValue )
{
	if ( ubExitValue == MSG_BOX_RETURN_YES )
	{
		// ok, proceed!
		SetBoxingState( NOT_BOXING );
		gfEnteringMapScreen = TRUE;
	}
	// otherwise do nothing
}

void GoToMapScreenFromTactical( void )
{
	if ( gTacticalStatus.bBoxingState != NOT_BOXING )
	{
		// pop up dialogue asking whether the player wants to abandon the fight
		DoMessageBox( MSG_BOX_BASIC_STYLE, Message[ STR_ABANDON_FIGHT ], GAME_SCREEN, ( UINT8 )MSG_BOX_FLAG_YESNO, AbandonBoxingCallback, NULL );
		return;
	}
	// ok, proceed!
	gfMouseLockedOnBorder = FALSE;
	gfEnteringMapScreen = TRUE;

}

//----LEGION 2---------
BOOLEAN HandleKlerykPistolet( SOLDIERTYPE *pSoldier, UINT32 uiHandPos, UINT16 usReplaceItem )
{
	BOOLEAN fRefuse = FALSE;

	if ( pSoldier->ubProfile == 57 )
	{
		if ( uiHandPos == HANDPOS || uiHandPos == SECONDHANDPOS )
		{
			if ( usReplaceItem == NOTHING )
			{
				fRefuse = TRUE;
			}
			else
			{	//legion
				// Do we have nothing or the leather vest or kevlar leather vest?
				if ( usReplaceItem == 4520 ) //z³oty pistolet items.xml
				{
					// This is good....
					fRefuse = FALSE;
				}
				else
				{
					fRefuse = TRUE;
				}
			}

			if ( fRefuse )
			{
				// Say quote!
				TacticalCharacterDialogue( pSoldier, 85 );
				return( TRUE );
			}
		}
	}

	return( FALSE );
}

//anv - extended panel functions
BOOLEAN DrawExtendedPanel(SOLDIERTYPE* pSoldier)
{
	// ini stuff
	UINT16 uiMaxPanels = gExtendedPanelsSettings.uepMaxPanels;
	// shifts from borders
	UINT16 uiExtendedPanelXLShift = gExtendedPanelsSettings.uepXLShift;
	UINT16 uiExtendedPanelXRShift = gExtendedPanelsSettings.uepXRShift;
	UINT16 uiExtendedPanelYShift = gExtendedPanelsSettings.uepYShift;

	// distances
	UINT16 uiExtendedPanelRowDist = gExtendedPanelsSettings.uepRowDist;
	UINT16 uiExtendedPanelColDist = gExtendedPanelsSettings.uepColDist;

	UINT16 uiExtendedPanelMaxRow = gExtendedPanelsSettings.uepMaxRow;  
	UINT16 uiExtendedPanelMinWidth = gExtendedPanelsSettings.uepMinWidth;

	//135 - map screen width
	//83 - one merc avatar width
	//UINT16 uiPlaceForPanels = (UINT16)( INTERFACE_WIDTH - 4 - 136 - gGameOptions.ubSquadSize * 83 );
	UINT16 uiPlaceForPanels = gsPanelWindowWidth;

	UINT16 uiPanels = 0;

	// how many panels can we fit in
	while( uiPlaceForPanels > ( uiExtendedPanelXLShift + ( uiPanels + 1) * uiExtendedPanelMinWidth + uiPanels * uiExtendedPanelColDist + uiExtendedPanelXRShift ) )
	{
		uiPanels++;
	}

	//cut off if too many
	if( uiPanels > uiMaxPanels)
		uiPanels = uiMaxPanels;

	UINT16 uiExtendedPanelWidth;
	if(uiPanels)
		uiExtendedPanelWidth=(UINT16)( uiPlaceForPanels - ( uiExtendedPanelXLShift + ( uiPanels - 1 ) * uiExtendedPanelColDist + uiExtendedPanelXLShift ) ) / uiPanels;
	else
		uiExtendedPanelWidth=0;

	UINT16 uiExtendedPanelX[10] = 
	{
		//(UINT16)(INTERFACE_START_X + NUM_TEAM_SLOTS*83 + uiExtendedPanelXLShift)
		//(UINT16)(INTERFACE_START_X + 4 + gGameOptions.ubSquadSize*83 + uiExtendedPanelXLShift)
		(UINT16)(gsPanelWideX + uiExtendedPanelXLShift)
	};
	for(UINT16 uiCnt = 1; (uiCnt < uiPanels) && uiCnt < 10; uiCnt++ )
	{
		uiExtendedPanelX[uiCnt] = uiExtendedPanelX[uiCnt-1] + uiExtendedPanelWidth + uiExtendedPanelColDist;
	}
	//UINT16 uiExtendedPanelY = INTERFACE_START_Y + uiExtendedPanelYShift;
	UINT16 uiExtendedPanelY = gsPanelWideY + uiExtendedPanelYShift;

	//translate layout from .ini
	//loop through panels
	for (UINT16 PP = 0; PP < uiPanels && PP < gExtendedPanelsSettings.uepMaxPanels; PP++)
	{
		//loop through rows
		for(UINT16 ERP = 0; ( ERP < gExtendedPanelsSettings.uepRowCapacity[PP] ) && ( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_BREAK" ) != 0 ); ERP++)
		{
			BOOLEAN bIgnoreNext = FALSE; // for trait display
			EXTENDED_PANEL_STRING expsTemp;

			if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_NAME" ) == 0 )
				expsTemp = GetNameString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_BATTLES" ) == 0  )
				expsTemp = GetBattlesString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_KILLS" ) == 0  )
				expsTemp = GetKillsString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_ASSISTS" ) == 0  )
				expsTemp = GetAssistsString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_SHOTS_FIRED" ) == 0  )
				expsTemp = GetShotsFiredString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_SHOTS_HIT" ) == 0  )
				expsTemp = GetShotsHitString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_HIT_PERCENTAGE" ) == 0  )
				expsTemp = GetHitPercentageString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_TIMES_WOUNDED" ) == 0  )
				expsTemp = GetTimesWoundedString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_ACHIEVEMENTS" ) == 0  )
				expsTemp = GetAchievementsString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_SKILLS" ) == 0  )
			{
				UINT16 uiHowManySkills = 0;
				for( INT16 iS = ERP; (iS < gExtendedPanelsSettings.uepRowCapacity[PP]) && ( strcmp(gExtendedPanelsSettings.sepPanel[PP][iS], "EXTENDED_PANEL_SKILLS" ) == 0 ) ; iS++)
				{
					uiHowManySkills++;
				}
				EXTENDED_PANEL_STRING *expsTemps = GetSkillsStrings(pSoldier,uiHowManySkills);
				for( INT16 iS = 0; iS < uiHowManySkills; iS++)
				{
					//override if type set to show only label or value 
					if( ( strcmp( gExtendedPanelsSettings.sepTypes[PP][ERP+iS], "BOTH") != 0) )
						if( strcmp( gExtendedPanelsSettings.sepTypes[PP][ERP+iS], "LABEL") == 0 )
							swprintf(expsTemps[iS].sPanelStringRight,L"");
						else if( strcmp( gExtendedPanelsSettings.sepTypes[PP][ERP+iS], "VALUE") == 0 )
							swprintf(expsTemps[iS].sPanelStringLeft,L"");

					//override colors if given 
					if( gExtendedPanelsSettings.sepLabelColors[PP][ERP+iS] != 0 ) 
						expsTemps[iS].uiFontColorLeft = gExtendedPanelsSettings.sepLabelColors[PP][ERP+iS];
					if( gExtendedPanelsSettings.sepValueColors[PP][ERP+iS] != 0 )
						expsTemps[iS].uiFontColorRight = gExtendedPanelsSettings.sepValueColors[PP][ERP+iS];
					//override fonts if given
					expsTemps[iS].uiFontL = ReturnFont(gExtendedPanelsSettings.sepLabelFonts[PP][ERP+iS]);
					expsTemps[iS].uiFontR = ReturnFont(gExtendedPanelsSettings.sepValueFonts[PP][ERP+iS]);
					WriteStringOnPanel(expsTemps[iS], ERP+iS, uiExtendedPanelX[PP], uiExtendedPanelY, uiExtendedPanelWidth, uiExtendedPanelRowDist);
				}
				bIgnoreNext = TRUE;
				ERP += uiHowManySkills-1;
			}
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_DISABILITY" ) == 0 )
				expsTemp = GetDisabilityString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_CHARACTER" ) == 0  )
				expsTemp = GetCharacterString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_ATTITUDE_OLD" ) == 0  )
				expsTemp = GetAttitudeOldString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_SQUAD" ) == 0  )
				expsTemp = GetSquadString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_TOTAL_COST" ) == 0  )
				expsTemp = GetTotalCostString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_TOTAL_SERVICE" ) == 0  )
				expsTemp = GetTotalServiceString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_MEDICAL_DEPOSIT" ) == 0  )
				expsTemp = GetMedicalDepositString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_DAILY_COST" ) == 0  )
				expsTemp = GetDailyCostString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_CURRENT_CONTRACT" ) == 0  )
				expsTemp = GetCurrentContractString(pSoldier);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_AGILITY" ) == 0  )
				expsTemp = GetStatString(pSoldier, EXTENDED_PANEL_AGILITY);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_DEXTERITY" ) == 0  )
				expsTemp = GetStatString(pSoldier, EXTENDED_PANEL_DEXTERITY);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_EXP_LVL" ) == 0  )
				expsTemp = GetStatString(pSoldier, EXTENDED_PANEL_EXP_LVL);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_MARKSMANSHIP" ) == 0  )
				expsTemp = GetStatString(pSoldier, EXTENDED_PANEL_MARKSMANSHIP);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_STRENGTH" ) == 0  )
				expsTemp = GetStatString(pSoldier, EXTENDED_PANEL_STRENGTH);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_EXPLOSIVES" ) == 0  )
				expsTemp = GetStatString(pSoldier, EXTENDED_PANEL_EXPLOSIVES);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_LEADERSHIP" ) == 0  )
				expsTemp = GetStatString(pSoldier, EXTENDED_PANEL_LEADERSHIP);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_WISDOM" ) == 0  )
				expsTemp = GetStatString(pSoldier, EXTENDED_PANEL_WISDOM);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_MECHANICAL" ) == 0  )
				expsTemp = GetStatString(pSoldier, EXTENDED_PANEL_MECHANICAL);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_MEDICAL" ) == 0  )
				expsTemp = GetStatString(pSoldier, EXTENDED_PANEL_MEDICAL);
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_EMPTY" ) == 0  )
				expsTemp = GetEmptyString();
			else if( strcmp( gExtendedPanelsSettings.sepPanel[PP][ERP], "EXTENDED_PANEL_BREAK" ) == 0  )
				ERP=uiExtendedPanelMaxRow;
			else
			{
				// STR8 to STR16 (wchar_t* to char*)
				CHAR16  sString16[64];
				swprintf(sString16, sizeof(sString16), L"%hs", gExtendedPanelsSettings.sepPanel[PP][ERP]);

				expsTemp = GetTextString( sString16, sString16 );
			}
			//check if not part of trait display
			if(!bIgnoreNext)
			{
				//override if type set to show only label or value 
				if( ( strcmp( gExtendedPanelsSettings.sepTypes[PP][ERP], "BOTH") != 0) )
				{
					if( strcmp( gExtendedPanelsSettings.sepTypes[PP][ERP], "LABEL") == 0 )
						swprintf(expsTemp.sPanelStringRight,L"");
					else if( strcmp( gExtendedPanelsSettings.sepTypes[PP][ERP], "VALUE") == 0 )
						swprintf(expsTemp.sPanelStringLeft,L"");
				}

				//override colors if given 
				if( gExtendedPanelsSettings.sepLabelColors[PP][ERP] != 0 ) 
					expsTemp.uiFontColorLeft = gExtendedPanelsSettings.sepLabelColors[PP][ERP];
				if( gExtendedPanelsSettings.sepValueColors[PP][ERP] != 0 )
					expsTemp.uiFontColorRight = gExtendedPanelsSettings.sepValueColors[PP][ERP];

				//override fonts if given
				expsTemp.uiFontL = ReturnFont(gExtendedPanelsSettings.sepLabelFonts[PP][ERP]);
				expsTemp.uiFontR = ReturnFont(gExtendedPanelsSettings.sepValueFonts[PP][ERP]);

				WriteStringOnPanel(expsTemp, ERP, uiExtendedPanelX[PP], uiExtendedPanelY, uiExtendedPanelWidth, uiExtendedPanelRowDist);
			}
		}
	}        
	return true;
}

EXTENDED_PANEL_STRING GetNameString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	swprintf(exps.sPanelStringLeft, L"%s", gMercProfiles[pSoldier->ubProfile].zName);
	swprintf(exps.sPanelStringRight, L"");
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	return exps;
}


EXTENDED_PANEL_STRING GetKillsString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	UINT16 uiKills = gMercProfiles[pSoldier->ubProfile].records.usKillsAdmins //kills
		+gMercProfiles[pSoldier->ubProfile].records.usKillsCreatures
		+gMercProfiles[pSoldier->ubProfile].records.usKillsElites
		+gMercProfiles[pSoldier->ubProfile].records.usKillsHostiles
		+gMercProfiles[pSoldier->ubProfile].records.usKillsOthers
		+gMercProfiles[pSoldier->ubProfile].records.usKillsRegulars
		+gMercProfiles[pSoldier->ubProfile].records.usKillsTanks;
	swprintf(exps.sPanelStringLeft, L"%s", pPersonnelScreenStrings[PRSNL_TXT_KILLS]);
	swprintf(exps.sPanelStringRight, L"%d", uiKills );

	if( uiKills >= gExtendedPanelsSettings.uepAwesomeKills )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepAwesomeColor;
	else if(uiKills >= gExtendedPanelsSettings.uepGreatKills)
		exps.uiFontColorRight = gExtendedPanelsSettings.uepGreatColor;
	else if(uiKills <= gExtendedPanelsSettings.uepAwfulKills)
		exps.uiFontColorRight = gExtendedPanelsSettings.uepAwfulColor;
	else if(uiKills <= gExtendedPanelsSettings.uepPoorKills)
		exps.uiFontColorRight = gExtendedPanelsSettings.uepPoorColor;
	else
		exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;

	return exps;
}

EXTENDED_PANEL_STRING GetShotsFiredString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	UINT16 uiShotsFired = gMercProfiles[pSoldier->ubProfile].records.usShotsFired;    
	swprintf(exps.sPanelStringLeft, L"%s\0", pPersonnelRecordsHelpTexts[ 10 ] );
	swprintf(exps.sPanelStringRight, L"%d", uiShotsFired );

	//remove that damn "%d\n"
	exps.sPanelStringLeft[wcslen(exps.sPanelStringLeft) - 4] = L'\0';

	exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;   
	return exps;
}

EXTENDED_PANEL_STRING GetShotsHitString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	UINT16 uiShotsHit = gMercProfiles[pSoldier->ubProfile].records.usShotsHit;
	swprintf(exps.sPanelStringLeft, L"%s", pPersonnelRecordsHelpTexts[ 16 ] );
	swprintf(exps.sPanelStringRight, L"%d", uiShotsHit );

	//remove that damn "%d\n"
	exps.sPanelStringLeft[wcslen(exps.sPanelStringLeft) - 4] = L'\0';

	exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;   
	return exps;
}

EXTENDED_PANEL_STRING GetAssistsString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	swprintf(exps.sPanelStringLeft, L"%s", pPersonnelScreenStrings[PRSNL_TXT_ASSISTS]);
	UINT16 uiAssists = gMercProfiles[pSoldier->ubProfile].records.usAssistsMercs //assists
		+gMercProfiles[pSoldier->ubProfile].records.usAssistsMilitia
		+gMercProfiles[pSoldier->ubProfile].records.usAssistsOthers;   
	swprintf(exps.sPanelStringRight, L"%d", uiAssists );

	if( uiAssists >= gExtendedPanelsSettings.uepAwesomeAssists )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepAwesomeColor;
	else if( uiAssists >= gExtendedPanelsSettings.uepGreatAssists )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepGreatColor;
	else if( uiAssists <= gExtendedPanelsSettings.uepAwfulAssists )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepAwfulColor;
	else if( uiAssists <= gExtendedPanelsSettings.uepPoorAssists )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepPoorColor;
	else
		exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;

	return exps;
}

EXTENDED_PANEL_STRING GetAchievementsString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	swprintf(exps.sPanelStringLeft, L"%s", pPersonnelScreenStrings[PRSNL_TXT_ACHIEVEMNTS]);


	UINT16 uiAchievements = CalculateMercsAchievemntPercentage( pSoldier->ubProfile ); // achievements
	swprintf(exps.sPanelStringRight, L"%d %%",uiAchievements);

	if( uiAchievements >= gExtendedPanelsSettings.uepAwesomeAchievements )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepAwesomeColor;
	else if( uiAchievements >= gExtendedPanelsSettings.uepGreatAchievements )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepGreatColor;
	else if( uiAchievements <= gExtendedPanelsSettings.uepAwfulAchievements )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepAwfulColor;
	else if( uiAchievements <= gExtendedPanelsSettings.uepPoorAchievements)
		exps.uiFontColorRight = gExtendedPanelsSettings.uepPoorColor;
	else
		exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;

	return exps;
}

EXTENDED_PANEL_STRING GetHitPercentageString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	swprintf( exps.sPanelStringLeft, L"%s", pPersonnelScreenStrings[PRSNL_TXT_HIT_PERCENTAGE]);
	//hit percentage
	UINT32 uiHits = ( UINT32 )gMercProfiles[pSoldier->ubProfile].records.usShotsHit;
	uiHits *= 100;
	// check we have shot at least once
	if( (gMercProfiles[pSoldier->ubProfile].records.usShotsFired 
		+gMercProfiles[pSoldier->ubProfile].records.usMissilesLaunched 
		+gMercProfiles[pSoldier->ubProfile].records.usGrenadesThrown
		+gMercProfiles[pSoldier->ubProfile].records.usKnivesThrown
		+gMercProfiles[pSoldier->ubProfile].records.usBladeAttacks
		+gMercProfiles[pSoldier->ubProfile].records.usHtHAttacks) > 0 )
	{
		uiHits /= ( UINT32 )(gMercProfiles[pSoldier->ubProfile].records.usShotsFired 
			+gMercProfiles[pSoldier->ubProfile].records.usMissilesLaunched 
			+gMercProfiles[pSoldier->ubProfile].records.usGrenadesThrown 
			+gMercProfiles[pSoldier->ubProfile].records.usKnivesThrown 
			+gMercProfiles[pSoldier->ubProfile].records.usBladeAttacks 
			+gMercProfiles[pSoldier->ubProfile].records.usHtHAttacks);
		if ( uiHits > 100 )
			uiHits = 100;
	}
	else
	{
		// no, set hit % to 0
		uiHits=0;
	}
	swprintf( exps.sPanelStringRight, L"%d %%",uiHits);
	if( uiHits >= gExtendedPanelsSettings.uepAwesomeAchievements )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepAwesomeColor;
	else if( uiHits >= gExtendedPanelsSettings.uepGreatAchievements )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepGreatColor;
	else if( uiHits <= gExtendedPanelsSettings.uepAwfulAchievements )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepAwfulColor;
	else if( uiHits <= gExtendedPanelsSettings.uepPoorAchievements )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepPoorColor;
	else
		exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;

	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	return exps;
}

EXTENDED_PANEL_STRING GetBattlesString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	//mprintf
	swprintf( exps.sPanelStringLeft, L"%s", pPersonnelScreenStrings[PRSNL_TXT_BATTLES]);
	UINT16 uiBattles = gMercProfiles[pSoldier->ubProfile].records.usBattlesTactical//battles
		+gMercProfiles[pSoldier->ubProfile].records.usBattlesTactical;
	swprintf( exps.sPanelStringRight, L"%d", uiBattles );
	if( uiBattles >= gExtendedPanelsSettings.uepAwesomeBattles )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepAwesomeColor;
	else if( uiBattles >= gExtendedPanelsSettings.uepGreatBattles )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepGreatColor;
	else if( uiBattles <= gExtendedPanelsSettings.uepAwfulBattles )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepAwfulColor;
	else if( uiBattles <= gExtendedPanelsSettings.uepPoorBattles )
		exps.uiFontColorRight = gExtendedPanelsSettings.uepPoorColor;
	else
		exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	return exps;
}

EXTENDED_PANEL_STRING GetTimesWoundedString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	swprintf(exps.sPanelStringLeft, L"%s", pPersonnelScreenStrings[PRSNL_TXT_TIMES_WOUNDED]);
	swprintf(exps.sPanelStringRight, L"%d",gMercProfiles[pSoldier->ubProfile].records.usTimesWoundedShot//wounds
		+gMercProfiles[pSoldier->ubProfile].records.usTimesWoundedStabbed
		+gMercProfiles[pSoldier->ubProfile].records.usTimesWoundedPunched/2
		+gMercProfiles[pSoldier->ubProfile].records.usTimesWoundedBlasted);
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	return exps;
}

EXTENDED_PANEL_STRING *GetSkillsStrings(SOLDIERTYPE* pSoldier, UINT16 uiSkills)
{

	EXTENDED_PANEL_STRING *sRet = new EXTENDED_PANEL_STRING[4];
	if ( !AM_A_ROBOT(pSoldier) )
	{
		if (gGameOptions.fNewTraitSystem) // SANDRO - old/new traits check
		{
			UINT8 ubTempSkillArray[30];
			INT8 bNumSkillTraits = 0;
			// lets rearrange our skills to a temp array
			// we also get the number of lines (skills) to be displayed 
			for ( UINT8 ubCnt = 1; ubCnt < NUM_SKILLTRAITS_NT; ubCnt++ )
			{
				if ( ProfileHasSkillTrait( pSoldier->ubProfile, ubCnt ) == 2 )
				{	
					ubTempSkillArray[bNumSkillTraits] = (ubCnt + NEWTRAIT_MERCSKILL_EXPERTOFFSET);
					bNumSkillTraits++;
				}
				else if ( ProfileHasSkillTrait( pSoldier->ubProfile, ubCnt ) == 1 )
				{
					ubTempSkillArray[bNumSkillTraits] = ubCnt;
					bNumSkillTraits++;
				}
			}

			if ( bNumSkillTraits == 0 )
			{
				swprintf( sRet[0].sPanelStringLeft, L"%s", pPersonnelScreenStrings[ PRSNL_TXT_SKILLS ] );
				sRet[0].uiFontColorLeft = gExtendedPanelsSettings.uepNormalColor;

				swprintf( sRet[0].sPanelStringRight, L"%s", pPersonnelScreenStrings[ PRSNL_TXT_NOSKILLS ] );
				sRet[0].uiFontColorRight = gExtendedPanelsSettings.uepAwfulColor;

				for( INT16 iC = 1; iC<uiSkills; iC++ )
				{
					sRet[iC].uiFontColorLeft = gExtendedPanelsSettings.uepNormalColor;	
					sRet[iC].uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;	
					swprintf(sRet[iC].sPanelStringLeft, L"");	
					swprintf(sRet[iC].sPanelStringRight, L"");	
				}
			}
			else
			{
				CHAR16 sString2[1024];
				swprintf( sString2, L"" );

				BOOLEAN fDisplayMoreTraits = FALSE;

				for (UINT8 ubCnt = 0; ubCnt < min(uiSkills, bNumSkillTraits); ubCnt++)
				{
					if ( (ubCnt + 1 == uiSkills) && (uiSkills<bNumSkillTraits)) // display "more..."
					{
						sRet[ubCnt].uiFontColorLeft = gExtendedPanelsSettings.uepNormalColor;
						if(ubCnt==0)
							swprintf( sRet[ubCnt].sPanelStringLeft, L"%s", pPersonnelScreenStrings[ PRSNL_TXT_SKILLS ] );		
						else
							swprintf( sRet[ubCnt].sPanelStringLeft, L"" );
						sRet[ubCnt].uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
						//swprintf( sRet[ubCnt].sPanelStringRight, L"%s", gzMercSkillTextNew[ 2 * NEWTRAIT_MERCSKILL_OFFSET_ALL + 1 ] );
						swprintf(sRet[ubCnt].sPanelStringRight, L"%s", gzMercSkillTextNew[2 * NEWTRAIT_MERCSKILL_EXPERTOFFSET + 1]);
					}
					else
					{
						if ( ubTempSkillArray[ubCnt] > NEWTRAIT_MERCSKILL_EXPERTOFFSET )
							sRet[ubCnt].uiFontColorRight = gExtendedPanelsSettings.uepAwesomeColor;// double major trait
						else
						{
							if ( ubTempSkillArray[ubCnt] < NUM_MAJOR_TRAITS )
								sRet[ubCnt].uiFontColorRight = gExtendedPanelsSettings.uepGreatColor;//single major trait
							else
								sRet[ubCnt].uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;// minor trait
						}
						sRet[ubCnt].uiFontColorLeft = gExtendedPanelsSettings.uepNormalColor;	
						if(ubCnt==0)
							swprintf( sRet[ubCnt].sPanelStringLeft, L"%s", pPersonnelScreenStrings[ PRSNL_TXT_SKILLS ] );
						else
							swprintf( sRet[ubCnt].sPanelStringLeft, L"" );
						swprintf( sRet[ubCnt].sPanelStringRight, L"%s", gzMercSkillTextNew[ ubTempSkillArray[ubCnt] ] );
					}
				}
				//fill unfilled
				for ( UINT8 ubCnt = bNumSkillTraits; ubCnt < uiSkills; ubCnt++ )
				{
					sRet[ubCnt].uiFontColorLeft = gExtendedPanelsSettings.uepNormalColor;	
					sRet[ubCnt].uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;	
					swprintf( sRet[ubCnt].sPanelStringLeft, L"" );
					swprintf( sRet[ubCnt].sPanelStringRight, L"");
				}
			}
		}
		else // old trait system, for crying out loud, more copy/paste
		{						
			INT8 bSkill1 = 0, bSkill2 = 0; 	
			bSkill1 = gMercProfiles[ pSoldier->ubProfile ].bSkillTraits[0];
			bSkill2 = gMercProfiles[ pSoldier->ubProfile ].bSkillTraits[1];

			//if the 2 skills are the same, add the '(expert)' at the end
			if( bSkill1 == bSkill2 && bSkill1 != 0 )
			{
				sRet[0].uiFontColorLeft = gExtendedPanelsSettings.uepNormalColor;	
				sRet[0].uiFontColorRight = gExtendedPanelsSettings.uepGreatColor;	
				swprintf( sRet[0].sPanelStringLeft, L"%s", pPersonnelScreenStrings[ PRSNL_TXT_SKILLS ] );
				swprintf( sRet[0].sPanelStringRight, L"%s %s", gzMercSkillText[bSkill1], gzMercSkillText[EXPERT] );
				for ( UINT8 ubCnt = 1; ubCnt < uiSkills; ubCnt++ )
				{
					sRet[ubCnt].uiFontColorLeft = gExtendedPanelsSettings.uepNormalColor;	
					sRet[ubCnt].uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;	
					swprintf( sRet[ubCnt].sPanelStringLeft, L"" );
					swprintf( sRet[ubCnt].sPanelStringRight, L"");
				}
			}
			else
			{
				UINT16 uiDisplayedSkills = 0; 
				//Display the first skill
				if( bSkill1 != 0 )
				{
					sRet[0].uiFontColorLeft = gExtendedPanelsSettings.uepNormalColor;	
					sRet[0].uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;	
					swprintf( sRet[0].sPanelStringLeft, L"%s", pPersonnelScreenStrings[ PRSNL_TXT_SKILLS ] );
					swprintf( sRet[0].sPanelStringRight, L"%s", gzMercSkillText[bSkill1] );
					uiDisplayedSkills++;
				}

				//Display the second skill
				if( bSkill2 != 0 )
				{
					sRet[1].uiFontColorLeft = gExtendedPanelsSettings.uepNormalColor;	
					sRet[1].uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;	
					swprintf( sRet[1].sPanelStringLeft, L"");
					swprintf( sRet[1].sPanelStringRight, L"%s", gzMercSkillText[bSkill2] );
					uiDisplayedSkills++;
				}

				//if no skill was displayed
				if( bSkill1 == 0 && bSkill2 == 0 )
				{
					sRet[0].uiFontColorLeft = gExtendedPanelsSettings.uepNormalColor;	
					sRet[0].uiFontColorRight = gExtendedPanelsSettings.uepAwfulColor;	
					swprintf( sRet[0].sPanelStringLeft, L"%s", pPersonnelScreenStrings[ PRSNL_TXT_SKILLS ] );
					swprintf( sRet[0].sPanelStringRight, L"%s", pPersonnelScreenStrings[ PRSNL_TXT_NOSKILLS ] );
					uiDisplayedSkills++;
				}
				for ( UINT16 ubCnt = uiDisplayedSkills; ubCnt < uiSkills; ubCnt++ )
				{
					sRet[ubCnt].uiFontColorLeft = gExtendedPanelsSettings.uepNormalColor;	
					sRet[ubCnt].uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;	
					swprintf( sRet[ubCnt].sPanelStringLeft, L"" );
					swprintf( sRet[ubCnt].sPanelStringRight, L"");
				}
			}
		}		
	}
	else // robot - skills n/a
	{  
		sRet[0].uiFontColorLeft = gExtendedPanelsSettings.uepNormalColor;	
		sRet[0].uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
		swprintf( sRet[0].sPanelStringLeft, L"%s", pPersonnelScreenStrings[ PRSNL_TXT_SKILLS ] );
		swprintf( sRet[0].sPanelStringRight, L"%s", gpStrategicString[ STR_PB_NOTAPPLICABLE_ABBREVIATION ] );
		for ( UINT8 ubCnt = 1; ubCnt < uiSkills; ubCnt++ )
		{
			sRet[ubCnt].uiFontColorLeft = gExtendedPanelsSettings.uepNormalColor;	
			sRet[ubCnt].uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;	
			swprintf( sRet[ubCnt].sPanelStringLeft, L"" );
			swprintf( sRet[ubCnt].sPanelStringRight, L"");
		}
	}
	return sRet;
}

EXTENDED_PANEL_STRING GetDisabilityString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	swprintf( exps.sPanelStringLeft, L"%s", pPersonnelRecordsHelpTexts[44] );
	swprintf( exps.sPanelStringRight, L"%s", gzIMPDisabilityTraitText[gMercProfiles[pSoldier->ubProfile].bDisability]);
	if(gMercProfiles[pSoldier->ubProfile].bDisability == 0)
		exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	else
		exps.uiFontColorRight = gExtendedPanelsSettings.uepAwfulColor;
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;

	return exps;
}

EXTENDED_PANEL_STRING GetCharacterString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	if (gGameOptions.fNewTraitSystem)			
		swprintf(exps.sPanelStringLeft, L"%s",pPersonnelRecordsHelpTexts[43]); //L"Character:"
	else
		swprintf(exps.sPanelStringLeft, L"%s",pPersonnelRecordsHelpTexts[45]); //L"Attitudes:"

	if ( gGameOptions.fNewTraitSystem)
		swprintf(exps.sPanelStringRight, L"%s",gzIMPCharacterTraitText[gMercProfiles[pSoldier->ubProfile].bCharacterTrait]);
	else
		swprintf(exps.sPanelStringRight, L"%s",gzIMPCharacterTraitText[gMercProfiles[pSoldier->ubProfile].bAttitude]);

	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	return exps;
}

EXTENDED_PANEL_STRING GetAttitudeOldString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	if (gGameOptions.fNewTraitSystem)		
	{
		swprintf(exps.sPanelStringLeft, L"%s",pPersonnelRecordsHelpTexts[45]); //L"Attitudes:"
		swprintf(exps.sPanelStringRight, L"%s",gzIMPCharacterTraitText[gMercProfiles[pSoldier->ubProfile].bAttitude]);
	}
	else
	{
		swprintf(exps.sPanelStringLeft, L"");
		swprintf(exps.sPanelStringRight, L"");
	}

	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	return exps;
}

EXTENDED_PANEL_STRING GetCurrentContractString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	swprintf( exps.sPanelStringLeft, L"%s", pPersonnelScreenStrings[PRSNL_TXT_CURRENT_CONTRACT] );
	static const UINT32 uiMinutesInDay = 24 * 60;
	if( pSoldier->ubWhatKindOfMercAmI == MERC_TYPE__AIM_MERC || pSoldier->ubProfile == SLAY )
	{
		INT32 iTimeLeftOnContract = CalcTimeLeftOnMercContract( pSoldier );
		// if there is going to be a both days and hours left on the contract
		if( iTimeLeftOnContract / uiMinutesInDay )
		{
			swprintf( exps.sPanelStringRight, L"%d%s %d%s / %d%s",( iTimeLeftOnContract / uiMinutesInDay ), gpStrategicString[ STR_PB_DAYS_ABBREVIATION ], (iTimeLeftOnContract % uiMinutesInDay)/60, gpStrategicString[ STR_PB_HOURS_ABBREVIATION ], pSoldier->iTotalContractLength, gpStrategicString[ STR_PB_DAYS_ABBREVIATION ] );
			exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
		}
		//else there is under a day left
		else
		{
			swprintf( exps.sPanelStringRight, L"%d%s / %d%s", (iTimeLeftOnContract % uiMinutesInDay)/60, gpStrategicString[ STR_PB_HOURS_ABBREVIATION ], pSoldier->iTotalContractLength, gpStrategicString[ STR_PB_DAYS_ABBREVIATION ] );
			exps.uiFontColorRight = gExtendedPanelsSettings.uepAwfulColor;
		}
	}
	else
	{
		wcscpy( exps.sPanelStringRight, gpStrategicString[ STR_PB_NOTAPPLICABLE_ABBREVIATION ] );
		exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	}
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	return exps;
}

EXTENDED_PANEL_STRING GetTotalServiceString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	swprintf( exps.sPanelStringLeft, L"%s", pPersonnelScreenStrings[PRSNL_TXT_TOTAL_SERVICE] );
	swprintf( exps.sPanelStringRight, L"%d %s",gMercProfiles[ pSoldier->ubProfile ].usTotalDaysServed, gpStrategicString[ STR_PB_DAYS_ABBREVIATION ] );
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	return exps;
}

EXTENDED_PANEL_STRING GetTotalCostString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	swprintf( exps.sPanelStringLeft, L"%s", pPersonnelScreenStrings[PRSNL_TXT_TOTAL_COST] );
	swprintf( exps.sPanelStringRight, L"%d", gMercProfiles[ pSoldier->ubProfile ].uiTotalCostToDate );
	InsertCommasForDollarFigure( exps.sPanelStringRight );
	InsertDollarSignInToString( exps.sPanelStringRight );
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	return exps;
}

EXTENDED_PANEL_STRING GetMedicalDepositString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	if( pSoldier->ubWhatKindOfMercAmI == MERC_TYPE__MERC )
		swprintf( exps.sPanelStringLeft, L"%s", pPersonnelScreenStrings[PRSNL_TXT_UNPAID_AMOUNT]);// med depo / debt
	else
		swprintf( exps.sPanelStringLeft, L"%s", pPersonnelScreenStrings[PRSNL_TXT_MED_DEPOSIT]);
	if( pSoldier->ubWhatKindOfMercAmI == MERC_TYPE__MERC )
		swprintf( exps.sPanelStringRight, L"%d", gMercProfiles[ pSoldier->ubProfile ].sSalary * gMercProfiles[pSoldier->ubProfile ].iMercMercContractLength );
	else
		swprintf( exps.sPanelStringRight, L"%d", gMercProfiles[ pSoldier->ubProfile ].sMedicalDepositAmount);
	InsertCommasForDollarFigure( exps.sPanelStringRight );
	InsertDollarSignInToString( exps.sPanelStringRight );
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	return exps;
}

EXTENDED_PANEL_STRING GetDailyCostString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	swprintf(exps.sPanelStringLeft, L"%s", pPersonnelScreenStrings[PRSNL_TXT_DAILY_COST]);

	if( pSoldier->ubWhatKindOfMercAmI == MERC_TYPE__AIM_MERC)
	{
		// daily rate
		if( pSoldier->bTypeOfLastContract == CONTRACT_EXTEND_2_WEEK )
			// 2 week contract
			swprintf( exps.sPanelStringRight, L"%d", gMercProfiles[pSoldier->ubProfile].uiBiWeeklySalary / 14 );
		else if( pSoldier->bTypeOfLastContract == CONTRACT_EXTEND_1_WEEK )
			// 1 week contract
			swprintf( exps.sPanelStringRight, L"%d", gMercProfiles[pSoldier->ubProfile].uiWeeklySalary / 7 );
		else
			swprintf( exps.sPanelStringRight, L"%d", gMercProfiles[pSoldier->ubProfile].sSalary );
	}
	else
	{
		swprintf( exps.sPanelStringRight, L"%d", gMercProfiles[pSoldier->ubProfile].sSalary );
	}
	InsertCommasForDollarFigure( exps.sPanelStringRight );
	InsertDollarSignInToString( exps.sPanelStringRight );
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	return exps;
}

EXTENDED_PANEL_STRING GetSquadString(SOLDIERTYPE* pSoldier)
{
	EXTENDED_PANEL_STRING exps;
	swprintf(exps.sPanelStringLeft, L"%s", pUpperLeftMapScreenStrings[ 0 ]);
	swprintf(exps.sPanelStringRight, L"%s", pAssignmentStrings[pSoldier->bAssignment] );
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	return exps;
}

EXTENDED_PANEL_STRING GetStatString(SOLDIERTYPE* pSoldier, UINT16 uiStat)
{
	EXTENDED_PANEL_STRING exps;
	if(AM_A_ROBOT(pSoldier))
	{
		wcscpy( exps.sPanelStringRight, gpStrategicString[ STR_PB_NOTAPPLICABLE_ABBREVIATION ] );
	}
	else
	{
		UINT16 uiStatValue = 0;
		CHAR16 sStatLabel[32];
		switch (uiStat)
		{
		case EXTENDED_PANEL_AGILITY:
			uiStatValue = gMercProfiles[pSoldier->ubProfile].bAgility;
			swprintf(sStatLabel, L"%s", pPersonnelScreenStrings[PRSNL_TXT_AGILITY]);
			break;
		case EXTENDED_PANEL_DEXTERITY:
			uiStatValue = gMercProfiles[pSoldier->ubProfile].bDexterity;
			swprintf(sStatLabel, L"%s", pPersonnelScreenStrings[PRSNL_TXT_DEXTERITY]);
			break;
		case EXTENDED_PANEL_STRENGTH:
			uiStatValue = gMercProfiles[pSoldier->ubProfile].bStrength;
			swprintf(sStatLabel, L"%s", pPersonnelScreenStrings[PRSNL_TXT_STRENGTH]);
			break;
		case EXTENDED_PANEL_LEADERSHIP:
			uiStatValue = gMercProfiles[pSoldier->ubProfile].bLeadership;
			swprintf(sStatLabel, L"%s", pPersonnelScreenStrings[PRSNL_TXT_LEADERSHIP]);
			break;
		case EXTENDED_PANEL_WISDOM:
			uiStatValue = gMercProfiles[pSoldier->ubProfile].bWisdom;
			swprintf(sStatLabel, L"%s", pPersonnelScreenStrings[PRSNL_TXT_WISDOM]);
			break;
		case EXTENDED_PANEL_EXP_LVL:
			uiStatValue = gMercProfiles[pSoldier->ubProfile].bExpLevel;
			swprintf(sStatLabel, L"%s", pPersonnelScreenStrings[PRSNL_TXT_EXP_LVL]);
			break;
		case EXTENDED_PANEL_MARKSMANSHIP:
			uiStatValue = gMercProfiles[pSoldier->ubProfile].bMarksmanship;
			swprintf(sStatLabel, L"%s", pPersonnelScreenStrings[PRSNL_TXT_MARKSMANSHIP]);
			break;
		case EXTENDED_PANEL_MECHANICAL:
			uiStatValue = gMercProfiles[pSoldier->ubProfile].bMechanical;
			swprintf(sStatLabel, L"%s", pPersonnelScreenStrings[PRSNL_TXT_MECHANICAL]);
			break;
		case EXTENDED_PANEL_EXPLOSIVES:
			uiStatValue = gMercProfiles[pSoldier->ubProfile].bExplosive;
			swprintf(sStatLabel, L"%s", pPersonnelScreenStrings[PRSNL_TXT_EXPLOSIVES]);
			break;
		case EXTENDED_PANEL_MEDICAL:
			uiStatValue = gMercProfiles[pSoldier->ubProfile].bMedical;
			swprintf(sStatLabel, L"%s", pPersonnelScreenStrings[PRSNL_TXT_MEDICAL]);
			break;
		}      
		swprintf(exps.sPanelStringLeft, L"%s", sStatLabel );  
		swprintf(exps.sPanelStringRight, L"%d", uiStatValue );
		if(uiStat==EXTENDED_PANEL_EXP_LVL)
		{
			if( uiStatValue >= gExtendedPanelsSettings.uepAwesomeLevel )
				exps.uiFontColorRight = gExtendedPanelsSettings.uepAwesomeColor;
			else if( uiStatValue >= gExtendedPanelsSettings.uepGreatLevel )
				exps.uiFontColorRight = gExtendedPanelsSettings.uepGreatColor;
			else if( uiStatValue <= gExtendedPanelsSettings.uepAwfulLevel )
				exps.uiFontColorRight = gExtendedPanelsSettings.uepAwfulColor;
			else if( uiStatValue <= gExtendedPanelsSettings.uepPoorLevel )
				exps.uiFontColorRight = gExtendedPanelsSettings.uepPoorColor;
			else
				exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
		}
		else
		{
			if( uiStatValue >= gExtendedPanelsSettings.uepAwesomeStats )
				exps.uiFontColorRight = gExtendedPanelsSettings.uepAwesomeColor;
			else if( uiStatValue >= gExtendedPanelsSettings.uepGreatStats  )
				exps.uiFontColorRight = gExtendedPanelsSettings.uepGreatColor;
			else if( uiStatValue <= gExtendedPanelsSettings.uepAwfulStats  )
				exps.uiFontColorRight = gExtendedPanelsSettings.uepAwfulColor;
			else if( uiStatValue <= gExtendedPanelsSettings.uepPoorStats)
				exps.uiFontColorRight = gExtendedPanelsSettings.uepPoorColor;
			else
				exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
		}
		exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	}
	return exps;
}

EXTENDED_PANEL_STRING GetEmptyString()
{
	EXTENDED_PANEL_STRING exps;
	swprintf(exps.sPanelStringLeft, L"");
	swprintf(exps.sPanelStringRight, L"");
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	return exps;
}

EXTENDED_PANEL_STRING GetTextString(STR16 sStringL, STR16 sStringR)
{
	EXTENDED_PANEL_STRING exps;
	swprintf( exps.sPanelStringLeft, L"%s", sStringL );
	swprintf( exps.sPanelStringRight, L"%s", sStringR );
	exps.uiFontColorLeft = gExtendedPanelsSettings.uepDescriptionColor;
	exps.uiFontColorRight = gExtendedPanelsSettings.uepNormalColor;
	return exps;
}

INT32 ReturnFont(STR8 FontName)
{
	if ( strcmp(FontName, "LARGEFONT1") == 0 )
		return LARGEFONT1;
	else if ( strcmp(FontName, "SMALLFONT1") == 0 )
		return SMALLFONT1;
	else if ( strcmp(FontName, "TINYFONT1") == 0 )
		return TINYFONT1;
	else if ( strcmp(FontName, "CLOCKFONT") == 0 )
		return CLOCKFONT;
	else if ( strcmp(FontName, "COMPFONT") == 0 )
		return COMPFONT;
	else if ( strcmp(FontName, "SMALLCOMPFONT") == 0 )
		return SMALLCOMPFONT;
	else if ( strcmp(FontName, "FONT10ROMAN") == 0 )
		return FONT10ROMAN;
	else if ( strcmp(FontName, "FONT12ROMAN") == 0 )
		return FONT12ROMAN;
	else if ( strcmp(FontName, "FONT14SANSERIF") == 0 )
		return FONT14SANSERIF;
	else if ( strcmp(FontName, "MILITARYFONT1") == 0 )
		return MILITARYFONT1;
	//else if ( strcmp(FontName, "BLOCKFONT") == 0 )
		//return BLOCKFONT;
	else if ( strcmp(FontName, "FONT10ARIAL") == 0 )
		return FONT10ARIAL;
	else if ( strcmp(FontName, "FONT14ARIAL") == 0 )
		return FONT14ARIAL;
	else if ( strcmp(FontName, "FONT12ARIAL") == 0 )
		return FONT12ARIAL;
	else if ( strcmp(FontName, "FONT10ARIALBOLD") == 0 )
		return FONT10ARIALBOLD;
	else if ( strcmp(FontName, "BLOCKFONT") == 0 )
		return BLOCKFONT;
	else if ( strcmp(FontName, "BLOCKFONT2") == 0 )
		return BLOCKFONT2;
	else if ( strcmp(FontName, "BLOCKFONT3") == 0 )
		return BLOCKFONT3;
	else if ( strcmp(FontName, "FONT12ARIALFIXEDWIDTH") == 0 )
		return FONT12ARIALFIXEDWIDTH;
	else if ( strcmp(FontName, "FONT16ARIAL") == 0 )
		return FONT16ARIAL;
	else if ( strcmp(FontName, "BLOCKFONTNARROW") == 0 )
		return BLOCKFONTNARROW;
	else if ( strcmp(FontName, "FONT14HUMANIST") == 0 )
		return FONT14HUMANIST;
	else
		return FONT10ARIAL;
}

BOOLEAN WriteStringOnPanel(EXTENDED_PANEL_STRING expsString, UINT16 uiLine, UINT16 uiPanelX, UINT16 uiPanelY, UINT16 uiPanelWidth, UINT16 uiRowDist)
{
	SetFont(expsString.uiFontL);
	SetFontBackground( FONT_MCOLOR_BLACK );
	SetFontForeground( expsString.uiFontColorLeft );
	mprintf( uiPanelX, uiPanelY + uiLine * uiRowDist, L"%s", expsString.sPanelStringLeft );
	gprintfRestore( uiPanelX, uiPanelY + uiLine * uiRowDist, L"%s", expsString.sPanelStringLeft );

	SetFont(expsString.uiFontR);    
	SetFontBackground( FONT_MCOLOR_BLACK );
	SetFontForeground( expsString.uiFontColorRight );

	INT16 sFontX, sFontY;
	FindFontRightCoordinates(uiPanelX, 0, uiPanelWidth, 0,  expsString.sPanelStringRight, expsString.uiFontR, &sFontX, &sFontY );
	mprintf( sFontX, uiPanelY + uiLine * uiRowDist, L"%s", expsString.sPanelStringRight );
	gprintfRestore( sFontX, uiPanelY + uiLine * uiRowDist, L"%s", expsString.sPanelStringRight );
	return TRUE;
}

// sevenfm:
extern INT32 InvItemType( UINT16 usItem );
extern UINT8 GetTemperatureString( FLOAT overheatpercentage, UINT32* apRed, UINT32* apGreen, UINT32* abBlue );

/*extern UINT8 GetActionModeCursor( SOLDIERTYPE *pSoldier );
extern UINT32 guiCurrentUICursor;
extern ScrollStringStPtr gMapScreenMessageList[256];
extern UINT8 gubCurrentMapMessageString;
extern UINT8 gubStartOfMapScreenMessageList;
extern UINT8 gubEndOfMapScreenMessageList;*/

extern void ClearWrappedStrings( WRAPPED_STRING *pStringWrapperHead );

void PrepareTacticalPanelData( BOOLEAN fDivideWide, BOOLEAN fUseRightHalf, INT16 sOffsetX, INT16 sOffsetY )
{
	gsPanelTotalWidth = INTERFACE_WIDTH - 4 - INTERFACE_PANEL_RIGHT - gGameOptions.ubSquadSize * 83;

	gsPanelWideX = INTERFACE_START_X + 4 + gGameOptions.ubSquadSize*83;
	gsPanelWideWidth = gsPanelTotalWidth;

	gsPanelWideY = INTERFACE_START_Y;	
	gsPanelWideHeight = INTERFACE_HEIGHT;

	if( gsPanelTotalWidth > INTERFACE_PANEL_MEDIUM )
	{
		if( fDivideWide )
			gsPanelWideWidth = gsPanelTotalWidth / 2;

		if( fUseRightHalf )
			gsPanelWideX += gsPanelTotalWidth / 2;
	}

	gsPanelWindowX = gsPanelWideX + sOffsetX;
	gsPanelWindowWidth = gsPanelWideWidth - 2 * sOffsetX;
	gsPanelWindowY = gsPanelWideY + sOffsetY;
	gsPanelWindowHeight = gsPanelWideHeight - 2 * sOffsetY;

	// if not enough place of tactical panel, try to use left side
	//if (gsPanelWideWidth < INTERFACE_PANEL_SMALL && !gGameSettings.fOptions[TOPTION_TACTICAL_LEFT_PANEL] && gsCurInterfacePanel == TEAM_PANEL)
	if (gsPanelWideWidth < INTERFACE_PANEL_SMALL && gsCurInterfacePanel == TEAM_PANEL)
	{
		gsPanelTotalWidth = INTERFACE_START_X;

		gsPanelWideX = 0;
		gsPanelWideWidth = gsPanelTotalWidth;

		gsPanelWideY = INTERFACE_START_Y;
		gsPanelWideHeight = INTERFACE_HEIGHT;

		if (gsPanelTotalWidth >= INTERFACE_PANEL_BIG)
		{
			if (fDivideWide)
				gsPanelWideWidth = gsPanelTotalWidth / 2;

			if (fUseRightHalf)
				gsPanelWideX += gsPanelTotalWidth / 2;
		}

		gsPanelWindowX = gsPanelWideX + sOffsetX;
		gsPanelWindowWidth = gsPanelWideWidth - 2 * sOffsetX;
		gsPanelWindowY = gsPanelWideY + sOffsetY;
		gsPanelWindowHeight = gsPanelWideHeight - 2 * sOffsetY;
	}
}

void PrepareTacticalLeftPanelData(INT16 sOffsetX, INT16 sOffsetY)
{
	gsPanelTotalWidth = INTERFACE_START_X;

	gsPanelWideX = 0;
	gsPanelWideWidth = gsPanelTotalWidth;

	if (gsCurInterfacePanel == TEAM_PANEL)
	{
		gsPanelWideY = INTERFACE_START_Y;
		gsPanelWideHeight = INTERFACE_HEIGHT;
	}
	else
	{
		gsPanelWideY = INV_INTERFACE_START_Y;
		gsPanelWideHeight = INV_INTERFACE_HEIGHT;		
	}

	gsPanelWindowX = gsPanelWideX + sOffsetX;
	gsPanelWindowWidth = gsPanelWideWidth - 2 * sOffsetX;
	gsPanelWindowY = gsPanelWideY + sOffsetY;
	gsPanelWindowHeight = gsPanelWideHeight - 2 * sOffsetY;	
}

void PrepareTacticalRightPanelData(INT16 sOffsetX, INT16 sOffsetY)
{
	if (INTERFACE_START_X < INTERFACE_PANEL_MIN &&
		gsCurInterfacePanel == TEAM_PANEL &&
		!gGameSettings.fOptions[TOPTION_TACTICAL_PANEL])
	{
		PrepareTacticalPanelData(FALSE, FALSE, sOffsetX + 2, sOffsetY + 6);
		return;
	}

	gsPanelTotalWidth = INTERFACE_START_X;

	gsPanelWideX = INTERFACE_START_X + INTERFACE_WIDTH;
	gsPanelWideWidth = gsPanelTotalWidth;

	if (gsCurInterfacePanel == TEAM_PANEL)
	{
		gsPanelWideY = INTERFACE_START_Y;
		gsPanelWideHeight = INTERFACE_HEIGHT;
	}
	else
	{
		gsPanelWideY = INV_INTERFACE_START_Y;
		gsPanelWideHeight = INV_INTERFACE_HEIGHT;
	}	

	gsPanelWindowX = gsPanelWideX + sOffsetX;
	gsPanelWindowWidth = gsPanelWideWidth - 2 * sOffsetX;
	gsPanelWindowY = gsPanelWideY + sOffsetY;
	gsPanelWindowHeight = gsPanelWideHeight - 2 * sOffsetY;
}

void DrawTacticalLeftPanel(void)
{
	const int buf_size = 1024;
	CHAR16 pStr[buf_size];
	CHAR16 pStr1[buf_size];
	CHAR16 pStr2[buf_size];

	INT16 sX, sY;

	if( gsPanelTotalWidth < INTERFACE_PANEL_SMALL )
		return;	

	UINT16 usFontHeight = GetFontHeight( TINYFONT1 );
	UINT16	usLineWidthIfWordIsWiderThenWidth;
	INT32 cnt;
	INT32 lines;
	WRAPPED_STRING *pStringWrapper=NULL;
	WRAPPED_STRING *pStringWrapperHead=NULL;
	INT16 sLineY;
	
	SetFontDestBuffer(guiSAVEBUFFER, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, FALSE);

	if (ubTacticalMessagePanelMode == LEFT_PANEL_MODE_STATISTICS)
	{
		if (gusSelectedSoldier != NOBODY && MercPtrs[gusSelectedSoldier])
			DrawExtendedPanel(MercPtrs[gusSelectedSoldier]);
	}
	else if( ubTacticalMessagePanelMode == LEFT_PANEL_MODE_MESSAGE )
	{
		if (gTacticalMessageVector.size() > 0)
		{
			// show message log
			sX = gsPanelWindowX;
			sY = gsPanelWindowY;
			sLineY = gsPanelWindowY + gsPanelWindowHeight;

			INT32 sStart = gsTacticalMessagesOffset;
			INT32 sSize = gTacticalMessageVector.size();

			for (cnt = sStart;
				cnt < sSize && sLineY >= gsPanelWindowY - usFontHeight;
				cnt++)
			{
				pStringWrapperHead = LineWrap(TINYFONT1, gsPanelWindowWidth, &usLineWidthIfWordIsWiderThenWidth, (CHAR16*)gTacticalMessageVector[sSize - cnt - 1].c_str());
				pStringWrapper=pStringWrapperHead;

				// count lines
				lines = 0;
				while ( pStringWrapper!=NULL )
				{
					lines++;
					pStringWrapper = pStringWrapper->pNextWrappedString;
				}

				sLineY = sLineY - usFontHeight * lines;
				sY = sLineY;

				// skip lines that don't fit the window
				pStringWrapper = pStringWrapperHead;
				while (sY < gsPanelWindowY && pStringWrapper != NULL)
				{
					sY += usFontHeight;
					pStringWrapper = pStringWrapper->pNextWrappedString;
				}
				
				while(pStringWrapper!=NULL && sY <= gsPanelWindowY + gsPanelWindowHeight - GetFontHeight( TINYFONT1 ))
				{
					swprintf( pStr, buf_size, L"%s", pStringWrapper->sString);
					PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, (UINT8)gusTacticalMessageColorVector[sSize - cnt - 1], sX, sY, pStr);

					sY += usFontHeight;
					pStringWrapper=pStringWrapper->pNextWrappedString;
				}

				ClearWrappedStrings( pStringWrapperHead );

				sLineY -= usFontHeight / 4;
			}
		}
	}
	else if ( ubTacticalMessagePanelMode == LEFT_PANEL_MODE_DEBUG )
	{
		if (gTacticalDebugMessageVector.size() > 0)
		{
			// show message log
			sX = gsPanelWindowX;
			sY = gsPanelWindowY;
			sLineY = gsPanelWindowY + gsPanelWindowHeight;

			INT32 sStart = gsTacticalMessagesOffset;
			INT32 sSize = gTacticalDebugMessageVector.size();

			for (cnt = sStart;
				cnt < sSize && sLineY >= gsPanelWindowY - usFontHeight;
				cnt++)
			{
				pStringWrapperHead = LineWrap(TINYFONT1, gsPanelWindowWidth, &usLineWidthIfWordIsWiderThenWidth, (CHAR16*)gTacticalDebugMessageVector[sSize - cnt - 1].c_str());
				pStringWrapper = pStringWrapperHead;

				// count lines
				lines = 0;
				while (pStringWrapper != NULL)
				{
					lines++;
					pStringWrapper = pStringWrapper->pNextWrappedString;
				}

				sLineY = sLineY - usFontHeight * lines;
				sY = sLineY;

				// skip lines that don't fit the window
				pStringWrapper = pStringWrapperHead;
				while (sY < gsPanelWindowY && pStringWrapper != NULL)
				{
					sY += usFontHeight;
					pStringWrapper = pStringWrapper->pNextWrappedString;
				}

				while (pStringWrapper != NULL && sY <= gsPanelWindowY + gsPanelWindowHeight - GetFontHeight(TINYFONT1))
				{
					swprintf(pStr, buf_size, L"%s", pStringWrapper->sString);
					//PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, (UINT8)gusTacticalMessageColorVector[sSize - cnt - 1], sX, sY, pStr);
					PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_DKGRAY, sX, sY, pStr);

					sY += usFontHeight;
					pStringWrapper = pStringWrapper->pNextWrappedString;
				}

				ClearWrappedStrings(pStringWrapperHead);

				sLineY -= usFontHeight / 4;
			}
		}
	}
	else if( ubTacticalMessagePanelMode == LEFT_PANEL_MODE_AI )
	{
		if (gTacticalAIMessageVector.size() > 0)
		{
			// show message log
			sX = gsPanelWindowX;
			sY = gsPanelWindowY;
			sLineY = gsPanelWindowY + gsPanelWindowHeight;

			INT32 sStart = gsTacticalMessagesOffset;
			INT32 sSize = gTacticalAIMessageVector.size();

			for (cnt = sStart;
				cnt < sSize && sLineY >= gsPanelWindowY - usFontHeight;
				cnt++)
			{
				pStringWrapperHead = LineWrap(TINYFONT1, gsPanelWindowWidth, &usLineWidthIfWordIsWiderThenWidth, (CHAR16*)gTacticalAIMessageVector[sSize - cnt - 1].c_str());
				pStringWrapper = pStringWrapperHead;

				// count lines
				lines = 0;
				while (pStringWrapper != NULL)
				{
					lines++;
					pStringWrapper = pStringWrapper->pNextWrappedString;
				}

				sLineY = sLineY - usFontHeight * lines;
				sY = sLineY;

				// skip lines that don't fit the window
				pStringWrapper = pStringWrapperHead;
				while (sY < gsPanelWindowY && pStringWrapper != NULL)
				{
					sY += usFontHeight;
					pStringWrapper = pStringWrapper->pNextWrappedString;
				}

				while (pStringWrapper != NULL && sY <= gsPanelWindowY + gsPanelWindowHeight - GetFontHeight(TINYFONT1))
				{
					swprintf(pStr, buf_size, L"%s", pStringWrapper->sString);
					//PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, (UINT8)gusTacticalMessageColorVector[sSize - cnt - 1], sX, sY, pStr);
					PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_DKGRAY, sX, sY, pStr);

					sY += usFontHeight;
					pStringWrapper = pStringWrapper->pNextWrappedString;
				}

				ClearWrappedStrings(pStringWrapperHead);

				sLineY -= usFontHeight / 4;
			}
		}
	}
	else if (ubTacticalMessagePanelMode == LEFT_PANEL_MODE_HELP)
	{
		if (gTacticalHelpMessageVector.size() > 0)
		{
			// show message log
			sX = gsPanelWindowX;
			sY = gsPanelWindowY;
			sLineY = gsPanelWindowY + gsPanelWindowHeight;

			INT32 sStart = gsTacticalMessagesOffset;
			INT32 sSize = gTacticalHelpMessageVector.size();

			for (cnt = sStart;
				cnt < sSize && sLineY >= gsPanelWindowY - usFontHeight;
				cnt++)
			{
				pStringWrapperHead = LineWrap(TINYFONT1, gsPanelWindowWidth, &usLineWidthIfWordIsWiderThenWidth, (CHAR16*)gTacticalHelpMessageVector[sSize - cnt - 1].c_str());
				pStringWrapper = pStringWrapperHead;

				// count lines
				lines = 0;
				while (pStringWrapper != NULL)
				{
					lines++;
					pStringWrapper = pStringWrapper->pNextWrappedString;
				}

				sLineY = sLineY - usFontHeight * lines;
				sY = sLineY;

				// skip lines that don't fit the window
				pStringWrapper = pStringWrapperHead;
				while (sY < gsPanelWindowY && pStringWrapper != NULL)
				{
					sY += usFontHeight;
					pStringWrapper = pStringWrapper->pNextWrappedString;
				}

				while (pStringWrapper != NULL && sY <= gsPanelWindowY + gsPanelWindowHeight - GetFontHeight(TINYFONT1))
				{
					swprintf(pStr, buf_size, L"%s", pStringWrapper->sString);
					//PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, (UINT8)gusTacticalMessageColorVector[sSize - cnt - 1], sX, sY, pStr);
					CHAR16 *p = wcsstr(pStr, L":");
					if (wcsstr(pStr, L"[ ") && wcsstr(pStr, L" ]"))
					{
						PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_LTGRAY, sX + (gsPanelWideWidth - StringPixLength(pStr, TINYFONT1)) / 2, sY, pStr);
					}
					else if (p)
					{						
						wcscpy(pStr2, p);
						*p = 0;
						wcscpy(pStr1, pStr);
						PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_DKYELLOW, sX, sY, pStr1);
						PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_DKGRAY, sX + StringPixLength(pStr1, TINYFONT1), sY, pStr2);
					}
					else
					{
						PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_DKGRAY, sX, sY, pStr);
					}					

					sY += usFontHeight;
					pStringWrapper = pStringWrapper->pNextWrappedString;
				}

				ClearWrappedStrings(pStringWrapperHead);

				sLineY -= usFontHeight / 4;
			}
		}
	}

	RestoreExternBackgroundRect( gsPanelWideX, gsPanelWideY, gsPanelTotalWidth, gsPanelWideHeight );

	// reset to frame buffer!
	SetFontDestBuffer(FRAME_BUFFER, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, FALSE);
}

extern INT16 DayVisionRange( void );

STR16 gStr16Attitude[] = {L"DEFENSIVE", L"BRAVE", L"BRAVE", L"CUNNING", L"CUNNING", L"AGGRESSIVE", L"MAXATTITUDES", L"ATTACKSLAYONLY"};
STR16 gStr16Orders[] = {L"STATIONARY", L"ONGUARD", L"CLOSEPATROL", L"FARPATROL", L"PATROL", L"ONCALL", L"SEEKENEMY", L"PATROL", L"SNIPER"};

void DrawTacticalInfoPanel( SOLDIERTYPE *pSoldier )
{
	const int buf_size = 1024;
	CHAR16 pStr[buf_size];

	INT16 sX, sY;
	INT16 sCenterX, sCenterY;
	INT16 sTextWidth;

	// sets sTotalWidth, sWideX, sWideWidth, sWideY, sWideHeight, sWindowX, sWindowY, sWindowWidth, sWindowHeight
	PrepareTacticalPanelData( TRUE, TRUE, 8, 8 );

	if( gsPanelTotalWidth < INTERFACE_PANEL_SMALL )
		return;	

	UINT8 ubID = (UINT8)gusUIFullTargetID;
	SOLDIERTYPE *pTarget = MercPtrs[ubID];

	if( ubID == NOBODY || 
		!pTarget || 
		!pSoldier || 
		pTarget->stats.bLife <= 0 ||
		pTarget->bVisible == -1)
	{
		return;
	}

	UINT16 usDistance = PythSpacesAway(pSoldier->sGridNo, pTarget->sGridNo);
	INT32 iObserved = pTarget->usSkillCounter[SOLDIER_COUNTER_ROLE_OBSERVED];		
	INT16 sMaxDistanceVisible = pSoldier->GetMaxDistanceVisible( pTarget->sGridNo, pTarget->pathing.bLevel, CALC_FROM_WANTED_DIR );

	INT16 sTextOffsetY = 0;
	INT16 sCellY = gsPanelWindowHeight / 4;

	// show face
	VOBJECT_DESC VObjectDesc;	
	HVOBJECT hVObject;
	UINT32 uiFaceID;
	UINT16 usFaceIndex;

	BOOLEAN fFaceLoaded = PrepareFaceData(pTarget, uiFaceID, usFaceIndex, VObjectDesc, hVObject);

	INT16 sTextOffsetX = 0;

	if( fFaceLoaded && usFaceIndex != MAX_65_FACES )
	{
		SetObjectHandleShade( uiFaceID, 0 );
		BltVideoObjectFromIndex( guiSAVEBUFFER, uiFaceID, usFaceIndex, gsPanelWindowX, gsPanelWindowY + 2, VO_BLT_SRCTRANSPARENCY, NULL );

		sTextOffsetX = 31 + 4;
	}

	swprintf( pStr, buf_size, L"%s", pTarget->GetName());
	sTextWidth = StringPixLength(pStr, TINYFONT1 );				
	//sX = sWindowX;
	sX = gsPanelWindowX + sTextOffsetX;
	//sY = sWindowY + 30;
	sY = gsPanelWindowY;
	PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, COLOR_BROWN, sX, sY, pStr );
	
	// determine health
	INT32 cnt, cntStart;
	if( pTarget->stats.bLife == pTarget->stats.bLifeMax )
	{
		cntStart = 4;
	}
	else
	{
		cntStart = 0;
	}
	for ( cnt = cntStart; cnt < 6; cnt ++ )
	{
		if ( pTarget->stats.bLife < bHealthStrRanges[ cnt ] )
		{
			break;
		}
	}

	swprintf( pStr, buf_size, L"%s", zHealthStr[ cnt ]);
	//sX = sWindowX;
	sX = gsPanelWindowX + sTextOffsetX;
	//sY = sWindowY + 30 + sTextOffsetY;
	sY = gsPanelWindowY + GetFontHeight( TINYFONT1 );
	PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_DKRED, sX, sY, pStr );

	// shift inventory items to the right if text goes down too far
	INT16 sItemsOffsetX = 0;

	if( pTarget->IsMercBodyType() || pTarget->ubBodyType >= FATCIV && pTarget->ubBodyType <= CRIPPLECIV )
	{
		// morale
		CHAR16 pMoraleStr[ 64 ];
		GetMoraleString( pTarget, pMoraleStr );

		sX = gsPanelWindowX + sTextOffsetX;
		sY = gsPanelWindowY + 2 * GetFontHeight( TINYFONT1 );
		PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, FONT_GREEN, sX, sY, pMoraleStr );

		// skills
		//sTextOffsetY = 3 * GetFontHeight( TINYFONT1 ) + 4;
		sTextOffsetY = 4 * GetFontHeight( TINYFONT1 );
		UINT8 ubSkillColor = FONT_KHAKI;	// COLOR_BROWN;
		UINT8 ubOrdersColor = COLOR_BROWN;	// FONT_METALGRAY
		UINT8 ubSideColor = FONT_DKBLUE;	// FONT_MCOLOR_DKGRAY
		
		if( pTarget->IsZombie() )
		{			
			swprintf(pStr, buf_size, szSoldierClassName[10]);
			sX = gsPanelWindowX + sTextOffsetX;
			sY = gsPanelWindowY + 3 * GetFontHeight( TINYFONT1 );
			PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, FONT_LTKHAKI, sX, sY, pStr );
		}
		else if ( pTarget->IsPOW() )
		{
			swprintf( pStr, buf_size, L"%s", zHealthStr[ 7 ]);
			sX = gsPanelWindowX + sTextOffsetX;
			sY = gsPanelWindowY + 3 * GetFontHeight( TINYFONT1 );
			PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, FONT_METALGRAY, sX, sY, pStr );
		}
		else if( pTarget->bTeam == CIV_TEAM && pTarget->ubCivilianGroup > 0 )
		{
			swprintf( pStr, buf_size, L"%s", zCivGroupName[pTarget->ubCivilianGroup].szCurGroup);
			sX = gsPanelWindowX + sTextOffsetX;
			sY = gsPanelWindowY + 3 * GetFontHeight( TINYFONT1 );
			PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, COLOR_BROWN, sX, sY, pStr );
		}

		if( pTarget->bTeam == CIV_TEAM && pTarget->aiData.bNeutral )
		{
			swprintf(pStr, buf_size, pConditionStrings[COND_NEUTRAL]);
			sX = gsPanelWindowX;
			sY = gsPanelWindowY + sTextOffsetY;
			PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, FONT_LTBLUE, sX, sY, pStr );
			sTextOffsetY += GetFontHeight( TINYFONT1 );
			if( sTextOffsetY > gsPanelWindowHeight - sCellY )
				sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
		}
		else if( pTarget->aiData.bAlertStatus >= STATUS_RED )
		{
			swprintf(pStr, buf_size, pConditionStrings[COND_ALERTED]);
			sX = gsPanelWindowX;
			sY = gsPanelWindowY + sTextOffsetY;
			PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_DKRED, sX, sY, pStr );
			sTextOffsetY += GetFontHeight( TINYFONT1 );
			if( sTextOffsetY > gsPanelWindowHeight - sCellY )
				sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
		}

		if (pSoldier->bSleepDrugCounter)
		{
			swprintf(pStr, buf_size, pConditionStrings[COND_SLEEP_DRUG]);

			if (sTextOffsetY + GetFontHeight(TINYFONT1) < gsPanelWindowHeight)
			{
				sX = gsPanelWindowX;
				sY = gsPanelWindowY + sTextOffsetY;
				PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_DKGRAY, sX, sY, pStr);
				sTextOffsetY += GetFontHeight(TINYFONT1);
				if (sTextOffsetY > gsPanelWindowHeight - sCellY)
					sItemsOffsetX = max(StringPixLength(pStr, TINYFONT1), sItemsOffsetX);
			}
		}		

		if (pSoldier->bBreath < OKBREATH && pSoldier->bCollapsed)
		{
			swprintf(pStr, buf_size, pConditionStrings[COND_UNCONCIOUS]);

			if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
			{
				sX = gsPanelWindowX;
				sY = gsPanelWindowY + sTextOffsetY;
				PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_DKGRAY, sX, sY, pStr );
				sTextOffsetY += GetFontHeight( TINYFONT1 );
				if( sTextOffsetY > gsPanelWindowHeight - sCellY )
					sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
			}
		}
		else if (pSoldier->bCollapsed)
		{
			swprintf(pStr, buf_size, pConditionStrings[COND_COLLAPSED]);

			if (sTextOffsetY + GetFontHeight(TINYFONT1) < gsPanelWindowHeight)
			{
				sX = gsPanelWindowX;
				sY = gsPanelWindowY + sTextOffsetY;
				PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_DKGRAY, sX, sY, pStr);
				sTextOffsetY += GetFontHeight(TINYFONT1);
				if (sTextOffsetY > gsPanelWindowHeight - sCellY)
					sItemsOffsetX = max(StringPixLength(pStr, TINYFONT1), sItemsOffsetX);
			}
		}
		else if (pTarget->bBreath < OKBREATH)
		{
			swprintf(pStr, buf_size, pConditionStrings[COND_EXHAUSTED]);

			if (sTextOffsetY + GetFontHeight(TINYFONT1) < gsPanelWindowHeight)
			{
				sX = gsPanelWindowX;
				sY = gsPanelWindowY + sTextOffsetY;
				PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_DKGRAY, sX, sY, pStr);
				sTextOffsetY += GetFontHeight(TINYFONT1);
				if (sTextOffsetY > gsPanelWindowHeight - sCellY)
					sItemsOffsetX = max(StringPixLength(pStr, TINYFONT1), sItemsOffsetX);
			}
		}
		else if (pTarget->bBreath < 50)
		{
			swprintf(pStr, buf_size, pConditionStrings[COND_FATIGUED]);

			if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
			{
				sX = gsPanelWindowX;
				sY = gsPanelWindowY + sTextOffsetY;
				PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_DKGRAY, sX, sY, pStr );
				sTextOffsetY += GetFontHeight( TINYFONT1 );
				if( sTextOffsetY > gsPanelWindowHeight - sCellY )
					sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
			}
		}		

		if( pTarget->usSoldierFlagMask2 & SOLDIER_SUPRESSED_THIS_TURN )
		{
			swprintf(pStr, buf_size, pConditionStrings[COND_SUPPRESSED]);

			if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
			{
				sX = gsPanelWindowX;
				sY = gsPanelWindowY + sTextOffsetY;
				PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_DKGRAY, sX, sY, pStr );
				sTextOffsetY += GetFontHeight( TINYFONT1 );
				if( sTextOffsetY > gsPanelWindowHeight - sCellY )
					sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
			}
		}
		if( pTarget->IsCowering() )
		{
			swprintf(pStr, buf_size, pConditionStrings[COND_COWERING]);

			if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
			{
				sX = gsPanelWindowX;
				sY = gsPanelWindowY + sTextOffsetY;
				PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, FONT_ORANGE, sX, sY, pStr );
				sTextOffsetY += GetFontHeight( TINYFONT1 );
				if( sTextOffsetY > gsPanelWindowHeight - sCellY )
					sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
			}
		}	
		if( pTarget->bBlindedCounter > 0 )
		{
			swprintf(pStr, buf_size, pConditionStrings[COND_BLIND]);

			if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
			{
				sX = gsPanelWindowX;
				sY = gsPanelWindowY + sTextOffsetY;
				PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, FONT_ORANGE, sX, sY, pStr );
				sTextOffsetY += GetFontHeight( TINYFONT1 );
				if( sTextOffsetY > gsPanelWindowHeight - sCellY )
					sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
			}
		}
		if (pTarget->IsGassed())
		{
			swprintf(pStr, buf_size, pConditionStrings[COND_GASSED]);

			if (sTextOffsetY + GetFontHeight(TINYFONT1) < gsPanelWindowHeight)
			{
				sX = gsPanelWindowX;
				sY = gsPanelWindowY + sTextOffsetY;
				PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_ORANGE, sX, sY, pStr);
				sTextOffsetY += GetFontHeight(TINYFONT1);
				if (sTextOffsetY > gsPanelWindowHeight - sCellY)
					sItemsOffsetX = max(StringPixLength(pStr, TINYFONT1), sItemsOffsetX);
			}
		}
		if( pTarget->bDeafenedCounter > 0 )
		{
			swprintf(pStr, buf_size, pConditionStrings[COND_DEAF]);

			if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
			{
				sX = gsPanelWindowX;
				sY = gsPanelWindowY + sTextOffsetY;
				PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, FONT_ORANGE, sX, sY, pStr );
				sTextOffsetY += GetFontHeight( TINYFONT1 );
				if( sTextOffsetY > gsPanelWindowHeight - sCellY )
					sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
			}
		}		
		if( pTarget->bPoisonSum > 0 )
		{
			swprintf(pStr, buf_size, pConditionStrings[COND_POISONED]);

			if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
			{
				sX = gsPanelWindowX;
				sY = gsPanelWindowY + sTextOffsetY;
				PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, FONT_GREEN, sX, sY, pStr );
				sTextOffsetY += GetFontHeight( TINYFONT1 );
				if( sTextOffsetY > gsPanelWindowHeight - sCellY )
					sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
			}
		}

		// militia orders / attitude
		if( pTarget->bTeam == MILITIA_TEAM )
		{
			// orders
			swprintf( pStr, buf_size, gStr16Orders[pTarget->aiData.bOrders]);
			if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
			{
				sX = gsPanelWindowX;
				sY = gsPanelWindowY + sTextOffsetY;
				PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, ubOrdersColor, sX, sY, pStr );
				sTextOffsetY += GetFontHeight( TINYFONT1 );
				if( sTextOffsetY > gsPanelWindowHeight - sCellY )
					sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
			}
			// attitude
			swprintf( pStr, buf_size, gStr16Attitude[pTarget->aiData.bAttitude]);
			if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
			{
				sX = gsPanelWindowX;
				sY = gsPanelWindowY + sTextOffsetY;
				PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, ubOrdersColor, sX, sY, pStr );
				sTextOffsetY += GetFontHeight( TINYFONT1 );
				if( sTextOffsetY > gsPanelWindowHeight - sCellY )
					sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
			}
		}

		// show skills
		// only personally seen target
		if( pTarget->IsMercBodyType() &&
			!pTarget->IsZombie() &&
			pTarget->bTeam != CIV_TEAM &&
			pSoldier->aiData.bOppList[pTarget->ubID] == SEEN_CURRENTLY && usDistance < iObserved * DayVisionRange() / 8 ||
			pSoldier->bTeam == pTarget->bTeam ||
			pSoldier->bTeam == OUR_TEAM && pTarget->bTeam == MILITIA_TEAM ||
			CHEATER_CHEAT_LEVEL() )
		{
			if( pTarget->ubProfile != NO_PROFILE )
			{
				if (gGameOptions.fNewTraitSystem)
				{
					UINT8 ubTempSkillArray[30];
					INT8 bNumSkillTraits = 0;

					// lets rearrange our skills to a temp array
					// we also get the number of lines (skills) to be displayed 
					for ( UINT8 ubCnt = 1; ubCnt < NUM_SKILLTRAITS_NT; ubCnt++ )
					{
						if ( ProfileHasSkillTrait( pTarget->ubProfile, ubCnt ) == 2 )
						{
							ubTempSkillArray[bNumSkillTraits] = (ubCnt + NEWTRAIT_MERCSKILL_EXPERTOFFSET);
							bNumSkillTraits++;
						}
						else if ( ProfileHasSkillTrait( pTarget->ubProfile, ubCnt ) == 1 )
						{
							ubTempSkillArray[bNumSkillTraits] = ubCnt;
							bNumSkillTraits++;
						}
					}

					if ( bNumSkillTraits == 0 )
					{
						if(CHEATER_CHEAT_LEVEL())
						{
							swprintf(pStr, buf_size, L"%s", pPersonnelScreenStrings[PRSNL_TXT_NOSKILLS]);

							if (sTextOffsetY + GetFontHeight(TINYFONT1) < gsPanelWindowHeight)
							{
								sX = gsPanelWindowX;
								sY = gsPanelWindowY + sTextOffsetY;
								PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, ubSkillColor, sX, sY, pStr);
								sTextOffsetY += GetFontHeight(TINYFONT1);
								if (sTextOffsetY > gsPanelWindowHeight - sCellY)
									sItemsOffsetX = max(StringPixLength(pStr, TINYFONT1), sItemsOffsetX);
							}
						}
					}
					else
					{
						for ( UINT8 ubCnt = 0; ubCnt < bNumSkillTraits; ubCnt++ )
						{
							swprintf( pStr, buf_size, L"%s", gzMercSkillTextNew[ ubTempSkillArray[ubCnt] ] );

							if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
							{
								sX = gsPanelWindowX;
								sY = gsPanelWindowY + sTextOffsetY;
								PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, ubSkillColor, sX, sY, pStr );
								sTextOffsetY += GetFontHeight( TINYFONT1 );
								if( sTextOffsetY > gsPanelWindowHeight - sCellY )
									sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
							}
						}
					}
				}
				else
				{
					INT8 bSkill1 = 0, bSkill2 = 0; 	
					bSkill1 = gMercProfiles[ pTarget->ubProfile ].bSkillTraits[0];
					bSkill2 = gMercProfiles[ pTarget->ubProfile ].bSkillTraits[1];

					if ( bSkill1 == 0 && bSkill2 == 0 )
					{
						swprintf( pStr, buf_size, L"%s", pPersonnelScreenStrings[ PRSNL_TXT_NOSKILLS ] );

						if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
						{
							sX = gsPanelWindowX;
							sY = gsPanelWindowY + sTextOffsetY;
							PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, ubSkillColor, sX, sY, pStr );
							sTextOffsetY += GetFontHeight( TINYFONT1 );
							if( sTextOffsetY > gsPanelWindowHeight - sCellY )
								sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
						}
					}
					else
					{
						//if the 2 skills are the same, add the '(expert)' at the end
						if( bSkill1 == bSkill2 )
						{
							swprintf( pStr, buf_size, L"%s %s", gzMercSkillText[bSkill1], gzMercSkillText[EXPERT] );

							if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
							{
								sX = gsPanelWindowX;
								sY = gsPanelWindowY + sTextOffsetY;
								PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, ubSkillColor, sX, sY, pStr );
								sTextOffsetY += GetFontHeight( TINYFONT1 );
								if( sTextOffsetY > gsPanelWindowHeight - sCellY )
									sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
							}
						}
						else
						{
							//Display the first skill
							if( bSkill1 != 0 )
							{
								swprintf( pStr, buf_size, L"%s", gzMercSkillText[bSkill1] );

								if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
								{
									sX = gsPanelWindowX;
									sY = gsPanelWindowY + sTextOffsetY;
									PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, ubSkillColor, sX, sY, pStr );
									sTextOffsetY += GetFontHeight( TINYFONT1 );
									if( sTextOffsetY > gsPanelWindowHeight - sCellY )
										sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
								}
							}
							if( bSkill2 != 0 )
							{
								swprintf( pStr, buf_size, L"%s", gzMercSkillText[bSkill2] );

								if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
								{
									sX = gsPanelWindowX;
									sY = gsPanelWindowY + sTextOffsetY;
									PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, ubSkillColor, sX, sY, pStr );
									sTextOffsetY += GetFontHeight( TINYFONT1 );
									if( sTextOffsetY > gsPanelWindowHeight - sCellY )
										sItemsOffsetX = max( StringPixLength(pStr, TINYFONT1 ), sItemsOffsetX );
								}
							}
						}
					}
				}
			}
			else
			{
				CHAR16 pStr1[buf_size];
				CHAR16 pStr2[buf_size];
				CHAR16 pStr3[buf_size];
				BOOLEAN fFound1 = FALSE;
				BOOLEAN fFound2 = FALSE;
				BOOLEAN fFound3 = FALSE;

				if ( gGameOptions.fNewTraitSystem )
				{
					if (( pTarget->stats.ubSkillTraits[0] == pTarget->stats.ubSkillTraits[1] ) && pTarget->stats.ubSkillTraits[0] != 0 )
					{
						swprintf( pStr1, buf_size, gzMercSkillTextNew[ pTarget->stats.ubSkillTraits[0] + NEWTRAIT_MERCSKILL_EXPERTOFFSET ] );
						fFound1 = TRUE;
						if( pTarget->stats.ubSkillTraits[2] != 0 )
						{
							swprintf( pStr2, buf_size, gzMercSkillTextNew[ pTarget->stats.ubSkillTraits[2] ]);
							fFound2= TRUE;
						}
					}
					else if (( pTarget->stats.ubSkillTraits[1] == pTarget->stats.ubSkillTraits[2] ) && pTarget->stats.ubSkillTraits[1] != 0 )
					{
						swprintf( pStr1, buf_size, gzMercSkillTextNew[ pTarget->stats.ubSkillTraits[1] + NEWTRAIT_MERCSKILL_EXPERTOFFSET ] );
						fFound1 = TRUE;
						if( pTarget->stats.ubSkillTraits[0] != 0 )
						{
							swprintf( pStr2, buf_size, gzMercSkillTextNew[ pTarget->stats.ubSkillTraits[0] ]);
							fFound2= TRUE;
						}
					}
					else if (( pTarget->stats.ubSkillTraits[0] == pTarget->stats.ubSkillTraits[2] ) && pTarget->stats.ubSkillTraits[0] != 0 )
					{
						swprintf( pStr1, buf_size, gzMercSkillTextNew[ pTarget->stats.ubSkillTraits[0] + NEWTRAIT_MERCSKILL_EXPERTOFFSET ] );
						fFound1 = TRUE;
						if( pTarget->stats.ubSkillTraits[1] != 0 )
						{
							swprintf( pStr2, buf_size, gzMercSkillTextNew[ pTarget->stats.ubSkillTraits[1] ]);
							fFound2= TRUE;
						}
					}
					else
					{
						if( pTarget->stats.ubSkillTraits[0] != 0 )
						{
							swprintf( pStr1, buf_size, gzMercSkillTextNew[ pTarget->stats.ubSkillTraits[0] ]);
							fFound1= TRUE;
						}
						if( pTarget->stats.ubSkillTraits[1] != 0 )
						{
							swprintf( pStr2, buf_size, gzMercSkillTextNew[ pTarget->stats.ubSkillTraits[1] ]);
							fFound2= TRUE;
						}
						if( pTarget->stats.ubSkillTraits[2] != 0 )
						{
							swprintf( pStr3, buf_size, gzMercSkillTextNew[ pTarget->stats.ubSkillTraits[2] ]);
							fFound3= TRUE;
						}
					}
				}
				else
				{
					if( pTarget->stats.ubSkillTraits[0] != 0 )
					{
						swprintf( pStr1, buf_size, gzMercSkillTextNew[ pTarget->stats.ubSkillTraits[0] ]);
						fFound1= TRUE;
					}
					if( pTarget->stats.ubSkillTraits[1] != 0 )
					{
						swprintf( pStr2, buf_size, gzMercSkillTextNew[ pTarget->stats.ubSkillTraits[1] ]);
						fFound2= TRUE;
					}
				}
				if( fFound1 )
				{
					if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
					{
						sX = gsPanelWindowX;
						sY = gsPanelWindowY + sTextOffsetY;
						PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, ubSkillColor, sX, sY, pStr1 );
						sTextOffsetY += GetFontHeight( TINYFONT1 );
						if( sTextOffsetY > gsPanelWindowHeight - sCellY )
							sItemsOffsetX = max( StringPixLength(pStr1, TINYFONT1 ), sItemsOffsetX );
					}
				}
				if( fFound2 )
				{
					if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
					{
						sX = gsPanelWindowX;
						sY = gsPanelWindowY + sTextOffsetY;
						PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, ubSkillColor, sX, sY, pStr2 );
						sTextOffsetY += GetFontHeight( TINYFONT1 );
						if( sTextOffsetY > gsPanelWindowHeight - sCellY )
							sItemsOffsetX = max( StringPixLength(pStr2, TINYFONT1 ), sItemsOffsetX );
					}
				}
				if( fFound3 )
				{
					if( sTextOffsetY + GetFontHeight( TINYFONT1 ) < gsPanelWindowHeight )
					{
						sX = gsPanelWindowX;
						sY = gsPanelWindowY + sTextOffsetY;
						PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK, ubSkillColor, sX, sY, pStr3 );
						sTextOffsetY += GetFontHeight( TINYFONT1 );
						if( sTextOffsetY > gsPanelWindowHeight - sCellY )
							sItemsOffsetX = max( StringPixLength(pStr3, TINYFONT1 ), sItemsOffsetX );
					}
				}
			}
		}		
	}	

	if( pTarget->inv[HELMETPOS].exists() )
	{
		sCenterX = gsPanelWindowX + gsPanelWindowWidth - 30 / 2;
		sCenterY = gsPanelWindowY + sCellY / 2;
		DrawBufferInterfacePic( pTarget->inv[HELMETPOS].usItem, sCenterX, sCenterY );
	}
	if( pTarget->inv[VESTPOS].exists() )
	{
		sCenterX = gsPanelWindowX + gsPanelWindowWidth - 30 / 2;
		sCenterY = gsPanelWindowY + sCellY + sCellY / 2;
		DrawBufferInterfacePic( pTarget->inv[VESTPOS].usItem, sCenterX, sCenterY );
	}
	if( pTarget->inv[LEGPOS].exists() )
	{
		sCenterX = gsPanelWindowX + gsPanelWindowWidth - 30 / 2;
		sCenterY = gsPanelWindowY + 2* sCellY + sCellY / 2;
		DrawBufferInterfacePic( pTarget->inv[LEGPOS].usItem, sCenterX, sCenterY );
	}
	if( pTarget->inv[HANDPOS].exists() )
	{
		sCenterX = gsPanelWindowX + gsPanelWindowWidth - 30 - GetInterfacePicWidth(pTarget->inv[HANDPOS].usItem) / 2;
		sCenterY = gsPanelWindowY + sCellY + sCellY / 2;
		DrawBufferInterfacePic( pTarget->inv[HANDPOS].usItem, sCenterX, sCenterY );
	}
	if( pTarget->inv[SECONDHANDPOS].exists() )
	{
		sCenterX = gsPanelWindowX + gsPanelWindowWidth - 30 - GetInterfacePicWidth(pTarget->inv[SECONDHANDPOS].usItem) / 2;
		sCenterY = gsPanelWindowY + 2 * sCellY + sCellY / 2;
		DrawBufferInterfacePic( pTarget->inv[SECONDHANDPOS].usItem, sCenterX, sCenterY );
	}
	if( pTarget->inv[HEAD2POS].exists() )
	{
		sCenterX = gsPanelWindowX + gsPanelWindowWidth - 30 - 30 / 2;
		sCenterY = gsPanelWindowY + sCellY / 2;
		DrawBufferInterfacePic( pTarget->inv[HEAD2POS].usItem, sCenterX, sCenterY );
	}
	if( pTarget->inv[HEAD1POS].exists() )
	{
		sCenterX = gsPanelWindowX + gsPanelWindowWidth - 30 - 30 / 2; 
		sCenterY = gsPanelWindowY + sCellY / 2;

		if( pTarget->inv[HEAD2POS].exists() )
		{
			sCenterX -= 30;
		}

		DrawBufferInterfacePic( pTarget->inv[HEAD1POS].usItem, sCenterX, sCenterY );
	}

	// show inventory items
	// only if current soldier sees target
	if( (pTarget->IsMercBodyType() || pTarget->ubBodyType >= FATCIV && pTarget->ubBodyType <= CRIPPLECIV) &&
		(pSoldier->aiData.bOppList[pTarget->ubID] == SEEN_CURRENTLY || 
		pSoldier->bTeam == pTarget->bTeam ||
		pSoldier->bTeam == OUR_TEAM && pTarget->bTeam == MILITIA_TEAM) ||
		CHEATER_CHEAT_LEVEL() )
	{
		UINT8 ubPerPocket;
		INT16 sInvOffsetX = sItemsOffsetX;
		INT16 sItemWidth;

		for(UINT8 cnt = VESTPOCKPOS; cnt < SMALLPOCK30POS; cnt++)
		{
			if (pTarget->inv[cnt].exists() && (!(pTarget->inv[cnt].fFlags & OBJECT_MODIFIED) || CHEATER_CHEAT_LEVEL()))
			{
				sItemWidth = GetInterfacePicWidth(pTarget->inv[cnt].usItem);

				// if no place for next item
				if( sInvOffsetX + sItemWidth > gsPanelWindowWidth)
				{
					break;
				}

				// check that we can see this item
				ubPerPocket = max(1, Item[pTarget->inv[cnt].usItem].ubPerPocket);

				// cannot see covert items on enemy
				if( usDistance < sMaxDistanceVisible / ubPerPocket && !HasItemFlag(pTarget->inv[cnt].usItem, COVERT) &&	iObserved >= ubPerPocket / 2  || 
					usDistance <= 1 && (pTarget->bCollapsed || pTarget->IsPOW()) ||
					pSoldier->bTeam == pTarget->bTeam ||
					pSoldier->bTeam == OUR_TEAM && pTarget->bTeam == MILITIA_TEAM ||
					CHEATER_CHEAT_LEVEL() )
				{
					sCenterX = gsPanelWindowX + sInvOffsetX + sItemWidth / 2;
					sCenterY = gsPanelWindowY + gsPanelWindowHeight - sCellY / 2;

					DrawBufferInterfacePic( pTarget->inv[cnt].usItem, sCenterX, sCenterY );

					sInvOffsetX += sItemWidth;
				}			
			}
		}
	}	

	RestoreExternBackgroundRect( gsPanelWideX, gsPanelWideY, gsPanelTotalWidth, gsPanelWideHeight );
}

extern BOOLEAN TeamHasListeningRadioOperator(INT8 bTeam);
extern UINT16 MaxTeamListenValue(INT8 bTeam);

void DrawTacticalPanel(SOLDIERTYPE* pSoldier)
{
	INT32		usItem;

	const int buf_size = 1024;
	CHAR16 pStr[buf_size];

	INT16 sX, sY;
	INT16 sTextWidth;

	INT16 sCenterX, sCenterY;

	if (!pSoldier || !pSoldier->bActive || !pSoldier->bInSector || pSoldier->stats.bLife <= 0)
	{
		return;
	}

	if( !UsingNewInventorySystem() )
		return;

	// sets sTotalWidth, sWideX, sWideWidth, sWideY, sWideHeight, sWindowX, sWindowY, sWindowWidth, sWindowHeight
	PrepareTacticalPanelData( TRUE, FALSE, 8, 6 );

	if (gsPanelWideWidth < INTERFACE_PANEL_SMALL)
	{
		return;
	}

	UINT16	usHandItem = NOTHING;
	CHAR16	strHandItemName[buf_size] = L"";
	//CHAR16	strHandItemLongName[buf_size] = L"";
	OBJECTTYPE *pHandObj = NULL;
	CHAR16	strHandItemStatus[buf_size] = L"";
	UINT8	ubHandItemStatusColor = FONT_MCOLOR_LTGRAY;
	OBJECTTYPE *pHandObjUsed = NULL;
	CHAR16	strHandMode[buf_size] = L"";
	UINT8	ubHandModeColor = FONT_YELLOW;

	UINT16 usSecondHandItem = NOTHING;
	CHAR16 strSecondHandItemName[buf_size] = L"";
	//CHAR16 strSecondHandItemLongName[buf_size] = L"";
	OBJECTTYPE *pSecondHandObj = NULL;
	CHAR16	strSecondHandItemStatus[buf_size] = L"";
	UINT8	ubSecondHandItemStatusColor = FONT_MCOLOR_LTGRAY;
	OBJECTTYPE *pSecondHandObjUsed = NULL;
	CHAR16	strSecondHandMode[buf_size] = L"";
	UINT8	ubSecondHandModeColor = FONT_YELLOW;

	INT32	sItemNameFont = TINYFONT1;
	INT32	sItemStatusFont = TINYFONT1;

	// determine mode
	BOOLEAN fFullHand = FALSE;
	BOOLEAN fTwoHands = FALSE;
	BOOLEAN fFullSecondHand = FALSE;	
	BOOLEAN fInfoPanel = FALSE;

	if( gfUIFullTargetFound && 
		gusUIFullTargetID != NOBODY &&
		MercPtrs[gusUIFullTargetID] &&
		MercPtrs[gusUIFullTargetID]->stats.bLife > 0 &&
		MercPtrs[gusUIFullTargetID]->bVisible != -1 &&
		(gCurrentUIMode == LOOKCURSOR_MODE || gCurrentUIMode == HANDCURSOR_MODE || gCurrentUIMode == TALKCURSOR_MODE || gCurrentUIMode == RADIOCURSOR_MODE) &&
		//gCurrentUIMode == ACTION_MODE || gCurrentUIMode == CONFIRM_ACTION_MODE) &&
		gGameSettings.fOptions[TOPTION_TACTICAL_INFO_PANEL])
	{
		fInfoPanel = TRUE;
	}

	if (gsPanelTotalWidth >= INTERFACE_PANEL_BIG)
	{
		if(fInfoPanel)
		{
			if( pSoldier->inv[SECONDHANDPOS].exists() )
			{
				fTwoHands = TRUE;
			}
			else if( pSoldier->inv[HANDPOS].exists() )
			{
				fFullHand = TRUE;
			}
		}
		else
		{
			if ( pSoldier->inv[HANDPOS].exists() )
			{
				fFullHand = TRUE;
			}
			if ( pSoldier->inv[SECONDHANDPOS].exists() )
			{
				fFullSecondHand = TRUE;
			}
		}				
	}
	else if (!fInfoPanel)
	{
		if( pSoldier->inv[SECONDHANDPOS].exists() )
		{
			fTwoHands = TRUE;
		}
		else if( pSoldier->inv[HANDPOS].exists() )
		{
			fFullHand = TRUE;
		}
	}

	// prepare data
	if( pSoldier->inv[HANDPOS].exists() )
	{
		usHandItem = pSoldier->inv[HANDPOS].usItem;
		swprintf( strHandItemName, buf_size, L"%s", ShortItemNames[ usHandItem ] );
		//swprintf( strHandItemLongName, buf_size, L"%s", ItemNames[ usHandItem ] );
		pHandObj = &(pSoldier->inv[HANDPOS]);
		pHandObjUsed = pSoldier->GetUsedWeapon( &(pSoldier->inv[HANDPOS]));

		// temperature, status
		INT16 sStatus;
		if ( ( Item[pSoldier->inv[HANDPOS].usItem].usItemClass & (IC_GUN | IC_LAUNCHER) ) )
		{
			FLOAT overheatjampercentage = GetGunOverheatDisplayPercentage( pHandObjUsed );

			UINT32 red, green, blue;
			UINT8 TemperatureStringNum;

			ubHandItemStatusColor = FONT_MCOLOR_LTGRAY;
			swprintf( strHandItemStatus, buf_size, L"" );

			if( gGameExternalOptions.fWeaponOverheating )
			{
				TemperatureStringNum = GetTemperatureString( overheatjampercentage, &red, &green, &blue );
				if( TemperatureStringNum >= 5  )
				{		// high
					ubHandItemStatusColor = FONT_MCOLOR_LTRED;
					swprintf(strHandItemStatus, buf_size, gzTacticalPanelStr[TACTICAL_PANEL_STR_HOT]);
				}
				else if( TemperatureStringNum >= 2 )
				{		// med
					ubHandItemStatusColor = FONT_ORANGE;
					swprintf(strHandItemStatus, buf_size, gzTacticalPanelStr[TACTICAL_PANEL_STR_WARM]);
				}
			}

			sStatus = (*pHandObjUsed)[0]->data.objectStatus;
			if( sStatus < 50 )
			{
				ubHandItemStatusColor = FONT_MCOLOR_LTRED;
				swprintf(strHandItemStatus, buf_size, gzTacticalPanelStr[TACTICAL_PANEL_STR_BROKEN]);
			}
			else if( sStatus < 80 )
			{
				ubHandItemStatusColor = FONT_ORANGE;
				swprintf(strHandItemStatus, buf_size, gzTacticalPanelStr[TACTICAL_PANEL_STR_DAMAGED]);
			}

			if ( (*pHandObjUsed)[0]->data.gun.bGunAmmoStatus < 0 )
			{
				ubHandItemStatusColor = FONT_MCOLOR_LTRED;
				swprintf(strHandItemStatus, buf_size, gzTacticalPanelStr[TACTICAL_PANEL_STR_JAMMED]);
			}
		}

		// firing mode
		if( Item[pSoldier->inv[HANDPOS].usItem].usItemClass & IC_GUN )
		{
			if ( pSoldier->bWeaponMode == WM_BURST )
			{
				swprintf( strHandMode, buf_size, New113Message[MSG113_BRST] );
				ubHandModeColor = FONT_DKRED;
			}
			else if(pSoldier->bWeaponMode == WM_AUTOFIRE)
			{
				swprintf( strHandMode, buf_size, New113Message[MSG113_AUTO] );
				ubHandModeColor = FONT_DKRED;
			}
			else if(pSoldier->bWeaponMode == WM_ATTACHED_GL)
			{
				swprintf( strHandMode, buf_size, New113Message[MSG113_GL] );
				ubHandModeColor = FONT_YELLOW;
			}
			else if(pSoldier->bWeaponMode == WM_ATTACHED_GL_BURST)
			{
				swprintf( strHandMode, buf_size, New113Message[MSG113_GL_BRST] );
				ubHandModeColor = FONT_YELLOW;
			}
			else if(pSoldier->bWeaponMode == WM_ATTACHED_GL_AUTO)
			{
				swprintf( strHandMode, buf_size, New113Message[MSG113_GL_AUTO] );
				ubHandModeColor = FONT_YELLOW;
			}
			else if(pSoldier->bWeaponMode == WM_ATTACHED_UB)
			{
				swprintf( strHandMode, buf_size, New113Message[MSG113_UB] );
				ubHandModeColor = FONT_ORANGE;
			}
			else if(pSoldier->bWeaponMode == WM_ATTACHED_UB_BURST)
			{
				swprintf( strHandMode, buf_size, New113Message[MSG113_UB_BRST] );
				ubHandModeColor = FONT_ORANGE;
			}
			else if(pSoldier->bWeaponMode == WM_ATTACHED_UB_AUTO)
			{
				swprintf( strHandMode, buf_size, New113Message[MSG113_UB_AUTO] );
				ubHandModeColor = FONT_ORANGE;
			}
			else if(pSoldier->bWeaponMode == WM_ATTACHED_BAYONET)
			{
				swprintf( strHandMode, buf_size, New113Message[MSG113_BAYONET] );
				ubHandModeColor = FONT_ORANGE;
			}
			else if ( pSoldier->bWeaponMode == WM_NORMAL )
			{
				swprintf( strHandMode, buf_size, L"Single" );
				ubHandModeColor = COLOR_BROWN;
			}
			else
			{
				swprintf( strHandMode, buf_size, L"" );
				ubHandModeColor = COLOR_BROWN;
			}
		}
	}

	if( pSoldier->inv[SECONDHANDPOS].exists() )
	{
		usSecondHandItem = pSoldier->inv[SECONDHANDPOS].usItem;
		swprintf( strSecondHandItemName, buf_size, L"%s", ShortItemNames[ usSecondHandItem ] );
		//swprintf( strSecondHandItemLongName, buf_size, L"%s", ItemNames[ usSecondHandItem ] );
		pSecondHandObj = &(pSoldier->inv[SECONDHANDPOS]);
		pSecondHandObjUsed = pSoldier->GetUsedWeapon( &(pSoldier->inv[SECONDHANDPOS]));

		// temperature, status
		INT16 sStatus;
		if ( ( Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass & (IC_GUN | IC_LAUNCHER) ) )
		{
			FLOAT overheatjampercentage = GetGunOverheatDisplayPercentage( pSecondHandObjUsed );

			UINT32 red, green, blue;
			UINT8 TemperatureStringNum;

			ubSecondHandItemStatusColor = FONT_MCOLOR_LTGRAY;
			swprintf( strSecondHandItemStatus, buf_size, L"" );

			if( gGameExternalOptions.fWeaponOverheating )
			{
				TemperatureStringNum = GetTemperatureString( overheatjampercentage, &red, &green, &blue );
				if( TemperatureStringNum >= 5  )
				{		// high
					ubSecondHandItemStatusColor = FONT_MCOLOR_LTRED;
					swprintf( strSecondHandItemStatus, buf_size, L"Hot!" );
				}
				else if( TemperatureStringNum >= 2 )
				{		// med
					ubSecondHandItemStatusColor = FONT_ORANGE;
					swprintf( strSecondHandItemStatus, buf_size, L"Warm" );
				}
			}

			sStatus = (*pSecondHandObjUsed)[0]->data.objectStatus;
			if( sStatus < 50 )
			{
				ubSecondHandItemStatusColor = FONT_MCOLOR_LTRED;
				swprintf( strSecondHandItemStatus, buf_size, L"Broken" );
			}
			else if( sStatus < 80 )
			{
				ubSecondHandItemStatusColor = FONT_ORANGE;
				swprintf( strSecondHandItemStatus, buf_size, L"Damaged" );
			}

			if ( (*pSecondHandObjUsed)[0]->data.gun.bGunAmmoStatus < 0 )
			{
				ubSecondHandItemStatusColor = FONT_MCOLOR_LTRED;
				swprintf( strSecondHandItemStatus, buf_size, L"Jammed" );
			}
		}

		// firing mode
		if( Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass & IC_GUN )
		{
			if ( pSoldier->bWeaponMode == WM_BURST && pSoldier->IsValidSecondHandBurst() )
			{
				swprintf( strSecondHandMode, buf_size, New113Message[MSG113_BRST] );
				ubSecondHandModeColor = FONT_DKRED;
			}
			else if(pSoldier->bWeaponMode == WM_AUTOFIRE && pSoldier->IsValidSecondHandBurst() )
			{
				swprintf( strSecondHandMode, buf_size, New113Message[MSG113_AUTO] );
				ubSecondHandModeColor = FONT_DKRED;
			}
			else if ( pSoldier->bWeaponMode == WM_NORMAL && pSoldier->IsValidSecondHandShot()  )
			{
				swprintf( strSecondHandMode, buf_size, L"Single" );
				ubSecondHandModeColor = COLOR_BROWN;
			}
			/*else if(pSoldier->bWeaponMode == WM_ATTACHED_GL)
			{
			swprintf( strHandMode, buf_size, New113Message[MSG113_GL] );
			ubHandModeColor = FONT_YELLOW;
			}
			else if(pSoldier->bWeaponMode == WM_ATTACHED_GL_BURST)
			{
			swprintf( strHandMode, buf_size, New113Message[MSG113_GL_BRST] );
			ubHandModeColor = FONT_YELLOW;
			}
			else if(pSoldier->bWeaponMode == WM_ATTACHED_GL_AUTO)
			{
			swprintf( strHandMode, buf_size, New113Message[MSG113_GL_AUTO] );
			ubHandModeColor = FONT_YELLOW;
			}
			else if(pSoldier->bWeaponMode == WM_ATTACHED_UB)
			{
			swprintf( strHandMode, buf_size, New113Message[MSG113_UB] );
			ubHandModeColor = FONT_ORANGE;
			}
			else if(pSoldier->bWeaponMode == WM_ATTACHED_UB_BURST)
			{
			swprintf( strHandMode, buf_size, New113Message[MSG113_UB_BRST] );
			ubHandModeColor = FONT_ORANGE;
			}
			else if(pSoldier->bWeaponMode == WM_ATTACHED_UB_AUTO)
			{
			swprintf( strHandMode, buf_size, New113Message[MSG113_UB_AUTO] );
			ubHandModeColor = FONT_ORANGE;
			}
			else if(pSoldier->bWeaponMode == WM_ATTACHED_BAYONET)
			{
			swprintf( strHandMode, buf_size, New113Message[MSG113_BAYONET] );
			ubHandModeColor = FONT_ORANGE;
			}*/
			else
			{
				swprintf( strSecondHandMode, buf_size, L"" );
				ubSecondHandModeColor = COLOR_BROWN;
			}
		}
	}

	if( fFullHand)
	{
		// show item pic and name
		sTextWidth = StringPixLength ( strHandItemName, sItemNameFont );

		DrawBufferBigPic( pHandObj, gsPanelWindowX + gsPanelWindowWidth / 2, gsPanelWindowY + gsPanelWindowHeight / 2);		

		sX = gsPanelWindowX + gsPanelWindowWidth / 2 - sTextWidth/2;
		sY = gsPanelWindowY + 2 + gsPanelWindowHeight / 2 - 48 / 2 - GetFontHeight( sItemNameFont );

		sY -= GetFontHeight( sItemNameFont );
		PrintCustomSaveBuffer( sItemNameFont, FONT_MCOLOR_BLACK, COLOR_BROWN, sX, sY, strHandItemName );

		// show temperature, status
		UINT32 uiStringLength = StringPixLength(strHandItemStatus, sItemStatusFont );

		sX = gsPanelWindowX + gsPanelWindowWidth - uiStringLength;
		sY = gsPanelWindowY + 2 + gsPanelWindowHeight / 2 - 48 / 2 - GetFontHeight( sItemStatusFont );

		PrintCustomSaveBuffer( sItemStatusFont, FONT_MCOLOR_BLACK, ubHandItemStatusColor, sX, sY, strHandItemStatus );

		// show firing mode
		if ( Item[pSoldier->inv[HANDPOS].usItem].usItemClass & IC_GUN )
		{
			// Get length of string
			sTextWidth = StringPixLength(strHandMode, TINYFONT1 );

			sX = gsPanelWindowX + gsPanelWindowWidth - sTextWidth;
			sY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetFontHeight( TINYFONT1 );

			PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK,  ubHandModeColor, sX, sY, strHandMode );
		}

		// scope mode
		if ( gGameExternalOptions.fScopeModes &&
			CanUseScopeMode(pSoldier->inv[HANDPOS].usItem) &&
			//( Item[ pSoldier->inv[HANDPOS].usItem ].usItemClass == IC_GUN || Item[ pSoldier->inv[HANDPOS].usItem ].usItemClass == IC_LAUNCHER ) &&
			//!Item[ pSoldier->inv[HANDPOS].usItem ].mortar && 
			(pSoldier->bWeaponMode == WM_NORMAL || pSoldier->bWeaponMode == WM_BURST || pSoldier->bWeaponMode == WM_AUTOFIRE) )
		{
			UINT32 uiItemInfoAdvancedIcon = 0;
			VOBJECT_DESC    VObjectDesc;
			VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
			strcpy( VObjectDesc.ImageFile, "INTERFACE\\ItemInfoAdvancedIcons.STI" );
			AddVideoObject( &VObjectDesc, &uiItemInfoAdvancedIcon);

			INT16 sNewX, sNewY;
			sNewX = gsPanelWindowX;
			sNewY = gsPanelWindowY + 2 + gsPanelWindowHeight / 2 - 48 / 2 - GetFontHeight( TINYFONT1 );

			std::map<INT8, OBJECTTYPE*> ObjList;
			GetScopeLists(pSoldier, &pSoldier->inv[HANDPOS], ObjList);

			if (ObjList[pSoldier->bScopeMode] != NULL &&
				(IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE) ||
				IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT) ||
				IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_IRONSIGHT)))
			{
				usItem = ObjList[pSoldier->bScopeMode]->usItem;

				FLOAT iScopeMagValue = __max(1.0f, Item[usItem].scopemagfactor);
				if (iScopeMagValue > 1.0)
				{
					swprintf(pStr, buf_size, L"%.1f", iScopeMagValue);
					sX = gsPanelWindowX + 12;
					sY = gsPanelWindowY + 2 + gsPanelWindowHeight / 2 - 48 / 2 - GetFontHeight(TINYFONT1);

					PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, COLOR_ORANGE, sX, sY, pStr);
				}
			}

			if ( pSoldier->bScopeMode == USE_ALT_WEAPON_HOLD )
			{		
				BltVideoObjectFromIndex( guiSAVEBUFFER, uiItemInfoAdvancedIcon, 56, sNewX, sNewY+1, VO_BLT_TRANSSHADOW, NULL );			
			}
			else if (ObjList[pSoldier->bScopeMode] == NULL)
			{
				SetFontForeground(FONT_LTRED);
				swprintf(pStr, L"X");
				mprintf(sNewX, sNewY, pStr);
				gprintfinvalidate(sNewY, sNewY, pStr);
			}
			else if (ObjList[pSoldier->bScopeMode] != NULL && IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE ) )
			{					
				BltVideoObjectFromIndex( guiSAVEBUFFER, uiItemInfoAdvancedIcon, 54, sNewX, sNewY+1, VO_BLT_TRANSSHADOW, NULL );
			}
			// improved iron sights are attachable iron sights (the 'normal' iron sight is the gun itself)
			else if (ObjList[pSoldier->bScopeMode] != NULL &&  IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_IRONSIGHT ) )
			{
				if (Item[ObjList[pSoldier->bScopeMode]->usItem].bestlaserrange > 0 && Item[ObjList[pSoldier->bScopeMode]->usItem].tohitbonus > 0)
				{
					//CHAR16 pStr[100];
					SetFontForeground(LaserColor(Item[ObjList[pSoldier->bScopeMode]->usItem].ubLaserType));
					swprintf(pStr, L"L");
					mprintf(sNewX, sNewY, pStr);
					gprintfinvalidate(sNewY, sNewY, pStr);
				}
				else
				{
					BltVideoObjectFromIndex(guiSAVEBUFFER, uiItemInfoAdvancedIcon, 52, sNewX, sNewY + 1, VO_BLT_TRANSSHADOW, NULL);
				}
			}
			else if (ObjList[pSoldier->bScopeMode] != NULL && IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT ) )
			{
				BltVideoObjectFromIndex( guiSAVEBUFFER, uiItemInfoAdvancedIcon, 53, sNewX, sNewY, VO_BLT_TRANSSHADOW, NULL );

				// sevenfm: show red "+" if sight has laser enabled
				if (Item[ObjList[pSoldier->bScopeMode]->usItem].bestlaserrange > 0 && Item[ObjList[pSoldier->bScopeMode]->usItem].tohitbonus > 0)
				{
					SetFontForeground(LaserColor(Item[ObjList[pSoldier->bScopeMode]->usItem].ubLaserType));

					INT16 sMagX = sNewX + 9;

					//swprintf( pStr, L"*" );
					swprintf(pStr, L"+");

					mprintf(sMagX, sNewY, pStr);
					gprintfinvalidate(sMagX, sNewY, pStr);
				}
			}
			else
			{
				BltVideoObjectFromIndex( guiSAVEBUFFER, uiItemInfoAdvancedIcon, 52, sNewX, sNewY + 1, VO_BLT_TRANSSHADOW, NULL );
			}			
		}

		// show ammo picture
		INT16 sAmmoOffset = 0;

		if ( pSoldier && pSoldier->inv[HANDPOS].exists() &&
			Item[pSoldier->inv[HANDPOS].usItem].usItemClass == IC_GUN &&
			pSoldier->bWeaponMode != WM_ATTACHED_GL &&
			pSoldier->bWeaponMode != WM_ATTACHED_GL_BURST &&
			pSoldier->bWeaponMode != WM_ATTACHED_GL_AUTO )		
		{
			OBJECTTYPE* pWeapon = pSoldier->GetUsedWeapon( &pSoldier->inv[ HANDPOS ] );
			usItem = (*pWeapon)[0]->data.gun.usGunAmmoItem;
			UINT16 usBulletsLeft = (*pWeapon)[0]->data.gun.ubGunShotsLeft;

			sCenterX = gsPanelWindowX + GetInterfacePicWidth(usItem) / 2;
			//sCenterY = sWindowY + sWindowHeight - 24 / 2;
			//sCenterY = sWindowY + sWindowHeight / 2 + 48 / 2 - 24/2;
			sCenterY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetInterfacePicHeight(usItem) / 2;

			if (usBulletsLeft > 0 && gsPanelWideWidth > INTERFACE_PANEL_SMALL)
			{
				DrawBufferInterfacePic( usItem, sCenterX, sCenterY );
				sAmmoOffset = GetInterfacePicWidth(usItem) + StringPixLength(L" ", FONT12ARIAL );
			}
		}

		// show remaining ammo
		if ( pSoldier && pSoldier->inv[HANDPOS].exists() )
		{
			OBJECTTYPE*	pObjShown;

			pObjShown = pSoldier->GetUsedWeapon( &(pSoldier->inv[HANDPOS]));

			if( Item[pSoldier->inv[HANDPOS].usItem].usItemClass == IC_GUN && 
				!Item[pObjShown->usItem].rocketlauncher &&
				(pSoldier->bWeaponMode == WM_NORMAL || pSoldier->bWeaponMode ==WM_BURST || pSoldier->bWeaponMode == WM_AUTOFIRE ))
			{
				//UINT8 ubBulletsLeft = pSoldier->inv[ HANDPOS ][0]->data.gun.ubGunShotsLeft;
				UINT16 usBulletsLeft = (*pObjShown)[0]->data.gun.ubGunShotsLeft;

				if (usBulletsLeft > 0)
				{
					// HEADROCK HAM 3.4: Get estimate of bullets left.
					if ((gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT))
					{
						// Soldier doesn't know.
						EstimateBulletsLeft(pSoldier, pObjShown);
						//swprintf(pStr, L"%s", gBulletCount);
						swprintf(pStr, buf_size, L"%s", gBulletCount);
					}
					else
					{
						//swprintf(pStr, L"%d", (*pObjShown)[iter]->data.gun.ubGunShotsLeft);
						swprintf(pStr, buf_size, L"%d", usBulletsLeft);
					}

					sTextWidth = StringPixLength(pStr, TINYFONT1);

					//sX = sWideX + sWideWidth - sOffsetX - 30 - uiStringLength;
					sX = gsPanelWindowX + sAmmoOffset;
					sY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetFontHeight( FONT12ARIAL );

					PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, AmmoTypes[(*pObjShown)[0]->data.gun.ubGunAmmoType].fontColour, sX, sY, pStr);
				}
			}
		}

		// show bayonet
		if( pSoldier->inv[HANDPOS].exists() && pSoldier->bWeaponMode == WM_ATTACHED_BAYONET )
		{
			OBJECTTYPE  *pObjectUnderBarrel = FindAttachedWeapon( &pSoldier->inv[ HANDPOS ], IC_BLADE );

			if ( pObjectUnderBarrel )
			{
				sCenterX = gsPanelWindowX + GetInterfacePicWidth(pObjectUnderBarrel->usItem) / 2;
				//sCenterY = sWindowY + sWindowHeight / 2 + 48 / 2 - 24/2;
				sCenterY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetInterfacePicHeight(pObjectUnderBarrel->usItem) / 2;

				DrawBufferInterfacePic( pObjectUnderBarrel->usItem, sCenterX, sCenterY );
			}
		}

		// show attached grenade/rocket
		if( pSoldier->inv[HANDPOS].exists() && 			
			( Item[ pSoldier->inv[HANDPOS].usItem ].usItemClass == IC_GUN || Item[ pSoldier->inv[HANDPOS].usItem ].usItemClass == IC_LAUNCHER ))
		{
			OBJECTTYPE* pWeapon = pSoldier->GetUsedWeapon( &pSoldier->inv[ HANDPOS ] );
			OBJECTTYPE* pFirstObj = FindAttachmentByClass( pWeapon, IC_BOMB | IC_GRENADE );

			if( pFirstObj )
			{
				if( Item[ pWeapon->usItem ].grenadelauncher || 
					pSoldier->bWeaponMode==WM_ATTACHED_GL || 
					pSoldier->bWeaponMode==WM_ATTACHED_GL_BURST || 
					pSoldier->bWeaponMode==WM_ATTACHED_GL_AUTO )
				{
					usItem = pFirstObj->usItem;

					// for 20mm, 25mm launchers
					UINT8 ubMagSize = Explosive[ Item[usItem].ubClassIndex ].ubMagSize;
					INT16 sStatus = (*pFirstObj)[0]->data.objectStatus;

					sCenterX = gsPanelWindowX + GetInterfacePicWidth(usItem) / 2;

					if(gsPanelTotalWidth > 177)
						sCenterY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetInterfacePicHeight(usItem) / 2;
					else
						sCenterY = gsPanelWindowY + gsPanelWindowHeight - 24 + GetInterfacePicHeight(usItem) / 2;

					DrawBufferInterfacePic( usItem, sCenterX, sCenterY );

					if( ubMagSize > 1 )
					{
						UINT32 sShotStatus = 100 / ubMagSize;
						UINT32 uiBulletsLeft = sStatus / sShotStatus;

						swprintf(pStr, buf_size, L"%d", uiBulletsLeft);

						sTextWidth = StringPixLength(pStr, FONT12ARIAL );

						sX = gsPanelWindowX + GetInterfacePicWidth(usItem) + StringPixLength(L" ", FONT12ARIAL);

						if (gsPanelTotalWidth > 177)
							sY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetFontHeight(FONT12ARIAL);
						else
							sY = gsPanelWindowY + gsPanelWindowHeight - 24 + GetInterfacePicHeight(usItem) / 2 - GetFontHeight(FONT12ARIAL);

						PrintCustomSaveBuffer( FONT12ARIAL, FONT_MCOLOR_BLACK, FONT_YELLOW, sX, sY, pStr );
					}
				}

				if ( (Item[ pSoldier->inv[HANDPOS].usItem ].rocketlauncher && gsPanelWideWidth >= INTERFACE_PANEL_MEDIUM || Item[ pSoldier->inv[HANDPOS].usItem ].mortar ) )
				{
					usItem = pFirstObj->usItem;

					sCenterX = gsPanelWindowX + GetInterfacePicWidth(usItem) / 2;

					if (gsPanelTotalWidth > 177)
						sCenterY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetInterfacePicHeight(usItem) / 2;
					else
						sCenterY = gsPanelWindowY + gsPanelWindowHeight - 24 + GetInterfacePicHeight(usItem) / 2;

					DrawBufferInterfacePic( usItem, sCenterX, sCenterY );
				}
			}
		}
	}

	if (!fTwoHands && !fInfoPanel)
	{
		// show remaining enemies in sector
		UINT8 ubArmy = NumEnemiesInSector(gWorldSectorX, gWorldSectorY);
		UINT8 ubReinforcements = NumEnemiesInFiveSectors(gWorldSectorX, gWorldSectorY, TRUE) - NumEnemiesInAnySector(gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
		UINT32 uiEnemyKnowledge = WhatPlayerKnowsAboutEnemiesInSector(gWorldSectorX, gWorldSectorY);
		BOOLEAN fTeamListenArmy = FALSE;

		if (TeamHasListeningRadioOperator(OUR_TEAM) && MaxTeamListenValue(OUR_TEAM) > max(1, ubArmy / 4))
		{
			fTeamListenArmy = TRUE;
		}

		if (ubArmy > 0 && (uiEnemyKnowledge == KNOWS_HOW_MANY || uiEnemyKnowledge == KNOWS_HOW_MANY_AND_WHERE_GOING || fTeamListenArmy))
		{
			swprintf(pStr, buf_size, L"%s: ", gpStrategicString[STR_PB_ENEMIES]);
			sTextWidth = StringPixLength(pStr, sItemNameFont);
			sX = gsPanelWindowX;
			sY = gsPanelWindowY;
			PrintCustomSaveBuffer(sItemStatusFont, FONT_MCOLOR_BLACK, FONT_KHAKI, sX, sY, pStr);

			swprintf(pStr, buf_size, L"%d", ubArmy);
			sX = gsPanelWindowX + sTextWidth;
			sY = gsPanelWindowY;
			if (gfPendingEnemies)
				PrintCustomSaveBuffer(sItemStatusFont, FONT_MCOLOR_BLACK, FONT_GREEN, sX, sY, pStr);
			else
				PrintCustomSaveBuffer(sItemStatusFont, FONT_MCOLOR_BLACK, FONT_LTKHAKI, sX, sY, pStr);
			sTextWidth += StringPixLength(pStr, sItemNameFont);

			if (gGameExternalOptions.gfAllowReinforcements &&
				gGameExternalOptions.fTacticalReinforcements &&
				ubReinforcements &&
				!gbWorldSectorZ)
			{
				swprintf(pStr, buf_size, L" [%d]", ubReinforcements);
				sX = gsPanelWindowX + sTextWidth;
				sY = gsPanelWindowY;
				if (SectorJammed())
					PrintCustomSaveBuffer(sItemStatusFont, FONT_MCOLOR_BLACK, FONT_BEIGE, sX, sY, pStr);
				else if (gGameExternalOptions.fRadioOperatorReinforcements && gGameOptions.fNewTraitSystem && gGameExternalOptions.fAssignTraitsToEnemy && !TeamHasActiveRadioOperator(ENEMY_TEAM))
					PrintCustomSaveBuffer(sItemStatusFont, FONT_MCOLOR_BLACK, FONT_METALGRAY, sX, sY, pStr);
				else
					PrintCustomSaveBuffer(sItemStatusFont, FONT_MCOLOR_BLACK, FONT_KHAKI, sX, sY, pStr);
				sTextWidth += StringPixLength(pStr, sItemNameFont);
			}
		}

		// count militia
		SECTORINFO *pSectorInfo;
		pSectorInfo = &(SectorInfo[SECTOR(gWorldSectorX, gWorldSectorY)]);
		UINT8 ubMilitia = CountMilitia(pSectorInfo);

		if (ubMilitia > 0)
		{
			swprintf(pStr, buf_size, L"%d", ubMilitia);
			sTextWidth = StringPixLength(pStr, sItemNameFont);
			sX = gsPanelWindowX + gsPanelWindowWidth - sTextWidth;
			sY = gsPanelWindowY;
			PrintCustomSaveBuffer(sItemStatusFont, FONT_MCOLOR_BLACK, FONT_LTKHAKI, sX, sY, pStr);

			swprintf(pStr, buf_size, L"%s: ", gpStrategicString[STR_PB_MILITIA]);
			sTextWidth += StringPixLength(pStr, sItemNameFont);
			sX = gsPanelWindowX + gsPanelWindowWidth - sTextWidth;
			sY = gsPanelWindowY;
			PrintCustomSaveBuffer(sItemStatusFont, FONT_MCOLOR_BLACK, FONT_KHAKI, sX, sY, pStr);
		}

		// draw jamming icon
		if (SectorJammed())
		{
			UINT32		usWidth, usHeight;
			INT32		sOffsetX, sOffsetY;
			GetVideoObjectDimensions(guiASSIGNMENTICONS, 20, usWidth, usHeight, sOffsetX, sOffsetY);

			//sX = gsPanelWindowX + gsPanelWindowWidth / 2;
			sX = gsPanelWindowX;
			sY = gsPanelWindowY + gsPanelWindowHeight - usHeight - 4;
			BltVideoObjectFromIndex(guiSAVEBUFFER, guiASSIGNMENTICONS, 20, sX, sY, VO_BLT_SRCTRANSPARENCY, NULL);
		}

		UINT32 usRightOffset = 0;
		// draw backpack
		if (pSoldier->inv[BPACKPOCKPOS].exists())
		{
			OBJECTTYPE *pBackpack = &(pSoldier->inv[BPACKPOCKPOS]);

			usItem = pBackpack->usItem;

			sCenterX = gsPanelWindowX + gsPanelWindowWidth - GetInterfacePicWidth(usItem) / 2;
			sCenterY = gsPanelWindowY + gsPanelWindowHeight - GetInterfacePicHeight(usItem) / 2 - 2;

			usRightOffset += GetInterfacePicWidth(usItem);

			DrawBufferInterfacePic(usItem, sCenterX, sCenterY);
		}

		// draw combat pack
		if (pSoldier->inv[CPACKPOCKPOS].exists())
		{
			OBJECTTYPE *pBackpack = &(pSoldier->inv[CPACKPOCKPOS]);

			usItem = pBackpack->usItem;

			sCenterX = gsPanelWindowX + gsPanelWindowWidth - usRightOffset - GetInterfacePicWidth(usItem) / 2;
			sCenterY = gsPanelWindowY + gsPanelWindowHeight - GetInterfacePicHeight(usItem) / 2 - 2;
			usRightOffset += GetInterfacePicWidth(usItem);

			DrawBufferInterfacePic(usItem, sCenterX, sCenterY);
		}

		// draw gun sling
		/*if (pSoldier->inv[GUNSLINGPOCKPOS].exists())
		{
			OBJECTTYPE *pBackpack = &(pSoldier->inv[GUNSLINGPOCKPOS]);

			usItem = pBackpack->usItem;

			sCenterX = gsPanelWindowX + gsPanelWindowWidth - usRightOffset - GetInterfacePicWidth(usItem) / 2;
			sCenterY = gsPanelWindowY + gsPanelWindowHeight - GetInterfacePicHeight(usItem) / 2 - 2;

			DrawBufferInterfacePic(usItem, sCenterX, sCenterY);
		}*/
	}

	if( fFullSecondHand)
	{
		gsPanelWindowX += gsPanelWideWidth;

		// show item pic and name
		sTextWidth = StringPixLength ( strSecondHandItemName, sItemNameFont );

		//DrawBufferBigPic( &(pSoldier->inv[HANDPOS]), sWideX + (sWideWidth - sWeaponOffsetX) / 2 + sWeaponOffsetX, sWideY + sOffsetY + 48 / 2 + sWeaponOffsetY);
		DrawBufferBigPic( pSecondHandObj, gsPanelWindowX + gsPanelWindowWidth / 2, gsPanelWindowY + gsPanelWindowHeight / 2);		

		sX = gsPanelWindowX + gsPanelWindowWidth / 2 - sTextWidth/2;
		sY = gsPanelWindowY + 2 + gsPanelWindowHeight / 2 - 48 / 2 - GetFontHeight( sItemNameFont );

		sY -= GetFontHeight( sItemNameFont );
		PrintCustomSaveBuffer( sItemNameFont, FONT_MCOLOR_BLACK, COLOR_BROWN, sX, sY, strSecondHandItemName );

		// show firing mode
		if ( Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass & IC_GUN )
		{
			// Get length of string
			sTextWidth = StringPixLength(strSecondHandMode, TINYFONT1 );

			sX = gsPanelWindowX + gsPanelWindowWidth - sTextWidth;
			sY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetFontHeight( TINYFONT1 );

			PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK,  ubSecondHandModeColor, sX, sY, strSecondHandMode );
		}

		// show temperature, status
		sTextWidth = StringPixLength(strSecondHandItemStatus, sItemStatusFont );

		sX = gsPanelWindowX + gsPanelWindowWidth - sTextWidth;
		sY = gsPanelWindowY + 2 + gsPanelWindowHeight / 2 - 48 / 2 - GetFontHeight( sItemStatusFont );

		PrintCustomSaveBuffer( sItemStatusFont, FONT_MCOLOR_BLACK, ubSecondHandItemStatusColor, sX, sY, strSecondHandItemStatus );			

		// show ammo picture
		INT16 sAmmoOffset = 0;

		if ( pSoldier && pSoldier->inv[SECONDHANDPOS].exists() &&
			Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass == IC_GUN &&
			pSoldier->bWeaponMode != WM_ATTACHED_GL &&
			pSoldier->bWeaponMode != WM_ATTACHED_GL_BURST &&
			pSoldier->bWeaponMode != WM_ATTACHED_GL_AUTO )		
		{
			OBJECTTYPE* pWeapon = pSoldier->GetUsedWeapon( &pSoldier->inv[ SECONDHANDPOS ] );
			usItem = (*pWeapon)[0]->data.gun.usGunAmmoItem;
			UINT16 usBulletsLeft = (*pWeapon)[0]->data.gun.ubGunShotsLeft;

			sCenterX = gsPanelWindowX + GetInterfacePicWidth(usItem) / 2;
			//sCenterY = sWindowY + sWindowHeight - 24 / 2;
			//sCenterY = sWindowY + sWindowHeight / 2 + 48 / 2 - 24/2;
			sCenterY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 -  GetInterfacePicHeight(usItem) / 2;

			if (usBulletsLeft > 0 && gsPanelWideWidth > INTERFACE_PANEL_SMALL)
			{
				DrawBufferInterfacePic( usItem, sCenterX, sCenterY );
				sAmmoOffset = GetInterfacePicWidth(usItem) + StringPixLength(L" ", FONT12ARIAL );
			}
		}

		// show remaining ammo
		if ( pSoldier && pSoldier->inv[SECONDHANDPOS].exists() )
		{
			OBJECTTYPE*	pObjShown;

			pObjShown = pSoldier->GetUsedWeapon( &(pSoldier->inv[SECONDHANDPOS]));

			if( Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass == IC_GUN && 
				!Item[pObjShown->usItem].rocketlauncher &&
				(pSoldier->bWeaponMode == WM_NORMAL || pSoldier->bWeaponMode ==WM_BURST || pSoldier->bWeaponMode == WM_AUTOFIRE ))
			{
				//UINT8 ubBulletsLeft = pSoldier->inv[ HANDPOS ][0]->data.gun.ubGunShotsLeft;
				UINT16 usBulletsLeft = (*pObjShown)[0]->data.gun.ubGunShotsLeft;

				if (usBulletsLeft > 0)
				{
					// HEADROCK HAM 3.4: Get estimate of bullets left.
					if ((gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT))
					{
						// Soldier doesn't know.
						EstimateBulletsLeft(pSoldier, pObjShown);
						//swprintf(pStr, L"%s", gBulletCount);
						swprintf(pStr, buf_size, L"%s", gBulletCount);
					}
					else
					{
						//swprintf(pStr, L"%d", (*pObjShown)[iter]->data.gun.ubGunShotsLeft);
						swprintf(pStr, buf_size, L"%d", usBulletsLeft);
					}

					sTextWidth = StringPixLength(pStr, TINYFONT1);

					//sX = sWideX + sWideWidth - sOffsetX - 30 - uiStringLength;
					sX = gsPanelWindowX + sAmmoOffset;
					sY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetFontHeight( FONT12ARIAL );

					PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, AmmoTypes[(*pObjShown)[0]->data.gun.ubGunAmmoType].fontColour, sX, sY, pStr);
				}
			}
		}			
	}

	if( fTwoHands )
	{
		gsPanelWindowHeight = gsPanelWindowHeight / 2;

		if( pSoldier->inv[HANDPOS].exists() )
		{
			// show item pic and name
			//sTextWidth = StringPixLength ( strHandItemName, sItemNameFont );

			DrawBufferBigPic( pHandObj, gsPanelWindowX + gsPanelWindowWidth / 2, gsPanelWindowY + gsPanelWindowHeight / 2);		

			/*sX = sWindowX + sWindowWidth / 2 - sTextWidth/2;
			sY = sWindowY + 2 + sWindowHeight / 2 - 48 / 2 - GetFontHeight( sItemNameFont );

			sY -= GetFontHeight( sItemNameFont );
			PrintCustomSaveBuffer( sItemNameFont, FONT_MCOLOR_BLACK, COLOR_BROWN, sX, sY, strHandItemName );*/

			// show temperature, status
			UINT32 uiStringLength = StringPixLength(strHandItemStatus, sItemStatusFont );

			sX = gsPanelWindowX + gsPanelWindowWidth - uiStringLength;
			sY = gsPanelWindowY + 2 + gsPanelWindowHeight / 2 - 48 / 2;

			PrintCustomSaveBuffer( sItemStatusFont, FONT_MCOLOR_BLACK, ubHandItemStatusColor, sX, sY, strHandItemStatus );

			// show firing mode		
			if ( Item[pSoldier->inv[HANDPOS].usItem].usItemClass & IC_GUN )
			{
				// Get length of string
				sTextWidth = StringPixLength(strHandMode, TINYFONT1 );

				sX = gsPanelWindowX + gsPanelWindowWidth - sTextWidth;
				sY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetFontHeight( TINYFONT1 );

				PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK,  ubHandModeColor, sX, sY, strHandMode );
			}

			// scope mode
			if( ( Item[ pSoldier->inv[HANDPOS].usItem ].usItemClass == IC_GUN || Item[ pSoldier->inv[HANDPOS].usItem ].usItemClass == IC_LAUNCHER ) &&
				!Item[ pSoldier->inv[HANDPOS].usItem ].mortar && 
				(pSoldier->bWeaponMode == WM_NORMAL || pSoldier->bWeaponMode == WM_BURST || pSoldier->bWeaponMode == WM_AUTOFIRE) )
			{
				UINT32 uiItemInfoAdvancedIcon = 0;
				VOBJECT_DESC    VObjectDesc;
				VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
				strcpy( VObjectDesc.ImageFile, "INTERFACE\\ItemInfoAdvancedIcons.STI" );
				AddVideoObject( &VObjectDesc, &uiItemInfoAdvancedIcon);

				INT16 sNewX, sNewY;
				sNewX = gsPanelWindowX;
				sNewY = gsPanelWindowY + 2 + gsPanelWindowHeight / 2 - 48 / 2;

				std::map<INT8, OBJECTTYPE*> ObjList;
				GetScopeLists(pSoldier, &pSoldier->inv[HANDPOS], ObjList);

				if ( pSoldier->bScopeMode == USE_ALT_WEAPON_HOLD )
				{		
					BltVideoObjectFromIndex( guiSAVEBUFFER, uiItemInfoAdvancedIcon, 56, sNewX, sNewY+1, VO_BLT_TRANSSHADOW, NULL );			
				}
				else if (ObjList[pSoldier->bScopeMode] != NULL && IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE ) )
				{					
					BltVideoObjectFromIndex( guiSAVEBUFFER, uiItemInfoAdvancedIcon, 54, sNewX, sNewY+1, VO_BLT_TRANSSHADOW, NULL );
				}
				// improved iron sights are attachable iron sights (the 'normal' iron sight is the gun itself)
				else if (ObjList[pSoldier->bScopeMode] != NULL &&  IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_IRONSIGHT ) )
				{
					BltVideoObjectFromIndex( guiSAVEBUFFER, uiItemInfoAdvancedIcon, 52, sNewX, sNewY-1, VO_BLT_TRANSSHADOW, NULL );
				}
				else if (ObjList[pSoldier->bScopeMode] != NULL && IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT ) )
				{
					BltVideoObjectFromIndex( guiSAVEBUFFER, uiItemInfoAdvancedIcon, 53, sNewX, sNewY, VO_BLT_TRANSSHADOW, NULL );
				}
				else
				{
					BltVideoObjectFromIndex( guiSAVEBUFFER, uiItemInfoAdvancedIcon, 52, sNewX, sNewY-1, VO_BLT_TRANSSHADOW, NULL );
				}

				if ( ObjList[pSoldier->bScopeMode] != NULL && 
					(IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE) || 
					IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT ) ||
					IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_IRONSIGHT ) ) )
				{
					usItem = ObjList[pSoldier->bScopeMode]->usItem;

					FLOAT iScopeMagValue = __max(1.0f, Item[ usItem ].scopemagfactor);
					if( iScopeMagValue > 1.0 )
					{	
						swprintf( pStr, buf_size, L"%.1f", iScopeMagValue );
						sX = gsPanelWindowX + 12;
						sY = gsPanelWindowY + 2 + gsPanelWindowHeight / 2 - 48 / 2;

						PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK,  COLOR_ORANGE, sX, sY, pStr );
					}
				}
			}

			// show ammo picture
			INT16 sAmmoOffset = 0;

			if ( pSoldier && pSoldier->inv[HANDPOS].exists() &&
				Item[pSoldier->inv[HANDPOS].usItem].usItemClass == IC_GUN &&
				pSoldier->bWeaponMode != WM_ATTACHED_GL &&
				pSoldier->bWeaponMode != WM_ATTACHED_GL_BURST &&
				pSoldier->bWeaponMode != WM_ATTACHED_GL_AUTO )		
			{
				OBJECTTYPE* pWeapon = pSoldier->GetUsedWeapon( &pSoldier->inv[ HANDPOS ] );
				usItem = (*pWeapon)[0]->data.gun.usGunAmmoItem;
				UINT16 usBulletsLeft = (*pWeapon)[0]->data.gun.ubGunShotsLeft;

				sCenterX = gsPanelWindowX + GetInterfacePicWidth(usItem) / 2;
				//sCenterY = sWindowY + sWindowHeight - 24 / 2;
				//sCenterY = sWindowY + sWindowHeight / 2 + 48 / 2 - 24/2;
				sCenterY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetInterfacePicHeight(usItem) / 2;

				if( usBulletsLeft > 0 && gsPanelWideWidth > INTERFACE_PANEL_SMALL)
				{
					DrawBufferInterfacePic( usItem, sCenterX, sCenterY );
					sAmmoOffset = GetInterfacePicWidth(usItem) + StringPixLength(L" ", FONT12ARIAL );
				}
			}

			// show remaining ammo
			if ( pSoldier && pSoldier->inv[HANDPOS].exists() )
			{
				OBJECTTYPE*	pObjShown;

				pObjShown = pSoldier->GetUsedWeapon( &(pSoldier->inv[HANDPOS]));

				if( Item[pSoldier->inv[HANDPOS].usItem].usItemClass == IC_GUN && 
					!Item[pObjShown->usItem].rocketlauncher &&
					(pSoldier->bWeaponMode == WM_NORMAL || pSoldier->bWeaponMode ==WM_BURST || pSoldier->bWeaponMode == WM_AUTOFIRE ))
				{
					UINT16 usBulletsLeft = (*pObjShown)[0]->data.gun.ubGunShotsLeft;

					if( usBulletsLeft > 0 )
					{				
						swprintf( pStr, buf_size, L"%d", usBulletsLeft);

						sTextWidth = StringPixLength(pStr, TINYFONT1);

						//sX = sWideX + sWideWidth - sOffsetX - 30 - uiStringLength;
						sX = gsPanelWindowX + sAmmoOffset;
						sY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetFontHeight( FONT12ARIAL );

						PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, AmmoTypes[(*pObjShown)[0]->data.gun.ubGunAmmoType].fontColour, sX, sY, pStr);
					}
				}
			}

			// show bayonet
			if( pSoldier->inv[HANDPOS].exists() && pSoldier->bWeaponMode == WM_ATTACHED_BAYONET )
			{
				OBJECTTYPE  *pObjectUnderBarrel = FindAttachedWeapon( &pSoldier->inv[ HANDPOS ], IC_BLADE );

				if ( pObjectUnderBarrel )
				{
					sCenterX = gsPanelWindowX + GetInterfacePicWidth(pObjectUnderBarrel->usItem) / 2;
					//sCenterY = sWindowY + sWindowHeight / 2 + 48 / 2 - 24/2;
					sCenterY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetInterfacePicHeight(pObjectUnderBarrel->usItem) / 2;

					DrawBufferInterfacePic( pObjectUnderBarrel->usItem, sCenterX, sCenterY );
				}
			}

			// show attached grenade/rocket
			if( pSoldier->inv[HANDPOS].exists() && 
				gsPanelTotalWidth > 177 &&
				( Item[ pSoldier->inv[HANDPOS].usItem ].usItemClass == IC_GUN || Item[ pSoldier->inv[HANDPOS].usItem ].usItemClass == IC_LAUNCHER ))
			{
				OBJECTTYPE* pWeapon = pSoldier->GetUsedWeapon( &pSoldier->inv[ HANDPOS ] );
				OBJECTTYPE* pFirstObj = FindAttachmentByClass( pWeapon, IC_BOMB | IC_GRENADE );


				if( pFirstObj )
				{
					if( Item[ pWeapon->usItem ].grenadelauncher || 
						pSoldier->bWeaponMode==WM_ATTACHED_GL || 
						pSoldier->bWeaponMode==WM_ATTACHED_GL_BURST || 
						pSoldier->bWeaponMode==WM_ATTACHED_GL_AUTO )
					{
						usItem = pFirstObj->usItem;

						// for 20mm, 25mm launchers
						UINT8 ubMagSize = Explosive[ Item[usItem].ubClassIndex ].ubMagSize;
						INT16 sStatus = (*pFirstObj)[0]->data.objectStatus;

						//sCenterX = sWindowX + 30 / 2;
						//sCenterY = sWindowY + sWindowHeight / 2 + 48 / 2 + 24 / 2;

						sCenterX = gsPanelWindowX + GetInterfacePicWidth(usItem) / 2;
						//sCenterY = sWindowY + sWindowHeight / 2 + 48 / 2 - 24/2;
						sCenterY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetInterfacePicHeight(usItem) / 2;

						DrawBufferInterfacePic( usItem, sCenterX, sCenterY );

						if( ubMagSize > 1 )
						{
							UINT32 sShotStatus = 100 / ubMagSize;
							UINT32 uiBulletsLeft = sStatus / sShotStatus;

							swprintf(pStr, buf_size, L"%d", uiBulletsLeft);

							sTextWidth = StringPixLength(pStr, FONT12ARIAL );

							sX = gsPanelWindowX + GetInterfacePicWidth(usItem) + StringPixLength(L" ", FONT12ARIAL );
							sY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetFontHeight( FONT12ARIAL );

							PrintCustomSaveBuffer( FONT12ARIAL, FONT_MCOLOR_BLACK, FONT_YELLOW, sX, sY, pStr );
						}
					}

					if ( (Item[ pSoldier->inv[HANDPOS].usItem ].rocketlauncher && gsPanelWideWidth >= INTERFACE_PANEL_MEDIUM || Item[ pSoldier->inv[HANDPOS].usItem ].mortar ) )
					{
						usItem = pFirstObj->usItem;

						sCenterX = gsPanelWindowX + GetInterfacePicWidth(usItem) / 2;
						//sCenterY = sWindowY + sWindowHeight / 2 + 48 / 2 - 24/2;
						sCenterY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetInterfacePicHeight(usItem) / 2;

						DrawBufferInterfacePic( usItem, sCenterX, sCenterY );
					}
				}
			}
		}
		// end of main hand

		if( pSoldier->inv[SECONDHANDPOS].exists() )
		{
			gsPanelWindowY += gsPanelWindowHeight;

			// show item pic and name
			//sTextWidth = StringPixLength ( strSecondHandItemName, sItemNameFont );

			DrawBufferBigPic( pSecondHandObj, gsPanelWindowX + gsPanelWindowWidth / 2, gsPanelWindowY + gsPanelWindowHeight / 2);		

			/*sX = sWindowX + sWindowWidth / 2 - sTextWidth/2;
			sY = sWindowY + 2 + sWindowHeight / 2 - 48 / 2 - GetFontHeight( sItemNameFont );

			sY -= GetFontHeight( sItemNameFont );
			PrintCustomSaveBuffer( sItemNameFont, FONT_MCOLOR_BLACK, COLOR_BROWN, sX, sY, strSecondHandItemName );*/

			// show firing mode
			if ( Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass & IC_GUN )
			{
				// Get length of string
				sTextWidth = StringPixLength(strSecondHandMode, TINYFONT1 );

				sX = gsPanelWindowX + gsPanelWindowWidth - sTextWidth;
				sY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetFontHeight( TINYFONT1 );

				PrintCustomSaveBuffer( TINYFONT1, FONT_MCOLOR_BLACK,  ubSecondHandModeColor, sX, sY, strSecondHandMode );
			}

			// show temperature, status
			UINT32 uiStringLength = StringPixLength(strSecondHandItemStatus, sItemStatusFont );

			sX = gsPanelWindowX + gsPanelWindowWidth - uiStringLength;
			sY = gsPanelWindowY + 2 + gsPanelWindowHeight / 2 - 48 / 2;

			PrintCustomSaveBuffer( sItemStatusFont, FONT_MCOLOR_BLACK, ubSecondHandItemStatusColor, sX, sY, strSecondHandItemStatus );

			// show ammo picture
			INT16 sAmmoOffset = 0;

			if ( pSoldier && pSoldier->inv[SECONDHANDPOS].exists() &&
				Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass == IC_GUN &&
				pSoldier->bWeaponMode != WM_ATTACHED_GL &&
				pSoldier->bWeaponMode != WM_ATTACHED_GL_BURST &&
				pSoldier->bWeaponMode != WM_ATTACHED_GL_AUTO )		
			{
				OBJECTTYPE* pWeapon = pSoldier->GetUsedWeapon( &pSoldier->inv[ SECONDHANDPOS ] );
				usItem = (*pWeapon)[0]->data.gun.usGunAmmoItem;
				UINT16 usBulletsLeft = (*pWeapon)[0]->data.gun.ubGunShotsLeft;

				sCenterX = gsPanelWindowX + GetInterfacePicWidth(usItem) / 2;
				//sCenterY = sWindowY + sWindowHeight - 24 / 2;
				//sCenterY = sWindowY + sWindowHeight / 2 + 48 / 2 - 24/2;
				sCenterY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetInterfacePicHeight(usItem) / 2;

				if( usBulletsLeft > 0 && gsPanelWideWidth > INTERFACE_PANEL_SMALL)
				{
					DrawBufferInterfacePic( usItem, sCenterX, sCenterY );
					sAmmoOffset = GetInterfacePicWidth(usItem) + StringPixLength(L" ", FONT12ARIAL );
				}
			}

			// show remaining ammo
			if ( pSoldier && pSoldier->inv[SECONDHANDPOS].exists() )
			{
				OBJECTTYPE*	pObjShown;

				pObjShown = pSoldier->GetUsedWeapon( &(pSoldier->inv[SECONDHANDPOS]));

				if( Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass == IC_GUN && 
					!Item[pObjShown->usItem].rocketlauncher &&
					(pSoldier->bWeaponMode == WM_NORMAL || pSoldier->bWeaponMode ==WM_BURST || pSoldier->bWeaponMode == WM_AUTOFIRE ))
				{
					UINT16 usBulletsLeft = (*pObjShown)[0]->data.gun.ubGunShotsLeft;

					if( usBulletsLeft > 0 )
					{				
						swprintf( pStr, buf_size, L"%d", usBulletsLeft);

						sTextWidth = StringPixLength(pStr, TINYFONT1);

						//sX = sWideX + sWideWidth - sOffsetX - 30 - uiStringLength;
						sX = gsPanelWindowX + sAmmoOffset;
						sY = gsPanelWindowY + gsPanelWindowHeight / 2 + 48 / 2 - GetFontHeight( FONT12ARIAL );
						
						PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, AmmoTypes[(*pObjShown)[0]->data.gun.ubGunAmmoType].fontColour, sX, sY, pStr);
					}
				}
			}
		}
		// end of second hand
	}

	RestoreExternBackgroundRect( gsPanelWideX, gsPanelWideY, gsPanelTotalWidth, gsPanelWideHeight );

	if( fInfoPanel )
	{
		DrawTacticalInfoPanel(pSoldier);
	}

	// start sling
	/*if( pSoldier->inv[GUNSLINGPOCKPOS].exists() )
	{
	sCenterX = sWideX + sOffsetX + 60 / 2;
	sCenterY = sWideY + sWideHeight - sOffsetY - 24 / 2;
	DrawBufferInterfacePic( pSoldier->inv[GUNSLINGPOCKPOS].usItem, sCenterX, sCenterY );
	}
	if( pSoldier->inv[CPACKPOCKPOS].exists() )
	{
	sCenterX = sWideX + sWideWidth - sOffsetX - 60 / 2;
	sCenterY = sWideY + sOffsetY + sWeaponOffsetY + 48 + 24 / 2;
	DrawBufferInterfacePic( pSoldier->inv[CPACKPOCKPOS].usItem, sCenterX, sCenterY );
	}
	if( pSoldier->inv[BPACKPOCKPOS].exists() )
	{
	sCenterX = sWideX + sWideWidth - sOffsetX - 60 / 2;
	sCenterY = sWideY + sWideHeight - sOffsetY - 24 / 2;
	DrawBufferInterfacePic( pSoldier->inv[BPACKPOCKPOS].usItem, sCenterX, sCenterY );
	}*/	

	return;
}

void GetVideoObjectDimensions( UINT32 uiImage, UINT16 usGraphicNum, UINT32 &usWidth, UINT32 &usHeight, INT32 &sOffsetX, INT32 &sOffsetY )
{
	HVOBJECT	hPixHandle;
	ETRLEObject		*pTrav;

	GetVideoObject(&hPixHandle, uiImage);

	if(hPixHandle->ubBitDepth == 8)
	{
		pTrav = &(hPixHandle->pETRLEObject[ usGraphicNum ] );

		usHeight				= (UINT32)pTrav->usHeight;
		usWidth					= (UINT32)pTrav->usWidth;
		sOffsetX				= (INT32)pTrav->sOffsetX;
		sOffsetY				= (INT32)pTrav->sOffsetY;
	}
	else if(hPixHandle->ubBitDepth == 16 && hPixHandle->p16BPPObject)
	{
		usHeight				= hPixHandle->p16BPPObject->usHeight;
		usWidth					= hPixHandle->p16BPPObject->usWidth;
		sOffsetX				= hPixHandle->p16BPPObject->sOffsetX;
		sOffsetY				= hPixHandle->p16BPPObject->sOffsetY;
	}
	else if(hPixHandle->ubBitDepth == 32)
	{
		usHeight				= hPixHandle->p16BPPObject->usHeight;
		usWidth					= hPixHandle->p16BPPObject->usWidth;
		sOffsetX				= hPixHandle->p16BPPObject->sOffsetX;
		sOffsetY				= hPixHandle->p16BPPObject->sOffsetY;
	}
	else
	{
		Assert(false);
	}
}

UINT32 GetInterfacePicWidth(UINT16 usItem )
{
	UINT16		usGraphicNum;
	INVTYPE		*pItem;
	UINT32		usWidth, usHeight;
	INT32		sOffsetX, sOffsetY;
	UINT32		uiImage;

	pItem = &Item[ usItem ];
	usGraphicNum = g_bUsePngItemImages ? 0 : pItem->ubGraphicNum;

	uiImage = GetInterfaceGraphicForItem( pItem );

	GetVideoObjectDimensions( uiImage, usGraphicNum, usWidth, usHeight, sOffsetX, sOffsetY );

	return usWidth;
}

UINT32 GetInterfacePicHeight(UINT16 usItem )
{
	UINT16		usGraphicNum;
	INVTYPE		*pItem;
	UINT32		usWidth, usHeight;
	INT32		sOffsetX, sOffsetY;
	UINT32		uiImage;

	pItem = &Item[ usItem ];
	usGraphicNum = g_bUsePngItemImages ? 0 : pItem->ubGraphicNum;

	uiImage = GetInterfaceGraphicForItem( pItem );

	GetVideoObjectDimensions( uiImage, usGraphicNum, usWidth, usHeight, sOffsetX, sOffsetY );

	return usHeight;
}

void DrawBufferInterfacePic(UINT16 usItem, INT16 sCenterX, INT16 sCenterY )
{
	UINT16		usGraphicNum;
	INVTYPE		*pItem;
	UINT32		usWidth, usHeight;
	INT32		sOffsetX, sOffsetY;
	INT16		sX, sY;
	UINT32		uiImage;
	HVOBJECT	hVObject;
	
	pItem = &Item[ usItem ];
	usGraphicNum = g_bUsePngItemImages ? 0 : pItem->ubGraphicNum;

	uiImage = GetInterfaceGraphicForItem( pItem );

	GetVideoObjectDimensions( uiImage, usGraphicNum, usWidth, usHeight, sOffsetX, sOffsetY );
	sX = sCenterX - usWidth/2 - sOffsetX;
	sY = sCenterY - usHeight/2 - sOffsetY;

	//sX = sCenterX - usWidth/2;	
	//sY = sCenterY - usHeight/2;
	
	GetVideoObject( &hVObject, GetInterfaceGraphicForItem( pItem ) );

	BltVideoObjectOutlineFromIndex( guiSAVEBUFFER, uiImage, usGraphicNum, sX, sY, Get16BPPColor( FROMRGB( 255, 255, 255 ) ), FALSE );
	//RestoreExternBackgroundRect( sX, sY, usWidth, usHeight );
}

void DrawBufferPic(UINT32 uiImage, UINT16 usIndex, INT16 sCenterX, INT16 sCenterY )
{
	UINT32		usWidth, usHeight;
	INT32		sOffsetX, sOffsetY;
	INT16		sX, sY;

	GetVideoObjectDimensions( uiImage, usIndex, usWidth, usHeight, sOffsetX, sOffsetY );
	sX = sCenterX - usWidth/2 - sOffsetX;
	sY = sCenterY - usHeight/2 - sOffsetY;

	BltVideoObjectOutlineFromIndex( guiSAVEBUFFER, uiImage, usIndex, sX, sY, Get16BPPColor( FROMRGB( 255, 255, 255 ) ), FALSE );

	DeleteVideoObjectFromIndex( uiImage );
}

void DrawBufferBigPic(OBJECTTYPE* pObject, INT16 sCenterX, INT16 sCenterY)
{
	UINT32		uiImage;

	// extended custom gun pictures
	if (gGameExternalOptions.fExtendedItemImages)
	{
		DrawBufferCustomPic(pObject, sCenterX, sCenterY);
	}
	// load default image
	else if(LoadTileGraphicForItem( &(Item[ pObject->usItem ]), &uiImage ))
	{
		DrawBufferPic( uiImage, 0, sCenterX, sCenterY );
	}	
}

void PrintCustomSaveBuffer( INT32 font, UINT8 ubBColor,  UINT8 ubFColor, INT16 sX, INT16 sY, CHAR16* pStr )
{
	SetFont( font );
	SetFontBackground( ubBColor );
	SetFontForeground ( ubFColor );
	//SetFontDestBuffer( guiSAVEBUFFER, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, FALSE );
	mprintf( sX, sY, pStr );
	//gprintfRestore( sX, sY, pStr );
	//SetFontDestBuffer( FRAME_BUFFER, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, FALSE );
	SetFont( TINYFONT1 );
}

void SwitchTacticalPanelMode( void )
{
	fInterfacePanelDirty = DIRTYLEVEL2;
}

BOOLEAN PrepareFaceData(SOLDIERTYPE *pSoldier, UINT32 &uiFaceID, UINT16 &usFaceIndex, VOBJECT_DESC &VObjectDesc, HVOBJECT &hVObject)
{
	BOOLEAN fFaceLoaded = FALSE;

	usFaceIndex = MAX_65_FACES;
	VObjectDesc.fCreateFlags = VOBJECT_CREATE_FROMFILE;

	if (pSoldier->ubProfile != NO_PROFILE)
	{
		if (gProfilesIMP[pSoldier->ubProfile].ProfilId == pSoldier->ubProfile)
		{
			if (gMercProfiles[pSoldier->ubProfile].ubFaceIndex < 100)
				sprintf(VObjectDesc.ImageFile, "IMPFaces\\65Face\\%02d.sti", gMercProfiles[pSoldier->ubProfile].ubFaceIndex);
			else
				sprintf(VObjectDesc.ImageFile, "IMPFaces\\65Face\\%03d.sti", gMercProfiles[pSoldier->ubProfile].ubFaceIndex);
		}
		else
		{
			if (gMercProfiles[pSoldier->ubProfile].ubFaceIndex < 100)
			{
				sprintf(VObjectDesc.ImageFile, "Faces\\65Face\\%02d.sti", gMercProfiles[pSoldier->ubProfile].ubFaceIndex);
				if (!FileExists(VObjectDesc.ImageFile))
					sprintf(VObjectDesc.ImageFile, "Faces\\33Face\\B%02d.sti", gMercProfiles[pSoldier->ubProfile].ubFaceIndex);
			}
			else
			{
				sprintf(VObjectDesc.ImageFile, "Faces\\65Face\\%03d.sti", gMercProfiles[pSoldier->ubProfile].ubFaceIndex);
				if (!FileExists(VObjectDesc.ImageFile))
					sprintf(VObjectDesc.ImageFile, "Faces\\33Face\\B%03d.sti", gMercProfiles[pSoldier->ubProfile].ubFaceIndex);
			}
		}

		if (AddVideoObject(&VObjectDesc, &uiFaceID))
		{
			if (GetVideoObject(&hVObject, uiFaceID))
			{
				hVObject->pShades[0] = Create16BPPPaletteShaded(hVObject->pPaletteEntry, 255, 255, 255, FALSE);
				hVObject->pShades[1] = Create16BPPPaletteShaded(hVObject->pPaletteEntry, 250, 25, 25, TRUE);

				usFaceIndex = 0;
				fFaceLoaded = TRUE;
			}
		}
	}
	else
	{
		if (pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE_MILITIA || pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE && pSoldier->IsAssassin())
		{
			if (pSoldier->ubBodyType == REGFEMALE)
				usFaceIndex = MILITIA3F_FACE;
			else
				usFaceIndex = MILITIA3_FACE;
		}
		else if (pSoldier->ubSoldierClass == SOLDIER_CLASS_REG_MILITIA || pSoldier->ubSoldierClass == SOLDIER_CLASS_ARMY && pSoldier->IsAssassin())
		{
			if (pSoldier->ubBodyType == REGFEMALE)
				usFaceIndex = MILITIA2F_FACE;
			else
				usFaceIndex = MILITIA2_FACE;
		}
		else if (pSoldier->ubSoldierClass == SOLDIER_CLASS_GREEN_MILITIA || pSoldier->ubSoldierClass == SOLDIER_CLASS_ADMINISTRATOR && pSoldier->IsAssassin())
		{
			if (pSoldier->ubBodyType == REGFEMALE)
				usFaceIndex = MILITIA1F_FACE;
			else
				usFaceIndex = MILITIA1_FACE;
		}
		else if (pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE)
		{
			if (pSoldier->ubBodyType == REGFEMALE)
				usFaceIndex = ELITEF_FACE;
			else
				usFaceIndex = ELITE_FACE;
		}
		else if (pSoldier->ubSoldierClass == SOLDIER_CLASS_ARMY)
		{
			usFaceIndex = TROOP_FACE;
		}
		else if (pSoldier->ubSoldierClass == SOLDIER_CLASS_ADMINISTRATOR)
		{
			usFaceIndex = ADMIN_FACE;
		}
		else if (pSoldier->ubBodyType == ADULTFEMALEMONSTER)
		{
			usFaceIndex = AF_CREATURE_FACE;
		}
		else if (pSoldier->ubBodyType == AM_MONSTER)
		{
			usFaceIndex = AM_CREATURE_FACE;
		}
		else if (pSoldier->ubBodyType == YAF_MONSTER)
		{
			usFaceIndex = YF_CREATURE_FACE;
		}
		else if (pSoldier->ubBodyType == YAM_MONSTER)
		{
			usFaceIndex = YM_CREATURE_FACE;
		}

		sprintf(VObjectDesc.ImageFile, "Interface\\SmFaces.sti");

		if (AddVideoObject(&VObjectDesc, &uiFaceID))
		{
			if (GetVideoObject(&hVObject, uiFaceID))
			{
				hVObject->pShades[0] = Create16BPPPaletteShaded(hVObject->pPaletteEntry, 255, 255, 255, FALSE);
				hVObject->pShades[1] = Create16BPPPaletteShaded(hVObject->pPaletteEntry, 250, 25, 25, TRUE);

				fFaceLoaded = TRUE;
			}
		}
	}

	return fFaceLoaded;
}

BOOLEAN TacticalLeftPanelActive(void)
{
	if (guiCurrentScreen == GAME_SCREEN && 
		gGameSettings.fOptions[TOPTION_TACTICAL_LEFT_PANEL] && 		
		INTERFACE_START_X >= INTERFACE_PANEL_SMALL)
		return TRUE;

	return FALSE;
}

void DisableLeftPanelHelpTooltip(void)
{
	/*if (gsCurInterfacePanel == TEAM_PANEL)
		SetRegionFastHelpText(&gTEAM_LeftPanelRegion, L"");
	else if (gsCurInterfacePanel == SM_PANEL)
		SetRegionFastHelpText(&gSM_LeftPanelRegion, L"");*/
}

void DisableRightPanelHelpTooltip(void)
{
	if (gsCurInterfacePanel == TEAM_PANEL)
		SetRegionFastHelpText(&gTEAM_RightPanelRegion, L"");
	else if (gsCurInterfacePanel == SM_PANEL)
		SetRegionFastHelpText(&gSM_RightPanelRegion, L"");
}

MOUSE_REGION ActionPanelMouseRegion;

INT16	sSelectedAction = -1;

void RenderActionPanel(void)
{
	PrepareTacticalRightPanelData(2, 0);

	if (gsPanelTotalWidth < INTERFACE_PANEL_MIN)
		return;	

	UINT16 usFontHeight = GetFontHeight(TINYFONT1);
	SetFontDestBuffer(guiSAVEBUFFER, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, FALSE);
	INT16 sX, sY;
	sX = gsPanelWindowX;
	sY = gsPanelWindowY;

	INT32 sSkip = gsTacticalActionOffset;

	for (std::vector<ActionData>::iterator iter = gActionDataVector.begin(); iter != gActionDataVector.end() && sY < gsPanelWindowY + gsPanelWindowHeight - usFontHeight; ++iter)
	{
		if (sSkip > 0)
		{
			sSkip--;
			continue;
		}

		if (!TacticalActionActive(iter->id))
			PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_GRAY6, sX, sY, (CHAR16*)(iter->name.c_str()));
		else if (sSelectedAction == iter->id)
			PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_LTYELLOW, sX, sY, (CHAR16*)(iter->name.c_str()));
		else
			PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_DKYELLOW, sX, sY, (CHAR16*)(iter->name.c_str()));

		sY += usFontHeight;
	}

	if (gsPanelTotalWidth >= INTERFACE_PANEL_MEDIUM)
	{		
		INT32 sSkip = gsTacticalActionOffset;

		sY = gsPanelWindowY;
		//sX += gsPanelWindowWidth / 2;
		sX += INTERFACE_PANEL_MEDIUM / 2;
		for (std::vector<ActionData>::iterator iter = gActionDataVector2.begin(); iter != gActionDataVector2.end() && sY < gsPanelWindowY + gsPanelWindowHeight - usFontHeight; ++iter)
		{
			if (sSkip > 0)
			{
				sSkip--;
				continue;
			}

			if (!TacticalActionActive(iter->id))
				PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_GRAY6, sX, sY, (CHAR16*)(iter->name.c_str()));
			else if (sSelectedAction == iter->id)
				PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_MCOLOR_LTYELLOW, sX, sY, (CHAR16*)(iter->name.c_str()));
			else
				PrintCustomSaveBuffer(TINYFONT1, FONT_MCOLOR_BLACK, FONT_DKYELLOW, sX, sY, (CHAR16*)(iter->name.c_str()));

			sY += usFontHeight;
		}
	}

	RestoreExternBackgroundRect(gsPanelWideX, gsPanelWideY, gsPanelTotalWidth, gsPanelWideHeight);
	// reset to frame buffer!
	SetFontDestBuffer(FRAME_BUFFER, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, FALSE);
}

void CreateMouseRegionsActionPanel(void)
{
	PrepareTacticalRightPanelData(2, 0);

	if (gsPanelTotalWidth < INTERFACE_PANEL_MIN)
		return;	

	gsActionRegionNum = 0;

	UINT16 usFontHeight = GetFontHeight(TINYFONT1);	
	INT16 sOffsetY = 0;
	INT16 sOffsetX = 0;

	INT32 sSkip = gsTacticalActionOffset;
	for (std::vector<ActionData>::iterator iter = gActionDataVector.begin(); iter != gActionDataVector.end() && gsPanelWindowY + sOffsetY < gsPanelWindowY + gsPanelWindowHeight - usFontHeight; ++iter)
	{
		if(sSkip > 0)
		{
			sSkip--;
			continue;
		}

		MSYS_DefineRegion(&gActionRegionVector[gsActionRegionNum], gsPanelWindowX, gsPanelWindowY + sOffsetY, gsPanelWindowX + StringPixLength((STR16)(iter->name.c_str()), TINYFONT1), gsPanelWindowY + sOffsetY + usFontHeight,
			MSYS_PRIORITY_HIGHEST, 0, ActionPanelMvtCallback, ActionPanelBtnCallback);

		MSYS_SetRegionUserData(&gActionRegionVector[gsActionRegionNum], 0, iter->id);
		SetRegionFastHelpText(&gActionRegionVector[gsActionRegionNum], (STR16)(iter->tooltip.c_str()));

		sOffsetY += usFontHeight;
		gsActionRegionNum++;
	}

	if (gsPanelTotalWidth >= INTERFACE_PANEL_MEDIUM)
	{
		INT32 sSkip = gsTacticalActionOffset;

		sOffsetY = 0;
		//sOffsetX = gsPanelWindowWidth / 2;
		sOffsetX = INTERFACE_PANEL_MEDIUM / 2;

		for (std::vector<ActionData>::iterator iter = gActionDataVector2.begin(); iter != gActionDataVector2.end() && gsPanelWindowY + sOffsetY < gsPanelWindowY + gsPanelWindowHeight - usFontHeight; ++iter)
		{
			if (sSkip > 0)
			{
				sSkip--;
				continue;
			}

			MSYS_DefineRegion(&gActionRegionVector[gsActionRegionNum], gsPanelWindowX + sOffsetX, gsPanelWindowY + sOffsetY, gsPanelWindowX + sOffsetX + StringPixLength((STR16)(iter->name.c_str()), TINYFONT1), gsPanelWindowY + sOffsetY + usFontHeight,
				MSYS_PRIORITY_HIGHEST, 0, ActionPanelMvtCallback, ActionPanelBtnCallback);

			MSYS_SetRegionUserData(&gActionRegionVector[gsActionRegionNum], 0, iter->id);
			SetRegionFastHelpText(&gActionRegionVector[gsActionRegionNum], (STR16)(iter->tooltip.c_str()));

			sOffsetY += usFontHeight;
			gsActionRegionNum++;
		}
	}
}

void DestroyMouseRegionsActionPanel(void)
{
	for (UINT16 i = 0; i < gsActionRegionNum; i++)
	{
		MSYS_RemoveRegion(&(gActionRegionVector[i]));
	}

	gsActionRegionNum = 0;

	if (guiCurrentScreen == GAME_SCREEN)
	{
		fInterfacePanelDirty = DIRTYLEVEL2;
	}
}

void ActionPanelMvtCallback(MOUSE_REGION * pRegion, INT32 iReason)
{
	INT32 iValue = -1;

	iValue = MSYS_GetRegionUserData(pRegion, 0);

	if (iReason & MSYS_CALLBACK_REASON_GAIN_MOUSE)
	{
		sSelectedAction = (INT16)iValue;
		fInterfacePanelDirty = DIRTYLEVEL2;
	}
	if (iReason & MSYS_CALLBACK_REASON_LOST_MOUSE)
	{
		sSelectedAction = -1;
		fInterfacePanelDirty = DIRTYLEVEL2;
	}

	return;
}

void ActionPanelBtnCallback(MOUSE_REGION * pRegion, INT32 iReason)
{
	INT32 iValue = 0;

	iValue = MSYS_GetRegionUserData(pRegion, 0);

	if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		if (TacticalActionActive(iValue))
		{
			ExecuteTacticalAction(iValue);
		}		
	}

	return;
}

void InitializeActionPanelData(void)
{
	PrepareTacticalRightPanelData(2, 0);

	gsActionRegionNum = 0;
	gActionDataVector.clear();
	gActionDataVector2.clear();

	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_DROP_BACKPACK));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_PICKUP_BACKPACK));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_DISGUISE));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_LOSE_DISGUISE));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_TEST_DISGUISE));

	if (!gGameOptions.fFoodSystem) 
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_DRINK_CANTEEN));

	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_SWAP_GOGGLES));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_RELOAD));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_RELOAD_ALL));

	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_UNJAM_GUN));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_TRANSFORM_ITEM));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_TRANSFORM_LASER));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_TRANSFORM_SCOPE));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_TRANSFORM_FLASHLIGHT));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_TRANSFORM_STOCK));

	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_MOVE_ITEMS));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_SORT_ITEMS));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_MERGE_ITEMS));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_REMOVE_UNLOAD));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_ORGANIZE_ITEMS));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_MAKE_BOXES));
	gActionDataVector.push_back(ActionData(TACTICAL_ACTION_MAKE_CRATES));

	if (gGameExternalOptions.fDirtSystem)
	{
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_CLEAN_GUN));
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_CLEAN_ALL_GUNS));
	}	

	if (gsPanelTotalWidth >= INTERFACE_PANEL_MEDIUM)
	{
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_SKILL_MENU));
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_SECTOR_MENU));
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_COVER_MENU));
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_INVENTORY_MENU));
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_HELP_MENU));

		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_DISPLAY_OFF));
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_DISPLAY_MERC));
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_DISPLAY_ENEMY));		
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_DISPLAY_TRAPS));		
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_DISPLAY_DETECTOR));
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_DISPLAY_NETWORK));

		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_DISPLAY_NETWORK_A));
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_DISPLAY_NETWORK_B));
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_DISPLAY_NETWORK_C));
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_DISPLAY_NETWORK_D));		
		
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_WATER_DRUM));
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_BAD_WATER));
		gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_AUTO_BANDAGE));

		if (gGameExternalOptions.fMilitiaUseSectorInventory)
		{
			gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_MILITIA_DROP_GEAR));
			gActionDataVector2.push_back(ActionData(TACTICAL_ACTION_MILITIA_TAKE_GEAR));
		}
	}
	else
	{
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_SKILL_MENU));
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_SECTOR_MENU));
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_COVER_MENU));
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_INVENTORY_MENU));
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_HELP_MENU));

		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_DISPLAY_OFF));
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_DISPLAY_MERC));
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_DISPLAY_ENEMY));		
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_DISPLAY_TRAPS));		
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_DISPLAY_DETECTOR));
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_DISPLAY_NETWORK));

		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_DISPLAY_NETWORK_A));
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_DISPLAY_NETWORK_B));
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_DISPLAY_NETWORK_C));
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_DISPLAY_NETWORK_D));
		
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_WATER_DRUM));
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_BAD_WATER));
		gActionDataVector.push_back(ActionData(TACTICAL_ACTION_AUTO_BANDAGE));

		if (gGameExternalOptions.fMilitiaUseSectorInventory)
		{
			gActionDataVector.push_back(ActionData(TACTICAL_ACTION_MILITIA_DROP_GEAR));
			gActionDataVector.push_back(ActionData(TACTICAL_ACTION_MILITIA_TAKE_GEAR));
		}
	}
}

extern COVER_DRAW_MODE gubDrawMode;
extern MINES_DRAW_MODE gubDrawModeMine;
extern void HandleCleanWeapons(BOOLEAN fEntireTeam);
extern void HandleTBDropBackpacks(BOOLEAN fAll);
extern void HandleTBPickUpBackpacks(BOOLEAN fAll);
extern void BeginAutoBandage();
extern void HandleTacticalCoverMenu(void);
extern void HandleTacticalInventoryMenu(void);
extern BOOLEAN ShouldTheHelpScreenComeUp(UINT8 ubScreenID, BOOLEAN fForceHelpScreenToComeUp);
extern void HandleTBSwapGoggles(void);
extern void HandleTBReload(void);
extern void HandleTBReloadAll(void);
extern void HandleTacticalMoveItems();
extern void StackAndSort(BOOLEAN fRestrictToAmmo);
extern void SeparateItems(void);
extern void HandleTacticalAmmoCrates(UINT8 magType);
extern void DisplayMines(const BOOLEAN& forceUpdate);
extern void HandleTBUnjamGun(void);
extern void HandleTacticalTransformItem(void);
extern void HandleTacticalTransformLaser(void);
extern void HandleTacticalTransformScope(void);
extern void HandleTacticalTransformFlashlight(void);
extern void HandleTacticalTransformStock(void);
extern void HandleTacticalDrinkCanteen(void);

extern BOOLEAN HandleTBCheckSkillsMenu(void);
extern BOOLEAN HandleTBCheckDropBackpacks(BOOLEAN fAll);
extern BOOLEAN HandleTBCheckPickUpBackpacks(BOOLEAN fAll);
extern BOOLEAN HandleTBCheckAutoBandage(void);
extern BOOLEAN HandleTBCheckWaterDrum(void);
extern BOOLEAN HandleTBCheckBadWater(void);
extern BOOLEAN HandleTBCheckCanDisguise(void);
extern BOOLEAN HandleTBCheckCanLoseDisguise(void);
extern BOOLEAN HandleTBCheckCanTestDisguise(void);
extern BOOLEAN HandleTBCheckCanCleanGun(BOOLEAN fAPCheck);
extern BOOLEAN HandleTBCheckCanCleanAllGuns(void);
extern BOOLEAN HandleTBCheckMilitiaDropGear(void);
extern BOOLEAN HandleTBCheckMilitiaTakeGear(void);
extern BOOLEAN HandleTBCheckSwapGoggles(void);
extern BOOLEAN HandleTBCheckActiveMerc(void);
extern BOOLEAN HandleTBCheckManageItems(void);
extern BOOLEAN HandleTBCheckUnjamGun(BOOLEAN fAPCheck);
extern BOOLEAN HandleTBCheckTransformItem(BOOLEAN fAPCheck);
extern BOOLEAN HandleTBCheckTransformLaser(BOOLEAN fAPCheck);
extern BOOLEAN HandleTBCheckTransformScope(BOOLEAN fAPCheck);
extern BOOLEAN HandleTBCheckTransformFlashlight(BOOLEAN fAPCheck);
extern BOOLEAN HandleTBCheckTransformStock(BOOLEAN fAPCheck);
extern BOOLEAN HandleTBCheckDrinkCanteen(BOOLEAN fAPCheck);

void ExecuteTacticalAction(UINT32 action_id)
{
	switch(action_id)
	{
	case TACTICAL_ACTION_DROP_BACKPACK:
		HandleTBDropBackpacks(FALSE);
		break;
	case TACTICAL_ACTION_PICKUP_BACKPACK:
		HandleTBPickUpBackpacks(FALSE);
		break;
	case TACTICAL_ACTION_SKILL_MENU:
		if (gusSelectedSoldier != NOBODY && MercPtrs[gusSelectedSoldier] && !TileIsOutOfBounds(MercPtrs[gusSelectedSoldier]->sGridNo))
		{
			LocateGridNo(MercPtrs[gusSelectedSoldier]->sGridNo);
			TraitsMenu(MercPtrs[gusSelectedSoldier]->sGridNo);
		}
		break;
	case TACTICAL_ACTION_SECTOR_MENU:
		if (gusSelectedSoldier != NOBODY)
		{
			//HandleTacticalFunctionSelection(MercPtrs[gusSelectedSoldier]);
			HandleTBSectorMenu();
		}
		break;
	case TACTICAL_ACTION_AUTO_BANDAGE:
		BeginAutoBandage();
		break;
	case TACTICAL_ACTION_COVER_MENU:
		HandleTacticalCoverMenu();
		break;
	case TACTICAL_ACTION_INVENTORY_MENU:
		HandleTacticalInventoryMenu();
		break;
	case TACTICAL_ACTION_HELP_MENU:
		ShouldTheHelpScreenComeUp(HELP_SCREEN_TACTICAL, TRUE);
		break;
	case TACTICAL_ACTION_WATER_DRUM:
		ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, TacticalStr[WATER_DRUM_REFILL]);
		SectorFillCanteensFromWaterDrum();
		break;
	case TACTICAL_ACTION_BAD_WATER:
		ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, TacticalStr[BAD_WATER_REFILL]);
		SectorFillCanteens(TRUE);
		break;
	case TACTICAL_ACTION_DISGUISE:
		if(gusSelectedSoldier != NOBODY)
		{
			SOLDIERTYPE *pSoldier = MercPtrs[gusSelectedSoldier];

			pSoldier->Disguise(TRUE);

			if (pSoldier->usSoldierFlagMask & (SOLDIER_COVERT_CIV | SOLDIER_COVERT_SOLDIER))
			{
				pSoldier->usSoldierFlagMask2 |= SOLDIER_COVERT_AUTO_DISGUISE;
				pSoldier->SpySelfTest();
			}
		}		
		break;
	case TACTICAL_ACTION_LOSE_DISGUISE:
		if (gusSelectedSoldier != NOBODY)
		{
			SOLDIERTYPE *pSoldier = MercPtrs[gusSelectedSoldier];

			if (pSoldier->usSoldierFlagMask & (SOLDIER_COVERT_CIV | SOLDIER_COVERT_SOLDIER))
			{
				pSoldier->usSoldierFlagMask2 &= ~SOLDIER_COVERT_AUTO_DISGUISE;
				pSoldier->LooseDisguise();
			}
		}
		break;
	case TACTICAL_ACTION_TEST_DISGUISE:
		if (gusSelectedSoldier != NOBODY)
		{
			SOLDIERTYPE *pSoldier = MercPtrs[gusSelectedSoldier];
			if (pSoldier->usSoldierFlagMask & (SOLDIER_COVERT_CIV | SOLDIER_COVERT_SOLDIER))
				pSoldier->SpySelfTest();
		}
		break;
	case TACTICAL_ACTION_DRINK_CANTEEN:
		HandleTacticalDrinkCanteen();
		break;
	case TACTICAL_ACTION_CLEAN_GUN:
		HandleCleanWeapons(FALSE);
		break;
	case TACTICAL_ACTION_CLEAN_ALL_GUNS:
		HandleCleanWeapons(TRUE);
		break;
	case TACTICAL_ACTION_MILITIA_DROP_GEAR:
		TeamDropAll(MILITIA_TEAM);
		break;
	case TACTICAL_ACTION_MILITIA_TAKE_GEAR:
		TeamRestock(MILITIA_TEAM);
		break;
	case TACTICAL_ACTION_SWAP_GOGGLES:
		HandleTBSwapGoggles();
		break;
	case TACTICAL_ACTION_RELOAD:
		HandleTBReload();
		break;
	case TACTICAL_ACTION_RELOAD_ALL:
		HandleTBReloadAll();
		break;
	case TACTICAL_ACTION_MOVE_ITEMS:
		HandleTacticalMoveItems();
		ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pImpButtonText[11]);
		break;
	case TACTICAL_ACTION_SORT_ITEMS:
		StackAndSort(TRUE);
		ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pImpButtonText[11]);
		break;
	case TACTICAL_ACTION_MERGE_ITEMS:
		StackAndSort(FALSE);
		ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pImpButtonText[11]);
		break;
	case TACTICAL_ACTION_REMOVE_UNLOAD:
		SeparateItems();
		break;
	case TACTICAL_ACTION_ORGANIZE_ITEMS:
		SeparateItems();
		StackAndSort(TRUE);
		break;
	case TACTICAL_ACTION_MAKE_BOXES:
		HandleTacticalAmmoCrates(AMMO_BOX);
		break;
	case TACTICAL_ACTION_MAKE_CRATES:
		HandleTacticalAmmoCrates(AMMO_CRATE);
		break;
	case TACTICAL_ACTION_DISPLAY_OFF:
		gubDrawModeMine = MINES_DRAW_OFF;
		gubDrawMode = COVER_DRAW_OFF;
		DisplayMines(TRUE);
		break;
	case TACTICAL_ACTION_DISPLAY_ENEMY:
		gubDrawMode = COVER_DRAW_ENEMY_VIEW;
		gubDrawModeMine = MINES_DRAW_OFF;
		DisplayMines(TRUE);
		break;
	case TACTICAL_ACTION_DISPLAY_MERC:
		gubDrawMode = COVER_DRAW_MERC_VIEW;
		gubDrawModeMine = MINES_DRAW_OFF;
		DisplayMines(TRUE);
		break;
	case TACTICAL_ACTION_DISPLAY_TRAPS:
		gubDrawMode = COVER_DRAW_OFF;
		gubDrawModeMine = MINES_DRAW_PLAYERTEAM_NETWORKS;
		DisplayMines(TRUE);
		break;
	case TACTICAL_ACTION_DISPLAY_NETWORK:
		gubDrawMode = COVER_DRAW_OFF;
		gubDrawModeMine = MINES_DRAW_NETWORKCOLOURING;
		DisplayMines(TRUE);
		break;
	case TACTICAL_ACTION_DISPLAY_DETECTOR:
		gubDrawMode = COVER_DRAW_OFF;
		gubDrawModeMine = MINES_DRAW_DETECT_ENEMY;
		DisplayMines(TRUE);
		break;
	case TACTICAL_ACTION_DISPLAY_NETWORK_A:
		gubDrawMode = COVER_DRAW_OFF;
		gubDrawModeMine = MINES_DRAW_NET_A;
		DisplayMines(TRUE);
		break;
	case TACTICAL_ACTION_DISPLAY_NETWORK_B:
		gubDrawMode = COVER_DRAW_OFF;
		gubDrawModeMine = MINES_DRAW_NET_B;
		DisplayMines(TRUE);
		break;
	case TACTICAL_ACTION_DISPLAY_NETWORK_C:
		gubDrawMode = COVER_DRAW_OFF;
		gubDrawModeMine = MINES_DRAW_NET_C;
		DisplayMines(TRUE);
		break;
	case TACTICAL_ACTION_DISPLAY_NETWORK_D:
		gubDrawMode = COVER_DRAW_OFF;
		gubDrawModeMine = MINES_DRAW_NET_D;
		DisplayMines(TRUE);
		break;
	case TACTICAL_ACTION_UNJAM_GUN:
		HandleTBUnjamGun();
		break;
	case TACTICAL_ACTION_TRANSFORM_ITEM:
		HandleTacticalTransformItem();
		break;
	case TACTICAL_ACTION_TRANSFORM_LASER:
		HandleTacticalTransformLaser();
		break;
	case TACTICAL_ACTION_TRANSFORM_SCOPE:
		HandleTacticalTransformScope();
		break;
	case TACTICAL_ACTION_TRANSFORM_FLASHLIGHT:
		HandleTacticalTransformFlashlight();
		break;
	case TACTICAL_ACTION_TRANSFORM_STOCK:
		HandleTacticalTransformStock();
		break;
	}
}

BOOLEAN TacticalActionActive(UINT32 action_id)
{
	// allow only at player's turn
	if (gTacticalStatus.ubCurrentTeam != gbPlayerNum)
		return FALSE;

	switch (action_id)
	{
	case TACTICAL_ACTION_DROP_BACKPACK:
		return HandleTBCheckDropBackpacks(FALSE);
	case TACTICAL_ACTION_PICKUP_BACKPACK:		
		return HandleTBCheckPickUpBackpacks(FALSE);
	case TACTICAL_ACTION_SKILL_MENU:
		return HandleTBCheckSkillsMenu();
	case TACTICAL_ACTION_SECTOR_MENU:
		return HandleTBCheckSkillsMenu();
	case TACTICAL_ACTION_AUTO_BANDAGE:
		return HandleTBCheckAutoBandage();
	case TACTICAL_ACTION_COVER_MENU:
		return HandleTBCheckSkillsMenu();
	case TACTICAL_ACTION_INVENTORY_MENU:
		return HandleTBCheckSkillsMenu();
	case TACTICAL_ACTION_HELP_MENU:
		return (gpItemPointer == 0);
	case TACTICAL_ACTION_WATER_DRUM:
		return HandleTBCheckWaterDrum();
	case TACTICAL_ACTION_BAD_WATER:
		return HandleTBCheckBadWater();
	case TACTICAL_ACTION_DISGUISE:
		return HandleTBCheckCanDisguise();
	case TACTICAL_ACTION_LOSE_DISGUISE:
		return HandleTBCheckCanLoseDisguise();
	case TACTICAL_ACTION_TEST_DISGUISE:
		return HandleTBCheckCanTestDisguise();
	case TACTICAL_ACTION_DRINK_CANTEEN:
		return HandleTBCheckDrinkCanteen(TRUE);
	case TACTICAL_ACTION_CLEAN_GUN:
		return HandleTBCheckCanCleanGun(TRUE);
	case TACTICAL_ACTION_CLEAN_ALL_GUNS:
		return HandleTBCheckCanCleanAllGuns();
	case TACTICAL_ACTION_MILITIA_DROP_GEAR:
		return HandleTBCheckMilitiaDropGear();
	case TACTICAL_ACTION_MILITIA_TAKE_GEAR:
		return HandleTBCheckMilitiaTakeGear();
	case TACTICAL_ACTION_SWAP_GOGGLES:
		return HandleTBCheckSwapGoggles();
	case TACTICAL_ACTION_RELOAD:
		return HandleTBCheckActiveMerc();
	case TACTICAL_ACTION_RELOAD_ALL:
		return HandleTBCheckActiveMerc();
	case TACTICAL_ACTION_MOVE_ITEMS:
	case TACTICAL_ACTION_SORT_ITEMS:
	case TACTICAL_ACTION_MERGE_ITEMS:
	case TACTICAL_ACTION_REMOVE_UNLOAD:
	case TACTICAL_ACTION_ORGANIZE_ITEMS:
	case TACTICAL_ACTION_MAKE_BOXES:
	case TACTICAL_ACTION_MAKE_CRATES:
		return HandleTBCheckManageItems();
	case TACTICAL_ACTION_DISPLAY_OFF:
		return !(gubDrawMode == COVER_DRAW_OFF && gubDrawModeMine == MINES_DRAW_OFF);
	case TACTICAL_ACTION_DISPLAY_ENEMY:
		return !(gubDrawMode == COVER_DRAW_ENEMY_VIEW && gubDrawModeMine == MINES_DRAW_OFF);
	case TACTICAL_ACTION_DISPLAY_MERC:
		return !(gubDrawMode == COVER_DRAW_MERC_VIEW && gubDrawModeMine == MINES_DRAW_OFF);
	case TACTICAL_ACTION_DISPLAY_TRAPS:
		return !(gubDrawMode == COVER_DRAW_OFF && gubDrawModeMine == MINES_DRAW_PLAYERTEAM_NETWORKS);
	case TACTICAL_ACTION_DISPLAY_NETWORK:
		return !(gubDrawMode == COVER_DRAW_OFF && gubDrawModeMine == MINES_DRAW_NETWORKCOLOURING);
	case TACTICAL_ACTION_DISPLAY_DETECTOR:
		return !(gubDrawMode == COVER_DRAW_OFF && gubDrawModeMine == MINES_DRAW_DETECT_ENEMY);
	case TACTICAL_ACTION_DISPLAY_NETWORK_A:
		return !(gubDrawMode == COVER_DRAW_OFF && gubDrawModeMine == MINES_DRAW_NET_A);
	case TACTICAL_ACTION_DISPLAY_NETWORK_B:
		return !(gubDrawMode == COVER_DRAW_OFF && gubDrawModeMine == MINES_DRAW_NET_B);
	case TACTICAL_ACTION_DISPLAY_NETWORK_C:
		return !(gubDrawMode == COVER_DRAW_OFF && gubDrawModeMine == MINES_DRAW_NET_C);
	case TACTICAL_ACTION_DISPLAY_NETWORK_D:
		return !(gubDrawMode == COVER_DRAW_OFF && gubDrawModeMine == MINES_DRAW_NET_D);
	case TACTICAL_ACTION_UNJAM_GUN:
		return HandleTBCheckUnjamGun(TRUE);
	case TACTICAL_ACTION_TRANSFORM_ITEM:
		return HandleTBCheckTransformItem(TRUE);
	case TACTICAL_ACTION_TRANSFORM_LASER:
		return HandleTBCheckTransformLaser(TRUE);
	case TACTICAL_ACTION_TRANSFORM_SCOPE:
		return HandleTBCheckTransformScope(TRUE);
	case TACTICAL_ACTION_TRANSFORM_FLASHLIGHT:
		return HandleTBCheckTransformFlashlight(TRUE);
	case TACTICAL_ACTION_TRANSFORM_STOCK:
		return HandleTBCheckTransformStock(TRUE);
	}
	return FALSE;
}

BOOLEAN UseLeftActionPanel(void)
{
	INT16 sPanelWidth = INTERFACE_WIDTH - 4 - INTERFACE_PANEL_RIGHT - gGameOptions.ubSquadSize * 83;
	
	if (sPanelWidth < INTERFACE_PANEL_SMALL && gsCurInterfacePanel == TEAM_PANEL && gGameSettings.fOptions[TOPTION_TACTICAL_PANEL])
		return TRUE;

	return FALSE;
}