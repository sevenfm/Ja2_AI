#ifdef PRECOMPILEDHEADERS
	#include "Strategic All.h"
	#include "GameSettings.h"
#else
	#include "sgp.h"
	#include "message.h"
	#include "quests.h"
	#include "game clock.h"
	#include "StrategicMap.h"
	#include "soldier profile.h"
	#include "LaptopSave.h"
	#include "Handle Items.h"
	#include "overhead.h"
	#include "Interface Dialogue.h"
	#include "Soldier Profile.h"
	#include "Isometric Utils.h"
	#include "Render Fun.h"
	#include "History.h"
	#include "Map Screen Helicopter.h"
	#include "Overhead.h"
	#include "Strategic Mines.h"
	#include "Boxing.h"
	#include "Campaign Types.h"
	#include "Strategic Town Loyalty.h"
	#include "Queen Command.h"
	#include "Campaign.h"
	#include "GameSettings.h"
	#include "Arms Dealer Init.h"
	#include "GameSettings.h"
	#include "Random.h"
	#include "Assignments.h"
	#include "strategic.h"
	#include "Strategic Event Handler.h"
#endif

#include "BobbyRMailOrder.h"
#include "connect.h"

#ifdef JA2UB
#include "email.h"
#include "Strategic Merc Handler.h"
#include "laptop.h"
#include "Ja25 Strategic Ai.h"
#include "ub_config.h"
#include "Ja25_Tactical.h"
#endif

#include "LuaInitNPCs.h"

#include "Luaglobal.h"

// anv: Waldo The Mechanic - for fact checking with helicopter
#include "Vehicles.h"

#define TESTQUESTS

extern SOLDIERTYPE * gpSrcSoldier;
extern SOLDIERTYPE * gpDestSoldier;

UINT8 gubQuest[MAX_QUESTS];
UINT8 gubFact[ NUM_FACTS ]; // this has to be updated when we figure out how many facts we have

INT16	gsFoodQuestSectorX;
INT16	gsFoodQuestSectorY;

extern void	GuaranteeAtLeastXItemsOfIndex( UINT8 ubArmsDealer, UINT16 usItemIndex, UINT8 ubHowMany );

extern void GiveQuestRewardPoint( INT16 sQuestSectorX, INT16 sQuestsSectorY, INT8 bExpReward, UINT8 bException );

extern void DebugQuestInfo(STR szOutput);

STR szQuest[] = {
	"QUEST_DELIVER_LETTER",
	"QUEST_FOOD_ROUTE",
	"QUEST_KILL_TERRORISTS",
	"QUEST_KINGPIN_IDOL",
	"QUEST_KINGPIN_MONEY",
	"QUEST_RUNAWAY_JOEY",
	"QUEST_RESCUE_MARIA",
	"QUEST_CHITZENA_IDOL",
	"QUEST_HELD_IN_ALMA",
	"QUEST_INTERROGATION",

	"QUEST_ARMY_FARM",
	"QUEST_FIND_SCIENTIST",
	"QUEST_DELIVER_VIDEO_CAMERA",
	"QUEST_BLOODCATS",
	"QUEST_FIND_HERMIT",
	"QUEST_CREATURES",
	"QUEST_CHOPPER_PILOT",
	"QUEST_ESCORT_SKYRIDER",
	"QUEST_FREE_DYNAMO",
	"QUEST_ESCORT_TOURISTS",

	"QUEST_FREE_CHILDREN",
	"QUEST_LEATHER_SHOP_DREAM",
	"QUEST_ESCORT_SHANK",

	"QUEST_23",
	"QUEST_24",
	"QUEST_KILL_DEIDRANNA",
};

STR szFact[] = {
	"FACT_OMERTA_LIBERATED",
	"FACT_DRASSEN_LIBERATED",
	"FACT_SANMONA_LIBERATED",
	"FACT_CAMBRIA_LIBERATED",
	"FACT_ALMA_LIBERATED",
	"FACT_GRUMM_LIBERATED",
	"FACT_TIXA_LIBERATED",
	"FACT_CHITZENA_LIBERATED",
	"FACT_ESTONI_LIBERATED",
	"FACT_BALIME_LIBERATED",
	"FACT_ORTA_LIBERATED",
	"FACT_MEDUNA_LIBERATED",

	"FACT_MIGUEL_FOUND",
	"FACT_LETTER_DELIVERED",
	"FACT_FOOD_ROUTE_EXISTS",
	"FACT_DIMITRI_DEAD",
	"FACT_CONVO_FATIMA_1",
	"FACT_CONVO_CARLOS_1",
	"FACT_CONVO_FATIMA_2",
	"FACT_CONVO_FATIMA_3",

	"FACT_CONVO_DIMITRI",
	"FACT_CONVO_CARLOS_2",
	"FACT_CONVO_MIGUEL_1",
	"FACT_MIGUEL_READ_LETTER",
	"FACT_24",

	"FACT_REBELS_HATE_PLAYER",
	"FACT_CONVO_FATIMA_4",
	"FACT_CONVO_MIGUEL_2",
	"FACT_CONVO_MIGUEL_IRA",
	"FACT_PACOS_KILLED",
	"FACT_CONVO_PACOS",
	"FACT_CURRENT_SECTOR_IS_SAFE",
	"FACT_BOBBYRAY_SHIPMENT_IN_TRANSIT",
	"FACT_NEW_BOBBYRAY_SHIPMENT_WAITING",
	"FACT_REALLY_NEW_BOBBYRAY_SHIPMENT_WAITING",
	"FACT_LARGE_SIZED_OLD_SHIPMENT_WAITING",
	"FACT_PLAYER_FOUND_ITEMS_MISSING",
	"FACT_PABLO_PUNISHED_BY_PLAYER",
	"FACT_CONVO_PABLO_1",
	"FACT_PABLO_RETURNED_GOODS",
	"FACT_MIGUEL_AND_ALL_REBELS_CAN_BE_RECRUITED",

	"FACT_PABLOS_BRIBED",
	"FACT_ESCORTING_SKYRIDER",
	"FACT_SKYRIDER_CLOSE_TO_CHOPPER",
	"FACT_CONVO_SKYRIDER_1",
	"FACT_SKYRIDER_USED_IN_MAPSCREEN",
	"FACT_NPC_OWED_MONEY",
	"FACT_NPC_WOUNDED",
	"FACT_NPC_WOUNDED_BY_PLAYER",
	"FACT_CONVO_FATHER_1",
	"FACT_IRA_NOT_PRESENT",
	"FACT_IRA_TALKING",
	"FACT_FOOD_QUEST_OVER",
	"FACT_PABLOS_STOLE_FROM_LATEST_SHIPMENT",
	"FACT_LAST_SHIPMENT_CRASHED",
	"FACT_LAST_SHIPMENT_WENT_TO_WRONG_AIRPORT",
	"FACT_SHIPMENT_DELAYED_24_HOUR",
	"FACT_PACKAGE_DAMAGED",
	"FACT_PACKAGE_LOST_PERMANENTLY",
	"FACT_NEXT_PACKAGE_CAN_BE_LOST",
	"FACT_NEXT_PACKAGE_CAN_BE_DELAYED",
	"FACT_MEDIUM_SIZED_SHIPMENT_WAITING",
	"FACT_LARGE_SIZED_SHIPMENT_WAITING",
	"FACT_DOREEN_HAD_CHANGE_OF_HEART",
	"FACT_CONVO_GORDON",
	"FACT_IRA_UNHIRED_AND_ALIVE",
	"FACT_CONVO_DYNAMO_1",
	"FACT_CONVO_DYNAMO_2",
	"FACT_NPC_BLEEDING",
	"FACT_CONVO_SHANK_1",
	"FACT_NPC_BLEEDING_BUT_OKAY",
	"FACT_PLAYER_HAS_HEAD_AND_CARMEN_IN_SAN_MONA",
	"FACT_PLAYER_HAS_HEAD_AND_CARMEN_IN_CAMBRIA",
	"FACT_PLAYER_HAS_HEAD_AND_CARMEN_IN_DRASSEN",
	"FACT_FATHER_DRUNK",
	"FACT_WOUNDED_MERCS_NEARBY",
	"FACT_ONE_WOUNDED_MERC_NEARBY",
	"FACT_MULTIPLE_WOUNDED_MERCS_NEARBY",
	"FACT_BRENDA_IN_STORE_AND_ALIVE",
	"FACT_BRENDA_DEAD",
	"FACT_CONVO_BRENDA_1",
	"FACT_NPC_IS_ENEMY",
	"FACT_PC_STRONG_AND_LESS_THAN_3_MALES_PRESENT",
	"FACT_PC_STRONG_AND_3_PLUS_MALES_PRESENT",
	"FACT_CONVO_HANS",
	"FACT_HANS_AT_SPOT",
	"FACT_TONY_NOT_AVAILABLE",
	"FACT_FEMALE_SPEAKING_TO_NPC",
	"FACT_PLAYER_USED_BROTHEL",
	"FACT_CARLA_AVAILABLE",
	"FACT_CINDY_AVAILABLE",
	"FACT_BAMBI_AVAILABLE",
	"FACT_NO_GIRLS_AVAILABLE",
	"FACT_PLAYER_WAITED_FOR_GIRL",
	"FACT_PLAYER_PAID_RIGHT_AMOUNT",
	"FACT_PLAYER_PASSED_GOON",
	"FACT_MULTIPLE_MERCS_CLOSE",
	"FACT_SOME_MERCS_CLOSE",
	"FACT_CONVO_KINGPIN_1",
	"FACT_DARREN_EXPECTING_MONEY",
	"FACT_PC_NEAR",
	"FACT_CARMEN_IN_C5",
	"FACT_CARMEN_EXPLAINED_DEAL",
	"FACT_KINGPIN_KNOWS_MONEY_GONE",
	"FACT_PLAYER_REPAID_KINGPIN",
	"FACT_FRANK_HAS_BEEN_BRIBED",
	"FACT_CONVO_DARREN",
	"FACT_PAST_CLUB_CLOSING_AND_PLAYER_WARNED",
	"FACT_JOEY_ESCORTED",
	"FACT_JOEY_IN_C5",
	"FACT_JOEY_NEAR_MARTHA",
	"FACT_JOEY_DEAD",
	"FACT_MERC_NEAR_MARTHA",
	"FACT_SPIKE_AT_DOOR",
	"FACT_CONVO_ANGEL_1",
	"FACT_ANGEL_SOLD_VEST",
	"FACT_MARIA_ESCORTED",
	"FACT_MARIA_ESCORTED_AT_LEATHER_SHOP",
	"FACT_PLAYER_WANTS_TO_BUY_LEATHER_VEST",
	"FACT_MARIA_ESCAPE_NOTICED",
	"FACT_ANGEL_LEFT_DEED",
	"FACT_CONVO_ANGEL_2",
	"FACT_NPC_BANDAGED_TODAY",
	"FACT_CONVO_DOREEN_1",
	"FACT_PABLO_WONT_STEAL",
	"FACT_AGENTS_PREVENTED_SHIPMENT",

	"FACT_HELICOPTER_IN_PERFECT_CONDITION ",
	"FACT_LARGE_AMOUNT_OF_MONEY",
	"FACT_SMALL_AMOUNT_OF_MONEY",
	"FACT_129",

	"FACT_HELICOPTER_LOST",
	"FACT_CONVO_WALDO",
	"FACT_CONVO_FATHER_2",
	"FACT_CONVO_FATHER_3",
	"FACT_CONVO_FATHER_4",
	"FACT_LOYALTY_OKAY",
	"FACT_LOYALTY_LOW",
	"FACT_LOYALTY_HIGH",
	"FACT_PLAYER_DOING_POORLY",
	"FACT_CONVO_CARMEN_1",
	"FACT_CURRENT_SECTOR_G9",
	"FACT_CURRENT_SECTOR_C5",
	"FACT_CURRENT_SECTOR_C13",
	"FACT_CARMEN_HAS_TEN_THOUSAND",
	"FACT_SLAY_HIRED_AND_WORKED_FOR_48_HOURS",
	"FACT_CONVO_CARMEN_2",
	"FACT_SLAY_IN_SECTOR",
	"FACT_CONVO_CARMEN_3",
	"FACT_VINCE_EXPLAINED_HAS_TO_CHARGE",
	"FACT_VINCE_EXPECTING_MONEY",
	"FACT_PLAYER_STOLE_MEDICAL_SUPPLIES",
	"FACT_PLAYER_STOLE_MEDICAL_SUPPLIES_AGAIN",
	"FACT_VINCE_RECRUITABLE",
	"FACT_153",

	"FACT_CONVO_VINCE",
	"FACT_SLAY_OFFERED_DEAL",
	"FACT_ALL_TERRORISTS_KILLED",
	"FACT_ELGIN_ALIVE",
	"FACT_CONVO_MARIA_1",
	"FACT_CONVO_SKYRIDER_2",
	"FACT_CONVO_JOEY_1",
	"FACT_CONVO_JOHN_1",
	"FACT_CONVO_MARY",
	"FACT_CONVO_WALTER",
	"FACT_SHANK_IN_SQUAD_BUT_NOT_SPEAKING",
	"FACT_CONVO_MADDOG_1",
	"FACT_CONVO_JAKE",
	"FACT_SHANK_NOT_IN_SECTOR",
	"FACT_BLOODCAT_QUEST_STARTED_TWO_DAYS_AGO",
	"FACT_CONVO_ARMAND",
	"FACT_QUEEN_DEAD",

	"FACT_SPEAKER_AIM_OR_AIM_NEARBY",
	"FACT_MINE_EMPTY",
	"FACT_MINE_RUNNING_OUT",
	"FACT_MINE_PRODUCING_BUT_LOYALTY_LOW",
	"FACT_CREATURES_IN_MINE",
	"FACT_PLAYER_LOST_MINE",
	"FACT_MINE_AT_FULL_PRODUCTION",
	"FACT_DYNAMO_SPEAKING_OR_NEARBY",
	"FACT_179",
	"FACT_180",
	"FACT_181",
	"FACT_182",
	"FACT_183",

	"FACT_CHALICE_STOLEN",
	"FACT_JOHN_EPC",
	"FACT_JOHN_AND_MARY_EPCS",
	"FACT_MARY_ALIVE",
	"FACT_MARY_EPC",
	"FACT_MARY_BLEEDING",
	"FACT_JOHN_ALIVE",
	"FACT_JOHN_BLEEDING",
	"FACT_MARY_OR_JOHN_ARRIVED",
	"FACT_MARY_DEAD",
	"FACT_MINERS_PLACED",
	"FACT_KROTT_GOT_ANSWER_NO",
	"FACT_CONVO_MADLAB_1",
	"FACT_MADLAB_EXPECTING_FIREARM",
	"FACT_MADLAB_EXPECTING_VIDEO_CAMERA",
	"FACT_ITEM_POOR_CONDITION",
	"FACT_CONVO_MADLAB_2",
	"FACT_CONVO_MADLAB_3",
	"FACT_ROBOT_READY",
	"FACT_FIRST_ROBOT_DESTROYED",
	"FACT_MADLAB_HAS_GOOD_CAMERA",
	"FACT_ROBOT_READY_SECOND_TIME",
	"FACT_SECOND_ROBOT_DESTROYED",
	"FACT_CONVO_FRED_MATT",
	"FACT_DYNAMO_IN_J9",
	"FACT_DYNAMO_ALIVE",
	"FACT_ANOTHER_FIGHT_POSSIBLE",
	"FACT_RECEIVING_INCOME_FROM_DCAC",
	"FACT_PLAYER_BEEN_TO_K4",
	"FACT_CONVO_BREWSTER_1",
	"FACT_WARDEN_DEAD",
	"FACT_CONVO_ERNEST",
	"FACT_FIRST_BARTENDER",
	"FACT_SECOND_BARTENDER",
	"FACT_THIRD_BARTENDER",
	"FACT_FOURTH_BARTENDER",
	"FACT_MANNY_IS_BARTENDER",
	"FACT_NOTHING_REPAIRED_YET",
	"FACT_222",

	"FACT_CONVO_DAVE_1",
	"FACT_OK_USE_HUMMER",
	"FACT_CONVO_DAVE_2",
	"FACT_DAVE_HAS_GAS",
	"FACT_VEHICLE_PRESENT",
	"FACT_FIRST_BATTLE_WON",
	"FACT_ROBOT_RECRUITED_AND_MOVED",
	"FACT_NO_CLUB_FIGHTING_ALLOWED",
	"FACT_PLAYER_FOUGHT_THREE_TIMES_TODAY",
	"FACT_PLAYER_SPOKE_TO_DRASSEN_MINER",
	"FACT_PLAYER_DOING_WELL",
	"FACT_PLAYER_DOING_VERY_WELL",
	"FACT_FATHER_DRUNK_AND_SCIFI_OPTION_ON",
	"FACT_MICKY_DRUNK",
	"FACT_PLAYER_FORCED_WAY_INTO_BROTHEL",
	"FACT_CONVO_RAT_1",
	"FACT_PLAYER_PAID_FOR_TWO_IN_BROTHEL",
	"FACT_CONVO_GABBY_1",
	"FACT_CONVO_RAT_GABBY",
	"FACT_PLAYER_OWNS_2_TOWNS_INCLUDING_OMERTA",
	"FACT_PLAYER_OWNS_3_TOWNS_INCLUDING_OMERTA",
	"FACT_PLAYER_OWNS_4_TOWNS_INCLUDING_OMERTA",

	"FACT_DIMITRI_CAN_BE_RECRUITED",
	"FACT_246",
	"FACT_247",

	"FACT_MALE_SPEAKING_FEMALE_PRESENT",
	"FACT_HICKS_MARRIED_PLAYER_MERC",
	"FACT_MUSEUM_OPEN",
	"FACT_BROTHEL_OPEN",
	"FACT_CLUB_OPEN",
	"FACT_FIRST_BATTLE_FOUGHT",
	"FACT_FIRST_BATTLE_BEING_FOUGHT",
	"FACT_KINGPIN_INTRODUCED_SELF",
	"FACT_KINGPIN_NOT_IN_OFFICE",
	"FACT_DONT_OWE_KINGPIN_MONEY",
	"FACT_PC_MARRYING_DARYL_IS_FLO",
	"FACT_BLOODCAT_LAIR_CLEANED",
	"FACT_260",

	"FACT_NPC_COWERING",
	"FACT_262",
	"FACT_263",

	"FACT_TOP_AND_BOTTOM_LEVELS_CLEARED",
	"FACT_TOP_LEVEL_CLEARED",
	"FACT_BOTTOM_LEVEL_CLEARED",
	"FACT_NEED_TO_SAY_SOMETHING",
	"FACT_ATTACHED_ITEM_BEFORE",
	"FACT_SKYRIDER_EVER_ESCORTED",
	"FACT_NPC_NOT_UNDER_FIRE",
	"FACT_WILLIS_HEARD_ABOUT_JOEY_RESCUE",
	"FACT_WILLIS_GIVES_DISCOUNT",
	"ACT_HILLBILLIES_KILLED",
	"ACT_KEITH_OUT_OF_BUSINESS",
	"FACT_MIKE_AVAILABLE_TO_ARMY",
	"FACT_KINGPIN_CAN_SEND_ASSASSINS",
	"FACT_ESTONI_REFUELLING_POSSIBLE",
	"FACT_MUSEUM_ALARM_WENT_OFF",
	"FACT_CONVO_KYLE",
	"FACT_MADDOG_IS_SPEAKER",

	"FACT_VAN_HAUSSEN_EXPLAINED_BOXING_RULES",

	"FACT_ANGEL_MENTIONED_DEED",
	"FACT_IGGY_AVAILABLE_TO_ARMY",
	"FACT_PC_HAS_CONRADS_RECRUIT_OPINION",
	"FACT_CONVO_ANGEL_3",
	"FACT_CONVO_ANGEL_4",
	"FACT_CONVO_ANGEL_5",
	"FACT_288",

	"FACT_NPC_HOSTILE_OR_PISSED_OFF",
	"FACT_CONVO_KINGPIN_2",
	"FACT_TONY_IN_BUILDING",
	"FACT_SHANK_SPEAKING",
	"FACT_PABLO_ALIVE",
	"FACT_DOREEN_ALIVE",
	"FACT_WALDO_ALIVE",
	"FACT_PERKO_ALIVE",
	"FACT_TONY_ALIVE",
	"FACT_CONVO_TONY",
	"FACT_VINCE_ALIVE",
	"FACT_JENNY_ALIVE",
	"FACT_CONVO_SHANK_DYNAMO_BREWSTER",
	"FACT_CONVO_ARNOLD",
	"FACT_ARNOLD_ALIVE",
	"FACT_ROCKET_RIFLE_EXISTS",
	"FACT_24_HOURS_SINCE_JOEY_RESCUED",
	"FACT_24_HOURS_SINCE_DOCTOR_TALKED_TO",
	"FACT_OK_USE_ICECREAM",
	"FACT_KINGPIN_DEAD",
	"FACT_CONVO_KINGPIN_3",
	"FACT_CONVO_DOREEN_2",
	"FACT_CONVO_DOREEN_JOE",
	"FACT_CONVO_JOE_2",
	"FACT_313",

	"FACT_CONVO_JOE_3",
	"FACT_CONVO_JOE_4",
	"FACT_CONVO_JOE_5",
	"FACT_CONVO_JOE_6",
	"FACT_KIDS_ARE_FREE",
	"FACT_PLAYER_IN_SAME_ROOM",
	"FACT_CONVO_BRENDA_2",
	"FACT_CONVO_BRENDA_3",
	"FACT_CONVO_MARIA_2",
	"FACT_323",

	"FACT_PLAYER_IN_CONTROLLED_DRASSEN_MINE",
	"FACT_PLAYER_SPOKE_TO_CAMBRIA_MINER",
	"FACT_PLAYER_IN_CONTROLLED_CAMBRIA_MINE",
	"FACT_PLAYER_SPOKE_TO_CHITZENA_MINER",
	"FACT_PLAYER_IN_CONTROLLED_CHITZENA_MINE",
	"FACT_PLAYER_SPOKE_TO_ALMA_MINER",
	"FACT_PLAYER_IN_CONTROLLED_ALMA_MINE",
	"FACT_PLAYER_SPOKE_TO_GRUMM_MINER",
	"FACT_PLAYER_IN_CONTROLLED_GRUMM_MINE",
	"FACT_CONVO_GENERAL_MADLAB",
	"FACT_LARRY_CHANGED",
	"FACT_PLAYER_KNOWS_ABOUT_BLOODCAT_LAIR",
	"FACT_HOSPITAL_FREEBIE_DECISION_MADE",
	"FACT_ENOUGH_LOYALTY_TO_TRAIN_MILITIA",
	"FACT_WALKER_AT_BAR",
	"FACT_CONVO_DAVE_3",
	"FACT_JOEY_ALIVE",
	"FACT_UNPROPOSITIONED_FEMALE_SPEAKING_TO_NPC",
	"FACT_84_AND_85_TRUE",
	"FACT_CONVO_DAVE_4",
	"FACT_344",

	"FACT_CONVO_JOHN_2 = 345",
	"FACT_CONVO_SKYRIDER_3",
	"FACT_CONVO_JOEY_2",
	"FACT_CONVO_IGGY",
	"FACT_349",

	"FACT_KINGPIN_WILL_LEARN_OF_MONEY_GONE",
	"FACT_CONVO_MADDOG_2",
	"FACT_CONVO_MADLAB_4",
	"FACT_CONVO_MADLAB_5",
	"FACT_SKYRIDER_IN_B15",
	"FACT_SKYRIDER_IN_C16",
	"FACT_SKYRIDER_IN_E14",
	"FACT_SKYRIDER_IN_D12",
	"FACT_SKYRIDER_HINT_GIVEN",
	"FACT_KINGPIN_IS_ENEMY",
	"FACT_BRENDA_PATIENCE_TIMER_EXPIRED",
	"FACT_CONVO_MATT",
	"FACT_DYNAMO_NOT_SPEAKER",
	"FACT_363",
	"FACT_364",

	"FACT_PABLO_BRIBED",
	"FACT_CONVO_PABLO_2",
	"FACT_CONRAD_SHOULD_GO",
	"FACT_PLAYER_KILLED_BOXERS",
	"FACT_MOBILE_RESTRICTIONS_VIEWED",

	"FACT_HELI_DAMAGED_CAN_START_REPAIR",
	"FACT_HELI_SERIOUSLY_DAMAGED_CAN_START_REPAIR",

	"FACT_GIVEN_ENOUGH_TO_REPAIR_HELI",
	"FACT_GIVEN_ENOUGH_TO_SERIOUSLY_REPAIR_HELI",

	"FACT_HELI_GIVEN_MONEY_CAN_START_REPAIR",
	"FACT_HELI_CANT_START_REPAIR",

	"FACT_WALDO_MET",
};

void SetFactTrue( UINT16 usFact )
{
	if (usFact <= FACT_WALDO_MET)
		DebugQuestInfo(String("%s %d true", szFact[usFact], usFact));
	else if (usFact == FACT_GLOBAL_OFFENSIVE_1_ORDERED)
		DebugQuestInfo(String("FACT_GLOBAL_OFFENSIVE_1_ORDERED true"));
	else if (usFact == FACT_GLOBAL_OFFENSIVE_2_ORDERED)
		DebugQuestInfo(String("FACT_GLOBAL_OFFENSIVE_2_ORDERED true"));
	else if (usFact == FACT_CONVO_MANUEL)
		DebugQuestInfo(String("FACT_CONVO_MANUEL true"));
	else
		DebugQuestInfo(String("fact %d true", usFact));

	// This function is here just for control flow purposes (debug breakpoints)
	// and code is more readable that way

	// must intercept when Jake is first triggered to start selling fuel
	if ( ( usFact == FACT_ESTONI_REFUELLING_POSSIBLE ) && ( CheckFact( usFact, 0 ) == FALSE ) )
	{
		// give him some gas...
		GuaranteeAtLeastXItemsOfIndex( ARMS_DEALER_JAKE, GAS_CAN, ( UINT8 ) ( 4 + Random( 3 ) ) );

		DebugQuestInfo(String("guarantee X items of %d (GAS_CAN) for dealer %d (ARMS_DEALER_JAKE)", GAS_CAN, ARMS_DEALER_JAKE));
	}

	gubFact[usFact] = TRUE;	
}

void SetFactFalse( UINT16 usFact )
{
	if (usFact <= FACT_WALDO_MET)
		DebugQuestInfo(String("%s %d false", szFact[usFact], usFact));
	else if (usFact == FACT_GLOBAL_OFFENSIVE_1_ORDERED)
		DebugQuestInfo(String("FACT_GLOBAL_OFFENSIVE_1_ORDERED false"));
	else if (usFact == FACT_GLOBAL_OFFENSIVE_2_ORDERED)
		DebugQuestInfo(String("FACT_GLOBAL_OFFENSIVE_2_ORDERED false"));
	else if (usFact == FACT_CONVO_MANUEL)
		DebugQuestInfo(String("FACT_CONVO_MANUEL false"));
	else
		DebugQuestInfo(String("fact %d false", usFact));

	gubFact[usFact] = FALSE;
}

BOOLEAN CheckForNewShipment( void )
{
	ITEM_POOL *			pItemPool;

	if ( ( gWorldSectorX == BOBBYR_SHIPPING_DEST_SECTOR_X ) && ( gWorldSectorY == BOBBYR_SHIPPING_DEST_SECTOR_Y ) && ( gbWorldSectorZ == BOBBYR_SHIPPING_DEST_SECTOR_Z ) )
	{
		if ( GetItemPoolFromGround( BOBBYR_SHIPPING_DEST_GRIDNO, &pItemPool ) )
		{
			return( !( ITEMPOOL_VISIBLE( pItemPool ) ) );
		}
	}
	return( FALSE );
}

BOOLEAN CheckNPCWounded( UINT8 ubProfileID, BOOLEAN fByPlayerOnly )
{
	SOLDIERTYPE * pSoldier;

	// is the NPC is wounded at all?
	pSoldier = FindSoldierByProfileID( ubProfileID, FALSE );
	if (pSoldier && pSoldier->stats.bLife < pSoldier->stats.bLifeMax)
	{
		if (fByPlayerOnly)
		{
			if (gMercProfiles[ubProfileID].ubMiscFlags & PROFILE_MISC_FLAG_WOUNDEDBYPLAYER)
			{
				return( TRUE );
			}
			else
			{
				return( FALSE );
			}
		}
		else
		{
			return( TRUE );
		}
	}
	else
	{
		return( FALSE );

	}
}

BOOLEAN CheckNPCInOkayHealth( UINT8 ubProfileID )
{
	SOLDIERTYPE * pSoldier;

	// is the NPC at better than half health?
	pSoldier = FindSoldierByProfileID( ubProfileID, FALSE );
	if (pSoldier && pSoldier->stats.bLife > (pSoldier->stats.bLifeMax / 2) && pSoldier->stats.bLife > 30)
	{
		return( TRUE );
	}
	else
	{
		return( FALSE );
	}
}

BOOLEAN CheckNPCBleeding( UINT8 ubProfileID )
{
	SOLDIERTYPE * pSoldier;

	// the NPC is wounded...
	pSoldier = FindSoldierByProfileID( ubProfileID, FALSE );
	if (pSoldier && pSoldier->stats.bLife > 0 && pSoldier->bBleeding > 0)
	{
		return( TRUE );
	}
	else
	{
		return( FALSE );
	}

}

BOOLEAN CheckNPCWithin( UINT8 ubFirstNPC, UINT8 ubSecondNPC, UINT8 ubMaxDistance )
{
	SOLDIERTYPE * pFirstNPC, * pSecondNPC;

	pFirstNPC = FindSoldierByProfileID( ubFirstNPC, FALSE );
	pSecondNPC = FindSoldierByProfileID( ubSecondNPC, FALSE );
	if (!pFirstNPC || !pSecondNPC)
	{
		return( FALSE );
	}
	return( PythSpacesAway( pFirstNPC->sGridNo, pSecondNPC->sGridNo ) <= ubMaxDistance );
}

BOOLEAN CheckGuyVisible( UINT8 ubNPC, UINT8 ubGuy )
{
	// NB ONLY WORKS IF ON DIFFERENT TEAMS
	SOLDIERTYPE * pNPC, * pGuy;

	pNPC = FindSoldierByProfileID( ubNPC, FALSE );
	pGuy = FindSoldierByProfileID( ubGuy, FALSE );
	if (!pNPC || !pGuy)
	{
		return( FALSE );
	}
	if (pNPC->aiData.bOppList[ pGuy->ubID ] == SEEN_CURRENTLY )
	{
		return( TRUE );
	}
	else
	{
		return( FALSE );
	}
}

BOOLEAN CheckNPCAt( UINT8 ubNPC, INT32 sGridNo )
{
	SOLDIERTYPE * pNPC;

	pNPC = FindSoldierByProfileID( ubNPC, FALSE );
	if (!pNPC)
	{
		return( FALSE );
	}
	return( pNPC->sGridNo == sGridNo );
}

BOOLEAN CheckNPCIsEnemy( UINT8 ubProfileID )
{
	SOLDIERTYPE * pNPC;

	pNPC = FindSoldierByProfileID( ubProfileID, FALSE );
	if (!pNPC)
	{
		return( FALSE );
	}
	if (pNPC->bSide == gbPlayerNum || pNPC->aiData.bNeutral)
	{
		if (pNPC->ubCivilianGroup != NON_CIV_GROUP)
		{
			// although the soldier is NOW the same side, this civ group could be set to "will become hostile"
			return( gTacticalStatus.fCivGroupHostile[ pNPC->ubCivilianGroup ] >= CIV_GROUP_WILL_BECOME_HOSTILE );
		}
		else
		{
			return( FALSE );
		}
	}
	else
	{
		return( TRUE );
	}
}

BOOLEAN CheckIfMercIsNearNPC( SOLDIERTYPE *pMerc, UINT8 ubProfileId )
{
	SOLDIERTYPE *		pNPC;
	INT32 sGridNo;

	// no merc nearby?
	if ( pMerc == NULL )
	{
		return( FALSE );
	}

	pNPC = FindSoldierByProfileID( ubProfileId, FALSE );
	if ( pNPC == NULL )
	{
		return( FALSE );
	}
	sGridNo = pNPC->sGridNo;

	// is the merc and NPC close enough?
	if (PythSpacesAway( sGridNo, pMerc->sGridNo ) <= 9)
	{
		return( TRUE );
	}

	return( FALSE );
}


INT8 NumWoundedMercsNearby( UINT8 ubProfileID )
{
	INT8						bNumber = 0;
	UINT32					uiLoop;
	SOLDIERTYPE *		pNPC;
	SOLDIERTYPE *		pSoldier;
	INT32 sGridNo;

	pNPC = FindSoldierByProfileID( ubProfileID, FALSE );
	if (!pNPC)
	{
		return( FALSE );
	}
	sGridNo = pNPC->sGridNo;

	for ( uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pSoldier = MercSlots[ uiLoop ];

		if (pSoldier && pSoldier->bTeam == gbPlayerNum && pSoldier->stats.bLife > 0 && 
			(pSoldier->stats.bLife < pSoldier->stats.bLifeMax || NumberOfDamagedStats(pSoldier) > 0) && pSoldier->bAssignment != ASSIGNMENT_HOSPITAL &&
			pSoldier->sSectorX == pNPC->sSectorX && pSoldier->sSectorY == pNPC->sSectorY && pSoldier->bSectorZ == pNPC->bSectorZ)
		{
			if (PythSpacesAway( sGridNo, pSoldier->sGridNo ) <= HOSPITAL_PATIENT_DISTANCE)
			{
				bNumber++;
			}
		}
	}

	return( bNumber );
}

INT8 NumMercsNear( UINT8 ubProfileID, UINT8 ubMaxDist )
{
	INT8						bNumber = 0;
	UINT32					uiLoop;
	SOLDIERTYPE *		pNPC;
	SOLDIERTYPE *		pSoldier;
	INT32 sGridNo;

	pNPC = FindSoldierByProfileID( ubProfileID, FALSE );
	if (!pNPC)
	{
		return( FALSE );
	}
	sGridNo = pNPC->sGridNo;

	for ( uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pSoldier = MercSlots[ uiLoop ];

		if ( pSoldier && pSoldier->bTeam == gbPlayerNum && pSoldier->stats.bLife >= OKLIFE )
		{
			if (PythSpacesAway( sGridNo, pSoldier->sGridNo ) <= ubMaxDist)
			{
				bNumber++;
			}
		}
	}

	return( bNumber );
}

BOOLEAN CheckNPCIsEPC( UINT8 ubProfileID )
{
	SOLDIERTYPE *		pNPC;

	if ( gMercProfiles[ ubProfileID ].bMercStatus == MERC_IS_DEAD )
	{
		return( FALSE );
	}

	pNPC = FindSoldierByProfileID( ubProfileID, TRUE );
	if (!pNPC)
	{
		return( FALSE );
	}
	return( (pNPC->ubWhatKindOfMercAmI == MERC_TYPE__EPC ) );
}

BOOLEAN CheckNPCIsRPC( UINT8 ubProfileID )
{
	SOLDIERTYPE *		pNPC;

	if ( gMercProfiles[ ubProfileID ].bMercStatus == MERC_IS_DEAD )
	{
		return( FALSE );
	}

	pNPC = FindSoldierByProfileID( ubProfileID, TRUE );
	if (!pNPC)
	{
		return( FALSE );
	}
	return( (pNPC->ubWhatKindOfMercAmI == MERC_TYPE__NPC ) );
}
//DBrot: More Rooms
BOOLEAN NPCInRoom( UINT8 ubProfileID, UINT16 usRoomID )
{
	SOLDIERTYPE *		pNPC;

	pNPC = FindSoldierByProfileID( ubProfileID, FALSE );
	if ( !pNPC || (gusWorldRoomInfo[ pNPC->sGridNo ] != usRoomID) )
	{
		return( FALSE );
	}
	return( TRUE );
}
//DBrot: More Rooms
BOOLEAN NPCInRoomRange( UINT8 ubProfileID, UINT16 usRoomID1, UINT16 usRoomID2 )
{
	SOLDIERTYPE *		pNPC;

	pNPC = FindSoldierByProfileID( ubProfileID, FALSE );
	if ( !pNPC || (gusWorldRoomInfo[ pNPC->sGridNo ] < usRoomID1) || (gusWorldRoomInfo[ pNPC->sGridNo ] > usRoomID2) )
	{
		return( FALSE );
	}
	return( TRUE );
}

BOOLEAN PCInSameRoom( UINT8 ubProfileID )
{
	SOLDIERTYPE *		pNPC;
	//DBrot: More Rooms
	//UINT8						ubRoom;
	UINT16 usRoom;
	INT8		bLoop;
	SOLDIERTYPE * pSoldier;

	pNPC = FindSoldierByProfileID( ubProfileID, FALSE );
	if ( !pNPC )
	{
		return( FALSE );
	}
	usRoom = gusWorldRoomInfo[ pNPC->sGridNo ];

	for ( bLoop = gTacticalStatus.Team[ gbPlayerNum ].bFirstID; bLoop <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; bLoop++ )
	{
		pSoldier = MercPtrs[ bLoop ];
		if ( pSoldier && pSoldier->bActive && pSoldier->bInSector )
		{
			if ( gusWorldRoomInfo[ pSoldier->sGridNo ] == usRoom )
			{
				return( TRUE );
			}
		}
	}

	return( FALSE );
}


BOOLEAN CheckTalkerStrong( void )
{
	if (gpSrcSoldier && gpSrcSoldier->bTeam == gbPlayerNum)
	{
		return( gpSrcSoldier->stats.bStrength >= 84 );
	}
	else if (gpDestSoldier && gpDestSoldier->bTeam == gbPlayerNum)
	{
		return( gpDestSoldier->stats.bStrength >= 84 );
	}
	return( FALSE );
}

BOOLEAN CheckTalkerFemale( void )
{
	if (gpSrcSoldier && gpSrcSoldier->bTeam == gbPlayerNum && gpSrcSoldier->ubProfile != NO_PROFILE)
	{
		return( gMercProfiles[ gpSrcSoldier->ubProfile ].bSex == FEMALE );
	}
	else if (gpDestSoldier && gpDestSoldier->bTeam == gbPlayerNum && gpDestSoldier->ubProfile != NO_PROFILE)
	{
		return( gMercProfiles[ gpDestSoldier->ubProfile ].bSex == FEMALE );
	}
	return( FALSE );
}

BOOLEAN CheckTalkerUnpropositionedFemale( void )
{
	if (gpSrcSoldier && gpSrcSoldier->bTeam == gbPlayerNum && gpSrcSoldier->ubProfile != NO_PROFILE)
	{
		if ( !(gMercProfiles[ gpSrcSoldier->ubProfile ].ubMiscFlags2 & PROFILE_MISC_FLAG2_ASKED_BY_HICKS) )
		{
			return( gMercProfiles[ gpSrcSoldier->ubProfile ].bSex == FEMALE );
		}
	}
	else if (gpDestSoldier && gpDestSoldier->bTeam == gbPlayerNum && gpDestSoldier->ubProfile != NO_PROFILE)
	{
		if ( !(gMercProfiles[ gpDestSoldier->ubProfile ].ubMiscFlags2 & PROFILE_MISC_FLAG2_ASKED_BY_HICKS) )
		{
			return( gMercProfiles[ gpDestSoldier->ubProfile ].bSex == FEMALE );
		}
	}
	return( FALSE );
}

INT8 NumMalesPresent( UINT8 ubProfileID )
{
	INT8						bNumber = 0;
	UINT32					uiLoop;
	SOLDIERTYPE *		pNPC;
	SOLDIERTYPE *		pSoldier;
	INT32 sGridNo;

	pNPC = FindSoldierByProfileID( ubProfileID, FALSE );
	if (!pNPC)
	{
		return( FALSE );
	}
	sGridNo = pNPC->sGridNo;

	for ( uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pSoldier = MercSlots[ uiLoop ];

		if ( pSoldier && pSoldier->bTeam == gbPlayerNum && pSoldier->stats.bLife >= OKLIFE)
		{
			if ( pSoldier->ubProfile != NO_PROFILE && gMercProfiles[ pSoldier->ubProfile].bSex == MALE )
			{
				if (PythSpacesAway( sGridNo, pSoldier->sGridNo ) <= 8)
				{
					bNumber++;
				}
			}
		}
	}

	return( bNumber );
}


BOOLEAN FemalePresent( UINT8 ubProfileID )
{
	UINT32					uiLoop;
	SOLDIERTYPE *		pNPC;
	SOLDIERTYPE *		pSoldier;
	INT32 sGridNo;

	pNPC = FindSoldierByProfileID( ubProfileID, FALSE );
	if (!pNPC)
	{
		return( FALSE );
	}
	sGridNo = pNPC->sGridNo;

	for ( uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pSoldier = MercSlots[ uiLoop ];

		if ( pSoldier && pSoldier->bTeam == gbPlayerNum && pSoldier->stats.bLife >= OKLIFE)
		{
			if ( pSoldier->ubProfile != NO_PROFILE && gMercProfiles[ pSoldier->ubProfile].bSex == FEMALE )
			{
				if (PythSpacesAway( sGridNo, pSoldier->sGridNo ) <= 10)
				{
					return( TRUE );
				}
			}
		}
	}

	return( FALSE );
}


BOOLEAN CheckPlayerHasHead( void )
{
	INT8						bLoop;
	SOLDIERTYPE *		pSoldier;

	for ( bLoop = gTacticalStatus.Team[ gbPlayerNum ].bFirstID; bLoop <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; bLoop++ )
	{
		pSoldier = MercPtrs[ bLoop ];

		if ( pSoldier->bActive && pSoldier->stats.bLife > 0 )
		{
			if ( FindObjInObjRange( pSoldier, HEAD_2, HEAD_7 ) != NO_SLOT )
			{
				return( TRUE );
			}
		}
	}

	return( FALSE );

}

BOOLEAN CheckNPCSector( UINT8 ubProfileID, INT16 sSectorX, INT16 sSectorY, INT8 bSectorZ )
{
	SOLDIERTYPE * pSoldier;

	pSoldier = FindSoldierByProfileID( ubProfileID, TRUE );

	if( pSoldier )
	{
		if (pSoldier->sSectorX == sSectorX &&
			pSoldier->sSectorY == sSectorY &&
			pSoldier->bSectorZ == bSectorZ )
		{
			return( TRUE );
		}
	}
	else if (gMercProfiles[ubProfileID].sSectorX == sSectorX &&
		gMercProfiles[ubProfileID].sSectorY == sSectorY &&
		gMercProfiles[ubProfileID].bSectorZ == bSectorZ )
	{
		return( TRUE );
	}

	return( FALSE );

}

BOOLEAN AIMMercWithin( INT32 sGridNo, INT16 sDistance )
{
	UINT32					uiLoop;
	SOLDIERTYPE *		pSoldier;

	for ( uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pSoldier = MercSlots[ uiLoop ];

		if ( pSoldier && (pSoldier->bTeam == gbPlayerNum) && (pSoldier->stats.bLife >= OKLIFE) && ( pSoldier->ubWhatKindOfMercAmI == MERC_TYPE__AIM_MERC ) )
		{
			if (PythSpacesAway( sGridNo, pSoldier->sGridNo ) <= sDistance)
			{
				return( TRUE );
			}
		}
	}

	return( FALSE );
}

BOOLEAN CheckNPCCowering( UINT8 ubProfileID )
{
	SOLDIERTYPE *		pNPC;

	pNPC = FindSoldierByProfileID( ubProfileID, FALSE );
	if ( !pNPC )
	{
		return( FALSE );
	}
	return( ( (pNPC->flags.uiStatusFlags & SOLDIER_COWERING) != 0) );
}

UINT8 CountBartenders( void )
{
	UINT8		ubLoop;
	UINT8		ubBartenders = 0;

	for( ubLoop = HERVE; ubLoop <= CARLO; ubLoop++ )
	{
		if (gMercProfiles[ ubLoop ].bNPCData != 0 )
		{
			ubBartenders++;
		}
	}
	return( ubBartenders );
}

BOOLEAN CheckNPCIsUnderFire( UINT8 ubProfileID )
{
	SOLDIERTYPE *		pNPC;

	pNPC = FindSoldierByProfileID( ubProfileID, FALSE );
	if ( !pNPC )
	{
		return( FALSE );
	}
	return( pNPC->aiData.bUnderFire != 0 );
}

BOOLEAN NPCHeardShot( UINT8 ubProfileID )
{
	SOLDIERTYPE *		pNPC;

	pNPC = FindSoldierByProfileID( ubProfileID, FALSE );
	if ( !pNPC )
	{
		return( FALSE );
	}
	return( pNPC->ubMiscSoldierFlags & SOLDIER_MISC_HEARD_GUNSHOT );
}

BOOLEAN InTownSectorWithTrainingLoyalty( INT16 sSectorX, INT16 sSectorY )
{
	UINT8	ubTown;
	INT32 iMinLoyaltyToTrain = gGameExternalOptions.iMinLoyaltyToTrain;
DebugMsg (TOPIC_JA2,DBG_LEVEL_3,"Quests");

	ubTown = GetTownIdForSector( sSectorX, sSectorY );
	if ( ( ubTown != BLANK_SECTOR ) && gTownLoyalty[ ubTown ].fStarted && gfTownUsesLoyalty[ ubTown ])
	{
		return( gTownLoyalty[ ubTown ].ubRating >= iMinLoyaltyToTrain );
	}
	else
	{
		return( FALSE );
	}
}

BOOLEAN CheckFact( UINT16 usFact, UINT8 ubProfileID )
{


//	LuaCheckFact ( usFact, ubProfileID, 0 );

//#if 0
	INT8 bTown = -1;


	switch( usFact )
	{
#ifdef JA2UB
//Ja25 No dimitri
#else
		case FACT_DIMITRI_DEAD:
			gubFact[ usFact ] = (gMercProfiles[ DIMITRI ].bMercStatus == MERC_IS_DEAD );
			break;
#endif
		case FACT_CURRENT_SECTOR_IS_SAFE:
			gubFact[FACT_CURRENT_SECTOR_IS_SAFE] = !( ( (gTacticalStatus.fEnemyInSector && NPCHeardShot( ubProfileID ) ) || gTacticalStatus.uiFlags & INCOMBAT ) );
			break;
		case FACT_BOBBYRAY_SHIPMENT_IN_TRANSIT:
		case FACT_NEW_BOBBYRAY_SHIPMENT_WAITING:
			if ( gubFact[FACT_PABLO_PUNISHED_BY_PLAYER] == TRUE && gubFact[FACT_PABLO_RETURNED_GOODS] == FALSE && gMercProfiles[ PABLO ].bMercStatus != MERC_IS_DEAD )
			{
				gubFact[FACT_BOBBYRAY_SHIPMENT_IN_TRANSIT] = FALSE;
				gubFact[FACT_NEW_BOBBYRAY_SHIPMENT_WAITING] = FALSE;
			}
			else
			{
				if (CheckForNewShipment()) // if new stuff waiting unseen in Drassen
				{
					gubFact[FACT_BOBBYRAY_SHIPMENT_IN_TRANSIT] = FALSE;
					gubFact[FACT_NEW_BOBBYRAY_SHIPMENT_WAITING] = TRUE;
				}
				else if ( CountNumberOfBobbyPurchasesThatAreInTransit() > 0) // if stuff in transit
				{
					if ( gubFact[ FACT_PACKAGE_DAMAGED ] == TRUE )
					{
						gubFact[FACT_BOBBYRAY_SHIPMENT_IN_TRANSIT] = FALSE;
					}
					else
					{
						gubFact[FACT_BOBBYRAY_SHIPMENT_IN_TRANSIT] = TRUE;
					}
					gubFact[FACT_NEW_BOBBYRAY_SHIPMENT_WAITING] = FALSE;
				}
				else
				{
					gubFact[FACT_BOBBYRAY_SHIPMENT_IN_TRANSIT] = FALSE;
					gubFact[FACT_NEW_BOBBYRAY_SHIPMENT_WAITING] = FALSE;
				}
			}
			break;
		case FACT_NPC_WOUNDED:
			gubFact[FACT_NPC_WOUNDED] = CheckNPCWounded( ubProfileID, FALSE );
			break;
		case FACT_NPC_WOUNDED_BY_PLAYER:
			gubFact[FACT_NPC_WOUNDED_BY_PLAYER] = CheckNPCWounded( ubProfileID, TRUE );
			break;
#ifdef JA2UB
// no ja25 UB
#else
		case FACT_IRA_NOT_PRESENT:
			gubFact[FACT_IRA_NOT_PRESENT] = !CheckNPCWithin( ubProfileID, IRA, 10 );
			break;
		case FACT_IRA_TALKING:
			gubFact[FACT_IRA_TALKING] = ( gubSrcSoldierProfile == IRA );
			break;
		case FACT_IRA_UNHIRED_AND_ALIVE:
			if ( gMercProfiles[ IRA ].bMercStatus != MERC_IS_DEAD && CheckNPCSector( IRA, 10, 1, 1) && !(gMercProfiles[IRA].ubMiscFlags & PROFILE_MISC_FLAG_RECRUITED) )
			{
				gubFact[FACT_IRA_UNHIRED_AND_ALIVE] = TRUE;
			}
			else
			{
				gubFact[FACT_IRA_UNHIRED_AND_ALIVE] = FALSE;
			}
			break;
#endif
		case FACT_NPC_BLEEDING:
			gubFact[FACT_NPC_BLEEDING] = CheckNPCBleeding( ubProfileID );
			break;
		case FACT_NPC_BLEEDING_BUT_OKAY:
			if ( CheckNPCBleeding( ubProfileID ) && CheckNPCInOkayHealth( ubProfileID ) )
			{
				gubFact[FACT_NPC_BLEEDING_BUT_OKAY] = TRUE;
			}
			else
			{
				gubFact[FACT_NPC_BLEEDING_BUT_OKAY] = FALSE;
			}
			break;
#ifdef JA2UB
//Ja25: NO Carmen
#else
		case FACT_PLAYER_HAS_HEAD_AND_CARMEN_IN_SAN_MONA:
			gubFact[usFact] = (CheckNPCSector( CARMEN, 5, MAP_ROW_C, 0 ) && CheckPlayerHasHead() );
			break;

		case FACT_PLAYER_HAS_HEAD_AND_CARMEN_IN_CAMBRIA:
			gubFact[usFact] = (CheckNPCSector( CARMEN, 9, MAP_ROW_G, 0 ) && CheckPlayerHasHead() );
			break;

		case FACT_PLAYER_HAS_HEAD_AND_CARMEN_IN_DRASSEN:
			gubFact[usFact] = (CheckNPCSector( CARMEN, 13, MAP_ROW_C, 0 ) && CheckPlayerHasHead() );
			break;
#endif
		case FACT_NPC_OWED_MONEY:
			gubFact[FACT_NPC_OWED_MONEY] = (ubProfileID < NUM_PROFILES && ubProfileID != NO_PROFILE) && gMercProfiles[ubProfileID].iBalance < 0;
			break;

		case FACT_FATHER_DRUNK:
			gubFact[FACT_FATHER_DRUNK] = ( gMercProfiles[ FATHER ].bNPCData >= 5 );
			break;

		case FACT_MICKY_DRUNK:
			gubFact[FACT_MICKY_DRUNK] = ( gMercProfiles[ MICKY ].bNPCData >= 5 );
			break;

		case FACT_BRENDA_IN_STORE_AND_ALIVE:
			// ensure alive
			if ( gMercProfiles[ BRENDA ].bMercStatus == MERC_IS_DEAD)
			{
				gubFact[FACT_BRENDA_IN_STORE_AND_ALIVE] = FALSE;
			}
			// ensure in a building and nearby
			else if ( !(NPCInRoom( BRENDA, gModSettings.usPornShopRoomBrenda ) )	)
			{
				gubFact[FACT_BRENDA_IN_STORE_AND_ALIVE] = FALSE;
			}
			else
			{
				gubFact[FACT_BRENDA_IN_STORE_AND_ALIVE] = CheckNPCWithin( ubProfileID, BRENDA, 12 );
			}
			break;
		case FACT_BRENDA_DEAD:
			gubFact[FACT_BRENDA_DEAD] = (gMercProfiles[ BRENDA ].bMercStatus == MERC_IS_DEAD);
			break;
		case FACT_NPC_IS_ENEMY:
			gubFact[FACT_NPC_IS_ENEMY] = (ubProfileID != NO_PROFILE && ubProfileID < NUM_PROFILES) && 
										(CheckNPCIsEnemy(ubProfileID) || gMercProfiles[ubProfileID].ubMiscFlags2 & PROFILE_MISC_FLAG2_NEEDS_TO_SAY_HOSTILE_QUOTE);
			break;
			/*
		case FACT_SKYRIDER_CLOSE_TO_CHOPPER:
			SetUpHelicopterForPlayer( 13, MAP_ROW_B, gNewVehicle[ HELICOPTER ].NewPilot, HELICOPTER );
			break;
			*/
		case FACT_SPIKE_AT_DOOR:
			gubFact[FACT_SPIKE_AT_DOOR] = CheckNPCAt( SPIKE, 9817 );
			break;
		case FACT_WOUNDED_MERCS_NEARBY:
			gubFact[usFact] = (NumWoundedMercsNearby( ubProfileID ) > 0);
			break;
		case FACT_ONE_WOUNDED_MERC_NEARBY:
			gubFact[usFact] = (NumWoundedMercsNearby( ubProfileID ) == 1);
			break;
		case FACT_MULTIPLE_WOUNDED_MERCS_NEARBY:
			gubFact[usFact] = (NumWoundedMercsNearby( ubProfileID ) > 1);
			break;
		case FACT_HANS_AT_SPOT:
			gubFact[usFact] = CheckNPCAt( HANS, gModSettings.iHansGridNo );
			break;
		case FACT_MULTIPLE_MERCS_CLOSE:
			gubFact[usFact] = ( NumMercsNear( ubProfileID, 3 ) > 1 );
			break;
		case FACT_SOME_MERCS_CLOSE:
			gubFact[usFact] = ( NumMercsNear( ubProfileID, 3 ) > 0 );
			break;
		case FACT_MARIA_ESCORTED:
			gubFact[usFact] = CheckNPCIsEPC( MARIA );
			break;
		case FACT_JOEY_ESCORTED:
			gubFact[usFact] = CheckNPCIsEPC( JOEY );
			break;
		case FACT_ESCORTING_SKYRIDER:
			gubFact[usFact] = CheckNPCIsEPC( SKYRIDER );
			break;
		case FACT_MARIA_ESCORTED_AT_LEATHER_SHOP:
			gubFact[usFact] = ( CheckNPCIsEPC( MARIA ) && (NPCInRoom( MARIA, gModSettings.usLeatherShop )) );
			break;
		case FACT_PC_STRONG_AND_LESS_THAN_3_MALES_PRESENT:
			gubFact[usFact] = ( CheckTalkerStrong() && (NumMalesPresent( ubProfileID ) < 3) );
			break;
		case FACT_PC_STRONG_AND_3_PLUS_MALES_PRESENT:
			gubFact[usFact] = ( CheckTalkerStrong() && (NumMalesPresent( ubProfileID ) >= 3) );
			break;
		case FACT_FEMALE_SPEAKING_TO_NPC:
			gubFact[usFact] = CheckTalkerFemale();
			break;
		case FACT_CARMEN_IN_C5:
			gubFact[usFact] = CheckNPCSector( CARMEN, 5, MAP_ROW_C, 0 );
			break;
		case FACT_JOEY_IN_C5:
			gubFact[usFact] = CheckNPCSector( JOEY, 5, MAP_ROW_C, 0 );
			break;
		case FACT_JOEY_NEAR_MARTHA:
			gubFact[usFact] = CheckNPCWithin( JOEY, MARTHA, 5 ) && ( CheckGuyVisible( MARTHA, JOEY ) || CheckGuyVisible( JOEY, MARTHA ) );
			break;
		case FACT_JOEY_DEAD:
			gubFact[usFact] = gMercProfiles[ JOEY ].bMercStatus == MERC_IS_DEAD;
			break;
		case FACT_MERC_NEAR_MARTHA:
			gubFact[usFact] = ( NumMercsNear( ubProfileID, 5 ) > 0 );
			break;
		case FACT_REBELS_HATE_PLAYER:
			gubFact[usFact] = (gTacticalStatus.fCivGroupHostile[ REBEL_CIV_GROUP ] == CIV_GROUP_HOSTILE);
			break;
		case FACT_CURRENT_SECTOR_G9:
			gubFact[usFact] = ( gWorldSectorX == 9 && gWorldSectorY == MAP_ROW_G && gbWorldSectorZ == 0 );
			break;
		case FACT_CURRENT_SECTOR_C5:
			gubFact[usFact] = ( gWorldSectorX == 5 && gWorldSectorY == MAP_ROW_C && gbWorldSectorZ == 0 );
			break;
		case FACT_CURRENT_SECTOR_C13:
			gubFact[usFact] = ( gWorldSectorX == 13 && gWorldSectorY == MAP_ROW_C && gbWorldSectorZ == 0 );
			break;
		case FACT_CARMEN_HAS_TEN_THOUSAND:
			gubFact[usFact] = ( gMercProfiles[ CARMEN ].uiMoney >= 10000 );
			break;
		case FACT_SLAY_IN_SECTOR:
			gubFact[usFact] = (gMercProfiles[ SLAY ].sSectorX == gWorldSectorX && gMercProfiles[ SLAY ].sSectorY == gWorldSectorY && gMercProfiles[ SLAY ].bSectorZ == gbWorldSectorZ );
			break;
		case FACT_SLAY_HIRED_AND_WORKED_FOR_48_HOURS:
			gubFact[usFact] = ( ( gMercProfiles[ SLAY ].ubMiscFlags & PROFILE_MISC_FLAG_RECRUITED ) && ( gMercProfiles[ SLAY ].usTotalDaysServed > 1 ) );
			break;
		case FACT_SHANK_IN_SQUAD_BUT_NOT_SPEAKING:
			gubFact[usFact] = ( ( FindSoldierByProfileID( SHANK, TRUE ) != NULL) && ( gMercProfiles[ SHANK ].ubMiscFlags & PROFILE_MISC_FLAG_RECRUITED ) && ( gpSrcSoldier == NULL || gpSrcSoldier->ubProfile != SHANK ) );
			break;
		case FACT_SHANK_NOT_IN_SECTOR:
			gubFact[usFact] = ( FindSoldierByProfileID( SHANK, FALSE ) == NULL );
			break;
#ifdef JA2UB
//Ja25 No queen
#else
		case FACT_QUEEN_DEAD:
			gubFact[usFact] = (gMercProfiles[ QUEEN ].bMercStatus == MERC_IS_DEAD);
			break;
#endif
		case FACT_MINE_EMPTY:
			gubFact[usFact] = IsHisMineEmpty( ubProfileID );
			break;
		case FACT_MINE_RUNNING_OUT:
			gubFact[usFact] = IsHisMineRunningOut( ubProfileID );
			break;
		case FACT_MINE_PRODUCING_BUT_LOYALTY_LOW:
			gubFact[usFact] = HasHisMineBeenProducingForPlayerForSomeTime( ubProfileID ) && IsHisMineDisloyal( ubProfileID );
			break;
		case FACT_CREATURES_IN_MINE:
			gubFact[usFact] = IsHisMineInfested( ubProfileID );
			break;
		case FACT_PLAYER_LOST_MINE:
			gubFact[usFact] = IsHisMineLostAndRegained( ubProfileID );
			break;
		case FACT_MINE_AT_FULL_PRODUCTION:
			gubFact[usFact] = IsHisMineAtMaxProduction( ubProfileID );
			break;
		case FACT_DYNAMO_IN_J9:
			gubFact[usFact] = CheckNPCSector( DYNAMO, gModSettings.ubDyanmoCaptiveSectorX, gModSettings.ubDyanmoCaptiveSectorY, gModSettings.ubDyanmoCaptiveSectorZ ) &&
				NumEnemiesInAnySector( gModSettings.ubDyanmoCaptiveSectorX, gModSettings.ubDyanmoCaptiveSectorY, gModSettings.ubDyanmoCaptiveSectorZ ); //( 9, 10, 0 )
			break;
		case FACT_DYNAMO_ALIVE:
			gubFact[usFact] = ( gMercProfiles[ DYNAMO ].bMercStatus != MERC_IS_DEAD );
			break;
		case FACT_DYNAMO_SPEAKING_OR_NEARBY:
			gubFact[usFact] = ( gpSrcSoldier != NULL && (gpSrcSoldier->ubProfile == DYNAMO || ( CheckNPCWithin( gpSrcSoldier->ubProfile, DYNAMO, 10 ) && CheckGuyVisible( gpSrcSoldier->ubProfile, DYNAMO ) ) ) );
			break;
		case FACT_JOHN_EPC:
			gubFact[usFact] = CheckNPCIsEPC( JOHN );
			break;
		case FACT_MARY_EPC:
			gubFact[usFact] = CheckNPCIsEPC( MARY );
			break;
		case FACT_JOHN_AND_MARY_EPCS:
			gubFact[usFact] = CheckNPCIsEPC( JOHN ) && CheckNPCIsEPC( MARY );
			break;
		case FACT_MARY_ALIVE:
			gubFact[usFact] = ( gMercProfiles[ MARY ].bMercStatus != MERC_IS_DEAD );
			break;
		case FACT_MARY_BLEEDING:
			gubFact[usFact] = CheckNPCBleeding( MARY );
			break;
		case FACT_JOHN_ALIVE:
			gubFact[usFact] = ( gMercProfiles[ JOHN ].bMercStatus != MERC_IS_DEAD );
			break;
		case FACT_JOHN_BLEEDING:
			gubFact[usFact] = CheckNPCBleeding( JOHN );
			break;
		case FACT_MARY_DEAD:
			gubFact[usFact] = ( gMercProfiles[ MARY ].bMercStatus == MERC_IS_DEAD );
			break;

		case FACT_ANOTHER_FIGHT_POSSIBLE:
			gubFact[usFact] = AnotherFightPossible();
			break;

		case FACT_RECEIVING_INCOME_FROM_DCAC:
			gubFact[usFact] = (
				( PredictDailyIncomeFromAMine( MINE_DRASSEN, TRUE ) > 0 ) &&
				( PredictDailyIncomeFromAMine( MINE_ALMA, TRUE ) > 0 ) &&
				( PredictDailyIncomeFromAMine( MINE_CAMBRIA, TRUE ) > 0 ) &&
				( PredictDailyIncomeFromAMine( MINE_CHITZENA, TRUE ) > 0 ) );
			break;

		case FACT_PLAYER_BEEN_TO_K4:
			{
				UNDERGROUND_SECTORINFO * pUnderGroundSector;

				pUnderGroundSector = FindUnderGroundSector( 4, MAP_ROW_K, 1 );
				gubFact[usFact] = pUnderGroundSector->fVisited;
			}
			break;
		case FACT_WARDEN_DEAD:
			gubFact[usFact] = ( gMercProfiles[ WARDEN ].bMercStatus == MERC_IS_DEAD );
			break;

		case FACT_PLAYER_PAID_FOR_TWO_IN_BROTHEL:
			gubFact[usFact] = (gMercProfiles[ MADAME ].bNPCData > 1);
			break;

		case FACT_LOYALTY_OKAY:
			bTown = (ubProfileID != NO_PROFILE && ubProfileID < NUM_PROFILES) ? gMercProfiles[ubProfileID].bTown : BLANK_SECTOR;
			if( ( bTown != BLANK_SECTOR ) && gTownLoyalty[ bTown ].fStarted && gfTownUsesLoyalty[ bTown ])
			{
				gubFact[usFact] = ( (gTownLoyalty[ bTown ].ubRating >= LOYALTY_LOW_THRESHOLD ) && (gTownLoyalty[ bTown ].ubRating < LOYALTY_OK_THRESHOLD ) );
			}
			else
			{
				gubFact[usFact] = FALSE;
			}
			break;

		case FACT_LOYALTY_LOW:
			bTown = (ubProfileID != NO_PROFILE && ubProfileID < NUM_PROFILES) ? gMercProfiles[ubProfileID].bTown : BLANK_SECTOR;
			if( ( bTown != BLANK_SECTOR ) && gTownLoyalty[ bTown ].fStarted && gfTownUsesLoyalty[ bTown ])
			{
				// if Skyrider, ignore low loyalty until he has monologues, and wait at least a day since the latest monologue to avoid a hot/cold attitude
				if ( ( ubProfileID == SKYRIDER ) &&
						( ( guiHelicopterSkyriderTalkState == 0 ) || ( ( GetWorldTotalMin() - guiTimeOfLastSkyriderMonologue ) < ( 24 * 60 ) ) || 
						( gHelicopterSettings.fHelicopterTownLoyaltyCheck == FALSE ) ) )
				{
					gubFact[usFact] = FALSE;
				}
				else
				{
					gubFact[usFact] = (gTownLoyalty[ bTown ].ubRating < LOYALTY_LOW_THRESHOLD );
				}
			}
			else
			{
				gubFact[usFact] = FALSE;
			}
			break;

		case FACT_LOYALTY_HIGH:
			bTown = (ubProfileID != NO_PROFILE && ubProfileID < NUM_PROFILES) ? gMercProfiles[ubProfileID].bTown : BLANK_SECTOR;
			if( ( bTown != BLANK_SECTOR ) && gTownLoyalty[ bTown ].fStarted && gfTownUsesLoyalty[ bTown ])
			{
				gubFact[usFact] = (gTownLoyalty[ gMercProfiles[ ubProfileID ].bTown ].ubRating >= LOYALTY_HIGH_THRESHOLD );
			}
			else
			{
				gubFact[usFact] = FALSE;
			}
			break;

		case FACT_ELGIN_ALIVE:
			gubFact[usFact] = ( gMercProfiles[ DRUGGIST ].bMercStatus != MERC_IS_DEAD );
			break;

		case FACT_SPEAKER_AIM_OR_AIM_NEARBY:
			gubFact[usFact] = gpDestSoldier && AIMMercWithin( gpDestSoldier->sGridNo, 10 );
			break;

		case FACT_MALE_SPEAKING_FEMALE_PRESENT:
			gubFact[usFact] = ( !CheckTalkerFemale() && FemalePresent( ubProfileID ) );
			break;

		case FACT_PLAYER_OWNS_2_TOWNS_INCLUDING_OMERTA:
#ifdef JA2UB
//UB
#else
			gubFact[usFact] = ( ( GetNumberOfWholeTownsUnderControl() == gGameExternalOptions.ubEarlyRebelsRecruitment[1] || gGameExternalOptions.ubEarlyRebelsRecruitment[0] == 1 
								|| ( gubQuest[QUEST_FOOD_ROUTE] == QUESTDONE && gGameExternalOptions.ubEarlyRebelsRecruitment[0] == 4 ) ) && IsTownUnderCompleteControlByPlayer( OMERTA ) );
#endif
			break;

		case FACT_PLAYER_OWNS_3_TOWNS_INCLUDING_OMERTA:
#ifdef JA2UB
//UB
#else
			gubFact[usFact] = ( ( GetNumberOfWholeTownsUnderControl() == gGameExternalOptions.ubEarlyRebelsRecruitment[2] || gGameExternalOptions.ubEarlyRebelsRecruitment[0] == 1 
								|| ( gubQuest[QUEST_FOOD_ROUTE] == QUESTDONE && gGameExternalOptions.ubEarlyRebelsRecruitment[0] == 4 ) ) && IsTownUnderCompleteControlByPlayer( OMERTA ) );
#endif
			break;

		case FACT_PLAYER_OWNS_4_TOWNS_INCLUDING_OMERTA:
#ifdef JA2UB
//UB
#else
			gubFact[usFact] = ( ( GetNumberOfWholeTownsUnderControl() >= gGameExternalOptions.ubEarlyRebelsRecruitment[3] || gGameExternalOptions.ubEarlyRebelsRecruitment[0] == 1 
								|| ( gubQuest[QUEST_FOOD_ROUTE] == QUESTDONE && gGameExternalOptions.ubEarlyRebelsRecruitment[0] == 4 ) ) && IsTownUnderCompleteControlByPlayer( OMERTA ) );
			// silversurfer: this is the highest requirement and therefore we will automatically enable recruitment of Miguel
			if ( gubFact[usFact] )
				SetFactTrue(FACT_MIGUEL_AND_ALL_REBELS_CAN_BE_RECRUITED); // Miguel will now be willing to join and so will the other RPC
#endif
			break;

		case FACT_PLAYER_FOUGHT_THREE_TIMES_TODAY:
			gubFact[usFact] = !BoxerAvailable();
			break;

		case FACT_PLAYER_DOING_POORLY:
			gubFact[usFact] = ( CurrentPlayerProgressPercentage() < 20 );
			break;

		case FACT_PLAYER_DOING_WELL:
			gubFact[usFact] = ( CurrentPlayerProgressPercentage() > 50 );
			break;

		case FACT_PLAYER_DOING_VERY_WELL:
			gubFact[usFact] = ( CurrentPlayerProgressPercentage() > 80 );
			break;

		case FACT_FATHER_DRUNK_AND_SCIFI_OPTION_ON:
			gubFact[usFact] = ( ( gMercProfiles[ FATHER ].bNPCData >= 5 ) && gGameOptions.ubGameStyle == STYLE_SCIFI && gGameExternalOptions.fEnableCrepitus);
			break;

		case FACT_BLOODCAT_QUEST_STARTED_TWO_DAYS_AGO:
			gubFact[usFact] = ( (gubQuest[ QUEST_BLOODCATS ] != QUESTNOTSTARTED) && (GetWorldTotalMin() - GetTimeQuestWasStarted( QUEST_BLOODCATS ) > 2 * NUM_SEC_IN_DAY / NUM_SEC_IN_MIN) );
			break;

		case FACT_NOTHING_REPAIRED_YET:
			gubFact[usFact] = RepairmanIsFixingItemsButNoneAreDoneYet( ubProfileID );
			break;

		case FACT_NPC_COWERING:
			gubFact[usFact] = CheckNPCCowering( ubProfileID );
			break;

		case FACT_TOP_AND_BOTTOM_LEVELS_CLEARED:
			gubFact[usFact] = ( gubFact[ FACT_TOP_LEVEL_CLEARED ] && gubFact[ FACT_BOTTOM_LEVEL_CLEARED ] );
			break;

		case FACT_FIRST_BARTENDER:
			gubFact[usFact] = (ubProfileID != NO_PROFILE && ubProfileID < NUM_PROFILES) && (gMercProfiles[ubProfileID].bNPCData == 1 || (gMercProfiles[ubProfileID].bNPCData == 0 && CountBartenders() == 0));
			break;

		case FACT_SECOND_BARTENDER:
			gubFact[usFact] = (ubProfileID != NO_PROFILE && ubProfileID < NUM_PROFILES) && (gMercProfiles[ubProfileID].bNPCData == 2 || (gMercProfiles[ubProfileID].bNPCData == 0 && CountBartenders() == 1));
			break;

		case FACT_THIRD_BARTENDER:
			gubFact[usFact] = (ubProfileID != NO_PROFILE && ubProfileID < NUM_PROFILES) && (gMercProfiles[ubProfileID].bNPCData == 3 || (gMercProfiles[ubProfileID].bNPCData == 0 && CountBartenders() == 2));
			break;

		case FACT_FOURTH_BARTENDER:
			gubFact[usFact] = (ubProfileID != NO_PROFILE && ubProfileID < NUM_PROFILES) && (gMercProfiles[ubProfileID].bNPCData == 4 || (gMercProfiles[ubProfileID].bNPCData == 0 && CountBartenders() == 3));
			break;

		case FACT_NPC_NOT_UNDER_FIRE:
			gubFact[usFact] = !CheckNPCIsUnderFire( ubProfileID );
			break;

		case FACT_KINGPIN_NOT_IN_OFFICE:
			// r7737
			//gubFact[usFact] = !( gWorldSectorX == 5 && gWorldSectorY == MAP_ROW_D && NPCInRoomRange( KINGPIN, 30, 39 ) );
			gubFact[usFact] = !( gWorldSectorX == gModSettings.ubKingpinHouseSectorX && gWorldSectorY == gModSettings.ubKingpinHouseSectorY && NPCInRoomRange( KINGPIN, gModSettings.usKingpinRoomRangeStart, gModSettings.usKingpinRoomRangeEnd ) );
			// 30 to 39
			break;

		case FACT_DONT_OWE_KINGPIN_MONEY:
			gubFact[usFact] = (gubQuest[ QUEST_KINGPIN_MONEY ] != QUESTINPROGRESS);
			break;

		case FACT_NO_CLUB_FIGHTING_ALLOWED:
			// anv: added !BoxersAvailable, otherwise van Haussen would offer fight even when all boxers are dead
			// r7737
			//gubFact[usFact] = ( gubQuest[ QUEST_KINGPIN_MONEY ] == QUESTINPROGRESS || gfBoxersResting || !BoxersAvailable() );// plus other conditions
			// silversurfer: additional fix - the check below failed when we killed all boxers because BoxersAvailable() returns 0 in this case.
			// Added additional check for Kingpins location so that the three initial fights can be completed. As soon as Kingpin is gone the original check will apply again.
			SOLDIERTYPE *pKingpin;
			pKingpin = FindSoldierByProfileID(KINGPIN, FALSE);
			//ScreenMsg(FONT_LTGREEN, MSG_INTERFACE, L"FACT_NO_CLUB_FIGHTING_ALLOWED %d !kingpin %d QUEST_KINGPIN_MONEY %d gfBoxersResting %d BoxersAvailable %d", gubFact[usFact], !pKingpin, gubQuest[QUEST_KINGPIN_MONEY] == QUESTINPROGRESS, gfBoxersResting, BoxersAvailable());
			// sevenfm:
			gubFact[usFact] = (!pKingpin || gubQuest[QUEST_KINGPIN_MONEY] == QUESTINPROGRESS || gfBoxersResting || !BoxersAvailable());
			/*if ( pKingpin )
				//gubFact[usFact] = ( gubQuest[ QUEST_KINGPIN_MONEY ] == QUESTINPROGRESS || gfBoxersResting || ( !BoxersAvailable() && PythSpacesAway(pKingpin->sGridNo, gModSettings.iKingpinRingTile) > 2 ) );// plus other conditions
				//gubFact[usFact] = (gubQuest[QUEST_KINGPIN_MONEY] == QUESTINPROGRESS || gfBoxersResting || !BoxersAvailable() && gTacticalStatus.bBoxingState == NOT_BOXING);// plus other conditions
				gubFact[usFact] = (gubQuest[QUEST_KINGPIN_MONEY] == QUESTINPROGRESS || gfBoxersResting);// plus other conditions
			else
				gubFact[usFact] = TRUE;*/			
			break;

		case FACT_MADDOG_IS_SPEAKER:
			gubFact[usFact] = ( gubSrcSoldierProfile == MADDOG );
			break;

		case FACT_PC_HAS_CONRADS_RECRUIT_OPINION:
			gubFact[usFact] = ( gpDestSoldier && (CalcDesireToTalk( gpDestSoldier->ubProfile, gubSrcSoldierProfile, APPROACH_RECRUIT ) >= 50) );
			break;

		case FACT_NPC_HOSTILE_OR_PISSED_OFF:
			gubFact[usFact] = CheckNPCIsEnemy(ubProfileID) || 
							(ubProfileID != NO_PROFILE && ubProfileID < NUM_PROFILES) && (gMercProfiles[ubProfileID].ubMiscFlags3 & PROFILE_MISC_FLAG3_NPC_PISSED_OFF);
			break;

		case FACT_TONY_IN_BUILDING:
			gubFact[usFact] = CheckNPCSector( TONY, gModSettings.ubPornShopTonySectorX, gModSettings.ubPornShopTonySectorY, gModSettings.ubPornShopTonySectorZ ) && 
				NPCInRoom( TONY, gModSettings.usPornShopRoomTony );
			break;

		case FACT_SHANK_SPEAKING:
			gubFact[usFact] = ( gpSrcSoldier && gpSrcSoldier->ubProfile == SHANK );
			break;

		case FACT_ROCKET_RIFLE_EXISTS:
			gubFact[usFact] = ItemTypeExistsAtLocation( gModSettings.sRocketRifleGridNo, ROCKET_RIFLE, 0, NULL );
			break;

		case FACT_PABLO_ALIVE:
			gubFact[usFact] = gMercProfiles[ PABLO ].bMercStatus != MERC_IS_DEAD;
			break;

		case FACT_DOREEN_ALIVE:
			gubFact[usFact] = gMercProfiles[ DOREEN ].bMercStatus != MERC_IS_DEAD;
			break;

		case FACT_WALDO_ALIVE:
			gubFact[usFact] = gMercProfiles[ WALDO ].bMercStatus != MERC_IS_DEAD;
			break;
#ifdef JA2UB
//UB
#else        
		case FACT_PERKO_ALIVE:
			gubFact[usFact] = gMercProfiles[ PERKO ].bMercStatus != MERC_IS_DEAD;
			break;
#endif
		case FACT_TONY_ALIVE:
			gubFact[usFact] = gMercProfiles[ TONY ].bMercStatus != MERC_IS_DEAD;
			break;

		case FACT_VINCE_ALIVE:
			gubFact[usFact] = gMercProfiles[ VINCE ].bMercStatus != MERC_IS_DEAD;
			break;

		case FACT_JENNY_ALIVE:
			gubFact[usFact] = gMercProfiles[ JENNY ].bMercStatus != MERC_IS_DEAD;
			break;

		case FACT_ARNOLD_ALIVE:
			gubFact[usFact] = gMercProfiles[ ARNIE ].bMercStatus != MERC_IS_DEAD;
			break;

		// HEADROCK HAM 3.6: No longer necessarily I16.
		case FACT_BLOODCAT_LAIR_CLEANED:
			gubFact[usFact] = (SectorInfo[ gubBloodcatLairSectorId ].bBloodCats == 0);
			break;

		case FACT_NPC_BANDAGED_TODAY:
			gubFact[usFact] = (ubProfileID != NO_PROFILE && ubProfileID < NUM_PROFILES) && 
							(gMercProfiles[ubProfileID].ubMiscFlags2 & PROFILE_MISC_FLAG2_BANDAGED_TODAY) != 0;
			break;

		case FACT_PLAYER_IN_SAME_ROOM:
			gubFact[usFact] = PCInSameRoom( ubProfileID );
			break;

		case FACT_PLAYER_SPOKE_TO_DRASSEN_MINER:
			gubFact[usFact] = SpokenToHeadMiner( MINE_DRASSEN );
			break;
		case FACT_PLAYER_IN_CONTROLLED_DRASSEN_MINE:
			gubFact[usFact] = ( GetIdOfMineForSector( gWorldSectorX, gWorldSectorY, gbWorldSectorZ ) == MINE_DRASSEN && !(StrategicMap[ gWorldSectorX + MAP_WORLD_X * gWorldSectorY ].fEnemyControlled) );
			break;
		case FACT_PLAYER_SPOKE_TO_CAMBRIA_MINER:
			gubFact[usFact] = SpokenToHeadMiner( MINE_CAMBRIA );
			break;
		case FACT_PLAYER_IN_CONTROLLED_CAMBRIA_MINE:
			gubFact[usFact] = ( GetIdOfMineForSector( gWorldSectorX, gWorldSectorY, gbWorldSectorZ ) == MINE_CAMBRIA && !(StrategicMap[ gWorldSectorX + MAP_WORLD_X * gWorldSectorY ].fEnemyControlled) );
			break;
		case FACT_PLAYER_SPOKE_TO_CHITZENA_MINER:
			gubFact[usFact] = SpokenToHeadMiner( MINE_CHITZENA );
			break;
		case FACT_PLAYER_IN_CONTROLLED_CHITZENA_MINE:
			gubFact[usFact] = ( GetIdOfMineForSector( gWorldSectorX, gWorldSectorY, gbWorldSectorZ ) == MINE_CHITZENA && !(StrategicMap[ gWorldSectorX + MAP_WORLD_X * gWorldSectorY ].fEnemyControlled) );
			break;
		case FACT_PLAYER_SPOKE_TO_ALMA_MINER:
			gubFact[usFact] = SpokenToHeadMiner( MINE_ALMA );
			break;
		case FACT_PLAYER_IN_CONTROLLED_ALMA_MINE:
			gubFact[usFact] = ( GetIdOfMineForSector( gWorldSectorX, gWorldSectorY, gbWorldSectorZ ) == MINE_ALMA && !(StrategicMap[ gWorldSectorX + MAP_WORLD_X * gWorldSectorY ].fEnemyControlled) );
			break;
		case FACT_PLAYER_SPOKE_TO_GRUMM_MINER:
			gubFact[usFact] = SpokenToHeadMiner( MINE_GRUMM );
			break;
		case FACT_PLAYER_IN_CONTROLLED_GRUMM_MINE:
			gubFact[usFact] = ( GetIdOfMineForSector( gWorldSectorX, gWorldSectorY, gbWorldSectorZ ) == MINE_GRUMM && !(StrategicMap[ gWorldSectorX + MAP_WORLD_X * gWorldSectorY ].fEnemyControlled) );
			break;

		case FACT_ENOUGH_LOYALTY_TO_TRAIN_MILITIA:
			gubFact[usFact] = InTownSectorWithTrainingLoyalty( gWorldSectorX, gWorldSectorY );
			break;

		case FACT_WALKER_AT_BAR:
			gubFact[usFact] = (gMercProfiles[ FATHER ].sSectorX == 13 && gMercProfiles[ FATHER ].sSectorY == MAP_ROW_C);
			break;

		case FACT_JOEY_ALIVE:
			gubFact[usFact] = gMercProfiles[ JOEY ].bMercStatus != MERC_IS_DEAD;
			break;

		case FACT_UNPROPOSITIONED_FEMALE_SPEAKING_TO_NPC:
			gubFact[usFact] = CheckTalkerUnpropositionedFemale();
			break;

		case FACT_84_AND_85_TRUE:
			gubFact[usFact] = CheckFact( 84, ubProfileID ) && CheckFact( FACT_HANS_AT_SPOT, ubProfileID );
			break;

		case FACT_SKYRIDER_IN_B15:
			gubFact[ usFact ] = CheckNPCSector( SKYRIDER, 15, MAP_ROW_B, 0 );
			break;

		case FACT_SKYRIDER_IN_C16:
			gubFact[ usFact ] = CheckNPCSector( SKYRIDER, 16, MAP_ROW_C, 0 );
			break;
		case FACT_SKYRIDER_IN_E14:
			gubFact[ usFact ] = CheckNPCSector( SKYRIDER, 14, MAP_ROW_E, 0 );
			break;
		case FACT_SKYRIDER_IN_D12:
			gubFact[ usFact ] = CheckNPCSector( SKYRIDER, 12, MAP_ROW_D, 0 );
			break;

		case FACT_KINGPIN_IS_ENEMY:
			gubFact[usFact] = (gTacticalStatus.fCivGroupHostile[ KINGPIN_CIV_GROUP ] >= CIV_GROUP_WILL_BECOME_HOSTILE);
			break;

		case FACT_DYNAMO_NOT_SPEAKER:
			gubFact[usFact] = !( gpSrcSoldier != NULL && (gpSrcSoldier->ubProfile == DYNAMO ) );
			break;

		case FACT_PABLO_BRIBED:
			gubFact[usFact] = !CheckFact( FACT_PABLOS_BRIBED, ubProfileID );
			break;

		case FACT_VEHICLE_PRESENT:
			gubFact[usFact] = CheckFact( FACT_OK_USE_HUMMER, ubProfileID ) && ( (FindSoldierByProfileID( PROF_HUMMER, TRUE ) != NULL) || (FindSoldierByProfileID( PROF_ICECREAM, TRUE ) != NULL) );
			break;

		case FACT_PLAYER_KILLED_BOXERS:
			gubFact[usFact] = !BoxerExists();
			break;

		case FACT_DIMITRI_CAN_BE_RECRUITED: // Can dimitri be recruited? should be true if already true, OR if Miguel has been recruited already OR is available for recruitment
			gubFact[usFact] = ( gubFact[usFact] || FindSoldierByProfileID( MIGUEL, TRUE ) || gubFact[FACT_MIGUEL_AND_ALL_REBELS_CAN_BE_RECRUITED] );
			break;
/*			
		case FACT_:
			gubFact[usFact] = ;
			break;
*/
		// anv: facts for checking if Waldo can repair helicopter

		case FACT_HELI_DAMAGED_CAN_START_REPAIR:
			gubFact[usFact] = ( gGameExternalOptions.fWaldoCanRepairHelicopter && 
				!fHelicopterDestroyed && !fHelicopterIsAirBorne && gubHelicopterHitsTaken == 1 &&
				pVehicleList[ iHelicopterVehicleId ].sSectorX == gMercProfiles[ WALDO ].sSectorX && pVehicleList[ iHelicopterVehicleId ].sSectorY == gMercProfiles[ WALDO ].sSectorY &&
				LaptopSaveInfo.iCurrentBalance >= CalculateHelicopterRepairCost( FALSE ) &&
				gubHelicopterHoursToRepair == 0 && CheckFact( FACT_WALDO_MET, 0 ) );
			break;

		case FACT_HELI_SERIOUSLY_DAMAGED_CAN_START_REPAIR:
			gubFact[usFact] = ( gGameExternalOptions.fWaldoCanRepairHelicopter && 
				!fHelicopterDestroyed && !fHelicopterIsAirBorne && gubHelicopterHitsTaken == 2 &&
				pVehicleList[ iHelicopterVehicleId ].sSectorX == gMercProfiles[ WALDO ].sSectorX && pVehicleList[ iHelicopterVehicleId ].sSectorY == gMercProfiles[ WALDO ].sSectorY && 
				LaptopSaveInfo.iCurrentBalance >= CalculateHelicopterRepairCost( TRUE ) &&
				gubHelicopterHoursToRepair == 0 && CheckFact( FACT_WALDO_MET, 0 ) );
			break;

		case FACT_HELI_GIVEN_MONEY_CAN_START_REPAIR:
			gubFact[usFact] = ( ( gGameExternalOptions.fWaldoCanRepairHelicopter && 
				!fHelicopterDestroyed && !fHelicopterIsAirBorne && gubHelicopterHitsTaken > 0 &&
				pVehicleList[ iHelicopterVehicleId ].sSectorX == gMercProfiles[ WALDO ].sSectorX && pVehicleList[ iHelicopterVehicleId ].sSectorY == gMercProfiles[ WALDO ].sSectorY && 
				gubHelicopterHoursToRepair == 0 ) &&
				( CheckFact(FACT_GIVEN_ENOUGH_TO_REPAIR_HELI, 0) || CheckFact(FACT_GIVEN_ENOUGH_TO_SERIOUSLY_REPAIR_HELI, 0) ) && CheckFact( FACT_WALDO_MET, 0 ) );
			break;

		case FACT_HELI_CANT_START_REPAIR:
			gubFact[usFact] = ( !CheckFact(FACT_HELI_GIVEN_MONEY_CAN_START_REPAIR, 0) && !CheckFact(FACT_HELICOPTER_IN_PERFECT_CONDITION, 0));
			break;			

		case FACT_HELICOPTER_IN_PERFECT_CONDITION:
			gubFact[usFact] = ( gubHelicopterHitsTaken == 0 );
			break;

		case FACT_HELICOPTER_LOST:
			gubFact[usFact] = fHelicopterDestroyed;
			break;

		default:
			break;
	}
	
//#endif
	return( gubFact[usFact] );
}

void StartQuest( UINT8 ubQuest, INT16 sSectorX, INT16 sSectorY )
{
	if (ubQuest <= QUEST_KILL_DEIDRANNA)
		DebugQuestInfo(String("start quest %s %d sector %d %d", szQuest[ubQuest], ubQuest, sSectorX, sSectorY));
	else
		DebugQuestInfo(String("start quest %d sector %d %d", ubQuest, sSectorX, sSectorY));

	InternalStartQuest( ubQuest, sSectorX, sSectorY, TRUE );
}


void InternalStartQuest( UINT8 ubQuest, INT16 sSectorX, INT16 sSectorY, BOOLEAN fUpdateHistory )
{
#ifdef LUA_QUESTS
		LuaInternalQuest( ubQuest, sSectorX, sSectorY, fUpdateHistory, 1);
#else
	if ( gubQuest[ubQuest ] == QUESTNOTSTARTED )
	{
		gubQuest[ubQuest] = QUESTINPROGRESS;

	if ( fUpdateHistory )
	{
			if (!is_networked)
		SetHistoryFact( HISTORY_QUEST_STARTED, ubQuest, GetWorldTotalMin(), sSectorX, sSectorY );
	}
	}
	else
	{
		gubQuest[ubQuest] = QUESTINPROGRESS;
	}
#endif
}

void EndQuest( UINT8 ubQuest, INT16 sSectorX, INT16 sSectorY )
{
	if (ubQuest <= QUEST_KILL_DEIDRANNA)
		DebugQuestInfo(String("end quest %s %d sector %d %d", szQuest[ubQuest], ubQuest, sSectorX, sSectorY));
	else
		DebugQuestInfo(String("end quest %d sector %d %d", ubQuest, sSectorX, sSectorY));

	InternalEndQuest( ubQuest, sSectorX, sSectorY, TRUE );
}

void InternalEndQuest( UINT8 ubQuest, INT16 sSectorX, INT16 sSectorY, BOOLEAN fUpdateHistory )
{

#ifdef LUA_QUESTS
		LuaInternalQuest( ubQuest, sSectorX, sSectorY, fUpdateHistory, 0);
#else

	// SANDRO merc records - quests handled counter
	if ( gubQuest[ubQuest] != QUESTDONE && fUpdateHistory ) // only public quests
	{
		switch (ubQuest)
		{
			case QUEST_FOOD_ROUTE :
				// food quest is handled differently (completing when spoken to father Walker)
				break;
			case QUEST_DELIVER_LETTER :
			case QUEST_LEATHER_SHOP_DREAM :
				GiveQuestRewardPoint( sSectorX, sSectorY, 3, NO_PROFILE );
				break;
			case QUEST_ESCORT_SKYRIDER :
			case QUEST_CHOPPER_PILOT :
			case QUEST_FREE_CHILDREN :
			case QUEST_RESCUE_MARIA :
			case QUEST_FIND_SCIENTIST :
			case QUEST_DELIVER_VIDEO_CAMERA :
			case QUEST_FIND_HERMIT :
				GiveQuestRewardPoint( sSectorX, sSectorY, 4, NO_PROFILE );
				break;
			case QUEST_FREE_DYNAMO :
				// don't give Dynamo reward for himself
				if( gMercProfiles[ DYNAMO ].bMercStatus != MERC_IS_DEAD )
					GiveQuestRewardPoint( sSectorX, sSectorY, 4, DYNAMO );
				break;
			case QUEST_KINGPIN_IDOL :
			case QUEST_CHITZENA_IDOL :
			case QUEST_HELD_IN_ALMA :
			case QUEST_RUNAWAY_JOEY :
			case QUEST_ESCORT_TOURISTS :
				GiveQuestRewardPoint( sSectorX, sSectorY, 5, NO_PROFILE );
				break;
			case QUEST_ARMY_FARM :
			case QUEST_KINGPIN_MONEY :
				GiveQuestRewardPoint( sSectorX, sSectorY, 6, NO_PROFILE );
				break;
			case QUEST_BLOODCATS :
				GiveQuestRewardPoint( sSectorX, sSectorY, 7, NO_PROFILE );
				break;
			case QUEST_INTERROGATION : // we get here only if wiped out enemies after interrogation
			case QUEST_CREATURES :
				GiveQuestRewardPoint( sSectorX, sSectorY, 8, NO_PROFILE );
				break;
			case QUEST_KILL_TERRORISTS :  // only reduced experiences if we chosen Slay over Carmen
				if( gMercProfiles[ CARMEN ].bMercStatus == MERC_IS_DEAD )
					GiveQuestRewardPoint( sSectorX, sSectorY, 5, NO_PROFILE );
				else
					GiveQuestRewardPoint( sSectorX, sSectorY, 9, NO_PROFILE );
				break;
#ifdef JA2UB
//off
#else
			case QUEST_KILL_DEIDRANNA :
				GiveQuestRewardPoint( sSectorX, sSectorY, 25, NO_PROFILE );
				break;
#endif
			default :
				GiveQuestRewardPoint( sSectorX, sSectorY, 4, NO_PROFILE );
				break;
		}
	}

	if ( gubQuest[ubQuest ] == QUESTINPROGRESS )
	{	
		gubQuest[ubQuest] = QUESTDONE;

	if ( fUpdateHistory )
	{
		ResetHistoryFact( ubQuest, sSectorX, sSectorY );
	}
	}
	else
	{
		gubQuest[ubQuest] = QUESTDONE;
	}

	if ( ubQuest == QUEST_RESCUE_MARIA )
	{
		// cheap hack to try to prevent Madame Layla from thinking that you are
		// still in the brothel with Maria...
		gMercProfiles[ MADAME ].bNPCData = 0;
		gMercProfiles[ MADAME ].bNPCData2 = 0;
	}
	
#endif
	
#ifdef JA2UB	
	//if the quest is the FIX LAPTOP quest
	if( ubQuest == QUEST_FIX_LAPTOP && gGameUBOptions.LaptopQuestEnabled == TRUE )
	{
		//Set the fact that AIM and MERC should start selling
		gJa25SaveStruct.fHaveAimandMercOffferItems = TRUE;

		//Remeber that we should send email in the next sector
		gJa25SaveStruct.fSendEmail_10_NextSector = TRUE;

		AddEmail( EMAIL_PILOTMISSING, EMAIL_PILOTMISSING_LENGTH, MAIL_ENRICO,  GetWorldTotalMin() ,-1,-1, TYPE_EMAIL_EMAIL_EDT);
		AddEmail( EMAIL_MAKECONTACT, EMAIL_MAKECONTACT_LENGTH, MAIL_ENRICO,  GetWorldTotalMin() ,-1,-1, TYPE_EMAIL_EMAIL_EDT);

		//Merc and Aim emails
		AddEmail( EMAIL_AIM_PROMOTION_1, EMAIL_AIM_PROMOTION_1_LENGTH, AIM_SITE,  GetWorldTotalMin(),-1 ,-1, TYPE_EMAIL_EMAIL_EDT);
		AddEmail( EMAIL_MERC_PROMOTION_1, EMAIL_MERC_PROMOTION_1_LENGTH, SPECK_FROM_MERC,  GetWorldTotalMin(),-1,-1, TYPE_EMAIL_EMAIL_EDT);
		AddEmail( EMAIL_AIM_PROMOTION_2, EMAIL_AIM_PROMOTION_2_LENGTH, AIM_SITE,  GetWorldTotalMin(),-1 ,-1, TYPE_EMAIL_EMAIL_EDT);

		//Manuel
		{
			SOLDIERTYPE *pSoldier=NULL;

			pSoldier = FindSoldierByProfileID( MANUEL_UB , TRUE ); //MANUEL

			if( pSoldier != NULL )
			{
				//Add the Manuel email
				AddEmail( EMAIL_MANUEL, EMAIL_MANUEL_LENGTH, MAIL_ENRICO,  GetWorldTotalMin() ,-1, -1, TYPE_EMAIL_EMAIL_EDT);
			}
		}

		//Miguel
		{
			//if miguel was dead, when importing the save
			if( gubFact[ FACT_PLAYER_IMPORTED_SAVE_MIGUEL_DEAD ] == FALSE )
			{
				//Add the miguel email
				AddEmail( EMAIL_MIGUELHELLO, EMAIL_MIGUELHELLO_LENGTH, MAIL_MIGUEL,  GetWorldTotalMin(),-1, -1, TYPE_EMAIL_EMAIL_EDT);
			}
		}

		//If any aim mercs were asked to send emails when the get back from duty elsewhere
		HandleAddingAnyAimAwayEmailsWhenLaptopGoesOnline();

		//Should we send the IMP reminder email when we go back online
		ShouldImpReminderEmailBeSentWhenLaptopBackOnline();

		//Force which ever of these emails that needed to be sent, to be sent
		HandleEmailBeingSentWhenEnteringSector( 0, 0, 0, TRUE );
	}
#endif
};

void InitQuestEngine()
{		
	memset(gubQuest, 0, sizeof(gubQuest));
	memset(gubFact,	0, sizeof(gubFact));

	DebugQuestInfo(String("init quest engine"));

	// semi-hack to make the letter quest start right away
	CheckForQuests( 1 );

	if ( gGameOptions.ubGameStyle == STYLE_SCIFI )
	{
		// 3 medical boosters
		gubCambriaMedicalObjects = 21;
	}
	else
	{
		gubCambriaMedicalObjects = 18;
	}

	gubBoxingMatchesWon = 0;
	gubBoxersRests = 0;
	gfBoxersResting = FALSE;

	DebugQuestInfo(String("reset gubBoxersRests 0, gubBoxingMatchesWon 0, gfBoxersResting FALSE gubCambriaMedicalObjects %d", gubCambriaMedicalObjects));
}



void CheckForQuests( UINT32 uiDay )
{
	// This function gets called at 8:00 AM time of the day

#ifdef TESTQUESTS
	ScreenMsg( MSG_FONT_RED, MSG_DEBUG, L"Checking For Quests, Day %d", uiDay );
#endif

#ifdef JA2UB
 // -------------------------------------------------------------------------------
	// QUEST 23 : Detroy missles
	// -------------------------------------------------------------------------------
	// The game always starts with destrouy missles quest, so turn it on if it hasn't
	// already started
	if( gubQuest[ QUEST_DESTROY_MISSLES ] == QUESTNOTSTARTED )
	{
		StartQuest( QUEST_DESTROY_MISSLES, -1, -1 );
#ifdef TESTQUESTS
		ScreenMsg( MSG_FONT_RED, MSG_DEBUG, L"Started DESTORY MISSLES quest");
#endif
	}
//Ja25: No deliver letter quest, dont start it
#else
	// -------------------------------------------------------------------------------
	// QUEST 0 : DELIVER LETTER
	// -------------------------------------------------------------------------------
	// The game always starts with DELIVER LETTER quest, so turn it on if it hasn't
	// already started
	if (gubQuest[QUEST_DELIVER_LETTER] == QUESTNOTSTARTED)
	{
		StartQuest( QUEST_DELIVER_LETTER, -1, -1 );
#ifdef TESTQUESTS

		if (!is_networked)
		ScreenMsg( MSG_FONT_RED, MSG_DEBUG, L"Started DELIVER LETTER quest");
#endif
	}
#endif
	// This quest gets turned OFF through conversation with Miguel - when user hands
	// Miguel the letter
}


BOOLEAN SaveQuestInfoToSavedGameFile( HWFILE hFile )
{
	UINT32	uiNumBytesWritten;

	//Save all the states if the Quests
	FileWrite( hFile, gubQuest, MAX_QUESTS, &uiNumBytesWritten );
	if( uiNumBytesWritten != MAX_QUESTS )
	{
		return(FALSE);
	}

	//Save all the states for the facts
	FileWrite( hFile, gubFact, NUM_FACTS, &uiNumBytesWritten );
	if( uiNumBytesWritten != NUM_FACTS )
	{
		return(FALSE);
	}


	return( TRUE );
}


BOOLEAN LoadQuestInfoFromSavedGameFile( HWFILE hFile, UINT8 MaxQuest )
{
	UINT32	uiNumBytesRead;

	//Save all the states if the Quests
	//FileRead( hFile, gubQuest, MAX_QUESTS, &uiNumBytesRead );
	//if( uiNumBytesRead != MAX_QUESTS )
	FileRead( hFile, gubQuest, MaxQuest, &uiNumBytesRead );
	if( uiNumBytesRead != MaxQuest )
	{
		return(FALSE);
	}

	//Save all the states for the facts
	FileRead( hFile, gubFact, NUM_FACTS, &uiNumBytesRead );
	if( uiNumBytesRead != NUM_FACTS )
	{
		return(FALSE);
	}

	return( TRUE );
}

// SANDRO - a function to add quest-done point to merc records, and award some exp possibly
void GiveQuestRewardPoint( INT16 sQuestSectorX, INT16 sQuestsSectorY, INT8 bExpReward, UINT8 bException )
{
	ScreenMsg( FONT_MCOLOR_LTBLUE, MSG_TESTVERSION, L"QUEST COMPLETED - Adding to merc records and awarding experiences (%d).", (bExpReward * gGameExternalOptions.usAwardSpecialExpForQuests) );

	for ( UINT8 i = gTacticalStatus.Team[ gbPlayerNum ].bFirstID; i <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; i++ )
	{
		if( MercPtrs[ i ]->bActive && MercPtrs[ i ]->stats.bLife >= CONSCIOUSNESS && !(MercPtrs[ i ]->flags.uiStatusFlags & SOLDIER_VEHICLE) && MercPtrs[ i ]->ubProfile != NO_PROFILE &&
			MercPtrs[ i ]->sSectorX == sQuestSectorX && MercPtrs[ i ]->sSectorY == sQuestsSectorY && !MercPtrs[ i ]->flags.fBetweenSectors && MercPtrs[ i ]->bTeam == gbPlayerNum &&
			MercPtrs[ i ]->bAssignment != IN_TRANSIT && MercPtrs[ i ]->bAssignment != ASSIGNMENT_DEAD && gMercProfiles[ MercPtrs[ i ]->ubProfile ].ubBodyType != 21 ) // != ROBOTNOWEAPON )
		{
			if ( MercPtrs[ i ]->ubProfile != bException )
			{
				gMercProfiles[ MercPtrs[ i ]->ubProfile ].records.ubQuestsHandled++;

				if ( bExpReward > 0 && gGameExternalOptions.usAwardSpecialExpForQuests > 0 )
					StatChange( MercPtrs[ i ], EXPERAMT, (bExpReward * gGameExternalOptions.usAwardSpecialExpForQuests), FALSE );
			}
		}
	}	
}







