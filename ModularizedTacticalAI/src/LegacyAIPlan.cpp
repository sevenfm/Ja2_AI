/**
 * @file
 * @author feynman (bears-pit.com)
 */

#include "../include/LegacyAIPlan.h"

#include "../../TacticalAI/ai.h"
#include "../../TacticalAI/AIInternals.h"       // ACTING_ON_SCHEDULE
#include "../../TacticalAI/NPC.h"               // NPCReachedDestination
#include "../../Tactical/Animation Control.h"   // defines ANIM_...
#include "../../Tactical/Soldier Macros.h"      // CREATURE_OR_BLOODCAT
#include "../../Tactical/opplist.h"             // EndMuzzleFlash
#include "../../Tactical/Dialogue Control.h"    // DialogueQueueIsEmpty
#include "../../TileEngine/Isometric Utils.h"   // defines NOWHERE
#include "../../Utils/Debug Control.h"          // LiveMessage
#include "../../Utils/Font Control.h"           // ScreenMsg about deadlock
#include "../../Utils/Text.h"                   // Sniper warning
#include "../../Utils/message.h"                // ditto

extern STR szAction[];

namespace AI
{
    namespace tactical
    {
        LegacyAIPlan::LegacyAIPlan(SOLDIERTYPE* npc)
            : Plan(npc)
        {
        }


        void LegacyAIPlan::execute(PlanInputData& environment)
        {
            if(!environment.turn_based())
            {
                if ( (get_npc()->ubProfile != NO_PROFILE) && (gMercProfiles[ get_npc()->ubProfile ].ubMiscFlags3 & PROFILE_MISC_FLAG3_HANDLE_DONE_TRAVERSAL ) )
                {
                    TriggerNPCWithGivenApproach( get_npc()->ubProfile, APPROACH_DONE_TRAVERSAL, FALSE );
                    gMercProfiles[ get_npc()->ubProfile ].ubMiscFlags3 &= (~PROFILE_MISC_FLAG3_HANDLE_DONE_TRAVERSAL);
                    get_npc()->ubQuoteActionID = 0;
                    // wait a tiny bit
                    get_npc()->aiData.usActionData = 100;
                    get_npc()->aiData.bAction =  AI_ACTION_WAIT;
                    return;
                }
                if (get_npc()->bTeam == gbPlayerNum)
                {
                    if (environment.get_tactical_status().fAutoBandageMode)
                    {
                        get_npc()->aiData.bAction = DecideAutoBandage( get_npc() );
                        return;
                    }
                }
            }

            if ( get_npc()->bTeam != MILITIA_TEAM )
            {
               /* if ( !sniperwarning && get_npc()->aiData.bOrders == SNIPER )
                {
                    ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, New113Message[MSG113_WATHCHOUTFORSNIPERS] );
                    sniperwarning = TRUE;
                }*/

                if (!gfBiggunWarning && FindRocketLauncherOrCannon(get_npc()) != NO_SLOT )
                {
					gfBiggunWarning = TRUE;
                    //TODO: don't say this again after reloading a savegame
                    SayQuoteFromAnyBodyInSector( QUOTE_WEARY_SLASH_SUSPUCIOUS );
                }
            }

            get_npc()->aiData.ubAIFlags &= (~AI_CAUTIOUS); // turn off cautious flag
            // if status override is set, bypass RED/YELLOW and go directly to GREEN!
            if ((get_npc()->aiData.bBypassToGreen) && (get_npc()->aiData.bAlertStatus < STATUS_BLACK))
            {
				if (get_npc()->IsTank())
					get_npc()->aiData.bAction = TankDecideActionGreen(get_npc());
				else
					get_npc()->aiData.bAction = DecideActionGreen(get_npc());

                if ( !gfTurnBasedAI )
                {
                    // reset bypass now
                    get_npc()->aiData.bBypassToGreen = 0;
                }
            }
            else
            {
                switch (get_npc()->aiData.bAlertStatus)
                {
                    case STATUS_GREEN:
						if (get_npc()->IsTank())
							get_npc()->aiData.bAction = TankDecideActionGreen(get_npc());
						else
							get_npc()->aiData.bAction = DecideActionGreen(get_npc());
                        break;
                    case STATUS_YELLOW:
						if (get_npc()->IsTank())
							get_npc()->aiData.bAction = TankDecideActionYellow(get_npc());
						else
							get_npc()->aiData.bAction = DecideActionYellow(get_npc());
                        break;
                    case STATUS_RED:
						if (get_npc()->IsTank())
							get_npc()->aiData.bAction = TankDecideActionRed(get_npc());
						else
							get_npc()->aiData.bAction = DecideActionRed(get_npc());
                        break;
                    case STATUS_BLACK:
						if (get_npc()->IsTank())
							get_npc()->aiData.bAction = TankDecideActionBlack(get_npc());
						else
							get_npc()->aiData.bAction = DecideActionBlack(get_npc());
                        break;
                }
            }
			
			if (get_npc()->aiData.bAction >= 0 && get_npc()->aiData.bAction <= AI_ACTION_LAST)
				DebugAI(String("%d decided %s data %d (GridNo %d Team %d AP %d)", get_npc()->ubID, szAction[get_npc()->aiData.bAction], get_npc()->aiData.usActionData, get_npc()->sGridNo, get_npc()->bTeam, get_npc()->bActionPoints));
			else
				DebugAI(String("%d decided Action %d data %d (GridNo %d Team %d AP %d)", get_npc()->ubID, get_npc()->aiData.bAction, get_npc()->aiData.usActionData, get_npc()->sGridNo, get_npc()->bTeam, get_npc()->bActionPoints));			

			DebugAI(AI_MSG_DECIDE, get_npc(), String(""), get_npc()->aiData.bAction);

            DEBUGAIMSG("Deciding for guynum "<<(int)get_npc()->ubID<<" at gridno "<<get_npc()->sGridNo<<", APs "<<get_npc()->bActionPoints<<
                    ", decided action: "<<(int)get_npc()->aiData.bAction<<", data "<<(int)get_npc()->aiData.usActionData);
        }

    } // namespace tactical
} // namespace AI

