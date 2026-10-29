#ifdef PRECOMPILEDHEADERS
	#include "Tactical All.h"
#else
	#include "items.h"
	#include "Action Items.h"
	#include "weapons.h"
	#include "Soldier Control.h"
	#include "overhead.h"
	#include "Handle UI.h"
	#include "Animation Control.h"
	#include "points.h"
	#include "Sound Control.h"
	#include "Isometric Utils.h"
	#include "Animation Data.h"
	#include "Random.h"
	#include "Campaign.h"
	#include "interface.h"
	#include "interface panels.h"
	#include "explosion control.h"
	#include "Keys.h"
	#include "faces.h"

	#include "wcheck.h"
	#include "soldier profile.h"
	#include "SkillCheck.h"
	#include "los.h"
	#include "message.h"
	#include "text.h"

	#include "fov.h"
	#include "MessageBoxScreen.h"

	#include "Interface Control.h"
	#include "ShopKeeper Interface.h"

	#include "GameSettings.h"
	#include "environment.h"
	#include "Auto Resolve.h"
	#include "Interface Items.h"
	#include "Campaign Types.h"
	#include "Game Clock.h"
	#include "strategicmap.h"
	#include "Inventory Choosing.h"
	#include "Soldier macros.h"
	#include "Smell.h"
	#include "lighting.h"
	#include "utilities.h"
	#include "english.h"
	#include "debug control.h"

	#include "math.h"
	// THE_BOB : added for pocket popup definitions
	#include <map>
	#include "popup_definition.h"

	#include "drugs and alcohol.h"
	#include "Food.h"
	#include "opplist.h"
	#include "Sys Globals.h"//dnl ch74 201013
	#include "CampaignStats.h"		// added by Flugente
	#include "Map Information.h"
	#include "ai.h"					// sevenfm
	#include "DisplayCover.h"
#endif

#ifdef JA2UB
#include "Ja25_Tactical.h"
#include "Ja25 Strategic Ai.h"
#endif

//forward declarations of common classes to eliminate includes
class OBJECTTYPE;
class SOLDIERTYPE;

extern UINT32 gMAXMAGAZINES_READ;

#define ANY_MAGSIZE 65535

void SetNewItem( SOLDIERTYPE *pSoldier, UINT8 ubInvPos, BOOLEAN fNewItem );

extern	SOLDIERTYPE *gpItemDescSoldier;
// HEADROCK HAM 5: We need access to these values for item transformation purposes
extern BOOLEAN			gfItemDescObjectIsAttachment;
extern OBJECTTYPE		*gpItemDescObject;
extern OBJECTTYPE		*gpItemDescPrevObject;
extern OBJECTTYPE		*gpItemDescOrigAttachmentObject;
extern OBJECTTYPE		gCloneItemDescObject;
extern BOOLEAN			fShowMapInventoryPool;
extern UINT32 guiCurrentItemDescriptionScreen;
extern BOOLEAN AutoPlaceObjectInInventoryStash( OBJECTTYPE *pItemPtr, INT32 sGridNo, INT8 ubLevel );
// HEADROCK HAM 5: Also need these to trigger Map Inventory changes appropriately.
extern BOOLEAN fMapInventoryZoom;
// HEADROCK HAM 5: And this, for checking whether an item is in the pool.
extern std::vector<WORLDITEM> pInventoryPoolList;
// sevenfm:
extern INT8 gbCurrentRainIntensity;

UINT16 OldWayOfCalculatingScopeBonus(SOLDIERTYPE *pSoldier);
// weight units are 100g each

////////////////////////////////////////////////////////////////////////////
//ATE: When adding new items, make sure to update text.c with text description
///////////////////////////////////////////////////////////////////////////

INVTYPE Item[MAXITEMS]; //=

// NB hack:  if an item appears in this array with an item class of IC_MISC,
// it is a slot used for noting the skill check required for a merge or multi-item attachment

AttachmentInfoStruct AttachmentInfo[MAXITEMS+1];// =

//WarmSteel - For the New Attachment System
AttachmentSlotStruct AttachmentSlots[MAXITEMS+1];
ItemReplacementStruct ItemReplacement[MAXATTACHMENTS];

AttachmentStruct Attachment[MAXATTACHMENTS];// =
std::multimap<UINT16, AttachmentStruct> AttachmentBackmap;  // key is itemId

UINT16 Launchable[MAXITEMS+1][2];// =
UINT16 CompatibleFaceItems[MAXITEMS+1][2];// =
UINT16 Merge[MAXITEMS + 1][MERGE_DATA_SIZE];
ComboMergeInfoStruct AttachmentComboMerge[MAXITEMS+1];// =

// HEADROCK HAM 5: Item Transformations table, containing all possible Transformations.
TransformInfoStruct Transform[MAXITEMS+1];

// Flugente: in order not to loop over MAXATTACHMENTS entries in IncompatibleAttachments[] if we only have a few thousand, remember the actual number read in
UINT32 extern gINCOMPATIBLEATTACHMENTS_READ;

UINT16 IncompatibleAttachments[MAXATTACHMENTS][2];// =

UINT16 ReplacementGuns[][2] =
{
	{ BARRACUDA,		DESERTEAGLE	},
	{ M1911,				GLOCK_17		},
	{ GLOCK_18,			BERETTA_93R	},
	{ BERETTA_92F,	GLOCK_17		},
	{ TYPE85,				BERETTA_93R	},
	{	THOMPSON,			MP5K				},
	{	MP53,					MP5K				},
	{ SPAS15,				M870				},
	{ AKSU74,				MAC10				},
	{ SKS,					MINI14			},
	{	AKM,					G41					},
	{ G3A3,					G41					},
	{ AK74,					G41					},
	{	DRAGUNOV,			M24					},
	{ FAMAS,				M14					},
	{ AUG,					C7					},
	{ RPK74,				MINIMI			},
	{ HK21E,				MINIMI			},
	{ 0,						0						}
};

// CHRISL: Structure Definitions for new inventory system items.
std::vector<LBETYPE> LoadBearingEquipment;

std::vector<POCKETTYPE> LBEPocketType;
//TODO make the indices of this a define, because I do not know what is a large pocket (I guess 3)

// THE_BOB : added for pocket popup definitions
std::map<UINT8,popupDef> LBEPocketPopup;

DRUGTYPE Drug[DRUG_TYPE_MAX];

FOODTYPE Food[FOOD_TYPE_MAX];

SECTOR_EXT_DATA	SectorExternalData[256][4];

BOOLEAN ItemIsLegal( UINT16 usItemIndex, BOOLEAN fIgnoreCoolness )
{
	if ( Item[usItemIndex].ubCoolness == 0 && !fIgnoreCoolness )
		return FALSE;

	//if the user has selected the reduced gun list
	if (!gGameOptions.fGunNut && Item[usItemIndex].biggunlist)
	{
		return(FALSE);
	}

	if ( gGameOptions.ubGameStyle != STYLE_SCIFI && Item[usItemIndex].scifi )
	{
		return FALSE;
	}

	// CHRISL: Restrict system specific items
	if (UsingNewInventorySystem())
	{
		if(Item[usItemIndex].ItemSize == gGameExternalOptions.guiOIVSizeNumber)
			return FALSE;
	}
	else
	{
		if(Item[usItemIndex].usItemClass == IC_LBEGEAR || Item[usItemIndex].newinv)
			return FALSE;
	}

	if(UsingNewAttachmentSystem()==true && Item[usItemIndex].ubAttachmentSystem == 1)
	{
		return FALSE;
	} 
	else if(UsingNewAttachmentSystem()==false && Item[usItemIndex].ubAttachmentSystem == 2)
	{
		return FALSE;
	}

	// sevenfm: restrict food items
	if (!gGameOptions.fFoodSystem && Item[usItemIndex].foodtype && !Item[usItemIndex].canteen && !Item[usItemIndex].drugtype)
	{
		return FALSE;
	}

	// sevenfm: cleaning kits
	if (!gGameExternalOptions.fDirtSystem && HasItemFlag(usItemIndex, CLEANING_KIT))
	{
		return FALSE;
	}

	// sevenfm: NADA items
	if (Item[usItemIndex].usItemClass == 0)
		return FALSE;

	return(TRUE);
}

// also used for ammo
BOOLEAN ExtendedGunListGun( UINT16 usGun )
{
	return( (Item[ usGun ].biggunlist ) != 0 );
}

UINT16 StandardGunListReplacement( UINT16 usGun )
{
	UINT8 ubLoop;

	if ( ExtendedGunListGun( usGun ) )
	{
		ubLoop = 0;
		while ( ReplacementGuns[ ubLoop ][ 0 ] != 0 )
		{
			if ( ReplacementGuns[ ubLoop ][ 0 ] == usGun )
			{
				return( ReplacementGuns[ ubLoop ][ 1 ] );
			}
			ubLoop++;
		}
		// ERROR!
		AssertMsg( 0, String( "Extended gun with no replacement %d, CC:0", usGun ) );
		return( NOTHING );
	}
	else
	{
		return( NOTHING );
	}
}

/*UINT16 StandardGunListAmmoReplacement( UINT16 usAmmo )
{
	UINT8 ubLoop;

	if ( ExtendedGunListGun( usAmmo ) )
	{
		ubLoop = 0;
		while ( ReplacementAmmo[ ubLoop ][ 0 ] != 0 )
		{
			if ( ReplacementAmmo[ ubLoop ][ 0 ] == usAmmo )
			{
				return( ReplacementAmmo[ ubLoop ][ 1 ] );
			}
			ubLoop++;
		}
		// ERROR!
		AssertMsg( 0, String( "Extended gun with no replacement %d, CC:0", usAmmo ) );
		return( NOTHING );
	}
	else
	{
		return( NOTHING );
	}
}*/

BOOLEAN WeaponInHand( SOLDIERTYPE * pSoldier )
{
	if ( Item[pSoldier->inv[HANDPOS].usItem].usItemClass & (IC_WEAPON | IC_THROWN) && pSoldier->inv[HANDPOS].exists() == true)
	{
		if (Item[pSoldier->inv[HANDPOS].usItem].fingerprintid )
		{
			if (pSoldier->inv[HANDPOS][0]->data.ubImprintID != NO_PROFILE)
			{
				if (pSoldier->ubProfile != NO_PROFILE && pSoldier->ubProfile != MADLAB )
				{
					if (pSoldier->inv[HANDPOS][0]->data.ubImprintID != pSoldier->ubProfile)
					{
						return( FALSE );
					}
				}
				else
				{
					if (pSoldier->inv[HANDPOS][0]->data.ubImprintID != (NO_PROFILE + 1) )
					{
						return( FALSE );
					}
				}
			}
		}
		if (pSoldier->inv[HANDPOS][0]->data.gun.bGunStatus >= USABLE)
		{
			return( TRUE );
		}
	}
	// return -1 or some "broken" value if weapon is broken?
	return( FALSE );
}

bool FitsInSmallPocket(OBJECTTYPE* pObj)
{
	if (UsingNewInventorySystem() == true) {
		return true;
	}
	return Item[pObj->usItem].ubPerPocket != 0;
}

bool IsBackpackSlot(INT8 bSlot)
{
	std::vector<INT8> pocketKey;
	GetLBESlots(BPACKPOCKPOS, pocketKey);

	for(UINT32 loop = 0; loop < pocketKey.size(); loop++)
	{
		if(pocketKey[loop] == bSlot)
			return true;
	}

	return false;
}

bool IsLBESlot(INT8 bSlot, UINT32 LBEType)
{
	std::vector<INT8> pocketKey;
	GetLBESlots(LBEType, pocketKey);

	for(UINT32 loop = 0; loop < pocketKey.size(); loop++)
	{
		if(pocketKey[loop] == bSlot)
			return true;
	}

	return false;
}

// CHRISL: New definition for this function so that we can look at soldiers LBE pockets.
UINT8 ItemSlotLimit( OBJECTTYPE * pObject, INT16 bSlot, SOLDIERTYPE *pSoldier, BOOLEAN cntAttach )
{
	UINT8	ubSlotLimit;
	UINT8	pIndex;
	UINT16	usItem, iSize;
	UINT16	sSize = 0;

	//doesn't matter what inventory method we are using
	usItem = pObject->usItem;
	
	// WANNE: This fixes the stacking problem of silver nuggets!!
	if (Item[usItem].usItemClass == IC_MONEY)
	{
		//need to have money "stackable" in all slots to trick it into merging
		return 2;
	}

	//doesn't matter what inventory method we are using, "body" slots always have a capacity of 1
	if (bSlot < BODYPOSFINAL)
		return 1;

	ubSlotLimit = min(Item[usItem].ubPerPocket, MAX_OBJECTS_PER_SLOT);

	if (bSlot == STACK_SIZE_LIMIT) 
	{
		//if it is stack size limit we want it to be a big slot or a vehicle slot
		if (UsingNewInventorySystem() == false)
			return (max(1, ubSlotLimit));
		else if(pSoldier != NULL && (pSoldier->flags.uiStatusFlags & SOLDIER_VEHICLE))
			return (max(1, LBEPocketType[VEHICLE_POCKET_TYPE].ItemCapacityPerSize[__min(gGameExternalOptions.guiMaxItemSize,Item[pObject->usItem].ItemSize)])); //JMich
		else
			return (max(1, min(255,LBEPocketType[VEHICLE_POCKET_TYPE].ItemCapacityPerSize[__min(gGameExternalOptions.guiMaxItemSize,Item[pObject->usItem].ItemSize)]*4)));
	}

	if (UsingNewInventorySystem() == false) 
	{
		if (ubSlotLimit == 0 && bSlot < BIGPOCKFINAL) 
		{
			return 1;
		}
		if (bSlot >= BIGPOCKFINAL && ubSlotLimit > 1)
		{
			ubSlotLimit /= 2;
		}
		return( ubSlotLimit );
	}

	//UsingNewInventorySystem == true

	// sevenfm: use unified function (with disabled Assert checks)
	pIndex = GetPocketIndex(pSoldier, bSlot);
	/*if (pSoldier != NULL && (pSoldier->flags.uiStatusFlags & SOLDIER_VEHICLE)) {
		pIndex = VEHICLE_POCKET_TYPE;
	}
	// IC Group Slots
	else if (bSlot == GUNSLINGPOCKPOS) {
		pIndex = GUNSLING_POCKET_TYPE;
	}
	else if (bSlot == KNIFEPOCKPOS) {
		pIndex = KNIFE_POCKET_TYPE;
	}
	else {
		Assert(icLBE[bSlot] != -1 && icDefault[bSlot] != -1 && icPocket[bSlot] != -1);
		//find the class of the LBE, then find what size the pocket of the slot in the LBE is
		if (pSoldier == NULL || pSoldier->inv[icLBE[bSlot]].exists() == false) {
			pIndex = LoadBearingEquipment[Item[icDefault[bSlot]].ubClassIndex].lbePocketIndex[icPocket[bSlot]];
		}
		else {
			pIndex = LoadBearingEquipment[Item[pSoldier->inv[icLBE[bSlot]].usItem].ubClassIndex].lbePocketIndex[icPocket[bSlot]];
			if (pIndex == 0 && LoadBearingEquipment[Item[pSoldier->inv[icLBE[bSlot]].usItem].ubClassIndex].lbePocketsAvailable & (UINT16)pow((double)2, icPocket[bSlot])){
				pIndex = GetPocketFromAttachment(&pSoldier->inv[icLBE[bSlot]], icPocket[bSlot]);
			}
		}
	}*/

	//We need to actually check the size of the largest stored item as well as the size of the current item
	if(cntAttach == TRUE)
	{
		iSize = CalculateItemSize(pObject);
		if(pSoldier != NULL && pSoldier->inv[bSlot].usItem == pObject->usItem)
		{
			sSize = CalculateItemSize(&pSoldier->inv[bSlot]);
			if(LBEPocketType[pIndex].ItemCapacityPerSize[sSize] < LBEPocketType[pIndex].ItemCapacityPerSize[iSize])
				iSize = sSize;
		}
	}
	else
	{
		iSize = Item[pObject->usItem].ItemSize;
	}	

	iSize = min(iSize,gGameExternalOptions.guiMaxItemSize);
	ubSlotLimit = LBEPocketType[pIndex].ItemCapacityPerSize[iSize];

	//this could be changed, we know guns are physically able to stack
	//if ( iSize < 10 && ubSlotLimit > 1)
	//	ubSlotLimit = 1;

	if(LBEPocketType[pIndex].pRestriction != 0 && !(LBEPocketType[pIndex].pRestriction & Item[usItem].usItemClass)) 
	{
		return 0;
	}

	return( ubSlotLimit );
}

UINT8 GetPocketIndex(SOLDIERTYPE* pSoldier, INT8 bSlot)
{
	Assert(pSoldier);
	Assert(bSlot >= 0 && bSlot < NUM_INV_SLOTS);

	UINT8 ubPocketIndex = 0;

	if (pSoldier != NULL && (pSoldier->flags.uiStatusFlags & SOLDIER_VEHICLE)) 
	{
		ubPocketIndex = VEHICLE_POCKET_TYPE;
	}
	// IC Group Slots
	else if (bSlot == GUNSLINGPOCKPOS) 
	{
		ubPocketIndex = GUNSLING_POCKET_TYPE;
	}
	else if (bSlot == KNIFEPOCKPOS) 
	{
		ubPocketIndex = KNIFE_POCKET_TYPE;
	}
	else if (icLBE[bSlot] != -1 && icDefault[bSlot] != -1 && icPocket[bSlot] != -1)
	{
		//find the class of the LBE, then find what size the pocket of the slot in the LBE is
		if (pSoldier == NULL || pSoldier->inv[icLBE[bSlot]].exists() == false) 
		{
			ubPocketIndex = LoadBearingEquipment[Item[icDefault[bSlot]].ubClassIndex].lbePocketIndex[icPocket[bSlot]];
		}
		else 
		{
			ubPocketIndex = LoadBearingEquipment[Item[pSoldier->inv[icLBE[bSlot]].usItem].ubClassIndex].lbePocketIndex[icPocket[bSlot]];
			if (ubPocketIndex == 0 && LoadBearingEquipment[Item[pSoldier->inv[icLBE[bSlot]].usItem].ubClassIndex].lbePocketsAvailable & (UINT16)pow((double)2, icPocket[bSlot]))
			{
				ubPocketIndex = GetPocketFromAttachment(&pSoldier->inv[icLBE[bSlot]], icPocket[bSlot]);
			}
		}
	}

	return ubPocketIndex;
};

UINT32 MoneySlotLimit( INT8 bSlot )
{
	if ( bSlot >= BIGPOCKFINAL )	/* CHRISL */
	{
		return( MAX_MONEY_PER_SLOT / 2 );
	}
	else
	{
		return( MAX_MONEY_PER_SLOT );
	}
}

INT8 FindBestWeaponIfCurrentIsOutOfRange(SOLDIERTYPE * pSoldier, INT8 bCurrentWeaponIndex, UINT16 bWantedRange)
{
	//assuming current weapon is in the handpos
	if (GunRange(&pSoldier->inv[bCurrentWeaponIndex], pSoldier) >= bWantedRange)  // SANDRO - added argument
	{
		//our current weapon is good enough
		return( bCurrentWeaponIndex );
	}

	UINT16 range;
	UINT16 bestRange = 0;
	INT8 bestWeaponThatMeetsRange = 0;
	INT8 secondBestWeapon = 0;
	//search for weapons that meet the range, then sort by damage.
	//if there are no weapons that meet the range, then use the longest range we can find
	INT8 invsize = (INT8)pSoldier->inv.size();
	for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		//if this is a weapon
		if (Item[pSoldier->inv[bLoop].usItem].usItemClass & (IC_WEAPON | IC_THROWN) && pSoldier->inv[bLoop].exists() == true)
		{
			range = GunRange(&pSoldier->inv[bLoop], pSoldier);  // SANDRO - added argument
			if (range >= bWantedRange)
			{
				if (bestWeaponThatMeetsRange == 0)
				{
					bestWeaponThatMeetsRange = bLoop;
				}
				else if (GetDamage(&pSoldier->inv[bLoop]) > GetDamage(&pSoldier->inv[bestWeaponThatMeetsRange]))
				{
					//does this weapon have more damage?
					bestWeaponThatMeetsRange = bLoop;
				}
			}
			else if (range > bestRange)
			{
				//weapon does not meet range, but it could be better anyways
				bestRange = range;
				secondBestWeapon = bLoop;
			}
			else if (range == bestRange)
			{
				//weapon ties with secondBestWeapon's range
				if (secondBestWeapon == 0)
				{
					//this if can happen if range of bLoop is 0!
					secondBestWeapon = bLoop;
				}
				else if (GetDamage(&pSoldier->inv[bLoop]) > GetDamage(&pSoldier->inv[secondBestWeapon]))
				{
					secondBestWeapon = bLoop;
				}
			}
		}
	}

	if (bestWeaponThatMeetsRange)
	{
		return bestWeaponThatMeetsRange;
	}
	else if (secondBestWeapon)
	{
		return secondBestWeapon;
	}
	return( bCurrentWeaponIndex );
}

INT8 FindMetalDetectorInHand(SOLDIERTYPE * pSoldier, BOOLEAN fBatteries)
{
	if ( (&(pSoldier->inv[HANDPOS] ))->exists() && 
		Item[pSoldier->inv[HANDPOS].usItem].metaldetector &&
		(!fBatteries || !Item[pSoldier->inv[HANDPOS].usItem].needsbatteries || FindAttachedBatteries(&(pSoldier->inv[HANDPOS]))))
	{
		return( HANDPOS );
	}
	
	if ( (&(pSoldier->inv[SECONDHANDPOS] ))->exists() && 
		Item[pSoldier->inv[SECONDHANDPOS].usItem].metaldetector &&
		(!fBatteries || !Item[pSoldier->inv[HANDPOS].usItem].needsbatteries || FindAttachedBatteries(&(pSoldier->inv[HANDPOS]))))
	{
		return( SECONDHANDPOS );
	}

	return( NO_SLOT );
}

INT8 FindKnifeInHand( SOLDIERTYPE * pSoldier )
{
	if( (&(pSoldier->inv[HANDPOS] ))->exists() &&
		(Item[pSoldier->inv[HANDPOS].usItem].usItemClass == IC_BLADE || Item[pSoldier->inv[HANDPOS].usItem].usItemClass == IC_THROWING_KNIFE) )
	{
		return( HANDPOS );
	}

	if( (&(pSoldier->inv[SECONDHANDPOS] ))->exists() &&
		(Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass == IC_BLADE || Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass == IC_THROWING_KNIFE) )
	{
		return( SECONDHANDPOS );
	}

	return( NO_SLOT );
}


INT8 FindLockBomb( SOLDIERTYPE * pSoldier )
{
	INT8 invsize = (INT8)pSoldier->inv.size();
	for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (Item[pSoldier->inv[bLoop].usItem].lockbomb && pSoldier->inv[bLoop].exists() == true)
		{
			return( bLoop );
		}
	}
	return( NO_SLOT );
}

INT8 FindUsableObj( SOLDIERTYPE * pSoldier, UINT16 usItem )
{
	INT8 invsize = (INT8)pSoldier->inv.size();
	for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if ( pSoldier->inv[bLoop].usItem == usItem
			&& pSoldier->inv[bLoop].exists() == true
			&& pSoldier->inv[bLoop][0]->data.objectStatus >= USABLE )
		{
			return( bLoop );
		}
	}
	return( NO_SLOT );
}


INT8 FindUsableItemWithFlag(SOLDIERTYPE * pSoldier, UINT32 iFlag)
{
	INT8 invsize = (INT8)pSoldier->inv.size();
	for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() && 
			HasItemFlag(pSoldier->inv[bLoop].usItem, iFlag) &&
			pSoldier->inv[bLoop][0]->data.objectStatus >= USABLE)
		{
			return(bLoop);
		}
	}
	return(NO_SLOT);
}


INT8 FindObjExcludingSlot( SOLDIERTYPE * pSoldier, UINT16 usItem, INT8 bExcludeSlot )
{
	INT8 invsize = (INT8)pSoldier->inv.size();
	for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (bLoop == bExcludeSlot)
		{
			continue;
		}
		if (pSoldier->inv[bLoop].usItem == usItem && pSoldier->inv[bLoop].exists() == true)
		{
			return( bLoop );
		}
	}
	return( NO_SLOT );
}

INT8 FindObj( SOLDIERTYPE * pSoldier, UINT16 usItem, INT8 bLower, INT8 bUpper )
{
	INT8	bLoop;

	for (bLoop = bLower; bLoop < bUpper; bLoop++)
	{
		//CHRISL: If in NIV, in combat and backpack is closed, don't look inside
		if(UsingNewAttachmentSystem() == true && (gTacticalStatus.uiFlags & INCOMBAT) && IsBackpackSlot(bLoop) == true && pSoldier->flags.ZipperFlag == FALSE)
			continue;

		//CHRISL: If we check exists() then we can't search for an empty pocket with this function, which is done.
		if (pSoldier->inv[bLoop].usItem == usItem/* && pSoldier->inv[bLoop].exists() == true*/)
		{
			return( bLoop );
		}
	}
	return( ITEM_NOT_FOUND );
}

INT8 FindObjInObjRange( SOLDIERTYPE * pSoldier, UINT16 usItem1, UINT16 usItem2 )
{
	UINT16	usTemp;

	if (usItem1 > usItem2 )
	{
		// swap the two...
		usTemp = usItem2;
		usItem2 = usItem1;
		usItem1 = usTemp;
	}

	INT8 invsize = (INT8)pSoldier->inv.size();
	for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		usTemp = pSoldier->inv[bLoop].usItem;
		if ( usTemp >= usItem1 && usTemp <= usItem2 && pSoldier->inv[bLoop].exists() == true)
		{
			return( bLoop );
		}
	}

	return( ITEM_NOT_FOUND );
}


INT8 FindObjClass( SOLDIERTYPE * pSoldier, 	UINT32 usItemClass )
{
	INT8 invsize = (INT8)pSoldier->inv.size();
	for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (Item[pSoldier->inv[bLoop].usItem].usItemClass & usItemClass && pSoldier->inv[bLoop].exists() == true)
		{
			return( bLoop );
		}
	}
	return( NO_SLOT );
}

INT8 FindAIUsableObjClass(SOLDIERTYPE * pSoldier, UINT32 usItemClass, BOOLEAN fSidearm)
{
	// finds the first object of the specified class which does NOT have
	// the "unusable by AI" flag set.
	// uses & rather than == so that this function can search for any weapon

	INT8 bBestSlot = NO_SLOT;
	INT8 bInvSize = (INT8)pSoldier->inv.size();

	for (INT8 bLoop = 0; bLoop < bInvSize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists())
		{
			if ((Item[pSoldier->inv[bLoop].usItem].usItemClass & usItemClass) &&
				!(pSoldier->inv[bLoop].fFlags & OBJECT_AI_UNUSABLE) &&
				(pSoldier->inv[bLoop][0]->data.objectStatus >= USABLE))
			{
				// Do not consider tank cannons or rocket launchers to be "guns" for AI
				if (usItemClass == IC_GUN && EXPLOSIVE_GUN(pSoldier->inv[bLoop].usItem))
				{
					continue;
				}
				// if not searching for gun, return first usable of required type
				if (usItemClass != IC_GUN)
				{
					bBestSlot = bLoop;
					break;
				}
				// we are searching for gun
				if (bBestSlot == NO_SLOT ||		// haven't found any gun yet
					// by default, search for weapon with longest range
					!fSidearm && Weapon[pSoldier->inv[bLoop].usItem].usRange > Weapon[pSoldier->inv[bBestSlot].usItem].usRange ||
					// when searching for sidearm, find fastest weapon
					fSidearm && Weapon[Item[pSoldier->inv[bLoop].usItem].ubClassIndex].ubReadyTime + BaseAPsToShootOrStab(APBPConstants[DEFAULT_APS], APBPConstants[DEFAULT_AIMSKILL], &pSoldier->inv[bLoop], pSoldier) <
					Weapon[Item[pSoldier->inv[bBestSlot].usItem].ubClassIndex].ubReadyTime + BaseAPsToShootOrStab(APBPConstants[DEFAULT_APS], APBPConstants[DEFAULT_AIMSKILL], &pSoldier->inv[bBestSlot], pSoldier))
				{
					bBestSlot = bLoop;
				}
			}
		}
	}
	return bBestSlot;
}

INT8 FindAIUsableObjClassWithin( SOLDIERTYPE * pSoldier, 	UINT32 usItemClass, INT8 bLower, INT8 bUpper )
{
	INT8 bLoop;

	// This is for the AI only so:
	// Do not consider tank cannons or rocket launchers to be "guns"

	for (bLoop = bLower; bLoop < bUpper; bLoop++)
	{
		if (pSoldier->inv[bLoop].exists() == true) {
			if ( (Item[pSoldier->inv[bLoop].usItem].usItemClass & usItemClass) && !(pSoldier->inv[bLoop].fFlags & OBJECT_AI_UNUSABLE) && (pSoldier->inv[bLoop][0]->data.objectStatus >= USABLE ) )
			{
				if ( usItemClass == IC_GUN && EXPLOSIVE_GUN( pSoldier->inv[bLoop].usItem ) )
				{
					continue;
				}
				return( bLoop );
			}
		}
	}
	return( NO_SLOT );
}

INT8 FindEmptySlotWithin( SOLDIERTYPE * pSoldier, INT8 bLower, INT8 bUpper )
{
	INT8	bLoop;

	for (bLoop = bLower; bLoop < bUpper; bLoop++)
	{
		// CHRISL: Only look at valid pockets
		if((UsingNewInventorySystem() == false) && !oldInv[bLoop])
			continue;
		if((pSoldier->flags.uiStatusFlags & SOLDIER_VEHICLE) && !vehicleInv[bLoop])
			continue;
		if (pSoldier->inv[bLoop].exists() == false)
		{
			if (bLoop == SECONDHANDPOS && Item[pSoldier->inv[HANDPOS].usItem].twohanded )
			{
				continue;
			}
			else
			{
				return( bLoop );
			}
		}
	}
	return( ITEM_NOT_FOUND );
}

// for grenade launchers
INT8 FindGLGrenade(SOLDIERTYPE * pSoldier, UINT8 ubGrenadeType)
{
	INT8 invsize = (INT8)pSoldier->inv.size();
	UINT16 usItem;
	UINT8 ubType;

	for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() && 
			Item[pSoldier->inv[bLoop].usItem].glgrenade)
		{
			usItem = pSoldier->inv[bLoop].usItem;
			ubType = (UINT8)Explosive[Item[usItem].ubClassIndex].ubType;

			if (ubGrenadeType == EXPLOSV_ANY_TYPE || ubType == ubGrenadeType)
			{
				return bLoop;
			}			
		}
	}
	return( NO_SLOT );
}

INT8 FindThrowableGrenade( SOLDIERTYPE * pSoldier, UINT8 ubGrenadeType, UINT8 ubMinDamage )
{
	INT8 bLoop;
	BOOLEAN fCheckForFlares = FALSE;
	UINT16 usItem;
	INT8 invsize = (INT8)pSoldier->inv.size();
	UINT16 usDamage;

	// JA2Gold: give some priority to looking for flares when at night
	// this is AI only so we can put in some customization for night
	// sevenfm: use flare only if cannot see opponent
	if( NightLight() &&
		pSoldier->aiData.bOppCnt == 0 &&
		ubGrenadeType == EXPLOSV_ANY_TYPE &&
		Chance(50) )
	{
		fCheckForFlares = TRUE;
	}

	INT8 bFoundSlot = NO_SLOT;
	UINT8 ubType;
	UINT8 ubFoundType;
	
	for (bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists())
		{
			usItem = pSoldier->inv[ bLoop ].usItem;
			ubType = (UINT8)Explosive[ Item[ usItem ].ubClassIndex ].ubType;
			usDamage = Explosive[Item[usItem].ubClassIndex].ubDamage;
			
			if( (Item[ usItem ].usItemClass & IC_GRENADE) &&
				Item[ usItem ].ubCursor == TOSSCURS &&
				GetLauncherFromLaunchable( usItem) == NOTHING &&
				(!Item[ usItem].flare || NightLight()) &&
				usDamage >= ubMinDamage &&
				(ubGrenadeType == EXPLOSV_ANY_TYPE || ubType == ubGrenadeType) )
			{
				// return if flare grenade has priority
				if( fCheckForFlares && Item[ usItem].flare )
				{
					return bLoop;
				}

				// return if found needed type
				if( ubType == ubGrenadeType )
				{
					return bLoop;
				}
				
				// always prefer normal type
				// smoke grenades have low priority
				// mustard gas has priority over tear gas
				// red smoke has lowest priority if artillery strikes not possible
				if( bFoundSlot == NO_SLOT ||
					ubType == EXPLOSV_NORMAL && ubFoundType != EXPLOSV_NORMAL ||
					ubType != EXPLOSV_SMOKE && ubFoundType == EXPLOSV_SMOKE ||
					ubType != EXPLOSV_SIGNAL_SMOKE && ubFoundType == EXPLOSV_SIGNAL_SMOKE && !gSkillTraitValues.fROAllowArtillery ||
					ubType == EXPLOSV_MUSTGAS && ubFoundType == EXPLOSV_TEARGAS)
				{
					bFoundSlot = bLoop;
					ubFoundType = ubType;
				}
			}
		}
	}

	return( bFoundSlot );
}

UINT8 CountThrowableGrenades(SOLDIERTYPE * pSoldier, UINT8 ubGrenadeType, UINT8 ubMinDamage)
{
	INT8 bLoop;
	UINT16 usItem;
	INT8 invsize = (INT8)pSoldier->inv.size();
	UINT16 usDamage;

	INT8 bFoundSlot = NO_SLOT;
	UINT8 ubType;
	//UINT8 ubFoundType;
	UINT8 ubCnt = 0;

	for (bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true)
		{
			usItem = pSoldier->inv[bLoop].usItem;
			ubType = (UINT8)Explosive[Item[usItem].ubClassIndex].ubType;
			usDamage = Explosive[Item[usItem].ubClassIndex].ubDamage;

			if ((Item[usItem].usItemClass & IC_GRENADE) &&
				Item[usItem].ubCursor == TOSSCURS &&
				GetLauncherFromLaunchable(usItem) == NOTHING &&
				usDamage >= ubMinDamage &&
				(ubGrenadeType == EXPLOSV_ANY_TYPE || ubType == ubGrenadeType))
			{
				ubCnt++;
			}
		}
	}

	return ubCnt;
}

INT16 FindAttachmentSlot( OBJECTTYPE* pObj, UINT16 usItem, UINT8 subObject)
{
	if(UsingNewAttachmentSystem()==false || pObj->exists() == false)
		return -1;

	UINT8 loop = 0;
	for(attachmentList::iterator iter = (*pObj)[subObject]->attachments.begin(); iter != (*pObj)[subObject]->attachments.end(); iter++, loop++)
	{
		if(iter->exists() && iter->usItem == usItem)
			return loop;
	}

	return -1;
}

OBJECTTYPE* FindAttachment( OBJECTTYPE * pObj, UINT16 usItem, UINT8 subObject )
{
	if (pObj->exists() == true) {
		for (attachmentList::iterator iter = (*pObj)[subObject]->attachments.begin(); iter != (*pObj)[subObject]->attachments.end(); ++iter) 
		{
			if (iter->exists() && iter->usItem == usItem)
			{
				return &(*iter);
			}
		}
	}
	return( 0 );
}

OBJECTTYPE* FindAttachmentByClass( OBJECTTYPE * pObj, UINT32 uiItemClass, UINT8 subObject )
{
	if (pObj->exists() == true) 
	{
		for (attachmentList::iterator iter = (*pObj)[subObject]->attachments.begin(); iter != (*pObj)[subObject]->attachments.end(); ++iter) 
		{
			// sevenfm: allow searching for several class types
			if ( iter->exists() && Item[iter->usItem].usItemClass & uiItemClass )
			{
				return &(*iter);
			}
		}
	}
	return( 0 );
}

OBJECTTYPE* FindAttachmentByAttachmentClass( OBJECTTYPE * pObj, UINT32 uiAttachmentClass, UINT8 subObject )
{
	if (pObj->exists() == true) 
	{
		for (attachmentList::iterator iter = (*pObj)[subObject]->attachments.begin(); iter != (*pObj)[subObject]->attachments.end(); ++iter) 
		{
			//if (iter->exists() && Item[iter->usItem].attachmentclass == uiAttachmentClass)
			if (iter->exists() && IsAttachmentClass(iter->usItem, uiAttachmentClass))
			{
				return &(*iter);
			}
		}
	}
	return( 0 );
}

INT8 FindLaunchable( SOLDIERTYPE * pSoldier, UINT16 usWeapon )
{
	INT8	bLoop;
	DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("FindLaunchable: weapon=%d",usWeapon));
	INT8 invsize = (INT8)pSoldier->inv.size();
	for (bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true) {
			if ( ValidLaunchable( pSoldier->inv[ bLoop ].usItem , usWeapon ) )
			{
				DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("FindLaunchable: returning slot %d",bLoop));
				return( bLoop );
			}
		}
	}

	DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("FindLaunchable: item not found"));
	return( ITEM_NOT_FOUND );
}

INT8 FindNonSmokeLaunchable( SOLDIERTYPE * pSoldier, UINT16 usWeapon )
{
	INT8	bLoop;
	DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("FindNonSmokeLaunchable: weapon=%d",usWeapon));
	INT8 invsize = (INT8)pSoldier->inv.size();
	for (bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true) {
			if ( ValidLaunchable( pSoldier->inv[ bLoop ].usItem , usWeapon ) && Explosive[Item[pSoldier->inv[ bLoop ].usItem].ubClassIndex].ubType != EXPLOSV_SMOKE )
			{
				DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("FindNonSmokeLaunchable: returning slot %d",bLoop));
				return( bLoop );
			}
		}
	}

	DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("FindNonSmokeLaunchable: item not found, so find items including smoke"));
	return( FindLaunchable(pSoldier,usWeapon) );
}

INT8 FindSmokeLaunchable(SOLDIERTYPE * pSoldier, UINT16 usWeapon)
{
	INT8	bLoop;
	DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("FindSmokeLaunchable: weapon=%d", usWeapon));
	INT8 invsize = (INT8)pSoldier->inv.size();
	for (bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true) {
			if (ValidLaunchable(pSoldier->inv[bLoop].usItem, usWeapon) && Explosive[Item[pSoldier->inv[bLoop].usItem].ubClassIndex].ubType == EXPLOSV_SMOKE)
			{
				DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("FindSmokeLaunchable: returning slot %d", bLoop));
				return(bLoop);
			}
		}
	}

	DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("FindSmokeLaunchable: item not found"));
	return(ITEM_NOT_FOUND);
}

OBJECTTYPE* FindLaunchableAttachment( OBJECTTYPE * pObj, UINT16 usWeapon )
{
	if (pObj->exists() == true)
	{
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists() && ValidLaunchable(iter->usItem, usWeapon))
			{
				return &(*iter);
			}
		}
	}

	return( 0 );
}

//CHRISL: This function attempts to find a legal, single-shot grenade item based on the illegal, multi-shot one we send it
UINT16 FindLegalGrenade(UINT16 usItem)
{
	UINT16	newItem = 0;
	UINT16	usClass = Item[usItem].ubClassIndex;
	if(UsingNewAttachmentSystem()==false)
		return usItem;
	// find matching explosive entry in explosives.xml
	for (UINT16 loop = 0; loop < MAXITEMS + 1; loop++)
	{
		if(loop == usClass)
			continue;
		if (Explosive[usClass].ubType == Explosive[loop].ubType && 
			Explosive[usClass].ubDamage == Explosive[loop].ubDamage && 
			Explosive[usClass].ubStunDamage == Explosive[loop].ubStunDamage &&
			Explosive[usClass].ubRadius == Explosive[loop].ubRadius	&& 
			Explosive[usClass].ubVolume == Explosive[loop].ubVolume && 
			Explosive[usClass].ubVolatility == Explosive[loop].ubVolatility && 
			Explosive[usClass].ubAnimationID == Explosive[loop].ubAnimationID && 
			Explosive[usClass].ubDuration == Explosive[loop].ubDuration	&& 
			Explosive[usClass].ubStartRadius == Explosive[loop].ubStartRadius && 
			Explosive[loop].ubMagSize == 1)
		{
				newItem = loop;
				break;
		}
		if(Explosive[loop].uiIndex == 0 && loop > 0)
			break;
	}
	// find item ID that uses found explosives entry
	if(newItem > 0)
	{
		for (UINT16 loop = 1; loop < gMAXITEMS_READ; loop++)
		{
			if(Item[loop].uiIndex == 0)
				break;

			if(Item[loop].usItemClass & IC_GRENADE && Item[loop].ubClassIndex == newItem)
			{
				return Item[loop].uiIndex;
			}
		}
	}
	return usItem;
}

OBJECTTYPE* FindNonSmokeLaunchableAttachment( OBJECTTYPE * pObj, UINT16 usWeapon )
{

	if (pObj->exists() == true) {
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && ValidLaunchable(iter->usItem, usWeapon) && Explosive[Item[iter->usItem].ubClassIndex].ubType != EXPLOSV_SMOKE)
			{
				return &(*iter);
			}
		}
	}

	return( FindLaunchableAttachment(pObj,usWeapon) );
}

OBJECTTYPE* FindSmokeLaunchableAttachment(OBJECTTYPE * pObj, UINT16 usWeapon)
{

	if (pObj->exists() == true) {
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && ValidLaunchable(iter->usItem, usWeapon) && Explosive[Item[iter->usItem].ubClassIndex].ubType == EXPLOSV_SMOKE)
			{
				return &(*iter);
			}
		}
	}

	//return(FindLaunchableAttachment(pObj, usWeapon));
	return 0;
}

//Simple check to see if the item has any attachments
//Madd: if there are only hidden attachments this will now return false, so the asterisk won't be display if the hiddenAttachment tag is true
BOOLEAN ItemHasAttachments( OBJECTTYPE * pObj, SOLDIERTYPE * pSoldier, UINT8 iter )
{
	BOOLEAN attachmentHidden = TRUE;
	if (pObj->exists() == true) {
		if(pSoldier != NULL){
			for (iter = 0; iter != pObj->objectStack.size(); ++iter) 
			{
				if((*pObj)[iter]->AttachmentListSize() > 0)
				{
					for(attachmentList::iterator att = (*pObj)[iter]->attachments.begin(); att != (*pObj)[iter]->attachments.end(); ++att)
					{
						if ( att->exists() && att->usItem != 0 && !Item[att->usItem].hiddenattachment )
						{
							attachmentHidden = FALSE;
							break;
						}
					}
					return !attachmentHidden;
				}
			}
		}
		else
		{
			if((*pObj)[iter]->AttachmentListSize() > 0)
			{
				for(attachmentList::iterator att = (*pObj)[iter]->attachments.begin(); att != (*pObj)[iter]->attachments.end(); ++att)
				{
					if ( att->exists() && att->usItem != 0 && !Item[att->usItem].hiddenattachment )
					{
						attachmentHidden = FALSE;
						break;
					}
				}
				return !attachmentHidden;
			}
		}
	}
	return FALSE;
}

// Determine if it is possible to add this attachment to the CLASS of this item
// (i.e. to any item in the class)
BOOLEAN ValidAttachmentClass( UINT16 usAttachment, UINT16 usItem )
{
	INT32 iLoop = 0;
	while( 1 )
	{
		// see comment for AttachmentInfo array for why we skip IC_NONE
		if ( AttachmentInfo[ iLoop ].uiItemClass != IC_NONE )
		{
			if ( AttachmentInfo[ iLoop ].usItem == usAttachment )
			{
				if ( AttachmentInfo[ iLoop ].uiItemClass == Item[ usItem ].usItemClass )
				{
					return( TRUE );
				}
			}
		}
		iLoop++;
		if (AttachmentInfo[iLoop].usItem == 0)
		{
			// end of the array
			break;
		}
	}
	return( FALSE );
}

INT32 GetAttachmentInfoIndex( UINT16 usItem )
{
	INT32 iLoop = 0;

	while( 1 )
	{
		if ( AttachmentInfo[ iLoop ].usItem == usItem )
		{
			return( (INT8) iLoop );
		}
		iLoop++;
		if (AttachmentInfo[iLoop].usItem == 0)
		{
			// end of the array
			break;
		}
	}
	return( -1 );
}

//Determine if it is possible to add this attachment to the item.
BOOLEAN ValidAttachment( UINT16 usAttachment, UINT16 usItem, UINT8 * pubAPCost )
{
	//INT32 iLoop = 0;
	if (pubAPCost) 
	{
		*pubAPCost = (UINT8)APBPConstants[AP_RELOAD_GUN]; //default value
	}

	//Madd: all guns can be attached to tripwires
	if ( Item[usItem].tripwire && Item[usAttachment].usItemClass & IC_GUN )
		return TRUE;

	if( gGameExternalOptions.bAllowExplosiveAttachments )
	{
		// sevenfm: can attach rubber band to any grenade with normal type explosion
		/*if ( (usAttachment == ELASTIC || usAttachment == STRING) && 
			Item[usItem].usItemClass & IC_GRENADE &&
			Explosive[Item[usItem].ubClassIndex].ubType == EXPLOSV_NORMAL )
		{
			return TRUE;
		}
		// if there's rubber band attached, allow attaching another grenade
		if ( (usItem == ELASTIC || usItem == STRING) &&
			(Item[usAttachment].usItemClass & IC_GRENADE && Explosive[Item[usAttachment].ubClassIndex].ubVolatility > 0 || 
			Item[usAttachment].usBuddyItem != 0 && Item[ Item[usAttachment].usBuddyItem ].usItemClass & IC_GRENADE) )
		{
			return TRUE;
		}*/
		// can attach duct tape to grenade of normal type
		if (usAttachment == DUCT_TAPE &&
			Item[usItem].usItemClass & IC_GRENADE &&
			Explosive[Item[usItem].ubClassIndex].ubType == EXPLOSV_NORMAL)
		{
			return TRUE;
		}
		// can attach duct tape to any explosives of normal type
		if ( usAttachment == DUCT_TAPE && 
			Item[usItem].usItemClass & IC_BOMB &&
			!Item[usItem].tripwire &&
			Explosive[Item[usItem].ubClassIndex].ubType == EXPLOSV_NORMAL )
		{
			return TRUE;
		}
		// can attach explosives to duct tape
		if ( usItem == DUCT_TAPE &&
			(Item[usAttachment].usItemClass & (IC_GRENADE | IC_BOMB) && Explosive[Item[usAttachment].ubClassIndex].ubVolatility > 0 ||
			Item[usAttachment].usBuddyItem != 0 && Item[Item[usAttachment].usBuddyItem].usItemClass & (IC_GRENADE | IC_BOMB) ) )
		{
			return TRUE;
		}
		// can attach tripwire activated item to tripwire
		if ( Item[usItem].tripwire  && Item[usAttachment].tripwireactivation && !Item[usAttachment].tripwire )
		{
			return TRUE;
		}
	}

	//Madd: Common Attachment Framework
	if ( IsAttachmentPointAvailable(usItem, usAttachment))
	{
		if (pubAPCost)
			*pubAPCost = Item[usAttachment].ubAttachToPointAPCost;
		return TRUE;
	}

	// look for the section of the array pertaining to this attachment...
	UINT32 startIndex = 0, endIndex = 0;
	if (FindAttachmentRange(usAttachment, &startIndex, &endIndex) == FALSE)
		return FALSE;

	// now look through this section for the item in question
	for (UINT32 iLoop = startIndex; iLoop <= endIndex; iLoop++)
	{
		if (Attachment[iLoop].itemIndex == usItem)
		{
			if (UsingNewAttachmentSystem() || Attachment[iLoop].NASOnly != 1)
			{
				if (pubAPCost)
					*pubAPCost = (UINT8)Attachment[iLoop].APCost; //Madd: get ap cost of attaching items :)
			}
			return TRUE;
		}
	}
	return FALSE;
}

BOOLEAN ValidAttachment( UINT16 usAttachment, OBJECTTYPE *pObj, UINT8 *pubAPCost, UINT8 subObject, std::vector<UINT16> usAttachmentSlotIndexVector)
{
	if (pObj->exists() == false)
		return FALSE;

	// shadooow: efficiency check, we are passing all kinds of items into this function that are not necessary attachments at all
	if (!Item[usAttachment].attachment && 
		!Item[usAttachment].attachmentclass &&
		!Item[usAttachment].nasAttachmentClass &&
		!Item[usAttachment].hiddenaddon &&
		!((Item[usAttachment].usItemClass & IC_GUN) && Item[pObj->usItem].tripwire))
		return FALSE;

	if(UsingNewAttachmentSystem()==true)
	{
		UINT16 usSlotIndex = 0;
		BOOLEAN foundValidAttachment = FALSE;
		UINT16 usLoop = 0;

		//It's possible we've entered this function without being passed the usAttachmentSlotIndexVector parameter
		if(usAttachmentSlotIndexVector.empty())
			usAttachmentSlotIndexVector = GetItemSlots(pObj);

		//Still no slots means nothing will ever be valid
		if(usAttachmentSlotIndexVector.empty())
			return FALSE;

		//Madd: Common Attachment Framework
		if (pubAPCost && IsAttachmentPointAvailable(pObj, subObject, usAttachment))
		{
			*pubAPCost = Item[usAttachment].ubAttachToPointAPCost;

			return TRUE;
		}
		else
		{
			//Check if the attachment is valid with the main item
			foundValidAttachment = (ValidAttachment(usAttachment, pObj->usItem, pubAPCost) || ValidLaunchable(usAttachment, pObj->usItem));

			//Loop through all attachment points on the main item
			for(attachmentList::iterator iter = (*pObj)[subObject]->attachments.begin(); iter != (*pObj)[subObject]->attachments.end() && !foundValidAttachment; ++iter)
			{
				if(iter->exists())
					foundValidAttachment = (ValidAttachment(usAttachment, iter->usItem, pubAPCost) || ValidLaunchable(usAttachment, iter->usItem));
			}

			// sevenfm: determine if we attach launchable to launcher
			if (foundValidAttachment && pubAPCost)
			{
				if (Item[pObj->usItem].usItemClass == IC_LAUNCHER && ValidLaunchable(usAttachment, pObj->usItem))
				{
					*pubAPCost = (UINT8)GetAPsToReload(pObj);
				}
				else if (Item[pObj->usItem].usItemClass & IC_WEAPON && Item[usAttachment].usItemClass & IC_EXPLOSV)
				{
					for (attachmentList::iterator iter_launcher = (*pObj)[0]->attachments.begin(); iter_launcher != (*pObj)[0]->attachments.end(); ++iter_launcher)
					{
						if (iter_launcher->exists() && ValidLaunchable(usAttachment, iter_launcher->usItem))
						{
							*pubAPCost = (UINT8)GetAPsToReload(&(*iter_launcher));
							break;
						}
					}
				}
			}
		}
		return ( foundValidAttachment );
	}
	else
	{
		return( ValidAttachment(usAttachment, pObj->usItem, pubAPCost) );
	}
}

UINT8 AttachmentAPCost( UINT16 usAttachment, UINT16 usItem, SOLDIERTYPE * pSoldier ) // SANDRO - added argument
{
	UINT8 ubAPCost;

	ValidAttachment( usAttachment, usItem, &ubAPCost);

	// SANDRO - STOMP traits - Ambidextrous attaching objects speed bonus
	if ( pSoldier != NULL )
	{
		if ( gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( pSoldier, AMBIDEXTROUS_NT ) )
		{
			ubAPCost = (UINT8)(((FLOAT)ubAPCost * (FLOAT)(100 - gSkillTraitValues.ubAMAttachingItemsAPsReduction) / 100.0f) + 0.5f);
		}
	}

	return ubAPCost;
}

//Also need one with pObj, for the one with usItem is not always correct.
UINT8 AttachmentAPCost( UINT16 usAttachment, OBJECTTYPE * pObj, SOLDIERTYPE * pSoldier, UINT8 subObject, std::vector<UINT16> usAttachmentSlotIndexVector )
{
	UINT8 ubAPCost;

	ValidAttachment(usAttachment, pObj, &ubAPCost, subObject, usAttachmentSlotIndexVector);

	BOOLEAN fLaunchable = FALSE;
	if (Item[pObj->usItem].usItemClass == IC_LAUNCHER && ValidLaunchable(usAttachment, pObj->usItem))
	{
		fLaunchable = TRUE;
	}
	// sevenfm: different cost when attaching grenade to attached launcher
	else if (Item[pObj->usItem].usItemClass & IC_WEAPON && Item[usAttachment].usItemClass & IC_EXPLOSV)
	{
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists() && ValidLaunchable(usAttachment, iter->usItem))
			{
				fLaunchable = TRUE;
			}
		}
	}

	// SANDRO - STOMP traits - Ambidextrous attaching objects speed bonus
	// sevenfm: don't apply when attaching launchable to launcher
	if (pSoldier != NULL && !fLaunchable)
	{
		if ( gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( pSoldier, AMBIDEXTROUS_NT ) )
		{
			ubAPCost = (UINT8)(((FLOAT)ubAPCost * (FLOAT)(100 - gSkillTraitValues.ubAMAttachingItemsAPsReduction) / 100.0f) + 0.5f);
		}
	}

	return ubAPCost;
}

// Flugente: as we call this function quite a lot during rendering, I've modified it so that we evaluate as quickly as possible
//Determine if this slot can receive this attachment.  This is different, in that it may
//be possible to have this attachment on this item, but may already have an attachment
//in the slot we're trying to attach to.
BOOLEAN ValidItemAttachmentSlot( OBJECTTYPE * pObj, UINT16 usAttachment, BOOLEAN fAttemptingAttachment, BOOLEAN fDisplayMessage, UINT8 subObject, INT16 slotCount, BOOLEAN fIgnoreAttachmentInSlot, OBJECTTYPE ** ppAttachInSlot, std::vector<UINT16> usAttachmentSlotIndexVector)
{
	BOOLEAN		fSimilarItems = FALSE, fSameItem = FALSE;
	UINT16		usSimilarItem = NOTHING;
	INT16		ubSlotIndex = 0;
	UINT8		curSlot = 0;
	UINT8		ubVolumeTaken;
	BOOLEAN		foundValidAttachment = FALSE;

	if (pObj->exists() == false)
		return FALSE;

	// shadooow: efficiency check, we are passing all kinds of items into this function that are not necessary attachments at all
	// this code prevents 40mm grenades from loading in certain mods (Aimnas) if <attachment> tag is not set for grenade
	if (!Item[usAttachment].attachment && 
		!Item[usAttachment].hiddenaddon && 
		!(Item[usAttachment].usItemClass & IC_EXPLOSV) &&
		!((Item[usAttachment].usItemClass & IC_GUN) && Item[pObj->usItem].tripwire))
		return FALSE;

	//It's possible we could get here without being sent the usAttachmentSlotIndexVector parameter
	if(usAttachmentSlotIndexVector.empty())
		usAttachmentSlotIndexVector = GetItemSlots(pObj, subObject);

	//No slots means nothing will ever be valid, also a slotCount outside this vector will never be valid either.
	if ( (INT16)usAttachmentSlotIndexVector.size() <= slotCount)
		return FALSE;

	// some checks are only necessary if we want to attach the thing in the first place
	if ( fAttemptingAttachment )
	{
		//Search for incompatible attachments
		//Madd: check for gun on tripwire first
		if ( Item[pObj->usItem].tripwire && Item[usAttachment].usItemClass & IC_GUN && FindAttachmentByClass( pObj, IC_GUN, subObject ) != 0 )
		{
			fSimilarItems = TRUE;
			OBJECTTYPE * tmpObj = FindAttachmentByClass( pObj, IC_GUN, subObject );
			usSimilarItem = tmpObj->usItem;
		}

		if ( !fSameItem )	//Nav: why is this check here? fSameItem can only be false here, doesn't hurt I guess...
		{
			if ( !IsAttachmentClass( usAttachment, (AC_GRENADE | AC_ROCKET | AC_MODPOUCH) ) && FindAttachment( pObj, usAttachment, subObject ) != 0 )
			{
				//Search for identical attachments unless we're dealing with rifle grenades
				//DBrot: or pouches
				fSameItem = TRUE;
			}

			if ( !fSameItem )
			{
				for ( UINT32 i = 0; i < gINCOMPATIBLEATTACHMENTS_READ; ++i )
				{
					if ( IncompatibleAttachments[i][0] == usAttachment && FindAttachment (pObj,IncompatibleAttachments[i][1],subObject) != 0 ) //TODO fill in comment 1
					{
						fSimilarItems = TRUE;
						usSimilarItem = IncompatibleAttachments[i][1];
						break;
					}
				}
			}
		}

		if ( fSameItem )
		{
			if ( fDisplayMessage ) ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, Message[STR_ATTACHMENT_ALREADY] );
			return(FALSE);
		}

		if (Item[pObj->usItem].usItemClass == IC_LBEGEAR && Item[usAttachment].usItemClass == IC_LBEGEAR)
		{
			ubVolumeTaken = GetVolumeAlreadyTaken(pObj, slotCount);
			if(LoadBearingEquipment[Item[pObj->usItem].ubClassIndex].lbeAvailableVolume	< (ubVolumeTaken +  LBEPocketType[GetFirstPocketOnItem(usAttachment)].pVolume))
			{
				if ( fDisplayMessage ) ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, Message[STR_NO_SPACE_FOR_POCKET] );
				return(FALSE);
			}
		}
	}

	//Do we want to check all attachment slots or just the one in slotcount?
	if(slotCount == -1)
	{
		//Loop through slots
		UINT32 uiSlots = min((*pObj)[subObject]->attachments.size(), usAttachmentSlotIndexVector.size());//dnl ch76 091113 ugly fix as AK74 after attach/remove GL will have more attachments then usAttachmentSlotIndexVector which lead to CTD
		for ( UINT8 curSlot = 0; curSlot < uiSlots; ++curSlot)
		{
			//Any attachment that is already in this slot will go here.
			//Get the current attachment in the slot we're looking at.
			OBJECTTYPE* pAttachment = (*pObj)[subObject]->GetAttachmentAtIndex( curSlot );
			ubSlotIndex = usAttachmentSlotIndexVector[curSlot];

			//WarmSteel - does this particular slot already hold an item?
			if(ppAttachInSlot && pAttachment->exists() )
				*ppAttachInSlot = pAttachment;

			//Search for any valid attachments in this slot
			//CHRISL: Valid attachments are determined by the old "ValidItemAttachment" function and comparing the attachment class of the item and slot
			//Madd: gun on tripwire always allowed
			if((AttachmentSlots[ubSlotIndex].nasAttachmentClass & Item[usAttachment].nasAttachmentClass || (Item[usAttachment].usItemClass & IC_GUN && Item[pObj->usItem].tripwire)) &&
				(ValidItemAttachment(pObj,usAttachment,fAttemptingAttachment,fDisplayMessage,subObject,usAttachmentSlotIndexVector) ||
				ValidLaunchable(usAttachment, GetAttachedGrenadeLauncher(pObj)) ||
				ValidLaunchable(usAttachment, pObj->usItem)))
			{
				foundValidAttachment = TRUE;
				break;
			}
		}
	} 
	else
	{
		OBJECTTYPE* pAttachment = (*pObj)[subObject]->GetAttachmentAtIndex( (UINT8)slotCount );

		ubSlotIndex = usAttachmentSlotIndexVector[slotCount];

		//WarmSteel - does this particular slot already hold an item? :( If we have a pAttachInSlot we're trying to switch, so then it doesn't matter.
		if(!fIgnoreAttachmentInSlot && pAttachment->exists() && fAttemptingAttachment && (!ppAttachInSlot || Item[pAttachment->usItem].inseparable == 1))
		{
			//If we have a parameter to return pAttachment to, store it, else the item does not attach to this slot.
			fSimilarItems = TRUE;
			usSimilarItem = pAttachment->usItem;
		}
		else
		{
			//CHRISL: This should allow attachment swapping even if our attachments can't normally be on the weapon at the same time.
			if(slotCount != -1 && pAttachment->exists() && usSimilarItem == pAttachment->usItem && FindAttachmentSlot(pObj, pAttachment->usItem, subObject) == slotCount)
				fSimilarItems = FALSE;

			//If we have an item to return the existing attachment to.
			if(ppAttachInSlot && pAttachment->exists())
				*ppAttachInSlot = pAttachment;

			//Search for any valid attachments in this slot
			//CHRISL: Valid attachments are determined by the old "ValidItemAttachment" function and comparing the attachment class of the item and slot
			//Madd: gun on tripwire always allowed
			if((AttachmentSlots[ubSlotIndex].nasAttachmentClass & Item[usAttachment].nasAttachmentClass || (Item[usAttachment].usItemClass & IC_GUN && Item[pObj->usItem].tripwire)) &&
				(ValidItemAttachment(pObj,usAttachment,FALSE,FALSE,subObject,usAttachmentSlotIndexVector) ||
				ValidLaunchable(usAttachment, GetAttachedGrenadeLauncher(pObj)) ||
				ValidLaunchable(usAttachment, pObj->usItem)))
			{
				foundValidAttachment = TRUE;
			}
		}
	}

	if(fAttemptingAttachment)
	{
		if (fSimilarItems)
		{
			if(fDisplayMessage) ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, Message[ STR_CANT_USE_TWO_ITEMS ], ItemNames[ usSimilarItem ], ItemNames[ usAttachment ] );
			return( FALSE );
		} 
		else if ( !foundValidAttachment && fDisplayMessage && !ValidMerge( usAttachment, pObj->usItem ) )
		{
			//We don't want a message if we might be merging this little thingey later.
			//well, maybe the player thought he could
			CHAR16	zTemp[ 100 ];

			swprintf( zTemp, Message[ STR_CANNOT_ATTACH_SLOT ], ItemNames[ usAttachment ] );
			ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, zTemp );
		}
	}

	return( foundValidAttachment );
}

//Determine if this item can receive this attachment.  This is different, in that it may
//be possible to have this attachment on this item, but may already have an attachment on
//it which doesn't work simultaneously with the new attachment (like a silencer and duckbill).
BOOLEAN ValidItemAttachment( OBJECTTYPE * pObj, UINT16 usAttachment, BOOLEAN fAttemptingAttachment, BOOLEAN fDisplayMessage, UINT8 subObject, std::vector<UINT16> usAttachmentSlotIndexVector)
{
	if (pObj->exists() == false)
		return FALSE;

	if ( !ValidAttachment( usAttachment, pObj, NULL, subObject, usAttachmentSlotIndexVector ) )
	{
		// check for an underslung grenade launcher attached to the gun
		if ( (IsGrenadeLauncherAttached ( pObj, subObject ) ) && ValidLaunchable( usAttachment, GetAttachedGrenadeLauncher( pObj ) ) )
		{
			return ( TRUE );
		}
		else
		{
			if ( fAttemptingAttachment && fDisplayMessage && ValidAttachmentClass( usAttachment, pObj->usItem ) )//dnl ch75 251013
			{
				// well, maybe the player thought he could
				CHAR16	zTemp[ 100 ];

				swprintf( zTemp, Message[ STR_CANT_ATTACH ], ItemNames[ usAttachment ], ItemNames[ pObj->usItem ] );
				ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, zTemp );
			}

			return( FALSE );
		}
	}

	if (fAttemptingAttachment)
	{
		BOOLEAN		fSameItem = FALSE, fSimilarItems = FALSE;
		UINT16		usSimilarItem = NOTHING;

		//Madd: check for gun on tripwire first
		if ( Item[pObj->usItem].tripwire && Item[usAttachment].usItemClass & IC_GUN && FindAttachmentByClass( pObj, IC_GUN, subObject ) != 0 )
		{
			fSimilarItems = TRUE;
			OBJECTTYPE * tmpObj = FindAttachmentByClass( pObj, IC_GUN, subObject );
			usSimilarItem = tmpObj->usItem;
		}

		if ( !fSameItem )
		{
			if ( FindAttachment( pObj, usAttachment, subObject ) != 0 )
			{
				fSameItem = TRUE;
			}

			if ( !fSameItem )
			{
				for ( UINT32 i = 0; i < gINCOMPATIBLEATTACHMENTS_READ; ++i )
				{
					if ( IncompatibleAttachments[i][0] == usAttachment && FindAttachment( pObj, IncompatibleAttachments[i][1], subObject ) != 0 )
					{
						fSimilarItems = TRUE;
						usSimilarItem = IncompatibleAttachments[i][1];
						break;
					}
				}
			}
		}

		if (fSameItem)
		{
			if (fDisplayMessage) ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, Message[ STR_ATTACHMENT_ALREADY ] );
			return( FALSE );
		}
		else if (fSimilarItems)
		{
			if (fDisplayMessage) ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, Message[ STR_CANT_USE_TWO_ITEMS ], ItemNames[ usSimilarItem ], ItemNames[ usAttachment ] );
			return( FALSE );
		}
	}

	return( TRUE );
}

//Determines if it is possible to equip this weapon with this ammo.
BOOLEAN ValidAmmoType( UINT16 usItem, UINT16 usAmmoType )
{
	if (Item[usItem].usItemClass == IC_GUN && Item[usAmmoType].usItemClass == IC_AMMO)
	{
		if (Weapon[usItem].ubCalibre == Magazine[Item[usAmmoType].ubClassIndex].ubCalibre)
		{
			return( TRUE );
		}
	}
	return( FALSE );
}

BOOLEAN CompatibleFaceItem( UINT16 usItem1, UINT16 usItem2 )
{
	INT32 iLoop = 0;

	//Madd: skip this function if either item is nothing
	//this will let us trim some lines from compatiblefaceitems.xml
	if ( usItem1 == NONE || usItem2 == NONE )
		return TRUE;

	// look for the section of the array pertaining to this attachment...
	while( 1 )
	{
		if (CompatibleFaceItems[iLoop][0] == usItem1)
		{
			break;
		}
		iLoop++;
		if (CompatibleFaceItems[iLoop][0] == 0)
		{
			// the proposed item cannot fit with anything!
			return( FALSE );
		}
	}
	// now look through this section for the item in question
	while( 1 )
	{
		if (CompatibleFaceItems[iLoop][1] == usItem2)
		{
			break;
		}
		iLoop++;
		if (CompatibleFaceItems[iLoop][0] != usItem1)
		{
			// the proposed item cannot be attached to the item in question
			return( FALSE );
		}
	}
	return( TRUE );
}


//Determines if this item is a two handed item.
BOOLEAN TwoHandedItem( UINT16 usItem )
{
//	if (Item[usItem].fFlags & ITEM_TWO_HANDED)
	if (Item[usItem].twohanded )
	{
		return( TRUE );
	}
	return FALSE;
}

BOOLEAN ValidLaunchable( UINT16 usLaunchable, UINT16 usItem )
{
	// shadooow: efficiency check, we are passing all kinds of items into this function that are not necessary attachments at all
	// this code prevents 40mm grenades from loading in certain mods (Aimnas) if <attachment> tag is not set for grenade
	//if (!Item[usLaunchable].attachment && !Item[usLaunchable].hiddenaddon)
		//return FALSE;

	// sevenfm: only grenades and explosives can be launched
	if (!(Item[usLaunchable].usItemClass & IC_EXPLOSV))
		return FALSE;

	INT32 iLoop = 0;

	// look for the section of the array pertaining to this launchable item...
	while( 1 )
	{
		if (Launchable[iLoop][0] == usLaunchable)
		{
			break;
		}
		iLoop++;
		if (Launchable[iLoop][0] == 0)
		{
			// the proposed item cannot be attached to anything!
			return( FALSE );
		}
	}
	// now look through this section for the item in question
	while( 1 )
	{
		if (Launchable[iLoop][1] == usItem)
		{
			break;
		}
		iLoop++;
		if (Launchable[iLoop][0] != usLaunchable)
		{
			// the proposed item cannot be attached to the item in question
			return( FALSE );
		}
	}

	return( TRUE );
}

BOOLEAN ValidItemLaunchable( OBJECTTYPE * pObj, UINT16 usAttachment )
{
	if (pObj->exists() == false) {
		return FALSE;
	}
	//Madd: Common Attachment Framework
	if ( !ValidLaunchable( usAttachment, pObj->usItem ) && !IsAttachmentPointAvailable(pObj, 0, usAttachment) )
	{
		return( FALSE );
	}
	// if we can find another of the same class as the attachment, it's not possible
	if ( FindAttachmentByClass( pObj, Item[ usAttachment ].usItemClass ) != 0 )
	{
		return( FALSE );
	}
	return( TRUE );
}


UINT16 GetLauncherFromLaunchable( UINT16 usLaunchable )
{
	INT32 iLoop = 0;
	// look for the section of the array pertaining to this launchable item...
	while( 1 )
	{
		if (Launchable[iLoop][0] == usLaunchable)
		{
			break;
		}
		iLoop++;
		if (Launchable[iLoop][0] == 0)
		{
			// the proposed item cannot be attached to anything!
			return( NOTHING );
		}
	}

	return( Launchable[iLoop][1] );
}



BOOLEAN EvaluateValidMerge(UINT16 usMerge, UINT16 usItem, UINT16 * pusResult, UINT16 * pusResult2, UINT8 * pubType, UINT8 * pubAPCost, UINT16 *pusPointsUsed, UINT16 *pusPointsAdded, UINT16 *pusExtraItem, UINT16 *pusExtraItemPointsUsed, UINT16 *pusSkillCheck, UINT16 *pusSkillDifficulty)
{
	// NB "usMerge" is the object being merged with (e.g. compound 18)
	// "usItem" is the item being merged "onto" (e.g. kevlar vest)
	INT32 iLoop = 0;

	// sevenfm: initialize
	*pusResult = 0;
	*pusResult2 = 0;
	*pubType = DESTRUCTION;
	*pubAPCost = APBPConstants[AP_MAXIMUM] / 5;

	// extra tags
	*pusPointsUsed = 0;
	*pusPointsAdded = 0;
	*pusExtraItem = 0;
	*pusExtraItemPointsUsed = 0;
	*pusSkillCheck = 0;
	*pusSkillDifficulty = 0;

	//CHRISL: Update this so we can also merge IC_MONEY like wallets and nuggets.
	if (usMerge == usItem && (Item[ usItem ].usItemClass == IC_AMMO || Item[ usItem ].usItemClass == IC_MONEY))
	{
		*pusResult = usItem;
		*pubType = COMBINE_POINTS;
		*pubAPCost = (UINT8)APBPConstants[AP_MINIMUM];
		return( TRUE );
	}

	//ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"usItem %d usMerge %d", usItem, usMerge);
	// sevenfm: use cleaning kit on weapon to clean it
	if (gGameExternalOptions.fDirtSystem &&
		Item[usItem].usItemClass == IC_GUN &&
		(Item[usMerge].usItemClass & IC_KIT) &&
		HasItemFlag(usMerge, CLEANING_KIT))
	{
		*pusResult = usItem;
		*pubType = USE_ITEM_CLEAN;
		*pubAPCost = (UINT8)APBPConstants[AP_CLEANINGKIT];
		return(TRUE);
	}

	// look for the section of the array pertaining to this Merge...
	while( 1 )
	{
		if (Merge[iLoop][0] == usMerge)
		{
			break;
		}
		iLoop++;
		if (Merge[iLoop][0] == 0)
		{
			// the proposed item cannot be merged with anything!
			return( FALSE );
		}
	}
	// now look through this section for the item in question
	while( 1 )
	{
		if (Merge[iLoop][1] == usItem)
		{
			break;
		}
		iLoop++;
		if (Merge[iLoop][0] != usMerge)
		{
			// the proposed item cannot be merged with the item in question
			return( FALSE );
		}
	}

	// Flugente: for random items, we have to do the resolution here
	UINT16 result1 = Merge[iLoop][2];
	UINT16 rdresult1 = 0;
	if ( GetItemFromRandomItem(result1, &rdresult1) )
		result1 = rdresult1;

	UINT16 result2 = Merge[iLoop][3];
	UINT16 rdresult2 = 0;
	if ( GetItemFromRandomItem(result2, &rdresult2) )
		result2 = rdresult2;

	//WarmSteel - Return false if the results aren't valid.
	if( !ItemIsLegal(result1, TRUE) && !ItemIsLegal(result2, TRUE) ){
		return( FALSE );
	}

	*pusResult = result1;
	*pusResult2 = result2;
	*pubType = (UINT8) Merge[iLoop][4];
	*pubAPCost = (UINT8) Merge[iLoop][5];

	// extra tags
	*pusPointsUsed = Merge[iLoop][6];
	*pusPointsAdded = Merge[iLoop][7];
	*pusExtraItem = Merge[iLoop][8];
	*pusExtraItemPointsUsed = Merge[iLoop][9];
	*pusSkillCheck = Merge[iLoop][10];
	*pusSkillDifficulty = Merge[iLoop][11];

	return( TRUE );
}

BOOLEAN ValidMerge( UINT16 usMerge, UINT16 usItem )
{
	UINT16	usIgnoreResult, usIgnoreResult2;
	UINT8	ubIgnoreType, ubIgnoreAPCost;
	UINT16	usPointsUsed;
	UINT16	usPointsAdded;
	UINT16	usExtraItem;
	UINT16	usExtraItemPointsUsed;
	UINT16	usSkillCheck;
	UINT16	usSkillDifficulty;

	return(EvaluateValidMerge(usMerge, usItem, &usIgnoreResult, &usIgnoreResult2, &ubIgnoreType, &ubIgnoreAPCost, &usPointsUsed, &usPointsAdded, &usExtraItem, &usExtraItemPointsUsed, &usSkillCheck, &usSkillDifficulty));
}

int GetPocketSizeByDimensions(int sizeX, int sizeY)
{
	static const UINT8 cisPocketSize[6][4] =
	{
		11, 12, 13, 14,
		15, 16, 17, 18,
		19, 20, 21, 22,
		23, 24, 25, 26,
		27, 28, 29, 30,
		31, 32, 33, 34
	};
	return cisPocketSize[sizeX][sizeY];
}

void GetPocketDimensionsBySize(int pocketSize, int& sizeX, int& sizeY)
{
	static const UINT8 cisPocketSize[6][4] =
	{
		11, 12, 13, 14,
		15, 16, 17, 18,
		19, 20, 21, 22,
		23, 24, 25, 26,
		27, 28, 29, 30,
		31, 32, 33, 34
	};

	for(sizeX=0; sizeX<6; sizeX++)
	{
		for(sizeY=0; sizeY<4; sizeY++)
		{
			if(pocketSize == cisPocketSize[sizeX][sizeY])
			{
				return;
			}
		}
	}
}


// CHRISL: New function to dynamically modify ItemSize based on attachments, stack size, etc
UINT16 CalculateItemSize( OBJECTTYPE *pObject )
{
	UINT16		usSize;
	UINT16		currentSize = 0;
	UINT32		cisIndex;

	// Determine default ItemSize based on item and attachments
	cisIndex = pObject->usItem;
	usSize = min(Item[cisIndex].ItemSize, gGameExternalOptions.guiMaxItemSize);

	//for each object in the stack, hopefully there is only 1
	for (int numStacked = 0; numStacked < pObject->ubNumberOfObjects; ++numStacked) 
	{
		// sevenfm: attaching explosives can increase item size
		if ( Item[pObject->usItem].usItemClass & IC_GRENADE || Item[pObject->usItem].usItemClass & IC_BOMB || pObject->usItem == DUCT_TAPE)
		{
			for (attachmentList::iterator iter = (*pObject)[numStacked]->attachments.begin(); iter != (*pObject)[numStacked]->attachments.end(); ++iter) 
			{
				if (iter->exists()) 
				{
					// for weapons attached to tripwire - use weapon size
					if( Item[pObject->usItem].tripwire && Item[iter->usItem].usItemClass & IC_GUN )
					{
						usSize = Item[iter->usItem].ItemSize;
					}
					// some items can increase item size
					else if( Item[iter->usItem].itemsizebonus > 0 )
					{
						usSize = min(usSize + Item[iter->usItem].itemsizebonus, gGameExternalOptions.guiMaxItemSize);
					}
					// if item bonus not defined, use max size
					// fix: don't check attachment size for detonator
					else if (!IsAttachmentClass(iter->usItem, (AC_DETONATOR | AC_REMOTEDET | AC_DEFUSE)))
					{
						//usSize = max(usSize, Item[iter->usItem].ItemSize);
						// sevenfm: use vehicle pocket size to determine max size of object and attachment
						if (LBEPocketType[VEHICLE_POCKET_TYPE].ItemCapacityPerSize[Item[iter->usItem].ItemSize] < LBEPocketType[VEHICLE_POCKET_TYPE].ItemCapacityPerSize[usSize])
							usSize = Item[iter->usItem].ItemSize;
					}
				}
			}
		}
		//some weapon attachments can adjust the ItemSize of a weapon
		else if(usSize < gGameExternalOptions.guiMaxWeaponSize) 
		{
			for (attachmentList::iterator iter = (*pObject)[numStacked]->attachments.begin(); iter != (*pObject)[numStacked]->attachments.end(); ++iter) 
			{
				if (iter->exists()) 
				{
					usSize = min(usSize + Item[iter->usItem].itemsizebonus, gGameExternalOptions.guiMaxItemSize);
				}
			}
		}

		// LBENODE has it's ItemSize adjusted based on what it's storing
		if(pObject->IsActiveLBE(numStacked) == true)
		{
			LBENODE* pLBE = pObject->GetLBEPointer(numStacked);
			if(pLBE)
			{
				//start by determining the equivalent number of "small" pockets that this LBENODE has access to.  This
				//	is based on the pType value in Pockets.xml with 1=Small, 2=Medium and 3=Large
				UINT16	totalPocketValue = 0;
				FLOAT	percentOfItemUsed = 0;
				UINT16	pIndex, testSize, maxSize;
				UINT8	pocketCapacity, numberOfSizeIncrements;
				FLOAT	currentPocketPercent, currentPocketPartOfTotal;
				size_t invsize = pLBE->inv.size();
				for (size_t x = 0; x < invsize; ++x)
				{
					if(LoadBearingEquipment[Item[pObject->usItem].ubClassIndex].lbePocketIndex[x] != 0)
					{
						pIndex = LoadBearingEquipment[Item[pObject->usItem].ubClassIndex].lbePocketIndex[x];
						totalPocketValue += LBEPocketType[pIndex].pType;
					}
				}
				//Now, look through each active pocket
				for (size_t x = 0; x < invsize; ++x)
				{
					if(pLBE->inv[x].exists() == true)
					{
						pIndex = LoadBearingEquipment[Item[pObject->usItem].ubClassIndex].lbePocketIndex[x];
						testSize = CalculateItemSize(&pLBE->inv[x]);
						pocketCapacity = LBEPocketType[pIndex].ItemCapacityPerSize[testSize];
						currentPocketPercent = (FLOAT)pLBE->inv[x].ubNumberOfObjects / (FLOAT)pocketCapacity;
						currentPocketPartOfTotal = (FLOAT)LBEPocketType[pIndex].pType / (FLOAT)totalPocketValue;
						percentOfItemUsed += currentPocketPartOfTotal * currentPocketPercent;
					}
				}
				maxSize = max(usSize, LoadBearingEquipment[Item[pObject->usItem].ubClassIndex].lbeFilledSize);
				//Now, determine the increments between initial ItemSize and Filled Size, and adjust iSize by percentOfItemUsed
				if(percentOfItemUsed != 0)
				{
					numberOfSizeIncrements = LoadBearingEquipment[Item[pObject->usItem].ubClassIndex].lbeFilledSize - Item[pObject->usItem].ItemSize;
					testSize = (UINT16)((numberOfSizeIncrements * percentOfItemUsed) + .5);
					currentSize = __max(usSize + testSize, currentSize);
					currentSize = __min(currentSize, maxSize);
				}
			}
		}
	}
	//Finally, set the new iSize value
	usSize = __max(usSize, currentSize);

	return(usSize);
}

UINT16 CalculateAmmoWeight( UINT16 usGunAmmoItem, UINT16 ubShotsLeft )
{
	if( 0 == usGunAmmoItem ) /* Sergeant_Kolja: 2007-06-11, Fix for Creature Spit. This has no Ammo, so the old code calculated accidentally -1.6 resulting in 0xFFFF */
	{
		DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "'no ammo weight' FIX for Creatures\r\n" );
		return 0;
	}

	//Temporary calculation for minWeight. 0.2*ubWeight rounded correctly
	UINT32 uiMinWeight = (UINT32)(((FLOAT)Item[usGunAmmoItem].ubWeight / 5.0f) + 0.5f);
	if( uiMinWeight < 1 || uiMinWeight > Item[usGunAmmoItem].ubWeight)
	{
		uiMinWeight = 1;
	}

	double weight = 0.0;

	if(ubShotsLeft > 0)
	{
		if( uiMinWeight == Item[usGunAmmoItem].ubWeight )
		{
			weight += (double)uiMinWeight;
		}
		else
		{
			weight += (double)uiMinWeight + (( (double)ubShotsLeft / (double)Magazine[ Item[usGunAmmoItem].ubClassIndex ].ubMagSize) * ( (double)Item[usGunAmmoItem].ubWeight - (double)uiMinWeight ));
		}
	}
	weight += 0.5; //Pulmu:To round correctly
	return (UINT16)weight;
	//Pulmu end
}

/*CHRISL: Change to a 16bit integer for a max weight of 6553.5kg.  Also changed to account for
new inventory system. */
UINT16 CalculateObjectWeight( OBJECTTYPE *pObject )
{
	if (pObject->exists() == false || pObject->ubNumberOfObjects == 0) {
		return 0;
	}

	UINT16 weight = 0;
	INVTYPE* pItem = &(Item[pObject->usItem]);

	//ADB it is much easier and faster to calculate ammo in a lump rather than accumulate a stack's weight
	if ( pItem->usItemClass == IC_AMMO)//Pulmu: added weight allowance for ammo not being full
	{
		if ( gGameExternalOptions.fAmmoDynamicWeight == TRUE ) {
			for( int cnt = 0; cnt < pObject->ubNumberOfObjects; cnt++ )
			{
				weight += CalculateAmmoWeight(pObject->usItem, (*pObject)[cnt]->data.ubShotsLeft);
			}
		}
		else {
			// Start with base weight
			weight = pItem->ubWeight;
			//multiply by the number of objects, can be 0
			weight *= pObject->ubNumberOfObjects;
		}
	}
	else {
		for (int x = 0; x < pObject->ubNumberOfObjects; ++x) {
			weight += pObject->GetWeightOfObjectInStack(x);
		}
	}

	return( weight );
}

// r8624
UINT16 OBJECTTYPE::GetWeightOfObjectInStack(unsigned int index)
{
	//Item does not exist
	if (index >= ubNumberOfObjects)
	{
		return 0;
	}

	INVTYPE * pItem = &(Item[usItem]);

	// Start with base weight
	UINT16 weight = pItem->ubWeight;

	// Are we looking at an LBENODE item?  New inventory only.
	if (pItem->usItemClass == IC_LBEGEAR && IsActiveLBE(index) && (UsingNewInventorySystem() == true))
	{
		LBENODE* pLBE = GetLBEPointer(index);
		if (pLBE)
		{
			for (size_t subObjects = 0, invsize = pLBE->inv.size(); subObjects < invsize; ++subObjects)
			{
				if (pLBE->inv[subObjects].exists() == true)
				{
					weight += CalculateObjectWeight(&(pLBE->inv[subObjects]));
				}
			}
		}

		//do not search for attachments to an LBE
		return weight;
	}
	else if (pItem->usItemClass != IC_AMMO)
	{
		// account for any attachments
		for (attachmentList::iterator iter = (*this)[index]->attachments.begin(); iter != (*this)[index]->attachments.end(); ++iter)
		{
			if (iter->exists())
			{
				weight += CalculateObjectWeight(&(*iter));
			}
		}

		// add in weight of ammo
		if (Item[usItem].usItemClass == IC_GUN && (*this)[index]->data.gun.ubGunShotsLeft > 0)
		{
			if (gGameExternalOptions.fAmmoDynamicWeight == TRUE)
			{
				weight += CalculateAmmoWeight((*this)[index]->data.gun.usGunAmmoItem, (*this)[index]->data.gun.ubGunShotsLeft);
			}
			else
			{
				weight += Item[(*this)[index]->data.gun.usGunAmmoItem].ubWeight;
			}
		}

		if (gGameExternalOptions.fAmmoDynamicWeight && 
			(pItem->camouflagekit || pItem->canteen || pItem->drugtype || pItem->foodtype || usItem == JAR_ELIXIR || pItem->containsliquid || pItem->gascan || usItem ==  DUCT_TAPE))
		{
			weight = (UINT16)(weight * ((*this)[index])->data.objectStatus / 100.0f + 0.5f);
		}
	}
	else if (pItem->usItemClass == IC_AMMO && gGameExternalOptions.fAmmoDynamicWeight == TRUE)//Pulmu: added weight allowance for ammo not being full
	{
		weight = CalculateAmmoWeight(usItem, (*this)[index]->data.ubShotsLeft);
	}

	return weight;
}

UINT32 GetTotalWeight(SOLDIERTYPE* pSoldier)
{
	UINT32	uiTotalWeight = 0;

	UINT8 invsize = pSoldier->inv.size();
	for (UINT8 ubLoop = 0; ubLoop < invsize; ++ubLoop)
	{
		//ADB the weight of the object is already counting stacked objects, attachments, et al
		uiTotalWeight += CalculateObjectWeight(&pSoldier->inv[ubLoop]);
	}

	return uiTotalWeight;
}

UINT32 CalculateCarriedWeight(SOLDIERTYPE * pSoldier, BOOLEAN fConsiderDragging)
{
	UINT32	uiTotalWeight = 0;
	UINT32	uiPercent;
	UINT8	ubLoop;
	UINT32	ubStrengthForCarrying;

	//Pulmu: Changes for dynamic ammo weight
	UINT8 invsize = pSoldier->inv.size();
	for( ubLoop = 0; ubLoop < invsize; ++ubLoop)
	{
		//ADB the weight of the object is already counting stacked objects, attachments, et al
		uiTotalWeight += CalculateObjectWeight(&pSoldier->inv[ubLoop]);
	}

	// Flugente: if we drag someone, add their weight to ours
	if (fConsiderDragging && pSoldier->IsDragging())
	{
		if (pSoldier->ubDragPersonID != NOBODY)
		{
			SOLDIERTYPE* pOtherSoldier = MercPtrs[pSoldier->ubDragPersonID];

			uiTotalWeight += (UINT32)GetTotalWeight(pOtherSoldier);
			uiTotalWeight += (UINT32)pOtherSoldier->GetBodyWeight();
		}
		else if (pSoldier->sDragCorpseID >= 0)
		{
			// just give corpses a flat weight
			uiTotalWeight += 100;
		}
	}

	// for now, assume soldiers can carry 1/2 their strength in KGs without penalty.
	// instead of multiplying by 100 for percent, and then dividing by 10 to account
	// for weight units being in 10ths of kilos, not kilos... we just start with 10 instead of 100!
	ubStrengthForCarrying = EffectiveStrength( pSoldier, FALSE );
	if ( ubStrengthForCarrying > 80 )
	{
		ubStrengthForCarrying += (ubStrengthForCarrying - 80);
	}

	// SANDRO - STOMP traits - Bodybuilding carry weight bonus
	if ( gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( pSoldier, BODYBUILDING_NT ) )
	{
		ubStrengthForCarrying = (ubStrengthForCarrying * (100 + gSkillTraitValues.ubBBCarryWeightBonus) / 100); // plus one third
	}

	ubStrengthForCarrying = (ubStrengthForCarrying * (100 + pSoldier->GetBackgroundValue(BG_PERC_CARRYSTRENGTH)) / 100);

	// for now, assume soldiers can carry 1/2 their strength in KGs without penalty.
	// instead of multiplying by 100 for percent, and then dividing by 10 to account
	// for weight units being in 10ths of kilos, not kilos... we just start with 10 instead of 100!

	// HEADROCK HAM 3: STR required per 1/2 kilo has been externalized. Can someone tidy this up though? The
	// formula works great, but it's damn ugly now.
	uiPercent = (UINT32)(((FLOAT)20 * (FLOAT)gGameExternalOptions.iStrengthToLiftHalfKilo) * uiTotalWeight) / ( ubStrengthForCarrying );

	// sevenfm: limit carry weight percent for robot to 100% as it shouldn't have any penalties
	if (pSoldier->IsRobot())
		uiPercent = min(uiPercent, 100);

	return( uiPercent );
}

void DeleteObj(OBJECTTYPE * pObj )
{
	pObj->initialize();
}

void SwapObjs( OBJECTTYPE * pObj1, OBJECTTYPE * pObj2 )
{
	//cannot use gTempObject
	OBJECTTYPE Temp (*pObj1 );
	*pObj1 = *pObj2;
	*pObj2 = Temp;
}

//ADB these 2 functions were created because the code calls SwapObjs all over the place
//but never handles the effects of that swap!
void SwapObjs(SOLDIERTYPE* pSoldier, int leftSlot, int rightSlot, BOOLEAN fPermanent)
{
	SwapObjs(&pSoldier->inv[ leftSlot ], &pSoldier->inv[ rightSlot ]);

	if(fPermanent && !TANK(pSoldier))//dnl ch64 290813 for current tank don't go further as it lead to invalid animation
	{
		//old usItem for the left slot is now stored in the right slot, and vice versa
		HandleTacticalEffectsOfEquipmentChange(pSoldier, leftSlot, pSoldier->inv[ rightSlot ].usItem, pSoldier->inv[ leftSlot ].usItem);
		HandleTacticalEffectsOfEquipmentChange(pSoldier, rightSlot, pSoldier->inv[ leftSlot ].usItem, pSoldier->inv[ rightSlot ].usItem);
	}
}

void SwapObjs(SOLDIERTYPE* pSoldier, int slot, OBJECTTYPE* pObject, BOOLEAN fPermanent)
{
	SwapObjs(&pSoldier->inv[ slot ], pObject);

	if (fPermanent)
	{
		HandleTacticalEffectsOfEquipmentChange(pSoldier, slot, pObject->usItem, pSoldier->inv[ slot ].usItem);
	}
}

void DamageObj( OBJECTTYPE * pObj, INT8 bAmount, UINT8 subObject )
{
	// Flugente: lower repair threshold
	(*pObj)[subObject]->data.sRepairThreshold = max(1, (*pObj)[subObject]->data.sRepairThreshold - bAmount/3);

	//usually called from AttachObject, where the attachment is known to be a single item,
	//and the attachment is only being attached to the top of the stack
	if (bAmount >= (*pObj)[subObject]->data.objectStatus)
	{
		(*pObj)[subObject]->data.objectStatus = 1;
	}
	else
	{
		(*pObj)[subObject]->data.objectStatus -= bAmount;
	}
}

void DistributeStatus(OBJECTTYPE* pSourceObject, OBJECTTYPE* pTargetObject, INT16 bMaxPoints)
{
	INT16 bPointsToMove;
	for ( int bLoop = pSourceObject->ubNumberOfObjects - 1; bLoop >= 0; bLoop-- )
	{
		StackedObjectData* pSource = (*pSourceObject)[ bLoop ];
		if ( pSource->data.objectStatus > 0 )
		{
			// take the points here and distribute over the lower #d items
			int bLoop2;
			if (pSourceObject == pTargetObject) {
				//we are averaging out the same object
				bLoop2 = bLoop - 1;
			}
			else {
				//we are moving from the cursor object to this one
				bLoop2 = pTargetObject->ubNumberOfObjects - 1;
			}

			//for (; bLoop2 >= 0; bLoop2-- )
			for(int i = 0; i<=bLoop2; i++)
			{
				StackedObjectData* pDest = (*pTargetObject)[ i ];
				if ( pDest->data.objectStatus < bMaxPoints )
				{
					bPointsToMove = bMaxPoints - pDest->data.objectStatus;
					bPointsToMove = __min( bPointsToMove, pSource->data.objectStatus );

					pDest->data.objectStatus += bPointsToMove;
					pSource->data.objectStatus -= bPointsToMove;

					if ( pSource->data.objectStatus == 0 )
					{
						StackedObjects::iterator iter = pSourceObject->objectStack.begin();
						for (int x = 0; x < bLoop; ++x) {
							++iter;
						}
						pSourceObject->objectStack.erase(iter);
						pSourceObject->ubNumberOfObjects--;
						//ADB ubWeight has been removed, see comments in OBJECTTYPE
						//pSourceObject->ubWeight = CalculateObjectWeight(pSourceObject);
						// done!
						break;
					}
				}
			}
		}
	}
}

BOOLEAN PlaceObjectAtObjectIndex( OBJECTTYPE * pSourceObj, OBJECTTYPE * pTargetObj, UINT8 ubIndex, UINT32 ubCap )
{
	if (pSourceObj->usItem != pTargetObj->usItem)
	{
		return( TRUE );
	}
	if (ubIndex < pTargetObj->ubNumberOfObjects)
	{
		// swap
		//std::swap??
		StackedObjectData data = *((*pSourceObj)[0]);

		*((*pSourceObj)[0]) = *((*pTargetObj)[ubIndex]);
		//ADB ubWeight has been removed, see comments in OBJECTTYPE
		//pSourceObj->ubWeight = CalculateObjectWeight(pSourceObj);

		*((*pTargetObj)[ubIndex]) = data;
		//ADB ubWeight has been removed, see comments in OBJECTTYPE
		//pTargetObj->ubWeight = CalculateObjectWeight(pTargetObj);
		return( TRUE );
	}
	else
	{
		// add to end
		pTargetObj->AddObjectsToStack( *pSourceObj, 1, NULL, NUM_INV_SLOTS, ubCap );
		if (pSourceObj->exists() == true) {
			return( TRUE );
		}
		else {
			return FALSE;
		}
	}
}

#define RELOAD_NONE 0
#define RELOAD_PLACE 1
#define RELOAD_SWAP 2
#define RELOAD_TOPOFF 3
#define RELOAD_AUTOPLACE_OLD 4

BOOLEAN ReloadGun( SOLDIERTYPE * pSoldier, OBJECTTYPE * pGun, OBJECTTYPE * pAmmo, UINT32 subObject )
{
	UINT16			ubBulletsToMove;
	INT16			bAPs;
	UINT16			usReloadSound;
	BOOLEAN			fSameAmmoType;
	BOOLEAN			fSameMagazineSize;
	BOOLEAN			fReloadingWithStack;
	BOOLEAN			fEmptyGun;
	BOOLEAN			fEnoughAPs;
	INT8			bReloadType;
	UINT16			usNewAmmoItem;
	UINT16			usLargestMag;
	UINT32			ammoObject = 0;

	bAPs = 0;

	if ( (gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT) )
	{
		//CHRISL: Alter this so we treat clip fed weapons differently from weapons that load with loose rounds
		bAPs = GetAPsToReloadGunWithAmmo( pSoldier, pGun, pAmmo );
		// sevenfm: allow AI to always reload
		if ( !EnoughPoints( pSoldier, bAPs, 0, TRUE ) && pSoldier->flags.uiStatusFlags & SOLDIER_PC )
		{
			return( FALSE );
		}
	}

	if ( Item[ pGun->usItem ].usItemClass == IC_LAUNCHER || Item[pGun->usItem].cannon )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("ReloadGun: Loading launcher - new ammo type = %d, weight = %d", pAmmo->usItem, CalculateObjectWeight(pAmmo) ) );
		(*pGun)[subObject]->data.gun.usGunAmmoItem = pAmmo->usItem;
		if ( pGun->AttachObject( pSoldier, pAmmo ) == FALSE )
		{
			(*pGun)[subObject]->data.gun.usGunAmmoItem = NONE;
			// abort
			return( FALSE );
		}
	}
	else
	{
		fEmptyGun = ((*pGun)[subObject]->data.gun.ubGunShotsLeft == 0);
		fReloadingWithStack = (pAmmo->ubNumberOfObjects > 1);
		fSameAmmoType = ( (*pGun)[subObject]->data.gun.ubGunAmmoType == Magazine[Item[pAmmo->usItem].ubClassIndex].ubAmmoType );
		fSameMagazineSize = ( Magazine[ Item[ pAmmo->usItem ].ubClassIndex ].ubMagSize == GetMagSize( pGun, subObject));
		fEnoughAPs = EnoughPoints( pSoldier, GetAPsToReloadGunWithAmmo( pSoldier, pGun, pAmmo, FALSE ), 0,FALSE );

		if (fEmptyGun)
		{
			bReloadType = RELOAD_PLACE;
		}
		else
		{
			// record old ammo
			CreateAmmo((*pGun)[subObject]->data.gun.usGunAmmoItem, &gTempObject, (*pGun)[subObject]->data.gun.ubGunShotsLeft);

			// r8460
			// Flugente: safety check: if object is broken, wipe it
			// sevenfm: also check for incorrect ammo item
			if (gTempObject.usItem == NOTHING || !(Item[gTempObject.usItem].usItemClass & IC_AMMO))
				gTempObject.initialize( );

			if (fSameMagazineSize)
			{
				if (fSameAmmoType)
				{
					if ( (gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT) && Weapon[pGun->usItem].swapClips )
						bReloadType = RELOAD_SWAP;
					else
						bReloadType = RELOAD_TOPOFF;
				}
				else
				{
					if (!fReloadingWithStack)
						bReloadType = RELOAD_SWAP;
					else
						bReloadType = RELOAD_AUTOPLACE_OLD;
				}
			}
			else  // diff sized magazines
			{
				if (fSameAmmoType)
					bReloadType = RELOAD_TOPOFF;
				else
					bReloadType = RELOAD_AUTOPLACE_OLD;
			}
		}

		//CHRISL: We need to verify that the passed subObject actually exists.  We could get passed an empty subObject if we
		//	reload a stack of weapons from the stack popup.  If this is the case, reset subObject to the last item in the
		//	stack
		if(subObject >= pGun->ubNumberOfObjects)
			subObject = ammoObject = pGun->ubNumberOfObjects-1;

		//CHRISL: If reloading with a stack, we probably want the item with the most ammo still in it
		if (fReloadingWithStack)
		{
			usLargestMag = (*pAmmo)[ammoObject]->data.ubShotsLeft;
			for(int i = 0; i<pAmmo->ubNumberOfObjects; i++)
			{
				if((*pAmmo)[i]->data.ubShotsLeft == Magazine[Item[pAmmo->usItem].ubClassIndex].ubMagSize)
				{
					ammoObject = i;
					break;
				}
				else if((*pAmmo)[i]->data.ubShotsLeft > usLargestMag)
				{
					ammoObject = i;
				}
			}
		}

		if (fSameMagazineSize)
		{
			// record new ammo item for gun
			usNewAmmoItem = pAmmo->usItem;

			if (bReloadType == RELOAD_TOPOFF)
				ubBulletsToMove = min( (*pAmmo)[ammoObject]->data.ubShotsLeft, GetMagSize(pGun) - (*pGun)[subObject]->data.gun.ubGunShotsLeft );
			else
				ubBulletsToMove = (*pAmmo)[ammoObject]->data.ubShotsLeft;

		}
		//else if (Magazine[Item[pAmmo->usItem].ubClassIndex].ubMagSize > GetMagSize(pGun, subObject))
		else
		{
			//usNewAmmoItem = pAmmo->usItem - 1;
			usNewAmmoItem = FindReplacementMagazine(Weapon[pGun->usItem].ubCalibre ,GetMagSize(pGun, subObject),Magazine[Item[pAmmo->usItem].ubClassIndex].ubAmmoType);
			if (bReloadType == RELOAD_TOPOFF)
				ubBulletsToMove = min( (*pAmmo)[ammoObject]->data.ubShotsLeft, GetMagSize(pGun, subObject) - (*pGun)[subObject]->data.gun.ubGunShotsLeft );
			else
				ubBulletsToMove = min( (*pAmmo)[ammoObject]->data.ubShotsLeft, GetMagSize(pGun, subObject));
		}
		/*else // mag is smaller than weapon mag
		{
			//usNewAmmoItem = pAmmo->usItem + 1;
			usNewAmmoItem = FindReplacementMagazine(Weapon[pGun->usItem].ubCalibre ,GetMagSize(pGun, subObject),Magazine[Item[pAmmo->usItem].ubClassIndex].ubAmmoType);
			if (bReloadType == RELOAD_TOPOFF)
			{
				ubBulletsToMove = min( (*pAmmo)[ammoObject]->data.ubShotsLeft, GetMagSize(pGun, subObject) - (*pGun)[subObject]->data.gun.ubGunShotsLeft );
			}
			else
			{
				ubBulletsToMove = min( (*pAmmo)[ammoObject]->data.ubShotsLeft, GetMagSize(pGun, subObject));
			}
		}*/

		//CHRIS: This should reset the number of bullets moved to what we can actually afford when loading loose rounds
		if(Weapon[pGun->usItem].swapClips == 0 && (gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT))
		{
			if(fEnoughAPs)
			{
				bAPs = GetAPsToReloadGunWithAmmo( pSoldier, pGun, pAmmo, FALSE );
			}
			else
			{
				//how many can we reload?  remember, bAPs assumes 1 round at this stage
				bAPs = GetAPsToReloadGunWithAmmo( pSoldier, pGun, pAmmo, 2 );
				for(int i = 0; i<ubBulletsToMove; i++)
				{
					if(EnoughPoints(pSoldier, (bAPs+(i*APBPConstants[AP_RELOAD_LOOSE])), 0, FALSE) == FALSE)
					{
						ubBulletsToMove = i+1;
						break;
					}
				}
				bAPs = GetAPsToReloadGunWithAmmo( pSoldier, pGun, pAmmo );
			}
		}

		switch( bReloadType )
		{
			case RELOAD_PLACE:
				(*pGun)[subObject]->data.gun.ubGunShotsLeft = ubBulletsToMove;
				(*pGun)[subObject]->data.gun.ubGunAmmoType = Magazine[Item[pAmmo->usItem].ubClassIndex].ubAmmoType;
				(*pGun)[subObject]->data.gun.usGunAmmoItem = usNewAmmoItem;
				break;

			case RELOAD_SWAP:
				(*pGun)[subObject]->data.gun.ubGunShotsLeft = ubBulletsToMove;
				(*pGun)[subObject]->data.gun.ubGunAmmoType = Magazine[Item[pAmmo->usItem].ubClassIndex].ubAmmoType;
				(*pGun)[subObject]->data.gun.usGunAmmoItem = usNewAmmoItem;
				if (fReloadingWithStack)
				{
					// add to end of stack
					// r8460
					if ( gTempObject.exists( ) )
						pAmmo->AddObjectsToStack( gTempObject, 1 );
				}
				else
				{
					// Copying the old ammo to the cursor in turnbased could screw up for the player
					// (suppose his inventory is full!)

					//ADB copying the old ammo to the cursor at any time will screw it up if the cursor ammo is a stack!
					if ( (gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT) && !EnoughPoints( pSoldier, (INT8) (bAPs + GetBasicAPsToPickupItem( pSoldier )), 0, FALSE ) // SANDRO
						|| pAmmo->ubNumberOfObjects > 1)
					{
						// try autoplace
						// r8460
						if ( gTempObject.exists( ) && !AutoPlaceObject( pSoldier, &gTempObject, FALSE ) )
						{
							// put it on the ground
							AddItemToPool( pSoldier->sGridNo, &gTempObject, 1, pSoldier->pathing.bLevel, 0 , -1 );
						}
						// delete the object now in the cursor
						pAmmo->RemoveObjectsFromStack(1);
					}
					else
					{
						// copy the old ammo to the cursor
						*pAmmo = gTempObject;
					}
				}
				break;
			case RELOAD_AUTOPLACE_OLD:
				// r8460
				if ( gTempObject.exists() && !AutoPlaceObject( pSoldier, &gTempObject, TRUE ) )
				{
					// error msg!
					return( FALSE );
				}
				// place first ammo in gun
				(*pGun)[subObject]->data.gun.ubGunShotsLeft = ubBulletsToMove;
				(*pGun)[subObject]->data.gun.ubGunAmmoType = Magazine[Item[pAmmo->usItem].ubClassIndex].ubAmmoType;
				(*pGun)[subObject]->data.gun.usGunAmmoItem = usNewAmmoItem;

				break;

			case RELOAD_TOPOFF:
				// ADD that many bullets to gun
				(*pGun)[subObject]->data.gun.ubGunShotsLeft += ubBulletsToMove;
				break;
		}

		// CHRISL: If we don't completely reload a SwapClips==0 weapon, set the "reload" flag
		// sevenfm: need rechambering only if reloading empty weapon and APsToReloadManually > 0
		if (fEmptyGun && Weapon[pGun->usItem].APsToReloadManually > 0)
		//if (Weapon[pGun->usItem].swapClips == 0 && GetMagSize(pGun, subObject) != (*pGun)[0]->data.gun.ubGunShotsLeft && Weapon[pGun->usItem].APsToReloadManually > 0)
		{
			(*pGun)[subObject]->data.gun.ubGunState |= GS_WEAPON_BEING_RELOADED;
			(*pGun)[subObject]->data.gun.ubGunState &= ~GS_CARTRIDGE_IN_CHAMBER;
		}
		else
		{
			(*pGun)[subObject]->data.gun.ubGunState &= ~GS_WEAPON_BEING_RELOADED;
			(*pGun)[subObject]->data.gun.ubGunState |= GS_CARTRIDGE_IN_CHAMBER;
		}

		if ( ! ( bReloadType == RELOAD_SWAP && !fReloadingWithStack ) )
		{
			// remove # of bullets, delete 1 object if necessary
			(*pAmmo)[ammoObject]->data.ubShotsLeft -= ubBulletsToMove;
			if ((*pAmmo)[ammoObject]->data.ubShotsLeft == 0)
			{
				pAmmo->RemoveObjectAtIndex(ammoObject);
				//pAmmo->RemoveObjectsFromStack(1);
			}
		}
	}

	// OK, let's play a sound of reloading too...
	// If this guy is visible...
	// sevenfm: play reload sound with lower volume for invisible soldiers
	//if ( pSoldier->bVisible != -1 )
	{
		// Play some effects!
		usReloadSound	= Weapon[ pGun->usItem ].sReloadSound;

		if ( usReloadSound != 0 && !IsAutoResolveActive() )
		{
			UINT32 ubVolume;

			if (pSoldier->IsSoldierPC())
				ubVolume = HIGHVOLUME;
			else
				ubVolume = MIDVOLUME;

			if (pSoldier->bVisible != TRUE)
				ubVolume = BTNVOLUME;

			// sevenfm: set volume and pan based on soldier's gridno when reloading
			if (!TileIsOutOfBounds(pSoldier->sGridNo))
				PlayJA2Sample(usReloadSound, RATE_11025, SoundVolume((INT8)ubVolume, pSoldier->sGridNo), 1, SoundDir(pSoldier->sGridNo));
			else
				PlayJA2Sample(usReloadSound, RATE_11025, ubVolume, 1, MIDDLEPAN);

			if (!TileIsOutOfBounds(pSoldier->sGridNo))
				MakeNoise(pSoldier->ubID, pSoldier->sGridNo, pSoldier->pathing.bLevel, pSoldier->bOverTerrainType, WEAPON_LNL_VOLUME, NOISE_BULLET_IMPACT);
		}
	}

	if (pSoldier->bTeam == gbPlayerNum)
	{
		// spit out a message if this is one of our folks reloading
		ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, Message[STR_PLAYER_RELOADS], pSoldier->GetName() );
	}

	DeductPoints( pSoldier, bAPs, 0 );
	//ADB ubWeight has been removed, see comments in OBJECTTYPE
	//pGun->ubWeight = CalculateObjectWeight( pGun );

	if ( (*pGun)[subObject]->data.gun.bGunAmmoStatus >= 0 )
	{
		// make sure gun ammo status is 100, if gun isn't jammed
		(*pGun)[subObject]->data.gun.bGunAmmoStatus = 100;
	}

	return( TRUE );
}

BOOLEAN EmptyWeaponMagazine( OBJECTTYPE * pWeapon, OBJECTTYPE *pAmmo, UINT32 subObject )
{
	UINT16 usReloadSound;

	CHECKF( pAmmo != NULL );

	if ( (*pWeapon)[subObject]->data.gun.ubGunShotsLeft > 0 )
	{
		CreateAmmo((*pWeapon)[subObject]->data.gun.usGunAmmoItem, pAmmo, (*pWeapon)[subObject]->data.gun.ubGunShotsLeft);

		(*pWeapon)[subObject]->data.gun.ubGunShotsLeft		= 0;
		(*pWeapon)[subObject]->data.gun.ubGunAmmoType	  = 0;
		// HEADROCK HAM 3.5: Leaving the ammo inside the gun causes EDB stats to display values as though the magazine
		// still gives some effects (like autopen reduction, range bonus, etcetera). I'm going to try to work around
		// this issue.
		(*pWeapon)[subObject]->data.gun.usGunAmmoItem = 0; // leaving the ammo item the same for auto-reloading purposes

		// Play some effects!
		usReloadSound	= Weapon[ pWeapon->usItem ].sReloadSound;

		if ( usReloadSound != 0 )
		{
			PlayJA2Sample( usReloadSound, RATE_11025, HIGHVOLUME, 1, MIDDLEPAN );
		}

		//ADB ubWeight has been removed, see comments in OBJECTTYPE
		//pWeapon->ubWeight = CalculateObjectWeight( pWeapon );
		// Pulmu bugfix:
		//ADB ubWeight has been removed, see comments in OBJECTTYPE
		//pAmmo->ubWeight = CalculateObjectWeight( pAmmo );
		// Pulmu end:
		return( TRUE );
	}
	else
	{
		// HEADROCK HAM 3.5: Clear the ammo type and magazine on player command. This will remove all bonuses by
		// the ammo and allow viewing the gun's normal stats. It will also change the weapon's ammocolor back to grey.
		(*pWeapon)[subObject]->data.gun.ubGunAmmoType = 0;
		(*pWeapon)[subObject]->data.gun.usGunAmmoItem = 0;
		//CHRISL: Clear the contents of pAmmo just in case
		pAmmo->initialize();
		return( FALSE );
	}
}

// HEADROCK HAM 3.3: Added an additional argument which helps the program pick a magazine
// that matches the ammotype currently used in the weapon. This makes for much smarter
// ammo selection.
INT8 FindAmmo( SOLDIERTYPE * pSoldier, UINT8 ubCalibre, UINT16 ubMagSize, UINT8 ubAmmoType, INT8 bExcludeSlot )
{
	INT8				bLoop;
	INT8				capLoop = 0;
	UINT16				curCap = 0, stackCap = 0;
	BOOLEAN				found = FALSE;
	INVTYPE *		pItem;

	//CHRISL: Update this to search for the largest appropriate mag if ubMagSize = ANY_MAGSIZE
	INT8 invsize = (INT8)pSoldier->inv.size();
	for (bLoop = HANDPOS; bLoop < invsize; ++bLoop)
	{
		//CHRISL: If in NIV, in combat and backpack is closed, don't look inside
		if(UsingNewAttachmentSystem() == true && (gTacticalStatus.uiFlags & INCOMBAT) && IsBackpackSlot(bLoop) == true && pSoldier->flags.ZipperFlag == FALSE)
			continue;

		if (pSoldier->inv[bLoop].exists() == true) {
			if (bLoop == bExcludeSlot)
			{
				continue;
			}
			pItem = &(Item[pSoldier->inv[bLoop].usItem]);
			if (pItem->usItemClass == IC_AMMO)
			{
				if (Magazine[pItem->ubClassIndex].ubCalibre == ubCalibre)
				{
					if(ubMagSize != ANY_MAGSIZE && Magazine[pItem->ubClassIndex].ubMagSize == ubMagSize)
					{
						found = TRUE;
						// looking for specific size.  return if found
						// Find fullest mag
						for(int i = 0; i<pSoldier->inv[bLoop].ubNumberOfObjects; i++)
						{
							stackCap = __max(stackCap, pSoldier->inv[bLoop][i]->data.ubShotsLeft);
						}
						// If found a similar-sized magazine to the best one found so far, but this new one
						// has the same ammotype as specified in the arguments, then this is a better choice!
						if(stackCap > curCap || 
							(stackCap == curCap && Magazine[pItem->ubClassIndex].ubAmmoType == ubAmmoType))
						{
							curCap = stackCap;
							capLoop = bLoop;
						}
						//return( bLoop );
					}
					else if(ubMagSize == ANY_MAGSIZE)
					{
						found = TRUE;
						// looking for any mag size.  find the largest
						if(Magazine[pItem->ubClassIndex].ubMagSize > curCap)
						{
							curCap = Magazine[pItem->ubClassIndex].ubMagSize;
							capLoop = bLoop;
						}
					}
				}
			}
		}
	}
	if(found == TRUE)
		return( capLoop );
	return( NO_SLOT );
}


INT8 FindAmmoToReload( SOLDIERTYPE * pSoldier, INT8 bWeaponIn, INT8 bExcludeSlot )
{
	OBJECTTYPE *pObj;
	INT8		bSlot;
	UINT16		magSize;
	// sevenfm
	UINT16 usItem;
	INT8 bFoundSlot;
	UINT8 ubType;
	UINT8 ubFoundType;

	if (pSoldier == NULL)
	{
		return( NO_SLOT );
	}

	if(pSoldier->bWeaponMode == WM_ATTACHED_GL || pSoldier->bWeaponMode == WM_ATTACHED_GL_BURST || pSoldier->bWeaponMode == WM_ATTACHED_GL_AUTO)//dnl ch63 230813
	{
		pObj = FindAttachment_GrenadeLauncher(&pSoldier->inv[bWeaponIn]);
		AssertMsg(pObj, "FindAmmoToReload: could not find attached grenade launcher.");
	}
	else
	{
		// Flugente: check for underbarrel weapons and use that object if necessary
		pObj = pSoldier->GetUsedWeapon( &(pSoldier->inv[bWeaponIn]) );
		AssertMsg(pObj, "FindAmmoToReload: could not find underbarrel weapon.");

		//<SB> manual recharge
		if ((*pObj)[0]->data.gun.ubGunShotsLeft && !((*pObj)[0]->data.gun.ubGunState & GS_CARTRIDGE_IN_CHAMBER) )
			return bWeaponIn;
		//</SB>
	}

	if ( Item[pObj->usItem].usItemClass == IC_GUN && !Item[pObj->usItem].cannon )
	{
		//MM: make reload use crates/boxes if not in combat...
		if ( (gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT) )
			magSize = GetMagSize(pObj);
		else
			magSize = ANY_MAGSIZE;

		// look for any ammo that matches which is of the same calibre and magazine size
		bSlot = FindAmmo( pSoldier, Weapon[pObj->usItem].ubCalibre, GetMagSize(pObj), GetAmmoType(pObj), bExcludeSlot );

		if (bSlot != NO_SLOT)
		{
			return( bSlot );
		}
		else
		{
			// look for any ammo that matches which is of the same calibre (different size okay)
			return( FindAmmo( pSoldier, Weapon[pObj->usItem].ubCalibre, ANY_MAGSIZE, GetAmmoType(pObj), bExcludeSlot ) );
		}
	}
	else
	{
		bFoundSlot = NO_SLOT;

		for (UINT16 cnt = 0; cnt < gMAXLAUNCHABLES_READ; cnt++)
		{
			if ( Launchable[cnt][0] == 0 && Launchable[cnt][1] == 0)
				break;
			
			if ( pObj->usItem == Launchable[cnt][1] )
			{
				// found launchable item
				usItem = Launchable[cnt][0];
				ubType = (UINT8)Explosive[ Item[ usItem ].ubClassIndex ].ubType;

				// sevenfm: avoid using flares at night
				if (ubType == EXPLOSV_FLARE && !NightLight())
				{
					continue;
				}

				bSlot = FindObj( pSoldier, usItem );

				if( bSlot != NO_SLOT )
				{
					// found item in inventory, check type priority
					// always prefer normal type
					// smoke grenades have low priority
					// mustard gas has priority over tear gas
					// flare has low priority during day time
					if( bFoundSlot == NO_SLOT ||
						ubType == EXPLOSV_NORMAL && ubFoundType != EXPLOSV_NORMAL ||
						ubType == EXPLOSV_MUSTGAS && ubFoundType == EXPLOSV_TEARGAS ||
						ubType != EXPLOSV_SMOKE && ubFoundType == EXPLOSV_SMOKE ||						
						ubType != EXPLOSV_FLARE && ubType != EXPLOSV_SMOKE && ubFoundType == EXPLOSV_FLARE && pSoldier->aiData.bOppCnt > 0)
					{
						bFoundSlot = bSlot;
						ubFoundType = ubType;
					}					
				}
			}
		}
		return bFoundSlot;
	}
}

BOOLEAN AutoReload( SOLDIERTYPE * pSoldier )
{
	OBJECTTYPE *pObj, *pObj2;
	INT8		bSlot;
	INT16 bAPCost;
	BOOLEAN		fRet;

	CHECKF( pSoldier );

	// Flugente: check for underbarrel weapons and use that object if necessary
	pObj = pSoldier->GetUsedWeapon( &(pSoldier->inv[HANDPOS]) );

//<SB> manual recharge
	if ((*pObj)[0]->data.gun.ubGunShotsLeft && !((*pObj)[0]->data.gun.ubGunState & GS_CARTRIDGE_IN_CHAMBER) )
	{
		(*pObj)[0]->data.gun.ubGunState |= GS_CARTRIDGE_IN_CHAMBER;
		(*pObj)[0]->data.gun.ubGunState &= ~GS_WEAPON_BEING_RELOADED;

		INT16 sModifiedReloadAP = Weapon[Item[(pObj)->usItem].ubClassIndex].APsToReloadManually;

		// modify by ini values
		if ( Item[ pObj->usItem ].usItemClass == IC_GUN )
			sModifiedReloadAP = (INT16)(sModifiedReloadAP * gItemSettings.fAPtoReloadManuallyModifierGun[Weapon[pObj->usItem].ubWeaponType]);
		else if ( Item[ pObj->usItem ].usItemClass == IC_LAUNCHER )
			sModifiedReloadAP = (INT16)(sModifiedReloadAP * gItemSettings.fAPtoReloadManuallyModifierLauncher);

		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// STOMP traits - SANDRO
		if ( gGameOptions.fNewTraitSystem )
		{
			// Sniper trait makes chambering a round faster
			if (( Weapon[Item[(pObj)->usItem].ubClassIndex].ubWeaponType == GUN_SN_RIFLE || Weapon[Item[(pObj)->usItem].ubClassIndex].ubWeaponType == GUN_RIFLE ) && HAS_SKILL_TRAIT( pSoldier, SNIPER_NT ))
				DeductPoints(pSoldier, ((sModifiedReloadAP * (100 - gSkillTraitValues.ubSNChamberRoundAPsReduction * NUM_SKILL_TRAITS( pSoldier, SNIPER_NT )))/100), 0);
			// Ranger trait makes pumping shotguns faster
			else if (( Weapon[Item[(pObj)->usItem].ubClassIndex].ubWeaponType == GUN_SHOTGUN ) && HAS_SKILL_TRAIT( pSoldier, RANGER_NT ))
				DeductPoints(pSoldier, ((sModifiedReloadAP * (100 - gSkillTraitValues.ubRAPumpShotgunsAPsReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT )))/100), 0);
			else
				DeductPoints(pSoldier, sModifiedReloadAP, 0);
		}
		else
		{
			DeductPoints(pSoldier, sModifiedReloadAP, 0);
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////////

		//PlayJA2Sample(Weapon[Item[pObj->usItem].ubClassIndex].ManualReloadSound, RATE_11025, SoundVolume(LNLVOLUME, pSoldier->sGridNo), 1, SoundDir(pSoldier->sGridNo));
		pSoldier->PlayLNL(pObj->usItem);

		if ( pSoldier->IsValidSecondHandShot( ) )
		{
			pObj2 = &(pSoldier->inv[SECONDHANDPOS]);

			if ((*pObj2)[0]->data.gun.ubGunShotsLeft && !((*pObj2)[0]->data.gun.ubGunState & GS_CARTRIDGE_IN_CHAMBER) )
			{
				(*pObj2)[0]->data.gun.ubGunState |= GS_CARTRIDGE_IN_CHAMBER;
				//PlayJA2Sample(Weapon[Item[pObj2->usItem].ubClassIndex].ManualReloadSound, RATE_11025, SoundVolume(LNLVOLUME, pSoldier->sGridNo), 1, SoundDir(pSoldier->sGridNo));
				pSoldier->PlayLNL(pObj2->usItem);
			}
		}

		// sevenfm: update interface after chambering
		DirtyMercPanelInterface(pSoldier, DIRTYLEVEL2);

		return TRUE;
	}
	else
	{
		if ( pSoldier->IsValidSecondHandShot( ) )
		{
			pObj2 = &(pSoldier->inv[SECONDHANDPOS]);

			if ((*pObj2)[0]->data.gun.ubGunShotsLeft && !((*pObj2)[0]->data.gun.ubGunState & GS_CARTRIDGE_IN_CHAMBER) )
			{
				(*pObj2)[0]->data.gun.ubGunState |= GS_CARTRIDGE_IN_CHAMBER;

				INT16 sModifiedReloadAP = Weapon[Item[(pObj2)->usItem].ubClassIndex].APsToReloadManually;

				// modify by ini values
				if ( Item[ pObj2->usItem ].usItemClass == IC_GUN )
					sModifiedReloadAP = (INT16)(sModifiedReloadAP * gItemSettings.fAPtoReloadManuallyModifierGun[Weapon[pObj2->usItem].ubWeaponType]);
				else if ( Item[ pObj2->usItem ].usItemClass == IC_LAUNCHER )
					sModifiedReloadAP = (INT16)(sModifiedReloadAP * gItemSettings.fAPtoReloadManuallyModifierLauncher);

				////////////////////////////////////////////////////////////////////////////////////////////////////////
				// STOMP traits - SANDRO
				if ( gGameOptions.fNewTraitSystem )
				{
					// Sniper trait makes chambering a round faster
					if (( Weapon[Item[(pObj2)->usItem].ubClassIndex].ubWeaponType == GUN_SN_RIFLE || Weapon[Item[(pObj2)->usItem].ubClassIndex].ubWeaponType == GUN_RIFLE ) && HAS_SKILL_TRAIT( pSoldier, SNIPER_NT ))
						DeductPoints(pSoldier, ((sModifiedReloadAP * (100 - gSkillTraitValues.ubSNChamberRoundAPsReduction * NUM_SKILL_TRAITS( pSoldier, SNIPER_NT )))/100), 0);
					// Ranger trait makes pumping shotguns faster
					else if (( Weapon[Item[(pObj2)->usItem].ubClassIndex].ubWeaponType == GUN_SHOTGUN ) && HAS_SKILL_TRAIT( pSoldier, RANGER_NT ))
						DeductPoints(pSoldier, ((sModifiedReloadAP * (100 - gSkillTraitValues.ubRAPumpShotgunsAPsReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT )))/100), 0);
					else
						DeductPoints(pSoldier, sModifiedReloadAP, 0);
				}
				else
				{
					DeductPoints(pSoldier, sModifiedReloadAP, 0);
				}
				////////////////////////////////////////////////////////////////////////////////////////////////////////

				//PlayJA2Sample(Weapon[Item[pObj2->usItem].ubClassIndex].ManualReloadSound, RATE_11025, SoundVolume(MIDVOLUME, pSoldier->sGridNo), 1, SoundDir(pSoldier->sGridNo));
				pSoldier->PlayLNL(pObj2->usItem);

				// sevenfm: update interface after reloading
				DirtyMercPanelInterface(pSoldier, DIRTYLEVEL2);

				return TRUE;
			}
		}
	}
//</SB>

	if (Item[pObj->usItem].usItemClass == IC_GUN || Item[pObj->usItem].usItemClass == IC_LAUNCHER)
	{
		bSlot = FindAmmoToReload( pSoldier, HANDPOS, NO_SLOT );
		if (bSlot != NO_SLOT)
		{
			// reload using this ammo!
			fRet = ReloadGun( pSoldier, pObj, &(pSoldier->inv[bSlot]) );
			// if we are valid for two-pistol shooting (reloading) and we have enough APs still
			// then do a reload of both guns!
			if ( (fRet == TRUE) && pSoldier->IsValidSecondHandShotForReloadingPurposes( ) )
			{
				// Flugente: check for underbarrel weapons and use that object if necessary
				pObj = pSoldier->GetUsedWeapon( &(pSoldier->inv[SECONDHANDPOS]) );

				bSlot = FindAmmoToReload( pSoldier, SECONDHANDPOS, NO_SLOT );
				if (bSlot != NO_SLOT)
				{
					// ce would reload using this ammo!
					bAPCost = GetAPsToReloadGunWithAmmo( pSoldier, pObj, &(pSoldier->inv[bSlot] ) );
					if ( EnoughPoints( pSoldier, (INT16) bAPCost, 0, FALSE ) )
					{
						// reload the 2nd gun too
						fRet = ReloadGun( pSoldier, pObj, &(pSoldier->inv[bSlot]) );
					}
					else
					{
						ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, Message[ STR_RELOAD_ONLY_ONE_GUN ], pSoldier->GetName() );
					}
				}
			}

			DirtyMercPanelInterface( pSoldier, DIRTYLEVEL2 );
			return( fRet );
		}
	}

	// couldn't reload
	return( FALSE );
}

BOOLEAN CanAutoReload(SOLDIERTYPE * pSoldier)
{
	OBJECTTYPE *pObj;
	INT8		bSlot;

	CHECKF(pSoldier);

	// Flugente: check for underbarrel weapons and use that object if necessary
	pObj = pSoldier->GetUsedWeapon(&(pSoldier->inv[HANDPOS]));	

	if (pObj && 
		(Item[pObj->usItem].usItemClass == IC_GUN || Item[pObj->usItem].usItemClass == IC_LAUNCHER))
	{
		bSlot = FindAmmoToReload(pSoldier, HANDPOS, NO_SLOT);

		if (bSlot != NO_SLOT)
		{
			// if we are valid for two-pistol shooting (reloading) and we have enough APs still
			// then do a reload of both guns!
			if (pSoldier->IsValidSecondHandShotForReloadingPurposes())
			{
				// Flugente: check for underbarrel weapons and use that object if necessary
				//pObj = pSoldier->GetUsedWeapon(&(pSoldier->inv[SECONDHANDPOS]));
				bSlot = FindAmmoToReload(pSoldier, SECONDHANDPOS, NO_SLOT);

				if (bSlot != NO_SLOT)
				{
					return TRUE;
				}
			}
			else
			{
				return TRUE;
			}			
		}
	}

	return FALSE;
}

INT8 GetAttachmentComboMerge( OBJECTTYPE * pObj, UINT8 subObject )
{
	INT8		bIndex = 0;
	INT8		bAttachLoop;

	/* check the whole Array of possible Attachments, while there are still entries ... */
	while( AttachmentComboMerge[ bIndex ].usItem != NOTHING )
	{
		/* if we found our current Object as "basic hand" item, then
     * we have found at least ONE entry of our item (may be there are more)
     */
		OBJECTTYPE* pAttachment = 0;
		if ( pObj->usItem == AttachmentComboMerge[ bIndex ].usItem )
		{
			// search for all the appropriate attachments
		  /* every ComboMerge must have at least one attachments Field */
			for ( bAttachLoop = 0; bAttachLoop < MAX_DEFAULT_ATTACHMENTS; bAttachLoop++ )
			{
				/* if the none of both Fields contains anything, do not merge */
				if ( AttachmentComboMerge[ bIndex ].usAttachment[ bAttachLoop ] == NOTHING )
				{
					continue;
				}

			/* 2007-05-27, Sgt_Kolja:
			 * do not return, but break the inner loop moved away, otherwise
			 * we can only have ONE attachmentCombo per basic item. F.I. if we want
			 * to make a Dart gun from Dart pistol by adding (a buttstock and) wheter a
			 * steel tube /or/ a Gun Barrel Extender, the old code wouldn't work for
			 * the Gun Barel Extender, since it would never been tested.
			 */
				pAttachment = FindAttachment( pObj, AttachmentComboMerge[ bIndex ].usAttachment[ bAttachLoop ], subObject );
				if ( pAttachment->exists() == false )
				{
					// didn't find something required
					break;
				}
			}
				// found everything required?
		  /* 2007-05-27, Sgt_Kolja: Not-found-condition moved from above, otherwise we can only have ONE attachmentCombo per basic item */
			//WarmSteel - Added check to see if the resulting item is valid.
			if ( pAttachment->exists() && ItemIsLegal(AttachmentComboMerge[bIndex].usResult, TRUE) )
			{
				return( bIndex );
			}
		} /* end-if-this-is-our-item */

		/* try next Attachment Order */
		bIndex++;
	}

	return( -1 );
}



void PerformAttachmentComboMerge( OBJECTTYPE * pObj, INT8 bAttachmentComboMerge )
{
	INT8		bAttachLoop;
	UINT32	uiStatusTotal = 0;
	INT8		bNumStatusContributors = 0;

	// This object has been validated as available for an attachment combo merge.
	// - find all attachments in list and destroy them
	// - status of new object should be average of items including attachments
	// - change object

	for ( bAttachLoop = 0; bAttachLoop < MAX_DEFAULT_ATTACHMENTS; bAttachLoop++ )
	{
		if ( AttachmentComboMerge[ bAttachmentComboMerge ].usAttachment[ bAttachLoop ] == NOTHING )
		{
			continue;
		}

		OBJECTTYPE* pAttachment = FindAttachment( pObj, AttachmentComboMerge[ bAttachmentComboMerge ].usAttachment[ bAttachLoop ] );
		AssertMsg( pAttachment != 0, String( "Attachment combo merge couldn't find a necessary attachment" ) );

		uiStatusTotal += (*pAttachment)[0]->data.objectStatus;
		bNumStatusContributors++;

		pObj->RemoveAttachment(pAttachment);
	}

	uiStatusTotal += (*pObj)[0]->data.objectStatus;
	bNumStatusContributors++;

	pObj->usItem = AttachmentComboMerge[ bAttachmentComboMerge ].usResult;
	RemoveProhibitedAttachments(NULL, pObj, pObj->usItem);
	(*pObj)[0]->data.objectStatus = (INT8) (uiStatusTotal / bNumStatusContributors );
}

BOOLEAN OBJECTTYPE::AttachObjectOAS( SOLDIERTYPE * pSoldier, OBJECTTYPE * pAttachment, BOOLEAN playSound, UINT8 subObject )
{
	if (pAttachment->exists() == false) 
	{
		return FALSE;
	}
	if ((*this)[subObject]->attachments.size() >= OLD_MAX_ATTACHMENTS_101) 
	{
		return FALSE;
	}
	//CHRISL: If we're adding a loaded UGL, then we have to make sure there are actually 2 open attachment slots instead of 1
	if(Item[pAttachment->usItem].grenadelauncher && (*pAttachment)[0]->attachments.size() > 0) 
	{
		if ((*this)[subObject]->attachments.size() >= (OLD_MAX_ATTACHMENTS_101-1))
			return FALSE;
	}
	// sevenfm: don't allow attaching activated grenades
	if (Item[pAttachment->usItem].usItemClass & IC_EXPLOSV && (*pAttachment)[0]->data.misc.bDelay)
	{
		return FALSE;
	}

	static OBJECTTYPE attachmentObject;

	UINT16		usResult, usResult2, ubLimit;
	INT8		bLoop;
	UINT8		ubType, ubAPCost;
	INT32		iCheckResult;
	INT8		bAttachInfoIndex = -1;
	INT8		bAttachComboMerge;
	BOOLEAN		fValidLaunchable = FALSE;
	UINT16		oldMagSize = 0;

	// extra tags
	UINT16	usPointsUsed;
	UINT16	usPointsAdded;
	UINT16	usExtraItem;
	UINT16	usExtraItemPointsUsed;
	UINT16	usSkillCheck;
	UINT16	usSkillDifficulty;

	//CHRISL: This is so we can try to determine if the removed attachment altered our mag size.
	if(Item[this->usItem].usItemClass == IC_GUN)
		oldMagSize = GetMagSize(this);

	bool canOnlyAttach1 = false;

	//if this is an attachment or ammo for a launchable item
	fValidLaunchable = ValidLaunchable( pAttachment->usItem, this->usItem );
	//CHRISL: If we don't want to play the sound, it's a good bet we don't want to display any messages either
	if ( fValidLaunchable || ValidItemAttachment( this, pAttachment->usItem, TRUE, playSound, subObject ) )
	{
		//if there is already an attachment of the same type, we want to try swapping / replacing it
		OBJECTTYPE* pAttachmentPosition = 0;

		// find an attachment position...
		// second half of this 'if' is for attaching GL grenades to a gun w/attached GL
		if ( fValidLaunchable || (Item[pAttachment->usItem].glgrenade && FindAttachmentByClass(this, IC_LAUNCHER, subObject) != 0 ) )
		{
			canOnlyAttach1 = true;
			// try replacing if possible
			pAttachmentPosition = FindAttachmentByClass( this, Item[ pAttachment->usItem ].usItemClass, subObject );
		}
		else
		{
			// try replacing if possible
			pAttachmentPosition = FindAttachment( this, pAttachment->usItem, subObject );
		}

		if ( pSoldier )
		{
			//did the soldier damage it?
			bAttachInfoIndex = GetAttachmentInfoIndex( pAttachment->usItem );
			// in-game (not behind the scenes) attachment
			if ( bAttachInfoIndex != -1 && AttachmentInfo[ bAttachInfoIndex ].bAttachmentSkillCheck != NO_CHECK )
			{
				iCheckResult = SkillCheck( pSoldier, AttachmentInfo[ bAttachInfoIndex ].bAttachmentSkillCheck, AttachmentInfo[ bAttachInfoIndex ].bAttachmentSkillCheckMod );
				if (iCheckResult < 0)
				{
					// the attach failure damages both items
					DamageObj( this, (INT8) -iCheckResult, subObject );
					DamageObj( pAttachment, (INT8) -iCheckResult, subObject );

					// there should be a quote here!
					pSoldier->DoMercBattleSound( BATTLE_SOUND_CURSE1 );
					if ( gfInItemDescBox )
					{
						DeleteItemDescriptionBox();
					}
					return( FALSE );
				}
			}
			else if ( Item[this->usItem].tripwire && Item[pAttachment->usItem].usItemClass & IC_GUN ) // Madd: gun on tripwire
			{
				iCheckResult = SkillCheck( pSoldier, ATTACHING_DETONATOR_CHECK, 0 );
				if (iCheckResult < 0)
				{
					// the attach failure damages both items
					DamageObj( this, (INT8) -iCheckResult, subObject );
					DamageObj( pAttachment, (INT8) -iCheckResult, subObject );

					// there should be a quote here!
					pSoldier->DoMercBattleSound( BATTLE_SOUND_CURSE1 );
					if ( gfInItemDescBox )
					{
						DeleteItemDescriptionBox();
					}
					return( FALSE );
				}
			}

			if ( ValidItemAttachment( this, pAttachment->usItem, TRUE, TRUE, subObject  ) && playSound ) // not launchable
			{
				// attachment sounds
				if ( Item[ this->usItem ].usItemClass & IC_WEAPON || Item[this->usItem].tripwire )  //Madd: attaching items to tripwire makes gun attach sound
				{
					PlayJA2Sample( ATTACH_TO_GUN, RATE_11025, SoundVolume( MIDVOLUME, pSoldier->sGridNo ), 1, SoundDir( pSoldier->sGridNo ) );
				}
				else if ( Item[ this->usItem ].usItemClass & IC_ARMOUR )
				{
					PlayJA2Sample( ATTACH_CERAMIC_PLATES, RATE_11025, SoundVolume( MIDVOLUME, pSoldier->sGridNo ), 1, SoundDir( pSoldier->sGridNo ) );
				}
				else if ( Item[ this->usItem ].usItemClass & IC_BOMB )
				{
					PlayJA2Sample( ATTACH_DETONATOR, RATE_11025, SoundVolume( MIDVOLUME, pSoldier->sGridNo ), 1, SoundDir( pSoldier->sGridNo ) );
				}
			}
		}

		if (pAttachmentPosition) {
			if (canOnlyAttach1 == true) {
				if(pAttachment->ubNumberOfObjects > 1)
				{
					//We don't want to inadvertantly load two rounds during a swap so don't allow a swap if we're holding a stack
					return(FALSE);
				}
				//we have requested to swap the attachment on the cursor with the current attachment
				SwapObjs(pAttachmentPosition,pAttachment);
				return(TRUE);
			}
		}

		//ADB moved after skill check!
		//lalien: added to make a GL/RL work when reloaded manually
		// the AP costs for reloading GL/RL will be taken from weapons.xml ( wrong place!!! the AP's are deducted in DeleteItemDescriptionBox() )
		//if (pAttachmentPosition) {
		if(pAttachmentPosition || (pAttachmentPosition == NULL && (*this)[subObject]->attachments.size() < OLD_MAX_ATTACHMENTS_101)){
			//we know we are replacing this attachment
			if ( Item[ this->usItem ].usItemClass == IC_LAUNCHER || Item[this->usItem].cannon )
			{
				if ( fValidLaunchable )
				{
					(*this)[subObject]->data.gun.usGunAmmoItem = pAttachment->usItem;
					//we have reloaded a launchable, so the ammo is gone from the original object
				}
			}
		}

		//unfortunately must come before possible attachment swap
		if (Item[pAttachment->usItem].grenadelauncher )
		{
			// transfer any attachments from the grenade launcher to the gun
			(*this)[subObject]->attachments.splice((*this)[subObject]->attachments.begin(), (*pAttachment)[0]->attachments,
				(*pAttachment)[0]->attachments.begin(), (*pAttachment)[0]->attachments.end());
		}

		if (pAttachmentPosition) {
			//we are swapping the attachments, and we know we do NOT need to worry about attachment stack size
			//CHRISL: Actually, we do need to worry about attachment stack size since we might have a stack in our cursor.
			//	Rather then doing a simple swap, try moving the existing attachment to our cursor stack, then attach one item
			//	from the cursor stack.

			pAttachment->AddObjectsToStack(*pAttachmentPosition,-1,pSoldier,NUM_INV_SLOTS,MAX_OBJECTS_PER_SLOT);
			pAttachment->MoveThisObjectTo(attachmentObject,1,pSoldier,NUM_INV_SLOTS,1);
			*pAttachmentPosition = attachmentObject;
			//backup the original attachment
			//attachmentObject = *pAttachmentPosition;

			//place the new one
			//*pAttachmentPosition = *pAttachment;

			//whatever pAttachment pointed to is now the original attachment
			//*pAttachment = attachmentObject;
		}
		else {
			//we only placed one of the stack, pAttachment could have any number of objects
			if (pAttachment->MoveThisObjectTo(attachmentObject, 1, pSoldier, NUM_INV_SLOTS, 1) == 0)
			{
				(*this)[subObject]->attachments.push_back(attachmentObject);
			}
		}

		//Ammo might have changed after adding an attachment.
		if ( Item[this->usItem].usItemClass == IC_GUN && (*this)[subObject]->data.gun.usGunAmmoItem != NONE && (*this)[subObject]->data.gun.ubGunShotsLeft > 0 && oldMagSize != GetMagSize(this, subObject) )
		{
			if ( (*this)[subObject]->data.gun.ubGunShotsLeft > GetMagSize(this, subObject) )
			{ //Too many rounds, eject ammo
				EjectAmmoAndPlace(pSoldier, this, subObject);
			}
			//CHRISL: We should update the usGunAmmoItem if we've changed the ammo capacity
			if((*this)[subObject]->data.gun.usGunAmmoItem != NONE){
				UINT16 usNewAmmoItem;
				usNewAmmoItem = FindReplacementMagazine(Weapon[this->usItem].ubCalibre ,GetMagSize(this, subObject),Magazine[Item[(*this)[subObject]->data.gun.usGunAmmoItem].ubClassIndex].ubAmmoType);
				(*this)[subObject]->data.gun.usGunAmmoItem = usNewAmmoItem;
			}
		}

		if(Item[this->usItem].usItemClass == IC_GUN && oldMagSize != GetMagSize(this, subObject)){
			fInterfacePanelDirty = DIRTYLEVEL2;
			RenderBulletIcon(this, subObject);
		}

		// Check for attachment merge combos here
		//CHRISL: Only do this if we're looking at a single item.  Don't try a combo merge when dealing with a stack
		bAttachComboMerge = GetAttachmentComboMerge( this, subObject );
		if ( bAttachComboMerge != -1 )
		{
			if(this->ubNumberOfObjects == 1)
			{
				PerformAttachmentComboMerge( this, bAttachComboMerge );
				if ( bAttachInfoIndex != -1 && AttachmentInfo[ bAttachInfoIndex ].bAttachmentSkillCheckMod < 20 )
				{
					StatChange( pSoldier, MECHANAMT, (INT8) ( 20 - AttachmentInfo[ bAttachInfoIndex ].bAttachmentSkillCheckMod ), FALSE );
					StatChange( pSoldier, WISDOMAMT, (INT8) ( 20 - AttachmentInfo[ bAttachInfoIndex ].bAttachmentSkillCheckMod ), FALSE );
				}
			}
			//CHRISL: If we're looking at a stack, we need to unattach the object we just attached
			else if(this->RemoveAttachment(&attachmentObject, pAttachmentPosition, subObject))
			{
				pAttachment->AddObjectsToStack(attachmentObject, -1, pSoldier, NUM_INV_SLOTS, MAX_OBJECTS_PER_SLOT);
			}
		}

		//ADB ubWeight has been removed, see comments in OBJECTTYPE
		//this->ubWeight = CalculateObjectWeight( this );
		//ADB ubWeight has been removed, see comments in OBJECTTYPE
		//pAttachment->ubWeight = CalculateObjectWeight(pAttachment);

		if ( pSoldier != NULL )
			ApplyEquipmentBonuses(pSoldier);

		return( TRUE );
	}
	// check for merges
	else if (EvaluateValidMerge( pAttachment->usItem, this->usItem, &usResult, &usResult2, &ubType, &ubAPCost, &usPointsUsed, &usPointsAdded, &usExtraItem, &usExtraItemPointsUsed, &usSkillCheck, &usSkillDifficulty ))
	{
		//CHRISL: we don't want to try to merge IC_LBEGEAR items that are currently storing items.
		if(Item[this->usItem].usItemClass == IC_LBEGEAR)
		{
			if(this->IsActiveLBE(subObject) == true)
				return( FALSE );
		}
		//CHRISL: We don't want to do any merges if we're looking at a stack of items, with the exception of combines.
		if ( this->ubNumberOfObjects > 1 && ubType != COMBINE_POINTS )
		{
			return( FALSE );
		}
		//if ( ubType != COMBINE_POINTS )
		{
			if ( pSoldier )
			{
				if ( !EnoughPoints( pSoldier, ubAPCost, 0, TRUE ) )
				{
					return( FALSE );
				}

				DeductPoints( pSoldier, ubAPCost, 0 );
			}
		}

		switch( ubType )
		{
			case USE_ITEM:
			case USE_ITEM_HARD:
				// the merge will combine the two items
				if ( pSoldier )
				{
					if ( ubType == USE_ITEM_HARD )
					{
						// requires a skill check, and gives experience
						iCheckResult = SkillCheck( pSoldier, ATTACHING_SPECIAL_ITEM_CHECK, -30 );
						if (iCheckResult < 0)
						{
							// could have a chance of detonation
							// for now, damage both objects
							DamageObj( this, (INT8) -iCheckResult );
							DamageObj( pAttachment, (INT8) -iCheckResult );
							pSoldier->DoMercBattleSound( BATTLE_SOUND_CURSE1 );
							return( FALSE );
						}
						StatChange( pSoldier, MECHANAMT, 25, FALSE );
						StatChange( pSoldier, WISDOMAMT, 5, FALSE );

						// SANDRO - merc records - merging items
						if ( pSoldier->ubProfile != NO_PROFILE )
							gMercProfiles[ pSoldier->ubProfile ].records.usItemsCombined++;
					}

					//Madd: note that use_item cannot produce two different items!!! so it doesn't use usResult2

					//Madd: unload guns after merge if ammo caliber or mag size don't match
					if ( Item[this->usItem].usItemClass == IC_GUN && (*this)[0]->data.gun.usGunAmmoItem != NONE && (*this)[0]->data.gun.ubGunShotsLeft > 0 )
					{
						if ( Item[usResult].usItemClass != IC_GUN || Weapon[Item[usResult].ubClassIndex].ubCalibre != Weapon[Item[this->usItem].ubClassIndex].ubCalibre || (*this)[0]->data.gun.ubGunShotsLeft > Weapon[Item[usResult].ubClassIndex].ubMagSize )
						{ // item types/calibers/magazines don't match, spit out old ammo
							EjectAmmoAndPlace(pSoldier, this);
						}
					}

					RemoveProhibitedAttachments(pSoldier, this, usResult);

					this->usItem = usResult;
					//AutoPlaceObject( pAttachment );
					//ADB ubWeight has been removed, see comments in OBJECTTYPE
					//this->ubWeight = CalculateObjectWeight( this );
					if (pSoldier->bTeam == gbPlayerNum)
					{
						pSoldier->DoMercBattleSound( BATTLE_SOUND_COOL1 );
					}
					ApplyEquipmentBonuses(pSoldier);
					return TRUE;
				}
				else
					return FALSE;
			case COMBINE_POINTS:
				// transfer points...
				UINT32 combinedAmount;
				UINT32 attachmentAmount;
				//CHRISL: Treat differently if we're combining money items (other then MONEY)
				if(Item[this->usItem].usItemClass != IC_MONEY)
				{
					if ( Item[ this->usItem ].usItemClass == IC_AMMO )
					{
						ubLimit = Magazine[ Item[ this->usItem ].ubClassIndex ].ubMagSize;
					}
					else
					{
						ubLimit = 100;
					}

					// count down through # of attaching items and add to status of item in position 0
					for (bLoop = 0; bLoop < pAttachment->ubNumberOfObjects; )
					{
						//ADB need to watch for overflow here (thus UINT32), and need to cast to UINT8 before adding
						combinedAmount = (UINT16)(*this)[subObject]->data.objectStatus + (UINT16)(*pAttachment)[0]->data.objectStatus;
						if (combinedAmount <= ubLimit)
						{
							// consume this one totally and continue
							(*this)[subObject]->data.objectStatus = combinedAmount;
							pAttachment->RemoveObjectsFromStack(1);
						}
						else
						{
							// add part of this one and then we're done
							attachmentAmount = (UINT16)(*pAttachment)[0]->data.objectStatus;
							attachmentAmount -= (ubLimit - (UINT16)(*this)[subObject]->data.objectStatus);
							(*pAttachment)[0]->data.objectStatus = attachmentAmount;
							if ((*pAttachment)[0]->data.ubShotsLeft == 0) {
								pAttachment->RemoveObjectsFromStack(1);
							}
							(*this)[subObject]->data.ubShotsLeft = ubLimit;
							break;
						}
					}
				}
				else
				{
					ubLimit = MAX_MONEY_PER_SLOT;
					combinedAmount = (*this)[subObject]->data.money.uiMoneyAmount + (*pAttachment)[0]->data.money.uiMoneyAmount;
					if (combinedAmount <= ubLimit)
					{
						(*this)[subObject]->data.money.uiMoneyAmount = combinedAmount;
						pAttachment->RemoveObjectsFromStack(1);
					}
					else
					{
						attachmentAmount = (*pAttachment)[0]->data.money.uiMoneyAmount;
						attachmentAmount -= (ubLimit - (*this)[subObject]->data.money.uiMoneyAmount);
						(*pAttachment)[0]->data.money.uiMoneyAmount = attachmentAmount;
						if((*pAttachment)[0]->data.money.uiMoneyAmount == 0)
						{
							pAttachment->RemoveObjectsFromStack(1);
						}
						(*this)[subObject]->data.money.uiMoneyAmount = ubLimit;
						break;
					}
				}
				break;
			case DESTRUCTION:
				// the merge destroyed both items!
				this->RemoveObjectsFromStack(1);
				pAttachment->RemoveObjectsFromStack(1);
				if ( pSoldier )
				{
					pSoldier->DoMercBattleSound( BATTLE_SOUND_CURSE1 );
				}
				break;
			case ELECTRONIC_MERGE:
				if ( pSoldier )
				{
					iCheckResult = SkillCheck( pSoldier, ATTACHING_SPECIAL_ELECTRONIC_ITEM_CHECK, -30 );
					if ( iCheckResult < 0 )
					{
						DamageObj( this, (INT8) -iCheckResult );
						DamageObj( pAttachment, (INT8) -iCheckResult );
						pSoldier->DoMercBattleSound( BATTLE_SOUND_CURSE1 );
						return( FALSE );
					}
					// grant experience! ... SANDRO - so what?! Grant them already!
					StatChange( pSoldier, MECHANAMT, 40, FALSE ); //Madd: upped this to 40, since standard is now 25
					StatChange( pSoldier, WISDOMAMT, 10, FALSE );
					
					// SANDRO - merc records - merging items
					if ( pSoldier->ubProfile != NO_PROFILE )
						gMercProfiles[ pSoldier->ubProfile ].records.usItemsCombined++;
				}
				// fall through
			case EXPLOSIVE_MERGE_HARD: //Madd: new merge types
				if ( ubType == EXPLOSIVE_MERGE_HARD ) /// coulda fallen through
				{
					if (pSoldier)
					{
						// requires a skill check, and gives experience
						iCheckResult = SkillCheck( pSoldier, ATTACHING_DETONATOR_CHECK, -30 );
						if (iCheckResult < 0)
						{
							// could have a chance of detonation
							// for now, damage both objects
							DamageObj( this, (INT8) -iCheckResult );
							DamageObj( pAttachment, (INT8) -iCheckResult );
							pSoldier->DoMercBattleSound( BATTLE_SOUND_CURSE1 );
							return( FALSE );
						}
						StatChange( pSoldier, EXPLODEAMT, 25, FALSE );
						StatChange( pSoldier, WISDOMAMT, 5, FALSE );

						// SANDRO - merc records - merging items
						if ( pSoldier->ubProfile != NO_PROFILE )
							gMercProfiles[ pSoldier->ubProfile ].records.usItemsCombined++;
					}
				}
				// fall through
			case EXPLOSIVE_MERGE_EASY:
				if ( ubType == EXPLOSIVE_MERGE_EASY ) /// coulda fallen through
				{
					if (pSoldier)
					{
						// requires a skill check, and gives experience
						iCheckResult = SkillCheck( pSoldier, ATTACHING_DETONATOR_CHECK, 0 );
						if (iCheckResult < 0)
						{
							// could have a chance of detonation
							// for now, damage both objects
							DamageObj( this, (INT8) -iCheckResult );
							DamageObj( pAttachment, (INT8) -iCheckResult );
							pSoldier->DoMercBattleSound( BATTLE_SOUND_CURSE1 );
							return( FALSE );
						}
						StatChange( pSoldier, EXPLODEAMT, 10, FALSE );
						StatChange( pSoldier, WISDOMAMT, 2, FALSE );

						// SANDRO - merc records - merging items
						if ( pSoldier->ubProfile != NO_PROFILE )
							gMercProfiles[ pSoldier->ubProfile ].records.usItemsCombined++;
					}
				}
				// fall through
			case MECHANICAL_MERGE_HARD:
				if ( ubType == MECHANICAL_MERGE_HARD ) /// coulda fallen through
				{
					if (pSoldier)
					{
						// requires a skill check, and gives experience
						iCheckResult = SkillCheck( pSoldier, ATTACHING_SPECIAL_ITEM_CHECK, -30 );
						if (iCheckResult < 0)
						{
							// could have a chance of detonation
							// for now, damage both objects
							DamageObj( this, (INT8) -iCheckResult );
							DamageObj( pAttachment, (INT8) -iCheckResult );
							pSoldier->DoMercBattleSound( BATTLE_SOUND_CURSE1 );
							return( FALSE );
						}
						StatChange( pSoldier, MECHANAMT, 25, FALSE );
						StatChange( pSoldier, WISDOMAMT, 10, FALSE );

						// SANDRO - merc records - merging items
						if ( pSoldier->ubProfile != NO_PROFILE )
							gMercProfiles[ pSoldier->ubProfile ].records.usItemsCombined++;
					}
				}
				// fall through
			case MECHANICAL_MERGE_EASY:
				if ( ubType == MECHANICAL_MERGE_EASY ) /// coulda fallen through
				{
					if (pSoldier)
					{
						// requires a skill check, and gives experience
						iCheckResult = SkillCheck( pSoldier, ATTACHING_SPECIAL_ITEM_CHECK, 0 );
						if (iCheckResult < 0)
						{
							// could have a chance of detonation
							// for now, damage both objects
							DamageObj( this, (INT8) -iCheckResult );
							DamageObj( pAttachment, (INT8) -iCheckResult );
							pSoldier->DoMercBattleSound( BATTLE_SOUND_CURSE1 );
							return( FALSE );
						}
						StatChange( pSoldier, MECHANAMT, 10, FALSE );
						StatChange( pSoldier, WISDOMAMT, 2, FALSE );

						// SANDRO - merc records - merging items
						if ( pSoldier->ubProfile != NO_PROFILE )
							gMercProfiles[ pSoldier->ubProfile ].records.usItemsCombined++;
					}
				}
				// fall through
			default:
				// the merge will combine the two items
				//Madd: usResult2 only works for standard merges->item1 + item2 = item3 + item4

				//Madd: unload guns after merge if ammo caliber or mag size don't match
				if ( Item[this->usItem].usItemClass == IC_GUN && (*this)[subObject]->data.gun.usGunAmmoItem != NONE && (*this)[subObject]->data.gun.ubGunShotsLeft > 0 )
				{
					if ( Item[usResult].usItemClass != IC_GUN || Weapon[Item[usResult].ubClassIndex].ubCalibre != Weapon[Item[this->usItem].ubClassIndex].ubCalibre || (*this)[subObject]->data.gun.ubGunShotsLeft > Weapon[Item[usResult].ubClassIndex].ubMagSize )
					{ // item types/calibers/magazines don't match, spit out old ammo
						EjectAmmoAndPlace(pSoldier, this);
					}
				}

				RemoveProhibitedAttachments(pSoldier, this, usResult);

				this->usItem = usResult;
				if ( ubType != TREAT_ARMOUR )
				{
					(*this)[subObject]->data.objectStatus = ((*this)[subObject]->data.objectStatus + (*pAttachment)[0]->data.objectStatus) / 2;
				}

				//ADB ubWeight has been removed, see comments in OBJECTTYPE
				//this->ubWeight = CalculateObjectWeight( this );

				if ( usResult2 != NOTHING )
				{
					//Madd: usResult2 is what the original attachment/source item turns into

					//Madd: unload guns after merge if ammo caliber or mag size don't match
					if ( Item[pAttachment->usItem].usItemClass == IC_GUN && (*pAttachment)[0]->data.gun.usGunAmmoItem != NONE && (*pAttachment)[0]->data.gun.ubGunShotsLeft > 0 )
					{
						if ( Item[usResult2].usItemClass != IC_GUN || Weapon[Item[usResult2].ubClassIndex].ubCalibre != Weapon[Item[pAttachment->usItem].ubClassIndex].ubCalibre || (*pAttachment)[0]->data.gun.ubGunShotsLeft > Weapon[Item[usResult2].ubClassIndex].ubMagSize )
						{ // item types/calibers/magazines don't match, spit out old ammo
							EjectAmmoAndPlace(pSoldier, pAttachment);
						}
					}

					RemoveProhibitedAttachments(pSoldier, pAttachment, usResult2);

					pAttachment->usItem = usResult2;
					if ( ubType != TREAT_ARMOUR )
					{
						(*pAttachment)[0]->data.objectStatus = ((*pAttachment)[0]->data.objectStatus + (*this)[subObject]->data.objectStatus) / 2;
					}
					//ADB ubWeight has been removed, see comments in OBJECTTYPE
					//pAttachment->ubWeight = CalculateObjectWeight( pAttachment );
				}
				else
					pAttachment->RemoveObjectsFromStack(1);

				if (pSoldier && pSoldier->bTeam == gbPlayerNum)
				{
					pSoldier->DoMercBattleSound( BATTLE_SOUND_COOL1 );
				}
				break;
			}
			if ( pSoldier != NULL )
				ApplyEquipmentBonuses(pSoldier);
			return( TRUE );
	}
	return( FALSE );
}

BOOLEAN OBJECTTYPE::AttachObject( SOLDIERTYPE * pSoldier, OBJECTTYPE * pAttachment, BOOLEAN playSound, UINT8 subObject, INT32 iItemPos, BOOLEAN fRemoveProhibited, std::vector<UINT16> usAttachmentSlotIndexVector )
{	
	if(UsingNewAttachmentSystem()==true){
		return( this->AttachObjectNAS(pSoldier, pAttachment, playSound, subObject, iItemPos, fRemoveProhibited, usAttachmentSlotIndexVector ) );
	} else {
		return( this->AttachObjectOAS( pSoldier, pAttachment, playSound, subObject) );
	}
}


BOOLEAN OBJECTTYPE::AttachObjectNAS( SOLDIERTYPE * pSoldier, OBJECTTYPE * pAttachment, BOOLEAN playSound, UINT8 subObject, INT32 iItemPos, BOOLEAN fRemoveProhibited, std::vector<UINT16> usAttachmentSlotIndexVector )
{
	//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"AttachObjectNAS attachment %d iItemPos %d", pAttachment->usItem, iItemPos);
	if (pAttachment->exists() == false) 
	{
		return FALSE;
	}

	// sevenfm: don't allow attaching activated grenades
	if (Item[pAttachment->usItem].usItemClass & IC_EXPLOSV && (*pAttachment)[0]->data.misc.bDelay)
	{
		return FALSE;
	}

	UINT16		usResult, usResult2, ubLimit;
	INT8		bLoop;
	UINT8		ubType, ubAPCost;
	INT32		iCheckResult;
	INT8		bAttachInfoIndex = -1;
	INT8		bAttachComboMerge;
	BOOLEAN		fValidLaunchable = FALSE;
	BOOLEAN		fValidItemAttachment = FALSE;
	INT32		curSlot = 0;
	UINT16		oldMagSize = 0;

	INT16	sPercentDrain;
	UINT16	usPointsKit;

	// extra tags
	UINT16	usPointsUsed;
	UINT16	usPointsAdded;
	UINT16	usExtraItem;
	UINT16	usExtraItemPointsUsed;
	UINT16	usSkillCheck;
	UINT16	usSkillDifficulty;

	//if there is already an attachment of the same type, we want to try swapping / replacing it
	OBJECTTYPE* pAttachmentPosition = 0;
	//OBJECTTYPE* pAttachInSlot = 0;

	static OBJECTTYPE attachmentObject;

	//CHRISL: This is so we can try to determine if the removed attachment altered our mag size.
	if(Item[this->usItem].usItemClass == IC_GUN)
		oldMagSize = GetMagSize(this);

	//if this is an attachment or ammo for a launchable item
	fValidLaunchable = ValidLaunchable( pAttachment->usItem, this->usItem );

	//WarmSteel - if uiItemPos is -1, we're not checking for a specific slot, but scrolling through them all.
	if(iItemPos == -1)
	{
		//WarmSteel - repeat this for every slot
		for(curSlot = 0; curSlot < (INT32)(*this)[0]->attachments.size(); curSlot++)
		{	
			fValidItemAttachment = ValidItemAttachmentSlot( this, pAttachment->usItem, TRUE, FALSE, subObject, curSlot, 0, 0, usAttachmentSlotIndexVector);

			if(fValidItemAttachment)
			{
				//WarmSteel - apparently we found a attachable slot, exiting here will remember the curSlot
				break;
			}
		}

		//WarmSteel - if we didn't find any good slot, always just attach to the first slot. (because some functions will overrule this one).
		//It's basically just a general check so that we NEVER try to attach outside the attachment list range.
		if(curSlot == (*this)[subObject]->attachments.size())
			curSlot = 0;

	} 
	else 
	{
		//WarmSteel - We know in what slot we're trying to put this, so lets see if we can.
		curSlot = iItemPos;
		fValidItemAttachment = ValidItemAttachmentSlot( this, pAttachment->usItem, TRUE, playSound, subObject, curSlot, FALSE, &pAttachmentPosition, usAttachmentSlotIndexVector);
		//pAttachmentPosition = pAttachInSlot;
		// try replacing if possible
	}

	//CHRISL: If we don't want to play the sound, it's a good bet we don't want to display any messages either
	if (/*fValidLaunchable ||*/ fValidItemAttachment )
	{
		if ( pSoldier )
		{
			//did the soldier damage it?
			bAttachInfoIndex = GetAttachmentInfoIndex( pAttachment->usItem );
			// in-game (not behind the scenes) attachment
			if ( bAttachInfoIndex != -1 && AttachmentInfo[ bAttachInfoIndex ].bAttachmentSkillCheck != NO_CHECK )
			{
				iCheckResult = SkillCheck( pSoldier, AttachmentInfo[ bAttachInfoIndex ].bAttachmentSkillCheck, AttachmentInfo[ bAttachInfoIndex ].bAttachmentSkillCheckMod );
				if (iCheckResult < 0)
				{
					// the attach failure damages both items
					DamageObj( this, (INT8) -iCheckResult, subObject );
					DamageObj( pAttachment, (INT8) -iCheckResult, subObject );

					// there should be a quote here!
					pSoldier->DoMercBattleSound( BATTLE_SOUND_CURSE1 );
					if ( gfInItemDescBox )
					{
						//DeleteItemDescriptionBox();
					}
					return( FALSE );
				}
			}
			else if ( Item[this->usItem].tripwire && Item[pAttachment->usItem].usItemClass & IC_GUN ) // Madd: gun on tripwire
			{
				iCheckResult = SkillCheck( pSoldier, ATTACHING_DETONATOR_CHECK, 0 );
				if (iCheckResult < 0)
				{
					// the attach failure damages both items
					DamageObj( this, (INT8) -iCheckResult, subObject );
					DamageObj( pAttachment, (INT8) -iCheckResult, subObject );

					// there should be a quote here!
					pSoldier->DoMercBattleSound( BATTLE_SOUND_CURSE1 );
					if ( gfInItemDescBox )
					{
						//DeleteItemDescriptionBox();
					}
					return( FALSE );
				}
			}

			if (fValidItemAttachment && playSound ) // not launchable
			{
				// attachment sounds
				if ( Item[ this->usItem ].usItemClass & IC_WEAPON || Item[this->usItem].tripwire ) //Madd: attaching items to tripwire makes gun attach sound
				{
					PlayJA2Sample( ATTACH_TO_GUN, RATE_11025, SoundVolume( MIDVOLUME, pSoldier->sGridNo ), 1, SoundDir( pSoldier->sGridNo ) );
				}
				else if ( Item[ this->usItem ].usItemClass & IC_ARMOUR )
				{
					PlayJA2Sample( ATTACH_CERAMIC_PLATES, RATE_11025, SoundVolume( MIDVOLUME, pSoldier->sGridNo ), 1, SoundDir( pSoldier->sGridNo ) );
				}
				else if ( Item[ this->usItem ].usItemClass & IC_BOMB )
				{
					PlayJA2Sample( ATTACH_DETONATOR, RATE_11025, SoundVolume( MIDVOLUME, pSoldier->sGridNo ), 1, SoundDir( pSoldier->sGridNo ) );
				}
			}
		}

		/*	//NAS doesn't care about single attachments.
		if (pAttachmentPosition) {
			if (canOnlyAttach1 == true) {
				if(pAttachment->ubNumberOfObjects > 1)
				{
					//We don't want to inadvertantly load two rounds during a swap so don't allow a swap if we're holding a stack
					return(FALSE);
				}
				//we have requested to swap the attachment on the cursor with the current attachment
				SwapObjs(pAttachmentPosition,pAttachment);
				return(TRUE);
			}
		}*/

		//ADB moved after skill check!
		//lalien: added to make a GL/RL work when reloaded manually
		// the AP costs for reloading GL/RL will be taken from weapons.xml ( wrong place!!! the AP's are deducted in DeleteItemDescriptionBox() )

		//we know we are replacing this attachment
		if ( Item[ this->usItem ].usItemClass == IC_LAUNCHER || Item[this->usItem].cannon )
		{
			if ( fValidLaunchable )
			{
				(*this)[subObject]->data.gun.usGunAmmoItem = pAttachment->usItem;
				//we have reloaded a launchable, so the ammo is gone from the original object
			}
		}

		/* //WarmSteel - splice won't work with attachments anymore, because of the null attachments. Replaced this function somewhere lower, because it needs to be done after the attaching of the GL.
		//Unfortunately this must come before any attachment swap
		if (Item[pAttachment->usItem].grenadelauncher && gGameOptions.ubAttachmentSystem == ATTACHMENT_OLD)
		{
			// transfer any attachments from the grenade launcher to the gun
			(*this)[subObject]->attachments.splice((*this)[subObject]->attachments.begin(), (*pAttachment)[0]->attachments,
				(*pAttachment)[0]->attachments.begin(), (*pAttachment)[0]->attachments.end());
		}*/

		//WarmSteel - Attachment swapping prevents multiple objects that are the same to be attached.
		//This takes away freedom, which is bad. This is why I turned it off in NAS (If you want to turn it back on, it also doesn't work properly because it needs to swap xml slot indexes aswell).
		if (pAttachmentPosition->exists()) {
			//we are swapping the attachments, and we know we do NOT need to worry about attachment stack size
			//CHRISL: Actually, we do need to worry about attachment stack size since we might have a stack in our cursor.
			//	Rather then doing a simple swap, try moving the existing attachment to our cursor stack, then attach one item
			//	from the cursor stack.

			//CHRISL: Try to replace illegal IC_GRENADE items with legal ones
			if(!ItemIsLegal(pAttachment->usItem) && Item[pAttachment->usItem].usItemClass & IC_GRENADE){
				OBJECTTYPE*	newAttachment = new OBJECTTYPE;
				UINT16		newItem;
				newItem = FindLegalGrenade(pAttachment->usItem);
				if(newItem != pAttachment->usItem){
					CreateItem(newItem, 100, newAttachment);

					// sevenfm: find correct ubMagSize for UB GL
					UINT16 ubMagSize = Weapon[this->usItem].ubMagSize;
					if( !Item[ this->usItem ].grenadelauncher )
					{
						OBJECTTYPE *bAttachPos = FindAttachmentByClass( this, IC_LAUNCHER );
						if( bAttachPos && bAttachPos->exists() )
							ubMagSize = Weapon[bAttachPos->usItem].ubMagSize;
					}
					(*pAttachment)[0]->data.objectStatus -= (INT16)(100/ubMagSize);
					// fix for 6-shot GL
					if( (*pAttachment)[0]->data.objectStatus < ubMagSize )
						(*pAttachment)[0]->data.objectStatus = 0;

					//(*pAttachment)[0]->data.objectStatus -= (INT16)(100/Weapon[this->usItem].ubMagSize);

					if((*pAttachment)[0]->data.objectStatus <= 0 )
						pAttachment->RemoveObjectsFromStack(1);
					newAttachment->MoveThisObjectTo(attachmentObject,1,pSoldier,NUM_INV_SLOTS,1);
				} else {
					pAttachment->MoveThisObjectTo(attachmentObject,1,pSoldier,NUM_INV_SLOTS,1);
				}
			} else {
				pAttachment->MoveThisObjectTo(attachmentObject,1,pSoldier,NUM_INV_SLOTS,1);
			}
			if(pAttachment->ubNumberOfObjects > 0){
				if (pSoldier) {
					if ( !AutoPlaceObject( pSoldier, pAttachmentPosition, FALSE ) )
					{   // put it on the ground
						AddItemToPool( pSoldier->sGridNo, pAttachmentPosition, 1, pSoldier->pathing.bLevel, 0 , -1 );
					}
				}
			} else {
				pAttachment->AddObjectsToStack(*pAttachmentPosition,-1,pSoldier,NUM_INV_SLOTS,MAX_OBJECTS_PER_SLOT);
			}
			*pAttachmentPosition = attachmentObject;

			(*this)[subObject]->AddAttachmentAtIndex(curSlot, attachmentObject);

			//backup the original attachment
			//attachmentObject = *pAttachmentPosition;

			//place the new one
			//*pAttachmentPosition = *pAttachment;

			//whatever pAttachment pointed to is now the original attachment
			//*pAttachment = attachmentObject;
		}
		else {
			//it's a new attachment

			//pAttachment could have any number of objects, they have all been moved over
			//CHRISL: This doesn't work.  What if we have a stack of objects in the cursor?  We don't want the whole
			//	stack to be attached
			//(*this)[subObject]->attachments.push_back(*pAttachment);
			//DeleteObj(pAttachment);
			//CHRISL: Try to replace illegal IC_GRENADE items with legal ones
			if(!ItemIsLegal(pAttachment->usItem) && Item[pAttachment->usItem].usItemClass & IC_GRENADE){
				OBJECTTYPE*	newAttachment = new OBJECTTYPE;
				UINT16		newItem;
				newItem = FindLegalGrenade(pAttachment->usItem);
				if(newItem != pAttachment->usItem){
					CreateItem(newItem, 100, newAttachment);

					// sevenfm: find correct ubMagSize for UB GL
					UINT16 ubMagSize = Weapon[this->usItem].ubMagSize;
					if( !Item[ this->usItem ].grenadelauncher )
					{
						OBJECTTYPE *bAttachPos = FindAttachmentByClass( this, IC_LAUNCHER );
						if( bAttachPos && bAttachPos->exists() )
							ubMagSize = Weapon[bAttachPos->usItem].ubMagSize;
					}
					(*pAttachment)[0]->data.objectStatus -= (INT16)(100/ubMagSize);
					// fix for 6-shot GL
					if( (*pAttachment)[0]->data.objectStatus < ubMagSize )
						(*pAttachment)[0]->data.objectStatus = 0;

					//(*pAttachment)[0]->data.objectStatus -= (INT16)(100/Weapon[this->usItem].ubMagSize);

					if((*pAttachment)[0]->data.objectStatus <= 0 )
						pAttachment->RemoveObjectsFromStack(1);
					newAttachment->MoveThisObjectTo(attachmentObject,1,pSoldier,NUM_INV_SLOTS,1);
				} else {
					// sevenfm: r8399
					//pAttachment->MoveThisObjectTo(attachmentObject,1,pSoldier,NUM_INV_SLOTS,1);

					// if this is an active LBE (holding an item) it must be a MOLLE pocket. We need to move the item out of this pocket or it will be invisible.
					if ( pAttachment->IsActiveLBE(subObject) )
					{
						std::vector<INT8> LBESlots;
						switch( LoadBearingEquipment[Item[pAttachment->usItem].ubClassIndex].lbeClass )
						{
						case THIGH_PACK:
							GetLBESlots( LTHIGHPOCKPOS, LBESlots );
							break;
						case VEST_PACK:
							GetLBESlots( VESTPOCKPOS, LBESlots );
							break;
						case COMBAT_PACK:
							GetLBESlots( CPACKPOCKPOS, LBESlots );
							break;
						case BACKPACK:
							GetLBESlots( BPACKPOCKPOS, LBESlots );
							break;
							// this should never happen
						default:
							return FALSE;
						}

						LBENODE* pLBE = pAttachment->GetLBEPointer(0);
						for(unsigned int i=0; pLBE && i<LBESlots.size(); i++)
						{
							// Is there an item in this LBE pocket?
							if( pLBE->inv[i].exists() )
							{
								// place in soldiers inventory
								if( !AutoPlaceObject(pSoldier, &pLBE->inv[i], FALSE) )
									// that didn't work. Place it on the ground instead.
									AutoPlaceObjectToWorld( pSoldier, &pLBE->inv[i], TRUE );
							}
						}
						// finished removing items. Now attach the MOLLE pocket to its carrier.
						pAttachment->MoveThisObjectTo(attachmentObject,1,pSoldier,NUM_INV_SLOTS,1);
					}
					else
						pAttachment->MoveThisObjectTo(attachmentObject,1,pSoldier,NUM_INV_SLOTS,1);							
				}
			} else {
				pAttachment->MoveThisObjectTo(attachmentObject,1,pSoldier,NUM_INV_SLOTS,1);
			}

				//WarmSteel - Because we want every attachment to stay in place in NAS, we must first delete the "null" attachment, then insert the new one.
				//We know this attachment slot has a non-null object, otherwise ValidItemAttachmentSlot would not have returned true and we would not be here.
			(*this)[subObject]->AddAttachmentAtIndex(curSlot, attachmentObject);
		}


		//Ammo might have changed after adding an attachment.
		if ( Item[this->usItem].usItemClass == IC_GUN && (*this)[subObject]->data.gun.usGunAmmoItem != NONE && (*this)[subObject]->data.gun.ubGunShotsLeft > 0 && oldMagSize != GetMagSize(this, subObject) )
		{
			if ( (*this)[subObject]->data.gun.ubGunShotsLeft > GetMagSize(this, subObject) )
			{ //Too many rounds, eject ammo
				EjectAmmoAndPlace(pSoldier, this, subObject);
			}
			//CHRISL: We should update the usGunAmmoItem if we've changed the ammo capacity
			if((*this)[subObject]->data.gun.usGunAmmoItem != NONE){
				UINT16 usNewAmmoItem;
				usNewAmmoItem = FindReplacementMagazine(Weapon[this->usItem].ubCalibre ,GetMagSize(this, subObject),Magazine[Item[(*this)[subObject]->data.gun.usGunAmmoItem].ubClassIndex].ubAmmoType);
				(*this)[subObject]->data.gun.usGunAmmoItem = usNewAmmoItem;
			}
		}

		if(Item[this->usItem].usItemClass == IC_GUN && oldMagSize != GetMagSize(this, subObject)){
			fInterfacePanelDirty = DIRTYLEVEL2;
			RenderBulletIcon(this, subObject);
		}

		//WarmSteel - If we have just attached a UGL, remove it's grenade and put it on the gun itself
		//CHRISL: This section of code is also needed it we add any attachment that changes the valid attachments our item can use, so we should run it whenever we
		//	add an attachment in NAS
		//if (FindAttachment_GrenadeLauncher(this)->exists() && attachmentObject.exists())
		if (attachmentObject.exists() && attachmentObject[0]->attachments.size() > 0 && FindAttachment(this, attachmentObject.usItem, subObject)->exists())
		{
			//Make sure it's actually on that gun..
			//if(FindAttachment_GrenadeLauncher(this)->usItem == attachmentObject.usItem){
			// Flugente: if we attach a gun to another gun, do not transfer attachments
			if( Item[attachmentObject.usItem].usItemClass != IC_GUN  && FindAttachment(this, attachmentObject.usItem, subObject)->usItem == attachmentObject.usItem)
			{
				// transfer the grenade from the grenade launcher to the gun

				//To know wether this grenade is valid, we need to correct the slots, because they may have changed when attaching the UGL
				//Have to do it here, because from now on we don't need attachmentObject anymore.
				//CHRISL: Because we want this function to be useful for more then just UGLs, and because there is the probability that the attaching item will
				//	give us more attachment points, we need to just remove all attachments, tack them to the end of the attachmentList, and then go through the
				//	RemoveProhibitedAttachments function to force everything to store in the correct slot
				OBJECTTYPE * pGrenade = FindAttachment(this, attachmentObject.usItem, subObject);
				for(attachmentList::iterator iter = (*pGrenade)[0]->attachments.begin(); iter != (*pGrenade)[0]->attachments.end(); ++iter)
				{
					if(iter->exists())
					{
						(*this)[subObject]->attachments.push_back((*iter));
						iter = (*pGrenade)[0]->RemoveAttachmentAtIter(iter);
						fRemoveProhibited = TRUE;
					}
				}
//				if(fRemoveProhibited){
//					RemoveProhibitedAttachments(pSoldier, this, this->usItem);
//				}

/*				CHRISL: None of this is needed.  Instead we strip any attachments from attachmentObject in the above loop.  
				for(INT16 sSlot = 0; sSlot < (INT16) (*this)[subObject]->attachments.size(); sSlot++){
					OBJECTTYPE * pGrenade = FindAttachmentByClass(FindAttachment_GrenadeLauncher(this), IC_GRENADE, subObject);
					if(pGrenade->exists()){
						if(ValidItemAttachmentSlot(this, pGrenade->usItem, 1, 0, 0, sSlot)){
							OBJECTTYPE tempObject;
							FindAttachment_GrenadeLauncher(this)->RemoveAttachment(pGrenade,&tempObject,0,0,1,0);
							(*this)[subObject]->AddAttachmentAtIndex((UINT8) sSlot, tempObject);
						}
					}	
				}*/

				// Flugente: if this is a gun that we have in our first hand and it has a rifle grenade device
				if ( pSoldier && HasAttachmentOfClass( this, AC_RIFLEGRENADE) && Item[this->usItem].usItemClass == IC_GUN && this == &(pSoldier->inv[ HANDPOS ]) )
				{
					// get rifle grenade device
					OBJECTTYPE* pRifleGrenadeDeviceObj = FindAttachment_GrenadeLauncher( this );
					
					// if it exists...
					if ( pRifleGrenadeDeviceObj )
					{
						// check if it now has a launchable grenade attached
						OBJECTTYPE* pRiflefileGrenade = FindLaunchableAttachment( this, pRifleGrenadeDeviceObj->usItem);

						// if that grenade is of the same type as the item we attached, adjust firing mode
						if ( pRiflefileGrenade && pRiflefileGrenade->usItem == attachmentObject.usItem )
						{
							pSoldier->bWeaponMode = WM_ATTACHED_GL;
						}
					}
				}
			}
		}

		//Check if any of the attached items has changed the slots (including the UGL grenade we just added).
		if(fRemoveProhibited){
			RemoveProhibitedAttachments(pSoldier, this, this->usItem);
		}
		// Check for attachment merge combos here
		//CHRISL: Only do this if we're looking at a single item.  Don't try a combo merge when dealing with a stack
		bAttachComboMerge = GetAttachmentComboMerge( this, subObject );
		if ( bAttachComboMerge != -1 )
		{
			if(this->ubNumberOfObjects == 1)
			{
				PerformAttachmentComboMerge( this, bAttachComboMerge );
				if ( bAttachInfoIndex != -1 && AttachmentInfo[ bAttachInfoIndex ].bAttachmentSkillCheckMod < 20 )
				{
					StatChange( pSoldier, MECHANAMT, (INT8) ( 20 - AttachmentInfo[ bAttachInfoIndex ].bAttachmentSkillCheckMod ), FALSE );
					StatChange( pSoldier, WISDOMAMT, (INT8) ( 20 - AttachmentInfo[ bAttachInfoIndex ].bAttachmentSkillCheckMod ), FALSE );
				}
			}
			//CHRISL: If we're looking at a stack, we need to unattach the object we just attached
			else if(this->RemoveAttachment(&attachmentObject, pAttachmentPosition, subObject, pSoldier))
			{
				pAttachment->AddObjectsToStack(attachmentObject, -1, pSoldier, NUM_INV_SLOTS, MAX_OBJECTS_PER_SLOT);
			}
		}

		//ADB ubWeight has been removed, see comments in OBJECTTYPE
		//this->ubWeight = CalculateObjectWeight( this );
		//ADB ubWeight has been removed, see comments in OBJECTTYPE
		//pAttachment->ubWeight = CalculateObjectWeight(pAttachment);

		if ( pSoldier != NULL )
			ApplyEquipmentBonuses(pSoldier);
			
		return( TRUE );
	}
	// check for merges
	else if (EvaluateValidMerge(pAttachment->usItem, this->usItem, &usResult, &usResult2, &ubType, &ubAPCost, &usPointsUsed, &usPointsAdded, &usExtraItem, &usExtraItemPointsUsed, &usSkillCheck, &usSkillDifficulty))
	{
		//ScreenMsg(FONT_MCOLOR_LTBLUE, MSG_INTERFACE, L"check merge attachment %d item %d usResult %d usResult2 %d ubNumberOfObjects %d ap cost %d", pAttachment->usItem, this->usItem, usResult, usResult2, this->ubNumberOfObjects, ubAPCost);
		// sevenfm: safety check
		if (!pSoldier)
		{
			return FALSE;
		}
		//CHRISL: we don't want to try to merge IC_LBEGEAR items that are currently storing items.
		if (Item[this->usItem].usItemClass == IC_LBEGEAR && this->IsActiveLBE(subObject))
		{
			return FALSE;
		}
		//CHRISL: We don't want to do any merges if we're looking at a stack of items, with the exception of combines.
		if (this->ubNumberOfObjects > 1 && ubType != COMBINE_POINTS)
		{
			return(FALSE);
		}

		if (!EnoughPoints(pSoldier, ubAPCost, 0, TRUE))
		{
			return(FALSE);
		}

		DeductPoints(pSoldier, ubAPCost, 0);

		switch (ubType)
		{
		case USE_ITEM_CLEAN:
			// sevenfm: check that weapon can be cleaned
			if ((*this)[0]->data.dDirtLevel < DIRT_MIN_TO_CLEAN)
			{
				pSoldier->DoMercBattleSound(BATTLE_SOUND_NOTHING);
				return(FALSE);
			}

			(*this)[0]->data.dDirtLevel = 0.0f;
			ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, TacticalStr[WEAPON_CLEANING_STR], pSoldier->GetName(), Item[this->usItem].szItemName); 

			pSoldier->PlayLNL(this->usItem);

			MakeNoise(pSoldier->ubID, pSoldier->sGridNo, pSoldier->pathing.bLevel, pSoldier->bOverTerrainType, WEAPON_LNL_VOLUME, NOISE_BULLET_IMPACT);

			//UseKitPoints(pAttachment, 1, pSoldier);
			sPercentDrain = max(1, 100 - Item[pAttachment->usItem].percentstatusdrainreduction);
			usPointsKit = max(1, 1 * sPercentDrain / 100);

			(*pAttachment)[0]->data.objectStatus -= usPointsKit;

			if ((*pAttachment)[0]->data.objectStatus <= 0)
			{
				pAttachment->RemoveObjectsFromStack(1);
			}

			ApplyEquipmentBonuses(pSoldier);
			return TRUE;

		case USE_ITEM_NEW:

			if (usSkillCheck >= 1 && usSkillCheck <= 3)
			{
				switch (usSkillCheck)
				{
				case 1:
					iCheckResult = SkillCheck(pSoldier, ATTACHING_SPECIAL_ITEM_CHECK, -usSkillDifficulty);
					break;
				case 2:
					iCheckResult = SkillCheck(pSoldier, ATTACHING_SPECIAL_ELECTRONIC_ITEM_CHECK, -usSkillDifficulty);
					break;
				case 3:
					iCheckResult = SkillCheck(pSoldier, ATTACHING_DETONATOR_CHECK, -usSkillDifficulty);
					break;
				}

				if (iCheckResult < 0)
				{
					DamageObj(this, (INT8)-iCheckResult);
					DamageObj(pAttachment, (INT8)-iCheckResult);
					pSoldier->DoMercBattleSound(BATTLE_SOUND_CURSE1);
					return(FALSE);
				}

				switch (usSkillCheck)
				{
				case 1:
					StatChange(pSoldier, MECHANAMT, 25, FALSE);
					StatChange(pSoldier, WISDOMAMT, 10, FALSE);
					break;
				case 2:
					StatChange(pSoldier, MECHANAMT, 40, FALSE);
					StatChange(pSoldier, WISDOMAMT, 10, FALSE);
					break;
				case 3:
					StatChange(pSoldier, EXPLODEAMT, 25, FALSE);
					StatChange(pSoldier, WISDOMAMT, 5, FALSE);
					break;
				}
			}

			if (usExtraItem)
			{
				INT8 bExtraItemSlot = FindObj(pSoldier, usExtraItem);

				if (bExtraItemSlot == ITEM_NOT_FOUND)
				{
					pSoldier->DoMercBattleSound(BATTLE_SOUND_NOTHING);
					return FALSE;
				}

				if (usExtraItemPointsUsed)
				{
					if (pSoldier->inv[bExtraItemSlot][0]->data.objectStatus < usExtraItemPointsUsed)
					{
						pSoldier->DoMercBattleSound(BATTLE_SOUND_NOTHING);
						return FALSE;
					}

					pSoldier->inv[bExtraItemSlot][0]->data.objectStatus -= usExtraItemPointsUsed;

					if (pSoldier->inv[bExtraItemSlot][0]->data.objectStatus <= 0)
					{
						pSoldier->inv[bExtraItemSlot].RemoveObjectsFromStack(1);
					}
				}
			}

			if ((*this)[subObject]->data.objectStatus >= max(1, usPointsUsed) && usResult != NOTHING)
			{
				if (usPointsAdded)
					CreateItem(usResult, min(100, usPointsAdded), &gTempObject);
				else
					CreateItem(usResult, 100, &gTempObject);

				if (!AutoPlaceObject(pSoldier, &gTempObject, FALSE))
					AddItemToPool(pSoldier->sGridNo, &gTempObject, 1, 0, 0, -1);

				(*this)[subObject]->data.objectStatus -= max(1, usPointsUsed);
			}

			if ((*this)[subObject]->data.objectStatus >= max(1, usPointsUsed) && usResult2 != NOTHING)
			{
				if (usPointsAdded)
					CreateItem(usResult2, min(100, usPointsAdded), &gTempObject);
				else
					CreateItem(usResult2, 100, &gTempObject);

				if (!AutoPlaceObject(pSoldier, &gTempObject, FALSE))
					AddItemToPool(pSoldier->sGridNo, &gTempObject, 1, 0, 0, -1);

				(*this)[subObject]->data.objectStatus -= max(1, usPointsUsed);
			}

			if ((*this)[subObject]->data.objectStatus == 0)
			{
				this->RemoveObjectsFromStack(1);
				if (pSoldier->bTeam == gbPlayerNum)
				{
					pSoldier->DoMercBattleSound(BATTLE_SOUND_CURSE1);
				}
			}
			else
			{
				if (pSoldier->bTeam == gbPlayerNum)
				{
					pSoldier->DoMercBattleSound(BATTLE_SOUND_COOL1);
				}
			}
			ApplyEquipmentBonuses(pSoldier);
			return TRUE;

		case USE_ITEM:

			if (usSkillCheck >= 1 && usSkillCheck <= 3)
			{
				switch (usSkillCheck)
				{
				case 1:
					iCheckResult = SkillCheck(pSoldier, ATTACHING_SPECIAL_ITEM_CHECK, -usSkillDifficulty);
					break;
				case 2:
					iCheckResult = SkillCheck(pSoldier, ATTACHING_SPECIAL_ELECTRONIC_ITEM_CHECK, -usSkillDifficulty);
					break;
				case 3:
					iCheckResult = SkillCheck(pSoldier, ATTACHING_DETONATOR_CHECK, -usSkillDifficulty);
					break;
				}

				if (iCheckResult < 0)
				{
					DamageObj(this, (INT8)-iCheckResult);
					DamageObj(pAttachment, (INT8)-iCheckResult);
					pSoldier->DoMercBattleSound(BATTLE_SOUND_CURSE1);
					return(FALSE);
				}

				switch (usSkillCheck)
				{
				case 1:
					StatChange(pSoldier, MECHANAMT, 25, FALSE);
					StatChange(pSoldier, WISDOMAMT, 10, FALSE);
					break;
				case 2:
					StatChange(pSoldier, MECHANAMT, 40, FALSE);
					StatChange(pSoldier, WISDOMAMT, 10, FALSE);
					break;
				case 3:
					StatChange(pSoldier, EXPLODEAMT, 25, FALSE);
					StatChange(pSoldier, WISDOMAMT, 5, FALSE);
					break;
				}
			}

		case USE_ITEM_HARD:
			// the merge will combine the two items
			if (usExtraItem)
			{
				INT8 bExtraItemSlot = FindObj(pSoldier, usExtraItem);

				if (bExtraItemSlot == ITEM_NOT_FOUND)
				{
					pSoldier->DoMercBattleSound(BATTLE_SOUND_NOTHING);
					return FALSE;
				}

				if (usExtraItemPointsUsed)
				{
					if (pSoldier->inv[bExtraItemSlot][0]->data.objectStatus < usExtraItemPointsUsed)
					{
						pSoldier->DoMercBattleSound(BATTLE_SOUND_NOTHING);
						return FALSE;
					}

					pSoldier->inv[bExtraItemSlot][0]->data.objectStatus -= usExtraItemPointsUsed;

					if (pSoldier->inv[bExtraItemSlot][0]->data.objectStatus <= 0)
					{
						pSoldier->inv[bExtraItemSlot].RemoveObjectsFromStack(1);
					}
				}
			}

			if (ubType == USE_ITEM_HARD)
			{
				// requires a skill check, and gives experience
				iCheckResult = SkillCheck(pSoldier, ATTACHING_SPECIAL_ITEM_CHECK, -30);
				if (iCheckResult < 0)
				{
					// could have a chance of detonation
					// for now, damage both objects
					DamageObj(this, (INT8)-iCheckResult);
					DamageObj(pAttachment, (INT8)-iCheckResult);
					pSoldier->DoMercBattleSound(BATTLE_SOUND_CURSE1);
					return(FALSE);
				}
				StatChange(pSoldier, MECHANAMT, 25, FALSE);
				StatChange(pSoldier, WISDOMAMT, 5, FALSE);
			}

			// check that item has enough status
			//if ((*this)[subObject]->data.objectStatus > usPointsAdded)
			{
				//Madd: unload guns after merge if ammo caliber or mag size don't match
				if (Item[this->usItem].usItemClass == IC_GUN && (*this)[0]->data.gun.usGunAmmoItem != NONE && (*this)[0]->data.gun.ubGunShotsLeft > 0)
				{
					if (Item[usResult].usItemClass != IC_GUN || Weapon[Item[usResult].ubClassIndex].ubCalibre != Weapon[Item[this->usItem].ubClassIndex].ubCalibre || (*this)[0]->data.gun.ubGunShotsLeft > Weapon[Item[usResult].ubClassIndex].ubMagSize)
					{ // item types/calibers/magazines don't match, spit out old ammo
						EjectAmmoAndPlace(pSoldier, this);
					}
				}

				UINT16 usOldItem = this->usItem;
				this->usItem = usResult;
				UINT16 usOldItemStatus = (*this)[subObject]->data.objectStatus;

				// set status of new item
				if (usPointsAdded > 0)
				{
					(*this)[subObject]->data.objectStatus = usPointsAdded;
				}

				// reduce status of new object if attachment has not enough points
				if (usPointsUsed > 0 && (*pAttachment)[0]->data.objectStatus < usPointsUsed)
				{
					(*this)[subObject]->data.objectStatus = (*this)[subObject]->data.objectStatus * (usPointsUsed + (*pAttachment)[0]->data.objectStatus) / (2 * usPointsUsed);
				}

				//WarmSteel - Replaced this with one that also checks default attachments, otherwise you could not replace built-in bonuses with default inseparable attachments.
				//RemoveProhibitedAttachments(pSoldier, this, usResult);
				// HEADROCK HAM 5: Added argument for statusindex.
				ReInitMergedItem(pSoldier, this, usOldItem, 0);

				// sevenfm: add usResult2 item if defined
				if (usResult2 != NOTHING)
				{
					CreateItem(usResult2, usOldItemStatus, &gTempObject);

					// set status of new item
					if (usPointsAdded > 0)
					{
						(gTempObject)[0]->data.objectStatus = usPointsAdded;
					}

					// reduce status of new object if attachment has not enough points
					if (usPointsUsed > 0 && (*pAttachment)[0]->data.objectStatus < usPointsUsed)
					{
						(gTempObject)[0]->data.objectStatus = (gTempObject)[0]->data.objectStatus * (usPointsUsed + (*pAttachment)[0]->data.objectStatus) / (2 * usPointsUsed);
					}

					if (!AutoPlaceObject(pSoldier, &gTempObject, FALSE))
						AddItemToPool(pSoldier->sGridNo, &gTempObject, 1, 0, 0, -1);
				}

				(*this)[subObject]->data.objectStatus -= usPointsAdded;

				// use points for first result item
				if (usPointsUsed)
				{
					if ((*pAttachment)[0]->data.objectStatus < usPointsUsed)
					{
						//pSoldier->DoMercBattleSound(BATTLE_SOUND_NOTHING);
						//return FALSE;

						(*pAttachment)[0]->data.objectStatus = 0;
						pAttachment->RemoveObjectsFromStack(1);
					}
					else
					{
						(*pAttachment)[0]->data.objectStatus -= usPointsUsed;

						if ((*pAttachment)[0]->data.objectStatus == 0)
						{
							pAttachment->RemoveObjectsFromStack(1);
						}
					}
				}

				//AutoPlaceObject( pAttachment );
				//ADB ubWeight has been removed, see comments in OBJECTTYPE
				//this->ubWeight = CalculateObjectWeight( this );
				if (pSoldier->bTeam == gbPlayerNum)
				{
					pSoldier->DoMercBattleSound(BATTLE_SOUND_COOL1);
				}

				ApplyEquipmentBonuses(pSoldier);
				return TRUE;
			}
			/*else
			{
				pSoldier->DoMercBattleSound(BATTLE_SOUND_NOTHING);
				return FALSE;
			}*/

		case COMBINE_POINTS:

			if (usExtraItem)
			{
				INT8 bExtraItemSlot = FindObj(pSoldier, usExtraItem);

				if (bExtraItemSlot == ITEM_NOT_FOUND)
				{
					pSoldier->DoMercBattleSound(BATTLE_SOUND_NOTHING);
					return FALSE;
				}

				if (usExtraItemPointsUsed)
				{
					if (pSoldier->inv[bExtraItemSlot][0]->data.objectStatus < usExtraItemPointsUsed)
					{
						pSoldier->DoMercBattleSound(BATTLE_SOUND_NOTHING);
						return FALSE;
					}

					pSoldier->inv[bExtraItemSlot][0]->data.objectStatus -= usExtraItemPointsUsed;

					if (pSoldier->inv[bExtraItemSlot][0]->data.objectStatus <= 0)
					{
						pSoldier->inv[bExtraItemSlot].RemoveObjectsFromStack(1);
					}
				}
			}

			if (usSkillCheck >= 1 && usSkillCheck <= 3)
			{
				switch (usSkillCheck)
				{
				case 1:
					iCheckResult = SkillCheck(pSoldier, ATTACHING_SPECIAL_ITEM_CHECK, -usSkillDifficulty);
					break;
				case 2:
					iCheckResult = SkillCheck(pSoldier, ATTACHING_SPECIAL_ELECTRONIC_ITEM_CHECK, -usSkillDifficulty);
					break;
				case 3:
					iCheckResult = SkillCheck(pSoldier, ATTACHING_DETONATOR_CHECK, -usSkillDifficulty);
					break;
				}

				if (iCheckResult < 0)
				{
					DamageObj(this, (INT8)-iCheckResult);
					DamageObj(pAttachment, (INT8)-iCheckResult);
					pSoldier->DoMercBattleSound(BATTLE_SOUND_CURSE1);
					return(FALSE);
				}

				switch (usSkillCheck)
				{
				case 1:
					StatChange(pSoldier, MECHANAMT, 25, FALSE);
					StatChange(pSoldier, WISDOMAMT, 10, FALSE);
					break;
				case 2:
					StatChange(pSoldier, MECHANAMT, 40, FALSE);
					StatChange(pSoldier, WISDOMAMT, 10, FALSE);
					break;
				case 3:
					StatChange(pSoldier, EXPLODEAMT, 25, FALSE);
					StatChange(pSoldier, WISDOMAMT, 5, FALSE);
					break;
				}
			}

			// transfer points...
			UINT32 combinedAmount;
			UINT32 attachmentAmount;
			UINT32 addedAmount;

			//CHRISL: Treat differently if we're combining money items (other then MONEY)
			if (Item[this->usItem].usItemClass != IC_MONEY)
			{
				if (Item[this->usItem].usItemClass == IC_AMMO)
				{
					ubLimit = Magazine[Item[this->usItem].ubClassIndex].ubMagSize;
				}
				else
				{
					ubLimit = 100;
				}

				// count down through # of attaching items and add to status of item in position 0
				for (bLoop = 0; bLoop < pAttachment->ubNumberOfObjects;)
				{
					if (usPointsAdded == 0 || usPointsUsed == 0)
					{
						//ADB need to watch for overflow here (thus UINT32), and need to cast to UINT8 before adding
						combinedAmount = (UINT16)(*this)[subObject]->data.objectStatus + (UINT16)(*pAttachment)[0]->data.objectStatus;
					}
					else
					{
						combinedAmount = (UINT32)(*this)[subObject]->data.objectStatus + (UINT32)(*pAttachment)[0]->data.objectStatus * usPointsAdded / usPointsUsed;
					}

					if (combinedAmount <= ubLimit)
					{
						// consume this one totally and continue
						(*this)[subObject]->data.objectStatus = combinedAmount;

						if (Item[this->usItem].usItemClass == IC_AMMO)
						{
							(*this)[subObject]->data.ubShotsLeft = combinedAmount;
						}

						pAttachment->RemoveObjectsFromStack(1);
					}
					else
					{
						// add part of this one and then we're done
						attachmentAmount = (UINT16)(*pAttachment)[0]->data.objectStatus;
						addedAmount = ubLimit - (UINT16)(*this)[subObject]->data.objectStatus;

						if (usPointsAdded != 0 && usPointsUsed != 0)
						{
							addedAmount = (UINT32)ceil(addedAmount * usPointsUsed / (FLOAT)usPointsAdded);
						}

						attachmentAmount -= addedAmount;

						(*pAttachment)[0]->data.objectStatus = attachmentAmount;

						if ((*pAttachment)[0]->data.objectStatus == 0)
						{
							pAttachment->RemoveObjectsFromStack(1);
						}

						(*this)[subObject]->data.objectStatus = ubLimit;

						if (Item[this->usItem].usItemClass == IC_AMMO)
						{
							(*this)[subObject]->data.ubShotsLeft = ubLimit;
						}

						break;
					}
				}
			}
			else
			{
				ubLimit = MAX_MONEY_PER_SLOT;

				if (usPointsAdded == 0 || usPointsUsed == 0)
				{
					combinedAmount = (*this)[subObject]->data.money.uiMoneyAmount + (*pAttachment)[0]->data.money.uiMoneyAmount;
				}
				else
				{
					combinedAmount = (*this)[subObject]->data.money.uiMoneyAmount + (*pAttachment)[0]->data.money.uiMoneyAmount * usPointsAdded / usPointsUsed;
				}

				if (combinedAmount <= ubLimit)
				{
					(*this)[subObject]->data.money.uiMoneyAmount = combinedAmount;
					pAttachment->RemoveObjectsFromStack(1);
				}
				else
				{
					attachmentAmount = (*pAttachment)[0]->data.money.uiMoneyAmount;
					addedAmount = ubLimit - (*this)[subObject]->data.money.uiMoneyAmount;

					if (usPointsAdded != 0 && usPointsUsed != 0)
					{
						addedAmount = (UINT32)ceil(addedAmount * usPointsUsed / (FLOAT)usPointsAdded);
					}

					attachmentAmount -= addedAmount;

					(*pAttachment)[0]->data.money.uiMoneyAmount = attachmentAmount;
					if ((*pAttachment)[0]->data.money.uiMoneyAmount == 0)
					{
						pAttachment->RemoveObjectsFromStack(1);
					}
					(*this)[subObject]->data.money.uiMoneyAmount = ubLimit;

					break;
				}
			}
			break;

		case DESTRUCTION:

			// the merge destroyed both items!
			this->RemoveObjectsFromStack(1);
			pAttachment->RemoveObjectsFromStack(1);
			pSoldier->DoMercBattleSound(BATTLE_SOUND_CURSE1);
			break;

		case TEMPERATURE:

			// check if this can work
			if (Item[this->usItem].usItemClass == IC_GUN && Item[pAttachment->usItem].barrel == TRUE)
			{
				FLOAT guntemperature = (*this)[subObject]->data.bTemperature;
				FLOAT barreltemperature = (*pAttachment)[0]->data.bTemperature;

				(*pAttachment)[0]->data.bTemperature = guntemperature;
				(*this)[subObject]->data.bTemperature = barreltemperature;

				UINT16 usOldItem = this->usItem;
				this->usItem = usResult;

				//WarmSteel - Replaced this with one that also checks default attachments, otherwise you could not replace built-in bonuses with default inseperable attachments.
				//RemoveProhibitedAttachments(pSoldier, this, usResult);
				ReInitMergedItem(pSoldier, this, usOldItem, 0);
			}
			break;

		case TRIPWIRE_ROLL:

			// check if this can work
			if (Item[this->usItem].tripwire && Item[pAttachment->usItem].tripwire && Item[this->usItem].usItemFlag == TRIPWIREROLL)
			{
				if ((*this)[subObject]->data.objectStatus < 100)
					(*this)[subObject]->data.objectStatus++;
				else
				{
					pSoldier->DoMercBattleSound(BATTLE_SOUND_CURSE1);
					return(FALSE);
				}

				pAttachment->RemoveObjectsFromStack(1);
			}

			break;

		case ELECTRONIC_MERGE:

			iCheckResult = SkillCheck(pSoldier, ATTACHING_SPECIAL_ELECTRONIC_ITEM_CHECK, -30);
			if (iCheckResult < 0)
			{
				DamageObj(this, (INT8)-iCheckResult);
				DamageObj(pAttachment, (INT8)-iCheckResult);
				pSoldier->DoMercBattleSound(BATTLE_SOUND_CURSE1);
				return(FALSE);
			}
			// grant experience! ... SANDRO - so what?! Grant them already! -- Madd: this was missing from AttachObjectNAS for some reason
			StatChange(pSoldier, MECHANAMT, 40, FALSE);
			StatChange(pSoldier, WISDOMAMT, 10, FALSE);

			// SANDRO - merc records - merging items -- Madd: this was missing from AttachObjectNAS for some reason
			if (pSoldier->ubProfile != NO_PROFILE)
				gMercProfiles[pSoldier->ubProfile].records.usItemsCombined++;

			// fall through
		case EXPLOSIVE_MERGE_HARD:
			if (ubType == EXPLOSIVE_MERGE_HARD) // coulda fallen through
			{
				// requires a skill check, and gives experience
				iCheckResult = SkillCheck(pSoldier, ATTACHING_DETONATOR_CHECK, -30);
				if (iCheckResult < 0)
				{
					// could have a chance of detonation
					// for now, damage both objects
					DamageObj(this, (INT8)-iCheckResult);
					DamageObj(pAttachment, (INT8)-iCheckResult);
					pSoldier->DoMercBattleSound(BATTLE_SOUND_CURSE1);
					return(FALSE);
				}
				StatChange(pSoldier, EXPLODEAMT, 25, FALSE);
				StatChange(pSoldier, WISDOMAMT, 5, FALSE);

				// SANDRO - merc records - merging items -- Madd: this was missing from AttachObjectNAS for some reason
				if (pSoldier->ubProfile != NO_PROFILE)
					gMercProfiles[pSoldier->ubProfile].records.usItemsCombined++;
			}
			// fall through
		case EXPLOSIVE_MERGE_EASY://Madd: new merge types
			if (ubType == EXPLOSIVE_MERGE_EASY) // coulda fallen through
			{
				// requires a skill check, and gives experience
				iCheckResult = SkillCheck(pSoldier, ATTACHING_DETONATOR_CHECK, 0);
				if (iCheckResult < 0)
				{
					// could have a chance of detonation
					// for now, damage both objects
					DamageObj(this, (INT8)-iCheckResult);
					DamageObj(pAttachment, (INT8)-iCheckResult);
					pSoldier->DoMercBattleSound(BATTLE_SOUND_CURSE1);
					return(FALSE);
				}
				StatChange(pSoldier, EXPLODEAMT, 10, FALSE);
				StatChange(pSoldier, WISDOMAMT, 2, FALSE);

				// SANDRO - merc records - merging items
				if (pSoldier->ubProfile != NO_PROFILE)
					gMercProfiles[pSoldier->ubProfile].records.usItemsCombined++;
			}
			// fall through
		case MECHANICAL_MERGE_HARD:
			if (ubType == MECHANICAL_MERGE_HARD) // coulda fallen through
			{
				// requires a skill check, and gives experience
				iCheckResult = SkillCheck(pSoldier, ATTACHING_SPECIAL_ITEM_CHECK, -30);
				if (iCheckResult < 0)
				{
					// could have a chance of detonation
					// for now, damage both objects
					DamageObj(this, (INT8)-iCheckResult);
					DamageObj(pAttachment, (INT8)-iCheckResult);
					pSoldier->DoMercBattleSound(BATTLE_SOUND_CURSE1);
					return(FALSE);
				}
				StatChange(pSoldier, MECHANAMT, 25, FALSE);
				StatChange(pSoldier, WISDOMAMT, 10, FALSE);

				// SANDRO - merc records - merging items
				if (pSoldier->ubProfile != NO_PROFILE)
					gMercProfiles[pSoldier->ubProfile].records.usItemsCombined++;
			}
			// fall through
		case MECHANICAL_MERGE_EASY:
			if (ubType == MECHANICAL_MERGE_EASY) // coulda fallen through
			{
				// requires a skill check, and gives experience
				iCheckResult = SkillCheck(pSoldier, ATTACHING_SPECIAL_ITEM_CHECK, 0);
				if (iCheckResult < 0)
				{
					// could have a chance of detonation
					// for now, damage both objects
					DamageObj(this, (INT8)-iCheckResult);
					DamageObj(pAttachment, (INT8)-iCheckResult);
					pSoldier->DoMercBattleSound(BATTLE_SOUND_CURSE1);
					return(FALSE);
				}
				StatChange(pSoldier, MECHANAMT, 10, FALSE);
				StatChange(pSoldier, WISDOMAMT, 2, FALSE);

				// SANDRO - merc records - merging items
				if (pSoldier->ubProfile != NO_PROFILE)
					gMercProfiles[pSoldier->ubProfile].records.usItemsCombined++;
			}
			// fall through
		default:
			// the merge will combine the two items
			//Madd: usResult2 only works for standard merges->item1 + item2 = item3 + item4

			if (ubType == EASY_MERGE || ubType == TREAT_ARMOUR)
			{
				if (usSkillCheck >= 1 && usSkillCheck <= 3)
				{
					switch (usSkillCheck)
					{
					case 1:
						iCheckResult = SkillCheck(pSoldier, ATTACHING_SPECIAL_ITEM_CHECK, -usSkillDifficulty);
						break;
					case 2:
						iCheckResult = SkillCheck(pSoldier, ATTACHING_SPECIAL_ELECTRONIC_ITEM_CHECK, -usSkillDifficulty);
						break;
					case 3:
						iCheckResult = SkillCheck(pSoldier, ATTACHING_DETONATOR_CHECK, -usSkillDifficulty);
						break;
					}

					if (iCheckResult < 0)
					{
						DamageObj(this, (INT8)-iCheckResult);
						DamageObj(pAttachment, (INT8)-iCheckResult);
						pSoldier->DoMercBattleSound(BATTLE_SOUND_CURSE1);
						return(FALSE);
					}

					switch (usSkillCheck)
					{
					case 1:
						StatChange(pSoldier, MECHANAMT, 25, FALSE);
						StatChange(pSoldier, WISDOMAMT, 10, FALSE);
						break;
					case 2:
						StatChange(pSoldier, MECHANAMT, 40, FALSE);
						StatChange(pSoldier, WISDOMAMT, 10, FALSE);
						break;
					case 3:
						StatChange(pSoldier, EXPLODEAMT, 25, FALSE);
						StatChange(pSoldier, WISDOMAMT, 5, FALSE);
						break;
					}
				}
			}

			if (usExtraItem)
			{
				INT8 bExtraItemSlot = FindObj(pSoldier, usExtraItem);

				if (bExtraItemSlot == ITEM_NOT_FOUND)
				{
					pSoldier->DoMercBattleSound(BATTLE_SOUND_NOTHING);
					return FALSE;
				}

				if (usExtraItemPointsUsed)
				{
					if (pSoldier->inv[bExtraItemSlot][0]->data.objectStatus < usExtraItemPointsUsed)
					{
						pSoldier->DoMercBattleSound(BATTLE_SOUND_NOTHING);
						return FALSE;
					}

					pSoldier->inv[bExtraItemSlot][0]->data.objectStatus -= usExtraItemPointsUsed;

					if (pSoldier->inv[bExtraItemSlot][0]->data.objectStatus <= 0)
					{
						pSoldier->inv[bExtraItemSlot].RemoveObjectsFromStack(1);
					}
				}
			}

			//Madd: unload guns after merge if ammo caliber or mag size don't match
			if (Item[this->usItem].usItemClass == IC_GUN && (*this)[subObject]->data.gun.usGunAmmoItem != NONE && (*this)[subObject]->data.gun.ubGunShotsLeft > 0)
			{
				if (Item[usResult].usItemClass != IC_GUN || Weapon[Item[usResult].ubClassIndex].ubCalibre != Weapon[Item[this->usItem].ubClassIndex].ubCalibre || (*this)[subObject]->data.gun.ubGunShotsLeft > Weapon[Item[usResult].ubClassIndex].ubMagSize)
				{ // item types/calibers/magazines don't match, spit out old ammo
					EjectAmmoAndPlace(pSoldier, this);
				}
			}

			UINT16 usOldItem = this->usItem;
			this->usItem = usResult;

			//WarmSteel - Replaced this with one that also checks default attachments, otherwise you could not replace built-in bonuses with default inseperable attachments.
			//RemoveProhibitedAttachments(pSoldier, this, usResult);
			// HEADROCK HAM 5: Added argument for statusindex.
			ReInitMergedItem(pSoldier, this, usOldItem, subObject);

			//Madd: if the attaching/merging item had any attachments on it, then try to move them to the first result
			for (attachmentList::iterator iter = (*pAttachment)[0]->attachments.begin(); iter != (*pAttachment)[0]->attachments.end();)
			{
				if (iter->exists() && ValidItemAttachmentSlot(this, iter->usItem, TRUE, FALSE, subObject))
				{
					//This seems to be rather valid. Can't be 100% sure though.
					OBJECTTYPE tempAttachment; // Madd:  we must recreate the attachments because they may themselves have default inseparable attachments...
					CreateItem(iter->usItem, (*iter)[0]->data.objectStatus, &tempAttachment);
					if (this->AttachObject(NULL, &tempAttachment, FALSE, subObject))
					{
						//remove this object so it won't appear twice
						iter = (*pAttachment)[0]->attachments.erase(iter);
					}
					else
					{
						++iter;
					}
				}
				else
				{
					++iter;
				}
			}

			if (ubType != TREAT_ARMOUR)
			{
				if (usPointsUsed && usPointsAdded)
				{
					(*this)[subObject]->data.objectStatus = ((*this)[subObject]->data.objectStatus * usPointsAdded + (*pAttachment)[0]->data.objectStatus * usPointsUsed) / (usPointsUsed + usPointsAdded);
				}
				/*else if (usPointsAdded)
				{
				(*this)[subObject]->data.objectStatus = usPointsAdded;
				}*/
				else
				{
					(*this)[subObject]->data.objectStatus = ((*this)[subObject]->data.objectStatus + (*pAttachment)[0]->data.objectStatus) / 2;
				}

				// Flugente: if the new item has the TRIPWIREROLL property, it only gets status 2 upon creation via merges
				if (Item[this->usItem].usItemFlag == TRIPWIREROLL)
				{
					(*this)[subObject]->data.objectStatus = 2;
				}
			}

			//ADB ubWeight has been removed, see comments in OBJECTTYPE
			//this->ubWeight = CalculateObjectWeight( this );

			if (usResult2 != NOTHING)
			{
				//Madd: usResult2 is what the original attachment/source item turns into

				//Madd: unload guns after merge if ammo caliber or mag size don't match
				if (Item[pAttachment->usItem].usItemClass == IC_GUN && (*pAttachment)[0]->data.gun.usGunAmmoItem != NONE && (*pAttachment)[0]->data.gun.ubGunShotsLeft > 0)
				{
					if (Item[usResult2].usItemClass != IC_GUN || Weapon[Item[usResult2].ubClassIndex].ubCalibre != Weapon[Item[pAttachment->usItem].ubClassIndex].ubCalibre || (*pAttachment)[0]->data.gun.ubGunShotsLeft > Weapon[Item[usResult2].ubClassIndex].ubMagSize)
					{ // item types/calibers/magazines don't match, spit out old ammo
						EjectAmmoAndPlace(pSoldier, pAttachment);
					}
				}

				pAttachment->usItem = usResult2;

				RemoveProhibitedAttachments(pSoldier, pAttachment, usResult2);

				if (ubType != TREAT_ARMOUR)
				{
					if (usPointsUsed && usPointsAdded)
					{
						(*pAttachment)[0]->data.objectStatus = ((*pAttachment)[0]->data.objectStatus * usPointsAdded + (*this)[subObject]->data.objectStatus * usPointsUsed) / (usPointsUsed + usPointsAdded);
					}
					/*else if (usPointsUsed)
					{
					(*pAttachment)[0]->data.objectStatus = usPointsUsed;
					}*/
					else
					{
						(*pAttachment)[0]->data.objectStatus = ((*pAttachment)[0]->data.objectStatus + (*this)[subObject]->data.objectStatus) / 2;
					}
				}
				//ADB ubWeight has been removed, see comments in OBJECTTYPE
				//pAttachment->ubWeight = CalculateObjectWeight( pAttachment );
			}
			else
			{
				pAttachment->RemoveObjectsFromStack(1);
			}

			if (pSoldier->bTeam == gbPlayerNum)
			{
				pSoldier->DoMercBattleSound(BATTLE_SOUND_COOL1);
			}
			break;
		}

		ApplyEquipmentBonuses(pSoldier);

		return( TRUE );
	}
	return( FALSE );
}

//CHRISL: Use this function to sort through Attachments.xml and Launchables.xml
UINT64 SetAttachmentSlotsFlag(OBJECTTYPE* pObj)
{
	UINT64		uiSlotFlag = 0;

	if (pObj->exists() == false)
		return 0;

	std::pair<std::multimap<UINT16, AttachmentStruct>::iterator, std::multimap<UINT16, AttachmentStruct>::iterator> range;
	std::multimap<UINT16, AttachmentStruct>::iterator it;
	range = AttachmentBackmap.equal_range(pObj->usItem);
	for (it = range.first; it != range.second; it++)
	{
		UINT16 attachmentId = it->second.attachmentIndex;
		if (ItemIsLegal(attachmentId, TRUE))
			uiSlotFlag |= Item[attachmentId].nasAttachmentClass;
	}

	for (UINT32 i = 0; i < gMAXLAUNCHABLES_READ; i++)
	{
		if (Launchable[i][1] == pObj->usItem)
		{
			UINT16 attachmentId = Launchable[i][0];
			if (ItemIsLegal(attachmentId, TRUE))
				uiSlotFlag |= Item[attachmentId].nasAttachmentClass;
		}
	}

	UINT64 point = GetAvailableAttachmentPoint(pObj, 0);
	for (UINT32 itemId = 1; itemId < gMAXITEMS_READ; itemId++)
	{
		if (IsAttachmentPointAvailable(point, itemId, TRUE))
			uiSlotFlag |= Item[itemId].nasAttachmentClass;
	}

	return uiSlotFlag;
}

/*std::map<UINT64, UINT64, std::less<>> gPointSlotFlag;
std::map<UINT32, UINT64, std::less<>> gItemSlotFlag;

//CHRISL: Use this function to sort through Attachments.xml and Launchables.xml
UINT64 SetAttachmentSlotsFlag(OBJECTTYPE* pObj)
{
	UINT64		uiSlotFlag = 0;
	UINT32		uiLoop = 0;
	UINT32		uiItem;

	if(!pObj->exists())
		return 0;

	UINT64 point = GetAvailableAttachmentPoint(pObj, 0);

	UINT64 uiPointSlotFlag = 0;
	UINT64 uiItemSlotFlag = 0;

	std::map<UINT64, UINT64, std::less<>>::iterator point_iter = gPointSlotFlag.find(point);
	std::map<UINT32, UINT64, std::less<>>::iterator item_iter = gItemSlotFlag.find(pObj->usItem);

	if (point_iter != gPointSlotFlag.end() &&
		item_iter != gItemSlotFlag.end())
	{
		uiPointSlotFlag = point_iter->second;
		uiItemSlotFlag = item_iter->second;

		return uiPointSlotFlag | uiItemSlotFlag;
	}

	while (uiLoop < gMAXITEMS_READ && Item[uiLoop].usItemClass != 0 ||
			uiLoop < gMAXATTACHMENTS_READ && Attachment[uiLoop][0] != 0 ||
			uiLoop < gMAXLAUNCHABLES_READ && Launchable[uiLoop][0] != 0)
	{
		if (uiLoop > 0 && uiLoop < gMAXITEMS_READ && IsAttachmentPointAvailable(point, uiLoop, TRUE))
		{
			uiItem = uiLoop;
			if (uiItem && ItemIsLegal(uiItem, TRUE))
			{
				uiSlotFlag |= Item[uiItem].nasAttachmentClass;
				uiPointSlotFlag |= Item[uiItem].nasAttachmentClass;
			}
		}

		if (uiLoop < gMAXATTACHMENTS_READ && Attachment[uiLoop][1] == pObj->usItem)
		{
			uiItem = Attachment[uiLoop][0];
			if (uiItem && ItemIsLegal(uiItem, TRUE))
			{
				uiSlotFlag |= Item[uiItem].nasAttachmentClass;
				uiItemSlotFlag |= Item[uiItem].nasAttachmentClass;
			}
		}

		if (uiLoop < gMAXLAUNCHABLES_READ && Launchable[uiLoop][1] == pObj->usItem)
		{
			uiItem = Launchable[uiLoop][0];
			if (uiItem && ItemIsLegal(uiItem, TRUE))
			{
				uiSlotFlag |= Item[uiItem].nasAttachmentClass;
				uiItemSlotFlag |= Item[uiItem].nasAttachmentClass;
			}
		}

		uiLoop++;
	}

	gPointSlotFlag[point] = uiPointSlotFlag;
	gItemSlotFlag[pObj->usItem] = uiItemSlotFlag;

	return uiSlotFlag;
}*/

//WarmSteel - This function returns the available item slot indexes in a vector.
std::vector<UINT16> GetItemSlots(OBJECTTYPE* pObj, UINT8 subObject, BOOLEAN fAttachment)
{
	std::vector<UINT16> tempItemSlots;
	std::vector<UINT16> tempAttachmentSlots;
	std::vector<UINT16>	tempSlots;
	UINT8				numSlots = 0;
	UINT16				magSize = 0;
	UINT64				fItemSlots = 0; //MM: Bumped the NAS UINT32s to UINT64s
	UINT64				fItemLayout = 0;
	BOOLEAN				fIsLBE = FALSE;

	if (!UsingNewAttachmentSystem() || !pObj->exists())
		return tempItemSlots;

	//CHRISL: We no longer need the ItemSlotAssign.xml file but we do still need to figure out which slots an item can have
	//Start by searching Attachments.xml and Launchables.xml for valid attachments for the primary object
	fItemSlots = SetAttachmentSlotsFlag(pObj);

	//Next, let's figure out which slots the item gives us access to
	if (fItemSlots)
	{
		fItemLayout = Item[pObj->usItem].nasLayoutClass;
		fIsLBE = (Item[pObj->usItem].usItemClass == IC_LBEGEAR);
		if (Item[pObj->usItem].grenadelauncher || Item[pObj->usItem].rocketlauncher)
			magSize = GetMagSize(pObj);

		//We don't need to do anything if the item gets no slots
		for (UINT8 sClass = 0; sClass < 64; ++sClass)
		{
			//go through each attachment class and find the slots the item should have
			UINT64 uiClass = (UINT64)pow((double)2, (int)sClass);
			UINT32 slotSize = tempItemSlots.size();

			if (fItemSlots & uiClass)
			{
				//don't bother with this slot if it's not a valid class
				for (UINT16 sCount = 1; sCount < MAXITEMS + 1; ++sCount)
				{
					if (!AttachmentSlots[sCount].uiSlotIndex)
						break;

					if (AttachmentSlots[sCount].nasAttachmentClass & uiClass && AttachmentSlots[sCount].nasLayoutClass & fItemLayout)	//found a slot
					{
						if (AttachmentSlots[sCount].fMultiShot)
						{
							if (magSize)
								--magSize;
							else
								continue;
						}

						if (fIsLBE && AttachmentSlots[sCount].ubPocketMapping)
						{
							if (LoadBearingEquipment[Item[pObj->usItem].ubClassIndex].lbePocketsAvailable & (UINT16)pow((double)2, AttachmentSlots[sCount].ubPocketMapping - 1))
							{
								tempItemSlots.push_back(AttachmentSlots[sCount].uiSlotIndex);
							}
						}
						else
						{
							tempItemSlots.push_back(AttachmentSlots[sCount].uiSlotIndex);
						}
					}
				}

				if (slotSize == tempItemSlots.size())
				{
					//we didn't find a layout specific slot so try to find a default layout slot
					for (UINT16 sCount = 1; sCount < MAXITEMS + 1; ++sCount)
					{
						if (!AttachmentSlots[sCount].uiSlotIndex)
							break;

						if (AttachmentSlots[sCount].nasAttachmentClass & uiClass && AttachmentSlots[sCount].nasLayoutClass == 1)
						{
							//found a default slot
							tempItemSlots.push_back(AttachmentSlots[sCount].uiSlotIndex);
						}
					}
				}
			}
		}
	}

	//Now that we've setup tempItemSlots for the main item, let's look at the individual attachments
	//Madd: Only record these extra slots if the item has its attachment flag set!
	for (attachmentList::iterator iter = (*pObj)[subObject]->attachments.begin(); iter != (*pObj)[subObject]->attachments.end(); ++iter)
	{
		if (iter->exists() && Item[iter->usItem].attachment && !(*iter)[0]->attachments.empty())
		{
			OBJECTTYPE* pAttachment = &(*iter);
			tempSlots = GetItemSlots(pAttachment, 0, TRUE);

			for (UINT8 x = 0; x < tempSlots.size(); ++x)
				tempAttachmentSlots.push_back(tempSlots[x]);
		}
	}

	//Now that we have tempAttachmentSlots, put it all together, assuming we're woking on the main item
	if (!fAttachment)
	{
		if (!tempAttachmentSlots.empty())
		{
			//Add attachmentSlots to itemSlots
			for (UINT8 attachSlot = 0; attachSlot < tempAttachmentSlots.size(); ++attachSlot)
			{
				tempItemSlots.push_back(tempAttachmentSlots[attachSlot]);
			}
		}

		tempSlots = tempItemSlots;
		for (std::vector<UINT16>::iterator iter1 = tempSlots.begin(); iter1 != tempSlots.end(); ++iter1)
		{
			BOOLEAN fSlotDuplicated = FALSE;
			for (std::vector<UINT16>::iterator iter = tempItemSlots.begin(); iter != tempItemSlots.end();)
			{
				UINT16 i1 = *iter1;
				UINT16 i = *iter;
				if (i1 == i && !fSlotDuplicated)
				{
					fSlotDuplicated = TRUE;
					++iter;
					continue;
				}
				else if (i1 == i && fSlotDuplicated)
				{
					iter = tempItemSlots.erase(iter);
					continue;
				}
				else
					++iter;
			}
		}

		//If we still have no attachment slots, look through Merges.xml to see if we have merges and use the 4 default slots if we do
		//Should we throw on default slots regardless of whether there is an entry in Merges?  Ammo doesn't have Merges entries but we still need slots for them.
		if (tempItemSlots.empty())
		{
			//			INT32 iLoop = 0;
			//			while( 1 ){
			//				if (Merge[iLoop][1] == pObj->usItem)
			{
				tempItemSlots.push_back(1);
				tempItemSlots.push_back(2);
				tempItemSlots.push_back(3);
				tempItemSlots.push_back(4);
				//					break;
			}
			//				iLoop++;
			//				if (Merge[iLoop][1] == 0)
			//				{
			//					break;
			//				}
			//			}
		}
	}

	return tempItemSlots;
}

void InitItemAttachments(OBJECTTYPE* pObj){

	if(UsingNewAttachmentSystem()==false)
		return;

	//pObj->usAttachmentSlotIndexVector = GetItemSlots(pObj);
	(*pObj)[0]->attachments.resize((GetItemSlots(pObj).size()));
}
/*
//WarmSteel - Changed this function for NAS, because when the slots change many items will become invalid IN THAT SLOT, but not on the weapon.
To fix this we re-attach all invalid attachments. It also checks the slots that are on the gun.
IMPORTANT: If you use AttachObject/RemoveAttachment with fRemoveProhibited TRUE in this function, I will hunt you down and smite you.
*/
void RemoveProhibitedAttachments(SOLDIERTYPE* pSoldier, OBJECTTYPE* pObj, UINT16 usItem, BOOLEAN fOnlyRemoveWhenSlotsChange)
{
	if(!pObj->exists() || !(*pObj)[0]->AttachmentListSize())//dnl ch75 261013
		return;

	if(UsingNewAttachmentSystem()==true){
		UINT32				curSlot = 0;
		BOOLEAN				fCanAttach = FALSE;
		BOOLEAN				fRemoveProhibitedAttachments = FALSE;
		BOOLEAN				fDoneRemovingProhibited = FALSE;
		attachmentList		tempAttachList;
		std::vector<UINT16> tempItemSlots;
		std::vector<UINT16>	usAttachmentSlotIndexVector = GetItemSlots(pObj);

		//Get the item slots this item SHOULD have (but may not have right now), also counting any slots that were added or removed.
		//tempItemSlots = GetItemSlots(pObj);

		//Check if the slots have changed.
		if(fOnlyRemoveWhenSlotsChange)
		{
			if(usAttachmentSlotIndexVector.size() != (*pObj)[0]->attachments.size())
			{
				fRemoveProhibitedAttachments = TRUE;
			}
			else 
			{
				for(attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end() && !fRemoveProhibitedAttachments; ++iter, ++curSlot)
				{
					if(iter->exists())
					{
						UINT16 slotIndex = usAttachmentSlotIndexVector[curSlot];
						UINT16 sItem = iter->usItem;
						if(!(AttachmentSlots[slotIndex].nasAttachmentClass & Item[sItem].nasAttachmentClass))
						{
							fRemoveProhibitedAttachments = TRUE;
						}
					}
				}
			}
		}

		if(fRemoveProhibitedAttachments){
			UINT16 usInfiniteLoopCount = 0;
			//CHRISL: There is the chance that we could have attachments in slots that are no longer valid, but if we arbitrarily resize the attachmentList
			//	we could actually lose attachments.  So let's just remove all attachments and replace them in correct locations
			for(attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
			{
				if(iter->exists())
				{
					tempAttachList.push_back((*iter));
					iter = (*pObj)[0]->RemoveAttachmentAtIter(iter);
				}
			}
			//pObj->usAttachmentSlotIndexVector = tempItemSlots;
			//Keep checking till slots stop changing.
			while(!fDoneRemovingProhibited){
				//Surely 500 tries is enough to fix ANY item...
				AssertMsg(usInfiniteLoopCount < 500, "There was an inifinite loop while removing prohibited attachments");

				//Resize the object to the correct size
				(*pObj)[0]->attachments.resize(usAttachmentSlotIndexVector.size());

				//Start by trying to re-attach inseperable items.  They take precedence over items that can normally be removed
				for (attachmentList::iterator iter = tempAttachList.begin(); iter != tempAttachList.end();) 
				{
					if (iter->exists() && Item[iter->usItem].inseparable && ValidItemAttachmentSlot(pObj, iter->usItem, TRUE, FALSE, 0, -1, 0, NULL, usAttachmentSlotIndexVector))
					{
						//This seems to be rather valid. Can't be 100% sure though.
						if(pObj->AttachObject(NULL, &(*iter), FALSE, 0, -1, FALSE, usAttachmentSlotIndexVector))
						{
							//Ok now we can be sure, lets remove this object so we don't try to drop it later.
							iter = tempAttachList.erase(iter);
						} 
						else 
						{
							 ++iter;
						}
					} 
					else 
					{
						++iter;
					}
				}
				//Try to attach all the other attachments that didn't fit their current slot.
				for (attachmentList::iterator iter = tempAttachList.begin(); iter != tempAttachList.end();) 
				{
					if (iter->exists() && ValidItemAttachmentSlot(pObj, iter->usItem, TRUE, FALSE, 0, -1, 0, NULL, usAttachmentSlotIndexVector))
					{
						//This seems to be rather valid. Can't be 100% sure though.
						if(pObj->AttachObject(NULL, &(*iter), FALSE, 0, -1, FALSE, usAttachmentSlotIndexVector))
						{
							//Ok now we can be sure, lets remove this object so we don't try to drop it later.
							iter = tempAttachList.erase(iter);
						}
						else 
						{
							 ++iter;
						}
					}
					else 
					{
						++iter;
					}
				}

				tempItemSlots = GetItemSlots(pObj);

				//assume all attachments are replaced and we're good to break the look
				fDoneRemovingProhibited = TRUE;

				//Check if the slots have changed after possibly removing attachments.
				if(tempItemSlots.size() != usAttachmentSlotIndexVector.size()){
					//Changed, we need to correct again.
					fDoneRemovingProhibited = FALSE;
				} else if(!tempItemSlots.empty()){
					for (UINT16 cnt = 0; cnt < tempItemSlots.size(); ++cnt) {
						//If these slots don't match, something has changed, keep checking.
						if(tempItemSlots[cnt] != usAttachmentSlotIndexVector[cnt]){
							fDoneRemovingProhibited = FALSE;
							break;
						}
					}
				}

				if(usInfiniteLoopCount > 10){	//run through the loop 10 times before we drop items.
					//Anything that's still in our tempAttachList couldn't be attached, and should be dropped.
					for (attachmentList::iterator iter = tempAttachList.begin(); iter != tempAttachList.end();) 
					{
						if (iter->exists())
						{
							// Flugente: placing objects at a soldier is only advised if the soldier exists in the first place
							if ( pSoldier )
							{
								if ( !AutoPlaceObject( pSoldier, &(*iter), FALSE ) )
								{
									AutoPlaceObjectToWorld(pSoldier, &(*iter), TRUE);
								}
							}
							else
							{
								// put it on the ground //dnl ch75 281013
								INT32 sGridNo = NOWHERE;
								UINT8 ubLevel = 0;
								for(UINT32 i=0; i<guiNumWorldItems; ++i)
								{
									if(gWorldItems[i].fExists && (pObj == &gWorldItems[i].object))
									{
										sGridNo = gWorldItems[i].sGridNo;
										ubLevel = gWorldItems[i].ubLevel;
										break;
									}
								}

								if(sGridNo != NOWHERE)
								{
									if(guiCurrentItemDescriptionScreen == MAP_SCREEN && fShowMapInventoryPool)
										AutoPlaceObjectInInventoryStash(&(*iter), sGridNo, ubLevel);
									else
										AddItemToPool(sGridNo, &(*iter), 1, ubLevel, WORLD_ITEM_REACHABLE, 0);
								}
							}
							iter = tempAttachList.erase(iter);
						}
						else
						{
							++iter;
						}
					}
				}
				if(!tempAttachList.empty()){
					fDoneRemovingProhibited = FALSE;
				}

				usAttachmentSlotIndexVector = GetItemSlots(pObj);
				usInfiniteLoopCount++;
			}
		}
	} else {
		UINT16 x = 0;
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end();) 
		{
			if(!iter->exists())
			{
				//WarmSteel - erase null objects, we don't need them without the NAS.
				iter = (*pObj)[0]->attachments.erase(iter);
			} 
			else 
			{
				if ( (!ValidAttachment(iter->usItem, pObj) && !ValidLaunchable(iter->usItem, usItem) && !ValidLaunchable(iter->usItem, GetAttachedGrenadeLauncher(pObj)) ) || x >= OLD_MAX_ATTACHMENTS_101)
				{
					if ( !Item[iter->usItem].inseparable )
					{
						if (pSoldier) {
							if ( !AutoPlaceObject( pSoldier, &(*iter), FALSE ) )
							{   // put it on the ground
								AddItemToPool( pSoldier->sGridNo, &(*iter), 1, pSoldier->pathing.bLevel, 0 , -1 );
							}
						}
					}
					pObj->RemoveAttachment(&(*iter),0 ,0 , pSoldier, TRUE, FALSE);
					iter = (*pObj)[0]->attachments.begin();
					x = 0;
				} 
				else 
				{
					++x;
					++iter; 
				}
			}
		}
	}
	return;
}

//WarmSteel - Needed for merges, because the new items don't get their new default attachments.
//This does the same as RemoveProhibitedAttachments but is a bit more thorough at it.
//This is at the risk that items move around if more than one slot is valid, but since this is used for "new" guns after merges, that's acceptable.
// HEADROCK HAM 5: Added argument for statusindex.
attachmentList ReInitMergedItem(SOLDIERTYPE* pSoldier, OBJECTTYPE* pObj, UINT16 usOldItem, UINT8 ubStatusIndex)
{
	attachmentList		unattachableList; //Madd: list of attachments that couldn't be attached or dropped (ie: inseparable attachments that got separated)
	attachmentList		tempAttachList;
	attachmentList		tempSlotChangingAttachList;
	BOOLEAN				fFoundDefaultAttachment = FALSE;
	UINT8 slotCount;
	std::vector<UINT16>	usAttachmentSlotIndexVector = GetItemSlots(pObj);

	//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"ReInitMergedItem %d", pObj->usItem);

	if (!(*pObj)[ubStatusIndex]->attachments.empty())
	{
		//Have to take all attachments off, because of possible incompatibilities with the default attachments (can't NOT attach a default attachment because some stupid item already attached is incompatible with it).

		//Delete all default inseparable attachments from the old gun.
		//This is safe UNLESS the inseparable default attachments is not the only attachment of the same kind on the gun.
		//I can't think of any reason why this would happen, and if it does the worst that can happen is your attachment disappearing.
		for (slotCount = 0; slotCount < (*pObj)[ubStatusIndex]->attachments.size(); slotCount++)
		{
			UINT16 usAttach = (*pObj)[ubStatusIndex]->GetAttachmentAtIndex(slotCount)->usItem;
			if (Item[usAttach].inseparable == 1)
			{
				for (UINT16 cnt = 0; cnt < MAX_DEFAULT_ATTACHMENTS && Item[usOldItem].defaultattachments[cnt] != 0; cnt++)
				{
					if (Item[usOldItem].defaultattachments[cnt] == usAttach)
					{
						(*pObj)[ubStatusIndex]->RemoveAttachmentAtIndex(slotCount);
						break;
					}
				}
			}
		}
		slotCount = 0;
		//Put all other attachments into a temporary storage, so we can try to re-attach later.
		for (attachmentList::iterator iter = (*pObj)[ubStatusIndex]->attachments.begin(); iter != (*pObj)[ubStatusIndex]->attachments.end(); iter++, slotCount++) 
		{
			if (iter->exists())
			{
				UINT16 cnt = 0;
				tempAttachList.push_back((*iter));
			}
		}
	}

	//clear the attachment list, we've saved the attachments somewhere safe now.
	(*pObj)[ubStatusIndex]->attachments.clear();

	//Make sure the attachment slot data is correct.
	//std::vector<UINT16> tempItemSlots = GetItemSlots(pObj);
	(*pObj)[ubStatusIndex]->attachments.resize(usAttachmentSlotIndexVector.size());
	//pObj->usAttachmentSlotIndexVector = tempItemSlots;

	//Now add all default attachments, but add them with the same status as the gun. We don't want to make repairing guns easy :)
	for (UINT16 cnt = 0; cnt < MAX_DEFAULT_ATTACHMENTS && Item[pObj->usItem].defaultattachments[cnt] != 0; cnt++)
	{
		//ScreenMsg(FONT_LTGREEN, MSG_INTERFACE, L"default attachment %d", Item[pObj->usItem].defaultattachments[cnt]);
		//Only add inseparable default attachments, because they are likely "part" of the gun.
		if (Item[Item[pObj->usItem].defaultattachments[cnt]].inseparable == 1)
		{
			OBJECTTYPE defaultAttachment;
			CreateItem(Item[pObj->usItem].defaultattachments[cnt], (*pObj)[ubStatusIndex]->data.objectStatus, &defaultAttachment);
			AssertMsg(pObj->AttachObject(NULL, &defaultAttachment, FALSE, ubStatusIndex, -1, FALSE), String("A default attachment could not be attached after merging, this should not be possible. item %d attachment %d", pObj->usItem, defaultAttachment.usItem));
		}
	}

	//First re-attach any slot-changing attachments.
	for (attachmentList::iterator iter = tempSlotChangingAttachList.begin(); iter != tempSlotChangingAttachList.end();)
	{
		if (iter->exists() && ValidItemAttachmentSlot(pObj, iter->usItem, TRUE, FALSE, ubStatusIndex))
		{
			//This seems to be rather valid. Can't be 100% sure though.
			OBJECTTYPE tempAttachment; // Madd:  we must recreate the attachments because they may themselves have default inseparable attachments...
			CreateItem(iter->usItem, (*iter)[0]->data.objectStatus, &tempAttachment);
			if (pObj->AttachObject(NULL, &tempAttachment, FALSE, ubStatusIndex))
			{
				//Ok now we can be sure, lets remove this object so we don't try to drop it later.
				iter = tempSlotChangingAttachList.erase(iter);
			}
			else 
			{
				++iter;
			}
		}
		else 
		{
			++iter;
		}
	}

	//Time to re-attach the other attachments, if we can. I am the king of copy pasta.
	for (attachmentList::iterator iter = tempAttachList.begin(); iter != tempAttachList.end();)
	{
		if (iter->exists() && ValidItemAttachmentSlot(pObj, iter->usItem, TRUE, FALSE, ubStatusIndex))
		{
			//This seems to be rather valid. Can't be 100% sure though.
			OBJECTTYPE tempAttachment; // Madd:  we must recreate the attachments because they may themselves have default inseparable attachments...
			CreateItem(iter->usItem, (*iter)[0]->data.objectStatus, &tempAttachment);
			if (pObj->AttachObject(NULL, &tempAttachment, FALSE, ubStatusIndex))
			{
				//Ok now we can be sure, lets remove this object so we don't try to drop it later.
				iter = tempAttachList.erase(iter);
			}
			else 
			{
				++iter;
			}
		}
		else 
		{
			++iter;
		}
	}

	// sevenfm: try to attach remaining attachments again
	for (attachmentList::iterator iter = tempAttachList.begin(); iter != tempAttachList.end();)
	{
		if (iter->exists() && ValidItemAttachmentSlot(pObj, iter->usItem, TRUE, FALSE, ubStatusIndex))
		{
			//This seems to be rather valid. Can't be 100% sure though.
			OBJECTTYPE tempAttachment; // Madd:  we must recreate the attachments because they may themselves have default inseparable attachments...
			CreateItem(iter->usItem, (*iter)[0]->data.objectStatus, &tempAttachment);
			if (pObj->AttachObject(NULL, &tempAttachment, FALSE, ubStatusIndex))
			{
				//Ok now we can be sure, lets remove this object so we don't try to drop it later.
				iter = tempAttachList.erase(iter);
			}
			else
			{
				++iter;
			}
		}
		else
		{
			++iter;
		}
	}

	//drop all items we couldn't re-attach.
	for (attachmentList::iterator iter = tempSlotChangingAttachList.begin(); iter != tempSlotChangingAttachList.end(); ++iter) 
	{
		if (iter->exists())
		{
			if (Item[iter->usItem].inseparable != 1)
			{
				//WarmSteel - Couldn't re-attach this item, try to drop it.
				if (pSoldier)
				{
					OBJECTTYPE tempAttachment; // Madd:  we must recreate the attachments because they may themselves have default inseparable attachments...
					CreateItem(iter->usItem, (*iter)[0]->data.objectStatus, &tempAttachment);
					if (!AutoPlaceObject(pSoldier, &tempAttachment, FALSE))
					{   // put it on the ground
						// HEADROCK HAM 5: A much more suitable function. Works in both tactical and mapscreen!
						AutoPlaceObjectToWorld(pSoldier, &tempAttachment, TRUE);
						//AddItemToPool( pSoldier->sGridNo, &(*iter), 1, pSoldier->pathing.bLevel, 0 , -1 );
					}
				}
			}
			else
				unattachableList.push_back((*iter));
		}
	}
	//and the rest too
	for (attachmentList::iterator iter = tempAttachList.begin(); iter != tempAttachList.end(); ++iter) 
	{
		if (iter->exists())
		{
			if (Item[iter->usItem].inseparable != 1)
			{//WarmSteel - Couldn't re-attach this item, try to drop it.
				if (pSoldier)
				{
					OBJECTTYPE tempAttachment; // Madd:  we must recreate the attachments because they may themselves have default inseparable attachments...
					CreateItem(iter->usItem, (*iter)[0]->data.objectStatus, &tempAttachment);
					if (!AutoPlaceObject(pSoldier, &tempAttachment, FALSE))
					{   // put it on the ground
						// HEADROCK HAM 5: A much more suitable function. Works in both tactical and mapscreen!
						AutoPlaceObjectToWorld(pSoldier, &tempAttachment, TRUE);
						//AddItemToPool( pSoldier->sGridNo, &(*iter), 1, pSoldier->pathing.bLevel, 0 , -1 );
					}
				}
			}
			else
				unattachableList.push_back((*iter));
		}
	}

	return unattachableList;
}

void EjectAmmoAndPlace(SOLDIERTYPE* pSoldier, OBJECTTYPE* pObj, UINT8 subObject)
{
	CreateAmmo((*pObj)[subObject]->data.gun.usGunAmmoItem, &gTempObject, (*pObj)[subObject]->data.gun.ubGunShotsLeft);
	(*pObj)[subObject]->data.gun.ubGunShotsLeft = 0;
	(*pObj)[subObject]->data.gun.usGunAmmoItem = NONE;
	// HEADROCK HAM 3.5: Clear ammo type
	(*pObj)[subObject]->data.gun.ubGunAmmoType = NONE;
	if ( pSoldier )
	{
		AutoPlaceObjectAnywhere( pSoldier, &gTempObject, FALSE );
//		if ( !AutoPlaceObject( pSoldier, &gTempObject, FALSE ) )
//		{   // put it on the ground
//			AddItemToPool( pSoldier->sGridNo, &gTempObject, 1, pSoldier->pathing.bLevel, 0 , -1 );
//		}
	}
	return;
}

/* CHRISL: This function is edited to handle the new inventory system when we have an item in our cursor.
Not only do we have to hatch out pockets that the item won't fit in, we also have to hatch out pockets that
our current LBE gear haven't activated.  We'll also need to display the number of items of the type currently
held in the cursor that each active pocket can hold.*/
extern BOOLEAN CompatibleAmmoForGun( OBJECTTYPE *pTryObject, OBJECTTYPE *pTestObject );
BOOLEAN CanItemFitInVehicle( SOLDIERTYPE *pSoldier, OBJECTTYPE *pObj, INT8 bPos, BOOLEAN fDoingPlacement )
{
	if((UsingNewInventorySystem() == false) || !(pSoldier->flags.uiStatusFlags & SOLDIER_VEHICLE))
		return(FALSE);
	if(!vehicleInv[bPos])
		return(FALSE);

	UINT8					ubSlotLimit;
	ubSlotLimit = ItemSlotLimit( pObj, bPos, pSoldier );

	if ( ubSlotLimit == 0 )
		return ( CompatibleAmmoForGun(pObj, &pSoldier->inv[bPos]) );

	return( TRUE );
}


BOOLEAN CanItemFitInPosition( SOLDIERTYPE *pSoldier, OBJECTTYPE *pObj, INT8 bPos, BOOLEAN fDoingPlacement )
{
	UINT8	ubSlotLimit, lbePocket = 1;
	INT8	bNewPos = ITEM_NOT_FOUND;
	UINT32	pRestrict=0;

	// CHRISL: Only check valid pockets
	if((UsingNewInventorySystem() == false) && !oldInv[bPos])
		return(FALSE);
	if((pSoldier->flags.uiStatusFlags & SOLDIER_VEHICLE) && UsingNewInventorySystem() == true)
		return(CanItemFitInVehicle(pSoldier, pObj, bPos, fDoingPlacement));

	ubSlotLimit = ItemSlotLimit( pObj, bPos, pSoldier );

	switch( bPos )
	{
		case SECONDHANDPOS:
			if (Item[pSoldier->inv[HANDPOS].usItem].twohanded )
			{
				return( FALSE );
			}
			break;
		case HANDPOS:
			if (Item[ pObj->usItem ].twohanded )
			{
				if (pSoldier->inv[HANDPOS].exists() == true && pSoldier->inv[SECONDHANDPOS].exists() == true)
				{
					// two items in hands; try moving the second one so we can swap
					if (FitsInSmallPocket(&pSoldier->inv[SECONDHANDPOS]) == true)
					{
						bNewPos = FindEmptySlotWithin( pSoldier, BIGPOCKSTART, NUM_INV_SLOTS );
					}
					else
					{
						bNewPos = FindEmptySlotWithin( pSoldier, BIGPOCKSTART, MEDPOCKFINAL );
					}

					if (bNewPos == NO_SLOT)
					{
						// nowhere to put second item
						return( FALSE );
					}

					if ( fDoingPlacement )
					{
						// otherwise move it, forget about bNewPos!
						PlaceInAnyPocket(pSoldier, &pSoldier->inv[SECONDHANDPOS], FALSE);
					}
				}
			}
			break;
		case VESTPOS:
		case HELMETPOS:
		case LEGPOS:
			if (Item[pObj->usItem].usItemClass != IC_ARMOUR)
			{
				return( FALSE );
			}
			switch (bPos)
			{
				case VESTPOS:
					if (Armour[Item[pObj->usItem].ubClassIndex].ubArmourClass != ARMOURCLASS_VEST)
					{
						return( FALSE );
					}
					break;
				case HELMETPOS:
					if (Armour[Item[pObj->usItem].ubClassIndex].ubArmourClass != ARMOURCLASS_HELMET)
					{
						return( FALSE );
					}
					break;
				case LEGPOS:
					if (Armour[Item[pObj->usItem].ubClassIndex].ubArmourClass != ARMOURCLASS_LEGGINGS)
					{
						return( FALSE );
					}
					break;
				default:
					break;
			}
			break;
		case HEAD1POS:
		case HEAD2POS:
			if (Item[pObj->usItem].usItemClass != IC_FACE)
			{
				return( FALSE );
			}
			break;
		case VESTPOCKPOS:
			if (Item[pObj->usItem].usItemClass != IC_LBEGEAR || LoadBearingEquipment[Item[pObj->usItem].ubClassIndex].lbeClass != VEST_PACK)
			{
				return( FALSE );
			}
			break;
		case LTHIGHPOCKPOS:
		case RTHIGHPOCKPOS:
			if (Item[pObj->usItem].usItemClass != IC_LBEGEAR || LoadBearingEquipment[Item[pObj->usItem].ubClassIndex].lbeClass != THIGH_PACK)
			{
				return( FALSE );
			}
			break;
		case CPACKPOCKPOS:
			if (Item[pObj->usItem].usItemClass != IC_LBEGEAR || LoadBearingEquipment[Item[pObj->usItem].ubClassIndex].lbeClass != COMBAT_PACK)
			{
				return( FALSE );
			}
			if(pSoldier->inv[BPACKPOCKPOS].exists() == true)
			{
				//DBrot: changed to bitwise comparison
				if(((LoadBearingEquipment[Item[pSoldier->inv[BPACKPOCKPOS].usItem].ubClassIndex].lbeCombo & LoadBearingEquipment[Item[pObj->usItem].ubClassIndex].lbeCombo) == 0) ||
				LoadBearingEquipment[Item[pSoldier->inv[BPACKPOCKPOS].usItem].ubClassIndex].lbeCombo == 0)
				{
					return( FALSE );
				}
			}
			break;
		case BPACKPOCKPOS:
			if (Item[pObj->usItem].usItemClass != IC_LBEGEAR || LoadBearingEquipment[Item[pObj->usItem].ubClassIndex].lbeClass != BACKPACK)
			{
				return( FALSE );
			}
			// Removed backpack/gunsling restrictions
			//if(pSoldier->inv[GUNSLINGPOCKPOS].exists() == true)
			//	return( FALSE );
			if(pSoldier->inv[CPACKPOCKPOS].exists() == true)
			{
				//DBrot: changed to bitwise comparison
				if(((LoadBearingEquipment[Item[pSoldier->inv[CPACKPOCKPOS].usItem].ubClassIndex].lbeCombo & LoadBearingEquipment[Item[pObj->usItem].ubClassIndex].lbeCombo) ==0)||
				LoadBearingEquipment[Item[pSoldier->inv[CPACKPOCKPOS].usItem].ubClassIndex].lbeCombo == 0)
				{
					return( FALSE );
				}
			}
			break;
		case GUNSLINGPOCKPOS:	// Gun Sling
			//if (Item[pObj->usItem].usItemClass != IC_GUN && Item[pObj->usItem].usItemClass != IC_BLADE && Item[pObj->usItem].usItemClass != IC_LAUNCHER)
			if(pObj->usItem == MONEY)
				return( FALSE );
			if(Item[pObj->usItem].usItemClass == IC_AMMO || Item[pObj->usItem].usItemClass == IC_GRENADE)
				return(CompatibleAmmoForGun(pObj, &pSoldier->inv[GUNSLINGPOCKPOS]) || ValidAttachment(pObj->usItem, &(pSoldier->inv[GUNSLINGPOCKPOS]) ) || ValidLaunchable(pObj->usItem, pSoldier->inv[GUNSLINGPOCKPOS].usItem));
			//recalc slot limit to exclude ItemSize attachment modifiers
			//ubSlotLimit = ItemSlotLimit( pObj, bPos, pSoldier, FALSE );
			break;
		case KNIFEPOCKPOS:	// Knife sheath
			if(pObj->usItem == MONEY)
				return( FALSE );
			if (Item[pObj->usItem].usItemClass != IC_BLADE && Item[pObj->usItem].usItemClass != IC_THROWING_KNIFE )
				return(CompatibleAmmoForGun(pObj, &pSoldier->inv[KNIFEPOCKPOS]) || ValidAttachment(pObj->usItem, &(pSoldier->inv[KNIFEPOCKPOS]) ) || ValidLaunchable(pObj->usItem, pSoldier->inv[KNIFEPOCKPOS].usItem));
			break;
		// IC Pockets
		case BIGPOCK1POS:
		case BIGPOCK2POS:
		case BIGPOCK3POS:
		case BIGPOCK4POS:
		case BIGPOCK5POS:
		case BIGPOCK6POS:
		case BIGPOCK7POS:
		case MEDPOCK1POS:
		case MEDPOCK2POS:
		case MEDPOCK3POS:
		case MEDPOCK4POS:
		case SMALLPOCK1POS:
		case SMALLPOCK2POS:
		case SMALLPOCK3POS:
		case SMALLPOCK4POS:
		case SMALLPOCK5POS:
		case SMALLPOCK6POS:
		case SMALLPOCK7POS:
		case SMALLPOCK8POS:
		case SMALLPOCK9POS:
		case SMALLPOCK10POS:
		case SMALLPOCK11POS:
		case SMALLPOCK12POS:
		case SMALLPOCK13POS:
		case SMALLPOCK14POS:
		case SMALLPOCK15POS:
		case SMALLPOCK16POS:
		case SMALLPOCK17POS:
		case SMALLPOCK18POS:
		case SMALLPOCK19POS:
		case SMALLPOCK20POS:
		case SMALLPOCK21POS:
		case SMALLPOCK22POS:
		case SMALLPOCK23POS:
		case SMALLPOCK24POS:
		case SMALLPOCK25POS:
		case SMALLPOCK26POS:
		case SMALLPOCK27POS:
		case SMALLPOCK28POS:
		case SMALLPOCK29POS:
		case SMALLPOCK30POS:
			if((UsingNewInventorySystem() == true))
			{
				//if(icLBE[bPos] == BPACKPOCKPOS && (!(pSoldier->flags.ZipperFlag) || (pSoldier->flags.ZipperFlag && gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_STAND)) && (gTacticalStatus.uiFlags & INCOMBAT))
				if (icLBE[bPos] == BPACKPOCKPOS && (!(pSoldier->flags.ZipperFlag) || (pSoldier->flags.ZipperFlag && gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_STAND)) && (gTacticalStatus.uiFlags & INCOMBAT) && fDoingPlacement)
					return( FALSE );

				lbePocket = (pSoldier->inv[icLBE[bPos]].exists() == false) ? LoadBearingEquipment[Item[icDefault[bPos]].ubClassIndex].lbePocketIndex[icPocket[bPos]] : LoadBearingEquipment[Item[pSoldier->inv[icLBE[bPos]].usItem].ubClassIndex].lbePocketIndex[icPocket[bPos]];
				if( lbePocket == 0 && LoadBearingEquipment[Item[pSoldier->inv[icLBE[bPos]].usItem].ubClassIndex].lbePocketsAvailable & (UINT16)pow((double)2, icPocket[bPos])){
					lbePocket = GetPocketFromAttachment(&pSoldier->inv[icLBE[bPos]], icPocket[bPos]);
				}
				
				pRestrict = LBEPocketType[lbePocket].pRestriction;
				if(pRestrict != 0)
					if(!(pRestrict & Item[pObj->usItem].usItemClass))
						lbePocket = 0;
			}
			break;
		default:
			break;
	}

	if((UsingNewInventorySystem() == false))
	{
		if (ubSlotLimit == 0 && bPos >= SMALLPOCKSTART )
		{
			// doesn't fit!
			return( FALSE );
		}
	}
	else
	{
		// CHRISL: lbePocket==0 means pocket disabled.  ubSlotLimit==0 means pocket can't hold item
		if ( lbePocket == 0 || ubSlotLimit == 0 )
			return ( CompatibleAmmoForGun(pObj, &pSoldier->inv[bPos]) || ValidAttachment(pObj->usItem, &(pSoldier->inv[bPos]) ) || ValidLaunchable(pObj->usItem, pSoldier->inv[bPos].usItem) );

		// CHRISL: Adjust parameters to include the new inventory system
		if (ubSlotLimit == 0 && bPos >= BIGPOCKFINAL )
			return( CompatibleAmmoForGun(pObj, &pSoldier->inv[bPos]) || ValidAttachment(pObj->usItem, &(pSoldier->inv[bPos]) ) || ValidLaunchable(pObj->usItem, pSoldier->inv[bPos].usItem) );
	}

	return( TRUE );
}

//CHRISL: Wrote this function to try and clean up possible problems relating to the 16bit change for ammo capacity
void CleanUpItemStats( OBJECTTYPE * pObj )
{
	UINT16		magSize;

	if(Item[pObj->usItem].usItemClass == IC_GUN)
	{
		magSize = GetMagSize(pObj);
		if((*pObj)[0]->data.gun.ubGunShotsLeft > magSize)
		{
			(*pObj)[0]->data.gun.ubGunShotsLeft = magSize;
		}
		return;
	}
	if(Item[pObj->usItem].usItemClass == IC_AMMO)
	{
		magSize = Magazine[Item[pObj->usItem].ubClassIndex].ubMagSize;
		if((*pObj)[0]->data.ubShotsLeft > magSize)
		{
			(*pObj)[0]->data.ubShotsLeft = magSize;
		}
		return;
	}
}

BOOLEAN FreeUpSlotIfPossibleThenPlaceObject( SOLDIERTYPE * pSoldier, INT8 bPos, OBJECTTYPE * pObj )
{
	//this gets called if something doesn't fit in bPos, which can happen if something is there
	//or if it simply doesn't fit, if it doesn't fit return false to prevent recursion

	// try autoplacing item in bSlot elsewhere, excluding the slot it came from, then do a placement
	if ( pSoldier->inv[bPos].exists() == true && AutoPlaceObject( pSoldier, &(pSoldier->inv[bPos]), FALSE , bPos) )
	{
		//the old object has been placed somewhere, it's safe to place this one
		return( PlaceObject( pSoldier, bPos, pObj ) );
	}
	return( FALSE );
}

// CHRISL: Use to find best pocket to store item in.  Could probably be merged with FitsInSmallPocket
/*INT32 PickPocket(SOLDIERTYPE *pSoldier, UINT8 ppStart, UINT8 ppStop, UINT16 usItem, UINT8 iNumber, UINT8 * cap, int bExcludeSlot)
{
	UINT16	pIndex=0;
	INT32	pocket=0;
	UINT8	capacity=254;

	// sevenfm: r8436
	// Flugente: if we're going to fail miserably, at least tell us why
	AssertMsg( Item[usItem].ItemSize < gGameExternalOptions.guiMaxItemSize + 1, String( "Size of item %d (%d) exceeds max item size (%d)!", usItem, Item[usItem].ItemSize, gGameExternalOptions.guiMaxItemSize + 1 ) );

	for(UINT32 uiPos=ppStart; uiPos<ppStop; ++uiPos)
	{
		if(pSoldier->inv[icLBE[uiPos]].exists() == false)
		{
			pIndex=LoadBearingEquipment[Item[icDefault[uiPos]].ubClassIndex].lbePocketIndex[icPocket[uiPos]];
		}
		else 
		{
			// allow storing in MOLLE pockets (by Shadooow)
			if (pIndex == 0 && LoadBearingEquipment[Item[pSoldier->inv[icLBE[uiPos]].usItem].ubClassIndex].lbePocketsAvailable & (UINT16)pow((double)2, icPocket[uiPos]))
			{
				pIndex = GetPocketFromAttachment(&pSoldier->inv[icLBE[uiPos]], icPocket[uiPos]);
			}
		}

		// Here's were we get complicated.  We should look for the smallest pocket all items can fit in
		if(LBEPocketType[pIndex].ItemCapacityPerSize[Item[usItem].ItemSize] >= iNumber &&
			LBEPocketType[pIndex].ItemCapacityPerSize[Item[usItem].ItemSize] < capacity &&
			pSoldier->inv[uiPos].exists() == false && uiPos != bExcludeSlot) 
		{
				if((LBEPocketType[pIndex].pRestriction != 0 && (LBEPocketType[pIndex].pRestriction & Item[usItem].usItemClass)) ||
					LBEPocketType[pIndex].pRestriction == 0) 
				{
						capacity = LBEPocketType[pIndex].ItemCapacityPerSize[Item[usItem].ItemSize];
						pocket = uiPos;
				}
		}
	}

	if(pocket!=0)
	{
		*cap=capacity;
		return pocket;
	}
	else
	{
		*cap=254;
		return -1;
	}
}*/

// CHRISL: Use to find best pocket to store item in.  Could probably be merged with FitsInSmallPocket
INT32 PickPocket(SOLDIERTYPE *pSoldier, UINT8 ppStart, UINT8 ppStop, UINT16 usItem, UINT8 iNumber, UINT8 * cap, int bExcludeSlot)
{
	UINT16	pIndex = 0;
	INT32	pocket = 0;
	UINT8	capacity = 254;

	// sevenfm: r8436
	// Flugente: if we're going to fail miserably, at least tell us why
	AssertMsg(Item[usItem].ItemSize < gGameExternalOptions.guiMaxItemSize + 1, String("Size of item %d (%d) exceeds max item size (%d)!", usItem, Item[usItem].ItemSize, gGameExternalOptions.guiMaxItemSize + 1));

	for (UINT32 uiPos = ppStart; uiPos < ppStop; ++uiPos){
		if (pSoldier->inv[icLBE[uiPos]].exists() == false){
			pIndex = LoadBearingEquipment[Item[icDefault[uiPos]].ubClassIndex].lbePocketIndex[icPocket[uiPos]];
		}
		else {
			pIndex = LoadBearingEquipment[Item[pSoldier->inv[icLBE[uiPos]].usItem].ubClassIndex].lbePocketIndex[icPocket[uiPos]];
		}
		// Here's were we get complicated.  We should look for the smallest pocket all items can fit in
		if (LBEPocketType[pIndex].ItemCapacityPerSize[Item[usItem].ItemSize] >= iNumber &&
			LBEPocketType[pIndex].ItemCapacityPerSize[Item[usItem].ItemSize] < capacity &&
			pSoldier->inv[uiPos].exists() == false && uiPos != bExcludeSlot) {
			if ((LBEPocketType[pIndex].pRestriction != 0 && (LBEPocketType[pIndex].pRestriction & Item[usItem].usItemClass)) ||
				LBEPocketType[pIndex].pRestriction == 0) {
				capacity = LBEPocketType[pIndex].ItemCapacityPerSize[Item[usItem].ItemSize];
				pocket = uiPos;
			}
		}
	}
	if (pocket != 0){
		*cap = capacity;
		return pocket;
	}
	else{
		*cap = 254;
		return -1;
	}
}

BOOLEAN PlaceObject( SOLDIERTYPE * pSoldier, INT8 bPos, OBJECTTYPE * pObj, BOOLEAN fNewItem )
{
	if (PlaceObject(pSoldier, bPos, pObj) == TRUE) {
		SetNewItem(pSoldier, bPos, fNewItem);
		return TRUE;
	}
	return FALSE;
}

BOOLEAN PlaceObject( SOLDIERTYPE * pSoldier, INT8 bPos, OBJECTTYPE * pObj )
{
	// returns object to have in hand after placement... same as original in the
	// case of error

	UINT8					ubSlotLimit;
	OBJECTTYPE *	pInSlot;
	BOOLEAN				fObjectWasRobotRemote = FALSE;

	if ( Item[pObj->usItem].robotremotecontrol )
	{
		fObjectWasRobotRemote = TRUE;
	}

	//CHRISL: Failsafe to try and clean up ammo capacity problems
	CleanUpItemStats(pObj);

	if ( !CanItemFitInPosition( pSoldier, pObj, bPos, TRUE ) )
	{
		return( FALSE );
	}

	// If the position is either head slot, then the item must be IC_FACE (checked in
	// CanItemFitInPosition).
	if ( bPos == HEAD1POS )
	{
		if ( !CompatibleFaceItem( pObj->usItem, pSoldier->inv[ HEAD2POS ].usItem ) )
		{
			if (gpItemPointer)
			{
				CHAR16	zTemp[150];
				swprintf(zTemp, Message[STR_CANT_USE_TWO_ITEMS], ItemNames[pObj->usItem], ItemNames[pSoldier->inv[HEAD2POS].usItem]);
				ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, zTemp);
			}
			return( FALSE );
		}
	}
	else if ( bPos == HEAD2POS )
	{
		if ( !CompatibleFaceItem( pObj->usItem, pSoldier->inv[ HEAD1POS ].usItem ) )
		{
			if (gpItemPointer)
			{
				CHAR16	zTemp[150];

				swprintf(zTemp, Message[STR_CANT_USE_TWO_ITEMS], ItemNames[pObj->usItem], ItemNames[pSoldier->inv[HEAD1POS].usItem]);
				ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, zTemp);
			}			
			return( FALSE );
		}
	}

	if ( Item[ pObj->usItem ].usItemClass == IC_KEY && pSoldier->flags.uiStatusFlags & SOLDIER_PC )
	{
		if ( KeyTable[ (*pObj)[0]->data.key.ubKeyID ].usDateFound == 0 )
		{
			KeyTable[ (*pObj)[0]->data.key.ubKeyID ].usDateFound = (UINT16) GetWorldDay();
			KeyTable[ (*pObj)[0]->data.key.ubKeyID ].usSectorFound = SECTOR( pSoldier->sSectorX, pSoldier->sSectorY );
		}
	}

    // Lesh: bugfix - replacing weapon in auto with another weapon w/o auto-mode
    if (bPos == HANDPOS && Item[ pObj->usItem ].usItemClass == IC_GUN)
    {
		//Madd: added code for no semiauto tag
		if (!Weapon[ Item[pObj->usItem].ubClassIndex ].NoSemiAuto)
		{
			pSoldier->bWeaponMode = WM_NORMAL;
			pSoldier->bDoBurst = FALSE;
			pSoldier->bDoAutofire = 0;
		}
		else
		{
			pSoldier->bWeaponMode = WM_AUTOFIRE;
			pSoldier->bDoBurst = TRUE;
			pSoldier->bDoAutofire = 1;
		}

		pSoldier->FixScopeMode();
    }
    // Lesh: end

#ifdef JA2UB
	//handle the placing up of a new ja25 gun
	HandleNewGunComment( pSoldier, pObj->usItem, FALSE );
#endif	
	pInSlot = &(pSoldier->inv[bPos]);

	//we are placing an object, how we handle this depends on what is in the slot already
	if (pInSlot->exists() == false)
	{
		//if the object in the slot does not exist it is easy
		pObj->MoveThisObjectTo(*pInSlot, ALL_OBJECTS, pSoldier, bPos);
		if (pObj->exists() == false)
		{
			// dropped everything
			if (bPos == HANDPOS && Item[pInSlot->usItem].twohanded )
			{
				// We just performed a successful drop of a two-handed object into the
				// main hand
				if (pSoldier->inv[SECONDHANDPOS].exists() == true)
				{
					// swap what WAS in the second hand into the cursor
					pSoldier->inv[SECONDHANDPOS].MoveThisObjectTo(*pObj);
				}
			}
		 }
	}
	else
	{
		// replacement/reloading/merging/stacking
		//try to reload first
		switch (Item[pInSlot->usItem].usItemClass)
		{
			case IC_GUN:
				if (Item[pObj->usItem].usItemClass == IC_AMMO)
				{
					// Flugente: if we have an underbarrel weapon, we can reload that
					OBJECTTYPE* pObjUsed = pSoldier->GetUsedWeapon(pInSlot);

					if (Weapon[pObjUsed->usItem].ubCalibre == Magazine[Item[pObj->usItem].ubClassIndex].ubCalibre)
					{
						//CHRISL: Work differently with ammo crates but only when not in combat
						if(Magazine[Item[pObj->usItem].ubClassIndex].ubMagType >= AMMO_BOX)
						{
							if(!(gTacticalStatus.uiFlags & INCOMBAT))
							{
								UINT16		magSize, ubShotsLeft;
								OBJECTTYPE	tempClip;
								OBJECTTYPE	tempStack;
								bool		clipCreated;
								UINT32		newItem = 0;
								INT32		pocket=-1;
								UINT8		capacity=0;
								UINT8		bLoop;
								//find the ammo item we want to try and create
								for (UINT16 loop = 0; loop < gMAXITEMS_READ; loop++)
								{
									if(Item[loop].usItemClass == IC_AMMO)
									{
										if(Magazine[Item[loop].ubClassIndex].ubCalibre == Weapon[pObjUsed->usItem].ubCalibre && Magazine[Item[loop].ubClassIndex].ubAmmoType == Magazine[Item[pObj->usItem].ubClassIndex].ubAmmoType && Magazine[Item[loop].ubClassIndex].ubMagSize == GetMagSize(pObjUsed) && Magazine[Item[loop].ubClassIndex].ubMagType < AMMO_BOX )
											newItem = loop;
									}
								}
								//Create a stack of up to 5 "newItem" clips 
								tempStack.initialize();
								clipCreated = false;
								ubShotsLeft = (*pObj)[0]->data.ubShotsLeft;
								for(UINT8 clip = 0; clip < 5; clip++)
								{
									magSize = GetMagSize(pObjUsed);
									if(ubShotsLeft < magSize)
										magSize = ubShotsLeft;
									if(CreateAmmo(newItem, &tempClip, magSize))
									{
										tempStack.AddObjectsToStack(tempClip, -1, pSoldier, NUM_INV_SLOTS, MAX_OBJECTS_PER_SLOT);
										ubShotsLeft -= magSize;
										clipCreated = true;
										if(ubShotsLeft < 1)
											break;
									}
								}
								//Try to place the stack somewhere on the active merc
								if(clipCreated == true)
								{
									clipCreated = false;
									bLoop = tempStack.ubNumberOfObjects;
									while(tempStack.ubNumberOfObjects > 0)
									{
										pocket = PickPocket(pSoldier, BIGPOCKSTART, SMALLPOCKFINAL, tempStack.usItem, bLoop, &capacity, -1);
										if(pocket != -1)
										{
											pSoldier->inv[pocket].AddObjectsToStack(tempStack, bLoop, pSoldier, pocket);
										}
										else
										{
											bLoop--;
										}
										if(bLoop < 1)
											break;
									}
									if(tempStack.ubNumberOfObjects < 1)
										clipCreated = true;
									else
									{
										//Try to place stack on ground
										if( AutoPlaceObjectToWorld(pSoldier, &tempStack) )
										{
											clipCreated = true;
											if(guiCurrentScreen == GAME_SCREEN)
												NotifySoldiersToLookforItems( );
										}
										/*if(guiCurrentScreen == MAP_SCREEN && fShowMapInventoryPool == TRUE)
										{
											if(AutoPlaceObjectInInventoryStash(&tempStack, pSoldier->sGridNo))
												clipCreated = true;
										}
										else
										{
											if(AddItemToPool(pSoldier->sGridNo, &tempStack, 1, pSoldier->pathing.bLevel, WORLD_ITEM_REACHABLE, -1))
											{
												NotifySoldiersToLookforItems( );
												clipCreated = true;
											}
										}*/
									}
								}
								if(clipCreated == true)
								{
									(*pObj)[0]->data.ubShotsLeft = ubShotsLeft;
								}
								if((*pObj)[0]->data.ubShotsLeft < 1)
									pObj->RemoveObjectsFromStack(1);
								return( TRUE );
							}
							else
							{
								ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMapInventoryErrorString[6] );
								return( FALSE );
							}
						}
						else
							return( ReloadGun( pSoldier, pObjUsed, pObj ) );
					}
				}
				break;
			case IC_LAUNCHER:
				if ( ValidLaunchable( pObj->usItem, pInSlot->usItem ) ) {
					return( ReloadGun( pSoldier, pInSlot, pObj ) );
				}
			break;
			}
		//if we didn't reload, then we know we are stacking or swapping!

		if (IsSlotASmallPocket(bPos) == true && FitsInSmallPocket(pObj) == false) {
			//there is nothing we can do, just return
			return FALSE;
		}

		// CHRISL: When holding ammo and clicking on an appropriate ammo crate, add ammo to crate
		if(Magazine[Item[pInSlot->usItem].ubClassIndex].ubMagType >= AMMO_BOX && Item[pObj->usItem].usItemClass == IC_AMMO && Magazine[Item[pObj->usItem].ubClassIndex].ubMagType < AMMO_BOX )
		{
			if(Magazine[Item[pInSlot->usItem].ubClassIndex].ubCalibre == Magazine[Item[pObj->usItem].ubClassIndex].ubCalibre &&
				Magazine[Item[pInSlot->usItem].ubClassIndex].ubAmmoType == Magazine[Item[pObj->usItem].ubClassIndex].ubAmmoType)
			{
				UINT16	magSpace = Magazine[Item[pInSlot->usItem].ubClassIndex].ubMagSize-(*pInSlot)[0]->data.ubShotsLeft;
				while(pObj->ubNumberOfObjects > 0)
				{
					if(magSpace >= (*pObj)[0]->data.ubShotsLeft)
					{
						magSpace -= (*pObj)[0]->data.ubShotsLeft;
						(*pInSlot)[0]->data.ubShotsLeft += (*pObj)[0]->data.ubShotsLeft;
						pObj->RemoveObjectsFromStack(1);
					}
					else
					{
						(*pObj)[0]->data.ubShotsLeft -= magSpace;
						(*pInSlot)[0]->data.ubShotsLeft += magSpace;
						break;
					}
				}
				if(pObj->ubNumberOfObjects > 0)
					return( FALSE );
				else
					return( TRUE );
			}
		}

		// CHRISL:
		ubSlotLimit = ItemSlotLimit( pObj, bPos, pSoldier );
		if (ubSlotLimit == 0)
		{
			//we have tried to stack but the stack is full, or we have tried to swap but the slot is wrong
			return( FreeUpSlotIfPossibleThenPlaceObject( pSoldier, bPos, pObj ) );
			}


		if ( pObj->usItem == pInSlot->usItem && ubSlotLimit > 1 && IsSlotAnLBESlot(bPos) == false )
			{
			//we have tried to stack, but remember we can't stack 2 LBEs into 1 LBE slot, they get swapped instead
			pInSlot->AddObjectsToStack( *pObj, ALL_OBJECTS, pSoldier, bPos );
		}

		else if ( (Item[pObj->usItem].twohanded ) && (bPos == HANDPOS) )
		{
			if (pSoldier->inv[SECONDHANDPOS].exists() == true) {
				// both pockets have something in them, so we can't swap
				return( FALSE );
			}
			else {
				//we swapped a 2 handed object into the main hand
				SwapObjs( pObj, pInSlot );
			}
		}

		else if (IsSlotAnLBESlot(bPos) == true && Item[pObj->usItem].usItemClass == IC_LBEGEAR)
		{
			/* Buggler: Code commented due to weird results when swapping filled LBE items that has pockets of the same size
			//CHRISL: If we're trying to swap LBE items between IC pockets, we have to be careful that items are moved
			into active pockets or that an LBENODE is properly created.
			if(pObj->HasAnyActiveLBEs(pSoldier) == false && !(_KeyDown(SHIFT))){
				std::vector<INT8> LBESlots;
				GetLBESlots(bPos, LBESlots);
				MoveItemsToActivePockets(pSoldier, LBESlots, bPos, pObj);
			}*/
			pInSlot->MoveThisObjectTo(gTempObject, -1, pSoldier, bPos);
			pObj->MoveThisObjectTo(*pInSlot, -1, pSoldier, bPos);
			//CHRISL: We need to make sure there are no items left in pObj or we'll lose them
			if(pObj->ubNumberOfObjects == 0)
				gTempObject.MoveThisObjectTo(*pObj, -1);
			else
				AutoPlaceObjectAnywhere(pSoldier, &gTempObject, FALSE);
		}

		else if (ubSlotLimit < pObj->ubNumberOfObjects)
		{
			//not enough room, so we free up some space
			return( FreeUpSlotIfPossibleThenPlaceObject( pSoldier, bPos, pObj ) );
		}

		else
		{
			//item fits here, swapping
			SwapObjs( pObj, pInSlot );
		}
	}

	// ATE: Put this in to see if we should update the robot, if we were given a controller...
	if ( pSoldier->bTeam == gbPlayerNum && fObjectWasRobotRemote )
	{
		pSoldier->UpdateRobotControllerGivenController( );
	}

	ApplyEquipmentBonuses(pSoldier);
	//Pulmu bugfix
	//ADB ubWeight has been removed, see comments in OBJECTTYPE
	//pInSlot->ubWeight = CalculateObjectWeight(pInSlot);
	//Pulmu end
	return( TRUE );
}

bool TryToStackInSlot(SOLDIERTYPE* pSoldier, OBJECTTYPE* pObj, int bSlot)
{
	// CHRISL: Use new ItemSlotLimit function if we're using the new inventory system
	if (pSoldier->inv[bSlot].usItem == pObj->usItem && pSoldier->inv[bSlot].exists() == true)
	{
		if (pSoldier->inv[bSlot].ubNumberOfObjects < ItemSlotLimit( pObj, bSlot, pSoldier ) )
		{
			// NEW: If in SKI, don't auto-PLACE anything into a stackable slot that's currently hatched out!  Such slots
			// will disappear in their entirety if sold/moved, causing anything added through here to vanish also!
			if( !( ( guiTacticalInterfaceFlags & INTERFACE_SHOPKEEP_INTERFACE ) && ShouldSoldierDisplayHatchOnItem( pSoldier->ubProfile, bSlot ) ) )
			{
				pSoldier->inv[bSlot].AddObjectsToStack(*pObj, -1, pSoldier, bSlot);
				if (pObj->exists() == false) {
					return true;
				}
			}
		}
	}
	return false;
}

bool TryToPlaceInSlot(SOLDIERTYPE* pSoldier, OBJECTTYPE* pObj, BOOLEAN fNewItem, int& bSlot, int endSlot)
{
	//bSlot = FindEmptySlotWithin( pSoldier, bSlot, endSlot );
	//CHRISL: If something already exists, we want to fail since we can't place an object in this slot
	if (CanItemFitInPosition(pSoldier, pObj, bSlot, false) == false || pSoldier->inv[bSlot].exists() == true) {
		//bSlot = endSlot;
		return false;
	}

	if (bSlot == ITEM_NOT_FOUND) {
		bSlot = endSlot;
		return false;
	}

	if (bSlot == SECONDHANDPOS) {
		if (pSoldier->inv[HANDPOS].exists() == true && Item[pSoldier->inv[ HANDPOS ].usItem].twohanded) {
			return false;
		}
	}

	PlaceObject( pSoldier, bSlot, pObj, fNewItem );
	if (pObj->exists() == false) {
		return( true );
	}
	return false;
}

bool PlaceInAnySlot(SOLDIERTYPE* pSoldier, OBJECTTYPE* pObj, BOOLEAN fNewItem, int bExcludeSlot, BOOLEAN fStackOrSingleSlot)
{
	UINT8 ubInitialNumberOfObjects = pObj->ubNumberOfObjects;

	//now try to PLACE
	//try to PLACE in any body slot
	for(int bSlot = BODYPOSSTART; bSlot < BIGPOCKSTART; bSlot++) {
		if (bSlot != bExcludeSlot && TryToPlaceInSlot(pSoldier, pObj, fNewItem, bSlot, BODYPOSFINAL) == true)
			return true;
		
		if(pObj->ubNumberOfObjects < ubInitialNumberOfObjects && fStackOrSingleSlot)
			return true;
	}

	if(UsingNewInventorySystem() == true)
	{
		//CHRISL: Rather then a simple slot search, use PickPocket to find the most appropriate pocket to use
		INT32	sPocket=-1, mPocket=-1, lPocket=-1;
		UINT8	sCapacity=0;
		UINT8	mCapacity=0;
		UINT8	lCapacity=0;
		UINT8	capacity=0;
		int		bSlot;
		
		for( UINT8 i = 0; i < ubInitialNumberOfObjects; i++)
		{
			// pocket may not accomodate entire stack
			for( UINT8 ubCurrentObjects = pObj->ubNumberOfObjects; ubCurrentObjects > 0; ubCurrentObjects--)
			{
				//Start with active small pockets
				sPocket=PickPocket(pSoldier, SMALLPOCKSTART, SMALLPOCKFINAL, pObj->usItem, ubCurrentObjects, &sCapacity, bExcludeSlot);
				//Next search active medium pockets
				mPocket=PickPocket(pSoldier, MEDPOCKSTART, MEDPOCKFINAL, pObj->usItem, ubCurrentObjects, &mCapacity, bExcludeSlot);
				//Lastly search active big pockets
				lPocket=PickPocket(pSoldier, BIGPOCKSTART, BIGPOCKFINAL, pObj->usItem, ubCurrentObjects, &lCapacity, bExcludeSlot);
				//Finally, compare the three pockets we've found and return the pocket that is most logical to use
				capacity = min(sCapacity, mCapacity);
				capacity = min(lCapacity, capacity);
				
				// pocket can fit slot
				if(capacity != 254)
					break;
			}

			if(capacity == 254)	//no pocket found
				return false;
			else if(capacity == sCapacity)
				bSlot = sPocket;
			else if(capacity == mCapacity)
				bSlot = mPocket;
			else if(capacity == lCapacity)
				bSlot = lPocket;
			
			if(TryToPlaceInSlot(pSoldier, pObj, fNewItem, bSlot, NUM_INV_SLOTS) == true || fStackOrSingleSlot)
				return true;
		}
	}
	else
	{
		if (FitsInSmallPocket(pObj) == true) {
			//try to PLACE in small pockets
			for(int bSlot = SMALLPOCKSTART; bSlot < SMALLPOCKFINAL; bSlot++) {
				if (bSlot != bExcludeSlot && TryToPlaceInSlot(pSoldier, pObj, fNewItem, bSlot, NUM_INV_SLOTS) == true)
					return true;

				if(pObj->ubNumberOfObjects < ubInitialNumberOfObjects && fStackOrSingleSlot)
					return true;
			}
		}

		//try to PLACE in big pockets, and possibly medium pockets
		int bigPocketEnd = (UsingNewInventorySystem() == true) ? MEDPOCKFINAL : BIGPOCKFINAL;
		for(int bSlot = BIGPOCKSTART; bSlot < bigPocketEnd; bSlot++) {
			if (bSlot != bExcludeSlot && TryToPlaceInSlot(pSoldier, pObj, fNewItem, bSlot, bigPocketEnd) == true)
				return true;

			if(pObj->ubNumberOfObjects < ubInitialNumberOfObjects && fStackOrSingleSlot)
				return true;
		}
	}

	return false;
}

bool PlaceInAnyPocket(SOLDIERTYPE* pSoldier, OBJECTTYPE* pObj, BOOLEAN fNewItem, int bExcludeSlot)
{
	//first, try to STACK the item
	if (FitsInSmallPocket(pObj) == true) {
		//try to STACK in small pockets
		for(int bSlot = SMALLPOCKSTART; bSlot < SMALLPOCKFINAL; bSlot++) {
			if (bSlot != bExcludeSlot && TryToStackInSlot(pSoldier, pObj, bSlot) == true) {
				return true;
			}
		}
	}

	//try to STACK in big pockets, and possibly medium pockets
	int bigPocketEnd = (UsingNewInventorySystem() == true) ? MEDPOCKFINAL : BIGPOCKFINAL;
	for(int bSlot = BIGPOCKSTART; bSlot < bigPocketEnd; bSlot++) {
		if (bSlot != bExcludeSlot && TryToStackInSlot(pSoldier, pObj, bSlot) == true) {
			return true;
		}
	}


	if (FitsInSmallPocket(pObj) == true) {
		//try to PLACE in small pockets
		for(int bSlot = SMALLPOCKSTART; bSlot < SMALLPOCKFINAL; bSlot++) {
			if (bSlot != bExcludeSlot && TryToPlaceInSlot(pSoldier, pObj, fNewItem, bSlot, NUM_INV_SLOTS) == true) {
				return true;
			}
		}
	}
	//try to PLACE in big pockets, and possibly medium pockets
	for(int bSlot = BIGPOCKSTART; bSlot < bigPocketEnd; bSlot++) {
		if (bSlot != bExcludeSlot && TryToPlaceInSlot(pSoldier, pObj, fNewItem, bSlot, bigPocketEnd) == true) {
			return true;
		}
	}
	return false;
}

bool PlaceInAnyBigOrMediumPocket(SOLDIERTYPE* pSoldier, OBJECTTYPE* pObj, BOOLEAN fNewItem, int bExcludeSlot)
{
	//a special note, although some items do not fit in small pockets, and under the old system are restricted to big pockets,
	//under the new system they are intended to fit in medium pockets, if the item size and the pocket agree
	//An example would be a SMG fitting in a gun holster, which is medium.
	int bigPocketEnd = (UsingNewInventorySystem() == true) ? MEDPOCKFINAL : BIGPOCKFINAL;
	//first, try to STACK the item
	for(int bSlot = BIGPOCKSTART; bSlot < bigPocketEnd; bSlot++) {
		if (bSlot != bExcludeSlot && TryToStackInSlot(pSoldier, pObj, bSlot) == true) {
			return true;
		}
	}

	//now try to PLACE
	for(int bSlot = BIGPOCKSTART; bSlot < bigPocketEnd; bSlot++) {
		if (bSlot != bExcludeSlot && TryToPlaceInSlot(pSoldier, pObj, fNewItem, bSlot, bigPocketEnd) == true) {
			return true;
		}
	}
	return false;
}

bool PlaceInAnySmallPocket(SOLDIERTYPE* pSoldier, OBJECTTYPE* pObj, BOOLEAN fNewItem, int bExcludeSlot)
{
	if (FitsInSmallPocket(pObj) == false) {
		return false;
	}

	//first, try to STACK the item
	for(int bSlot = SMALLPOCKSTART; bSlot < SMALLPOCKFINAL; bSlot++) {
		if (bSlot != bExcludeSlot && TryToStackInSlot(pSoldier, pObj, bSlot) == true) {
			return true;
		}
	}

	//try to PLACE in small pockets
	for(int bSlot = SMALLPOCKSTART; bSlot < SMALLPOCKFINAL; bSlot++) {
		if (bSlot != bExcludeSlot && TryToPlaceInSlot(pSoldier, pObj, fNewItem, bSlot, NUM_INV_SLOTS) == true) {
			return true;
		}
	}
	return false;
}

BOOLEAN AutoPlaceObjectAnywhere(SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj, BOOLEAN fNewItem, INT8 bExcludeSlot)
{
	// This function will attempt to place an object into the soldier's inventory first.  If that doesn't work, it'll add the object to sector inventory
	if(pObj->exists() == false)
		return FALSE;

	if( AutoPlaceObject(pSoldier, pObj, fNewItem, bExcludeSlot) )
		return TRUE;
	else
		return (AutoPlaceObjectToWorld(pSoldier, pObj, TRUE) );

	return FALSE;
}

extern BOOLEAN IsMercInActiveSector(SOLDIERTYPE * pSoldier);
extern void CreateDestroyMapInventoryPoolButtons( BOOLEAN fExitFromMapScreen );
BOOLEAN AutoPlaceObjectToWorld(SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj, INT8 bVisible)
{
	if(pObj->exists() == false)
		return FALSE;

	// r8690
	// silversurfer: Bad idea. Our pSoldier always has sGridNo set but it could be from a previous sector so the tile is completely irrelevant if the sector isn't the current merc sector.
	// The same applies to bLevel. So before we assign anything we need to check if the merc sector is loaded.
	// INT32 sGridNo = pSoldier?pSoldier->sGridNo:0;
	// INT8 bLevel = pSoldier?pSoldier->pathing.bLevel:0;

	INT32 sGridNo = -1;
	INT8 bLevel = 0;

	// is this sector loaded?
	if (pSoldier && (pSoldier->sSectorX == gWorldSectorX) && (pSoldier->sSectorY == gWorldSectorY) && (pSoldier->bSectorZ == gbWorldSectorZ))
	{
		sGridNo = pSoldier->sGridNo;
		bLevel = pSoldier->pathing.bLevel;
	}

	if( guiCurrentScreen == MAP_SCREEN )
	{
		// the_bob : added the check for whether pSoldier actually points to something to handle calling this function with pSoldier = NULL
		if (pSoldier)
		{
			if(fShowMapInventoryPool && !IsMercInActiveSector(pSoldier) )
			{
				fShowMapInventoryPool = FALSE;
				CreateDestroyMapInventoryPoolButtons(FALSE);
			}

			 ChangeSelectedMapSector(pSoldier->sSectorX, pSoldier->sSectorY, pSoldier->bSectorZ);
		}

		// WANNE: This should fix the bug, that items get lost in the sector when switching between tactical sectors
		// This bug was introduced in revision 4571 (2011-07-14), fixed in 5228
		// Buggler:  Previously item get lost when holding CTRL & clicking on item in merc inventory when map screen shows other sectors
		// Fixed above issue on commit in 5781 (2013-01-13). Please remove all comments if no reported issue after adequate public testing
		
		if(!fShowMapInventoryPool)
		//if(fShowMapInventoryPool) // Buggler: to remove too
		{
			fShowMapInventoryPool = TRUE;
			CreateDestroyMapInventoryPoolButtons(FALSE);
		}
		
		// set a grid no for item from mercs with invalid grid no in sector inventory, e.g. merc arriving in sector with a different tactical map loaded
		if(!GridNoOnVisibleWorldTile(sGridNo))
		{
			// use the grid no of the first visible, reachable item
			for(UINT32 i = 0; i < pInventoryPoolList.size(); i++ )
			{
				if( pInventoryPoolList[i].bVisible == 1 && pInventoryPoolList[i].fExists == TRUE && pInventoryPoolList[i].usFlags & WORLD_ITEM_REACHABLE )
				{
					sGridNo = pInventoryPoolList[i].sGridNo;
					bLevel = pInventoryPoolList[i].ubLevel;
					break;
				}
			}

			// empty sector to use the center grid no of the loaded tactical sector, hope that it's accessible
			if(!GridNoOnVisibleWorldTile(sGridNo))
				sGridNo = gMapInformation.sCenterGridNo;
		}
		
		fMapPanelDirty = TRUE;
		return( AutoPlaceObjectInInventoryStash(pObj, sGridNo, bLevel) );
	}
	else
	{
		AddItemToPool(sGridNo, pObj, bVisible, bLevel, WORLD_ITEM_REACHABLE, 0);
		return TRUE;
	}

	return FALSE;
}

// CHRISL: Function needed for LBENODE
BOOLEAN AutoPlaceObject( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj, BOOLEAN fNewItem, INT8 bExcludeSlot, BOOLEAN fStackOrSingleSlot )
{
	INVTYPE	* pItem;
	UINT32			packCombo, backCombo;

	// statuses of extra objects would be 0 if the # exceeds the maximum
	//Assert( pObj->ubNumberOfObjects <= MAX_OBJECTS_PER_SLOT);

	//Pulmu bugfix		
	//ADB ubWeight has been removed, see comments in OBJECTTYPE
	//pObj->ubWeight = CalculateObjectWeight( pObj);
	
	UINT8 ubInitialNumberOfObjects = pObj->ubNumberOfObjects;
	
	//first, try to STACK the item
	//try to STACK in any slot
	for(int bSlot = BODYPOSSTART; bSlot < BIGPOCKSTART; bSlot++) {
		if (bSlot != bExcludeSlot && TryToStackInSlot(pSoldier, pObj, bSlot) == true) {
			return true;
		}
	}

	if (FitsInSmallPocket(pObj) == true) {
		//try to STACK in small pockets
		for(int bSlot = SMALLPOCKSTART; bSlot < SMALLPOCKFINAL; bSlot++) {
			if (bSlot != bExcludeSlot && TryToStackInSlot(pSoldier, pObj, bSlot) == true) {
				return true;
			}
		}
	}

	//try to STACK in big pockets, and possibly medium pockets
	int bigPocketEnd = (UsingNewInventorySystem() == true) ? MEDPOCKFINAL : BIGPOCKFINAL;
	for(int bSlot = BIGPOCKSTART; bSlot < bigPocketEnd; bSlot++) {
		if (bSlot != bExcludeSlot && TryToStackInSlot(pSoldier, pObj, bSlot) == true) {
			return true;
		}
	}

	if(pObj->ubNumberOfObjects < ubInitialNumberOfObjects && fStackOrSingleSlot)
		return true;
	
	// then overrides to the standard system: put guns in hand, armour on body (if slot empty)
	pItem = &(Item[pObj->usItem]);
	int lbeClass;

	switch (pItem->usItemClass)
	{
		case IC_GUN:
		case IC_THROWING_KNIFE:
		case IC_BLADE:
		case IC_LAUNCHER:
		case IC_BOMB:
		case IC_GRENADE:
			if (pSoldier->inv[HANDPOS].exists() == false)
			{
				// put the one-handed weapon in the guy's hand...
				if( PlaceObject( pSoldier, HANDPOS, pObj, fNewItem ) )
				{
					if ( pObj->exists() == false || fStackOrSingleSlot )
						return( TRUE );
				}
			}
			else if ( !(Item[pSoldier->inv[HANDPOS].usItem].twohanded ) && pSoldier->inv[SECONDHANDPOS].exists() == false)
			{
				// put the one-handed weapon in the guy's 2nd hand...
				if( PlaceObject( pSoldier, SECONDHANDPOS, pObj, fNewItem ) )
				{
					if ( pObj->exists() == false || fStackOrSingleSlot )
						return( TRUE );
				}
			}
			// two-handed objects are best handled in the main loop for large objects,
			// which checks the hands first anyhow
			//CHRISL: New switch.  Place items into Gunsling or Knife pocket is using NewInv, item is appropriate, and we
			//	didn't manage to place in hands.
			if(UsingNewInventorySystem() == true)
			{
				switch (pItem->usItemClass)
				{
					case IC_GUN:
						if(pSoldier->inv[GUNSLINGPOCKPOS].exists() == false)	// Long Gun use Gun Sling
						{
							if( PlaceObject( pSoldier, GUNSLINGPOCKPOS, pObj, fNewItem ) )
							{
								if (pObj->exists() == false || fStackOrSingleSlot)
									return( TRUE );
							}
						}
						break;
					case IC_THROWING_KNIFE:
					case IC_BLADE:
						if(pSoldier->inv[KNIFEPOCKPOS].exists() == false)	// Knife
						{
							if( PlaceObject( pSoldier, KNIFEPOCKPOS, pObj, fNewItem ) )
							{
								if (pObj->exists() == false || fStackOrSingleSlot)
									return( TRUE );
							}
						}
						break;
				}
			}
			break;
		case IC_ARMOUR:
			switch (Armour[Item[pObj->usItem].ubClassIndex].ubArmourClass)
			{
				case ARMOURCLASS_VEST:
					if (pSoldier->inv[VESTPOS].exists() == false)
					{
						// put on the armour!
						if( PlaceObject( pSoldier, VESTPOS, pObj, fNewItem ) )
						{
							if ( pObj->exists() == false || fStackOrSingleSlot )
								return( TRUE );
						}
					}
					break;
				case ARMOURCLASS_LEGGINGS:
					/* CHRISL:  If we're wearing leg protectors and pick up leggings, we want to leggings to override.
					This is only an issue during merc hiring when leggings will often be placed after leg protectors.
					However, this isn't as big an issue at this point because of the redesigns in the profile item sorting
					functions.*/
					if(Item[pSoldier->inv[LEGPOS].usItem].attachment)
					{
						SwapObjs(pSoldier, LEGPOS, pObj, TRUE);
						pSoldier->inv[LEGPOS].AttachObject(pSoldier, pObj, FALSE);
					}
					if (pSoldier->inv[LEGPOS].exists() == false)
					{
						// put on the armour!
						if( PlaceObject( pSoldier, LEGPOS, pObj, fNewItem ) )
						{
							if ( pObj->exists() == false || fStackOrSingleSlot )
								return( TRUE );
						}
					}
					break;
				case ARMOURCLASS_HELMET:
					if (pSoldier->inv[HELMETPOS].exists() == false)
					{
						// put on the armour!
						if( PlaceObject( pSoldier, HELMETPOS, pObj, fNewItem ) )
						{
							if ( pObj->exists() == false || fStackOrSingleSlot )
								return( TRUE );
						}
					}
					break;
				default:
					break;
			}
			// otherwise stuff it in a slot somewhere
			break;
		case IC_FACE:
			if ( (pSoldier->inv[HEAD1POS].exists() == false) && CompatibleFaceItem( pObj->usItem, pSoldier->inv[HEAD2POS].usItem ) )
			{
				if( PlaceObject( pSoldier, HEAD1POS, pObj, fNewItem ) )
				{
					if ( pObj->exists() == false || fStackOrSingleSlot )
						return( TRUE );
				}
			}
			else if ( (pSoldier->inv[HEAD2POS].exists() == false) && CompatibleFaceItem( pObj->usItem, pSoldier->inv[HEAD1POS].usItem ) )
			{
				if( PlaceObject( pSoldier, HEAD2POS, pObj, fNewItem ) )
				{
					if ( pObj->exists() == false || fStackOrSingleSlot )
						return( TRUE );
				}
			}
			break;
		// CHRISL:
		case IC_LBEGEAR:
			if(UsingNewInventorySystem() == false) {
				break;
			}
			lbeClass = LoadBearingEquipment[pItem->ubClassIndex].lbeClass;
			if(lbeClass == THIGH_PACK)	// Thigh pack
			{
				if (pSoldier->inv[LTHIGHPOCKPOS].exists() == false)
				{
					if( PlaceObject( pSoldier, LTHIGHPOCKPOS, pObj, fNewItem ) )
					{
						if(pObj->exists() == false || fStackOrSingleSlot)
							return( TRUE );
					}
				}
				if (pSoldier->inv[RTHIGHPOCKPOS].exists() == false)
				{
					if( PlaceObject( pSoldier, RTHIGHPOCKPOS, pObj, fNewItem ) )
					{
						if(pObj->exists() == false || fStackOrSingleSlot)
							return( TRUE );
					}
				}
			}
			else if(pSoldier->inv[VESTPOCKPOS].exists() == false && lbeClass == VEST_PACK)	// Vest pack
			{
				if( PlaceObject( pSoldier, VESTPOCKPOS, pObj, fNewItem ) )
				{
					if(pObj->exists() == false || fStackOrSingleSlot)
						return( TRUE );
				}
			}
			else if(pSoldier->inv[CPACKPOCKPOS].exists() == false && lbeClass == COMBAT_PACK)	// Combat pack
			{
				packCombo = LoadBearingEquipment[pItem->ubClassIndex].lbeCombo;
				backCombo = LoadBearingEquipment[Item[pSoldier->inv[BPACKPOCKPOS].usItem].ubClassIndex].lbeCombo;
				//DBrot: changed to bitwise comparison
				if((pSoldier->inv[BPACKPOCKPOS].exists() == true && packCombo != 0 && (backCombo & packCombo)) || pSoldier->inv[BPACKPOCKPOS].exists() == false)
				{
					if( PlaceObject( pSoldier, CPACKPOCKPOS, pObj, fNewItem ) )
					{
						if(pObj->exists() == false || fStackOrSingleSlot)
							return( TRUE );
					}
				}
			}
			else if(pSoldier->inv[BPACKPOCKPOS].exists() == false && lbeClass == BACKPACK)	// Backpack
			{
				//CHRISL: We're no longer restricting backpacks and gunslings from being used together
				//if(pSoldier->inv[GUNSLINGPOCKPOS].exists() == false)
				//{
					packCombo = LoadBearingEquipment[Item[pSoldier->inv[CPACKPOCKPOS].usItem].ubClassIndex].lbeCombo;
					backCombo = LoadBearingEquipment[pItem->ubClassIndex].lbeCombo;
					//DBrot: changed to bitwise comparison
					if((pSoldier->inv[CPACKPOCKPOS].exists() == true && backCombo != 0 && (backCombo & packCombo)) || pSoldier->inv[CPACKPOCKPOS].exists() == false)
					{
						if( PlaceObject( pSoldier, BPACKPOCKPOS, pObj, fNewItem ) )
						{
							pSoldier->flags.DropPackFlag = FALSE;
							pSoldier->flags.ZipperFlag = FALSE;
							RenderBackpackButtons(ACTIVATE_BUTTON);	/* CHRISL: Needed for new inventory backpack buttons */
							if(pObj->exists() == false || fStackOrSingleSlot)
								return( TRUE );
						}
					}
				//}
			}
			break;
		default:
			break;
	}

	if (PlaceInAnySlot(pSoldier, pObj, fNewItem, bExcludeSlot, fStackOrSingleSlot))
		return TRUE;

	return( FALSE );
}

BOOLEAN RemoveKeyFromSlot( SOLDIERTYPE * pSoldier, INT8 bKeyRingPosition, OBJECTTYPE * pObj )
{
	UINT8 ubItem = 0;

	CHECKF( pObj );

	if( ( pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber == 0 ) || ( pSoldier->pKeyRing[ bKeyRingPosition ].ubKeyID == INVALID_KEY_NUMBER ) )
	{
		return( FALSE );
	}
	else
	{
		// create an object
		ubItem = pSoldier->pKeyRing[ bKeyRingPosition ].ubKeyID;

		if( pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber > 1 )
		{
			pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber--;
		}
		else
		{

			pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber = 0;
			pSoldier->pKeyRing[ bKeyRingPosition ].ubKeyID = INVALID_KEY_NUMBER;
		}

		return( CreateKeyObject( pObj, 1, ubItem ) );
	}

	return( FALSE );
}


BOOLEAN RemoveKeysFromSlot( SOLDIERTYPE * pSoldier, INT8 bKeyRingPosition, UINT8 ubNumberOfKeys ,OBJECTTYPE * pObj )
{
	UINT8 ubItems = 0;

	CHECKF( pObj );


	if( ( pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber == 0 ) || ( pSoldier->pKeyRing[ bKeyRingPosition ].ubKeyID == INVALID_KEY_NUMBER ) )
	{
		return( FALSE );
	}
	else
	{
		//*pObj = pSoldier->inv[bPos];

		if( pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber < ubNumberOfKeys )
		{
			ubNumberOfKeys = pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber;
		}


		ubItems = pSoldier->pKeyRing[ bKeyRingPosition ].ubKeyID;
		if( pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber - ubNumberOfKeys > 0 )
		{
			pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber--;
		}
		else
		{
			pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber = 0;
			pSoldier->pKeyRing[ bKeyRingPosition ].ubKeyID = INVALID_KEY_NUMBER;
		}

		// create an object
		return( CreateKeyObject( pObj, ubNumberOfKeys, ubItems ) );
	}
}

// return number added
UINT8 AddKeysToSlot( SOLDIERTYPE * pSoldier, INT8 bKeyRingPosition, OBJECTTYPE * pObj )
{
	UINT8 ubNumberNotAdded = 0;

	if ( pSoldier->flags.uiStatusFlags & SOLDIER_PC ) // redundant but what the hey
	{
		if ( KeyTable[ (*pObj)[0]->data.key.ubKeyID ].usDateFound == 0 )
		{
			KeyTable[ (*pObj)[0]->data.key.ubKeyID ].usDateFound = (UINT16) GetWorldDay();
			KeyTable[ (*pObj)[0]->data.key.ubKeyID ].usSectorFound = SECTOR( pSoldier->sSectorX, pSoldier->sSectorY );
		}
	}

	// check if we are going to far
	if ( ( pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber + pObj->ubNumberOfObjects ) > ItemSlotLimit(pObj, STACK_SIZE_LIMIT) )
	{
		// only take what we can
		ubNumberNotAdded = pObj->ubNumberOfObjects - ( ItemSlotLimit(pObj, STACK_SIZE_LIMIT) - pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber );

		// set to max
		pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber = ItemSlotLimit(pObj, STACK_SIZE_LIMIT);

		if( pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber == 0 )
		{
			pSoldier->pKeyRing[ bKeyRingPosition ].ubKeyID = (*pObj)[0]->data.key.ubKeyID;
		}

		// return number used
		return( pObj->ubNumberOfObjects - ubNumberNotAdded );
	}
	else
	{
		// check
		if( pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber == 0 )
		{
			pSoldier->pKeyRing[ bKeyRingPosition ].ubKeyID = (*pObj)[0]->data.key.ubKeyID;
		}

		pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber += pObj->ubNumberOfObjects;
	}

	return( pObj->ubNumberOfObjects );
}

UINT8 SwapKeysToSlot( SOLDIERTYPE * pSoldier, INT8 bKeyRingPosition, OBJECTTYPE * pObj )
{
	// swap keys in keyring slot and keys in pocket
	// create temp object to hold keys currently in key ring slot
	CreateKeyObject( &gTempObject, pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber, pSoldier->pKeyRing[ bKeyRingPosition ].ubKeyID );

	pSoldier->pKeyRing[ bKeyRingPosition ].ubNumber = pObj->ubNumberOfObjects;
	pSoldier->pKeyRing[ bKeyRingPosition ].ubKeyID = (*pObj)[0]->data.key.ubKeyID;

	// swap params?
	*pObj = gTempObject;

	return( 1 );
}


BOOLEAN CreateKeyObject( OBJECTTYPE * pObj , UINT8 ubNumberOfKeys, UINT8 ubKeyID )
{
	BOOLEAN fRet;

	pObj->initialize();

	fRet = CreateItems((UINT16)(KeyTable[LockTable[ubKeyID].usKeyItem].usItem), 100, ubNumberOfKeys, pObj);

	if (fRet)
	{
		(*pObj)[0]->data.key.ubKeyID = ubKeyID;
	}
	//fRet = CreateItems( (UINT16)(ubKeyIdValue + FIRST_KEY) , 100, ubNumberOfKeys, pObj )
	//return(  );
	return( fRet );
}


BOOLEAN AllocateObject( OBJECTTYPE **pObj )
{
	// create a key object
	*pObj = new OBJECTTYPE;
	Assert( pObj );

	return( TRUE );
}

BOOLEAN DeleteKeyObject( OBJECTTYPE * pObj )
{
	if( pObj == FALSE )
	{
		return( FALSE );
	}

	// free up space
	delete( pObj );

	return( TRUE );
}

UINT16 TotalPoints( OBJECTTYPE * pObj )
{
	UINT16	usPoints = 0;
	UINT8		ubLoop;

	for (ubLoop = 0; ubLoop < pObj->ubNumberOfObjects; ubLoop++)
	{
		usPoints += (*pObj)[ubLoop]->data.objectStatus;
	}
	return( usPoints );
}

UINT16 UseKitPoints(OBJECTTYPE * pObj, UINT16 usPoints, SOLDIERTYPE *pSoldier)
{
	// start consuming from the last kit in, so we end up with fewer fuller kits rather than
	// lots of half-empty ones.
	INT8	bLoop;
	UINT16	usOriginalPoints = usPoints;
	INT16	sPercentDrain = max(1, 100 - Item[pObj->usItem].percentstatusdrainreduction);
	INT16	sStatus;
	UINT16	sConsume;

	// sevenfm: check
	if (usPoints == 0)
		return 0;

	for (bLoop = pObj->ubNumberOfObjects - 1; bLoop >= 0; bLoop--)
	{
		sStatus = (*pObj)[bLoop]->data.objectStatus;
		// sevenfm: always consume at least 1 point of status
		sConsume = max(1, usPoints * sPercentDrain / 100);

		if (sConsume < sStatus)
		{
			(*pObj)[bLoop]->data.objectStatus -= sConsume;

			// Flugente: campaign stats
			// sevenfm: for player mercs only
			if(pSoldier->IsSoldierPC())
			{
				if (Item[pObj->usItem].foodtype || Item[pObj->usItem].canteen)
					gCampaignStats.AddConsumption(CAMPAIGN_CONSUMED_FOOD, (FLOAT)(usOriginalPoints * Item[pObj->usItem].ubWeight / 100.0));
				else if (Item[pObj->usItem].medical || Item[pObj->usItem].drugtype)
					gCampaignStats.AddConsumption(CAMPAIGN_CONSUMED_MEDICAL, (FLOAT)(usOriginalPoints * Item[pObj->usItem].ubWeight / 100.0));
				else if (Item[pObj->usItem].toolkit || HasItemFlag(pObj->usItem, CLEANING_KIT))
					gCampaignStats.AddConsumption(CAMPAIGN_CONSUMED_REPAIR, (FLOAT)(usOriginalPoints * Item[pObj->usItem].ubWeight / 100.0));
			}			
			// consumed all points
			return(usOriginalPoints);
		}
		// Flugente: we no longer destroy canteens upon emptying them - as we can now refill them
		else if (Item[pObj->usItem].canteen == TRUE)
		{
			// consume sStatus - 1
			usPoints -= min(usPoints, max(1, 100 * (sStatus - 1) / sPercentDrain));
			(*pObj)[bLoop]->data.objectStatus = 1;
			// sevenfm: restore food status when canteen is empty
			(*pObj)[bLoop]->data.bTemperature = OVERHEATING_MAX_TEMPERATURE;
		}
		else
		{
			// consume this kit totally
			usPoints -= min(usPoints, max(1, 100 * sStatus / sPercentDrain));
			(*pObj)[bLoop]->data.objectStatus = 0;

			pObj->ubNumberOfObjects--;
		}
	}

	// sevenfm: for player mercs only
	if(pSoldier->IsSoldierPC())
	{
		// Flugente: campaign stats
		if (Item[pObj->usItem].foodtype || Item[pObj->usItem].canteen)
			gCampaignStats.AddConsumption(CAMPAIGN_CONSUMED_FOOD, (FLOAT)((usOriginalPoints - usPoints) * Item[pObj->usItem].ubWeight / 100.0));
		else if (Item[pObj->usItem].medical || Item[pObj->usItem].drugtype)
			gCampaignStats.AddConsumption(CAMPAIGN_CONSUMED_MEDICAL, (FLOAT)((usOriginalPoints - usPoints) * Item[pObj->usItem].ubWeight / 100.0));
		else if (Item[pObj->usItem].toolkit || HasItemFlag(pObj->usItem, CLEANING_KIT))
			gCampaignStats.AddConsumption(CAMPAIGN_CONSUMED_REPAIR, (FLOAT)((usOriginalPoints - usPoints) * Item[pObj->usItem].ubWeight / 100.0));
	}	

	// check if pocket/hand emptied..update inventory, then update panel
	if (pObj->exists() == false)
	{
		// Delete object
		DeleteObj(pObj);

		// dirty interface panel
		// sevenfm: for player mercs only
		if(pSoldier->IsSoldierPC())
			DirtyMercPanelInterface(pSoldier, DIRTYLEVEL2);
	}

	return(usOriginalPoints - usPoints);
}

#ifdef PATHAI_VISIBLE_DEBUG

	extern BOOLEAN gfDrawPathPoints;

#else

	#ifdef AI_TIMING_TESTS
	extern UINT32 guiGreenTimeTotal, guiYellowTimeTotal, guiRedTimeTotal, guiBlackTimeTotal;
	extern UINT32 guiGreenCounter, guiYellowCounter, guiRedCounter, guiBlackCounter;
	extern UINT32 guiRedSeekTimeTotal, guiRedHelpTimeTotal, guiRedHideTimeTotal;
	extern UINT32 guiRedSeekCounter, guiRedHelpCounter; guiRedHideCounter;
	#endif

#endif


UINT16 MagazineClassIndexToItemType(UINT16 usMagIndex)
{
	// sun_alf: uiIndex is according itemId now 
	return Magazine[usMagIndex].uiItemIndex;


	// Note: if any ammo items in the item table are separated from the main group,
	// this function will have to be rewritten to scan the item table for an item
	// with item class ammo, which has class index usMagIndex
	/*DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String( "MagazineClassIndexToItemType" ) );
	
	// WANNE: Now ammo can be inserted anywhere in Items.xml (before only on index > 70 [FIRST_AMMO] (fix by Realist)
	for (UINT16 usLoop = 0; usLoop < gMAXITEMS_READ; usLoop++)
	{
		// sevenfm: don't stop at empty item records as it will prevent game from finding default magazine
		//if ( Item[usLoop].usItemClass  == 0 )
		//{
		//	DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String( "MagazineClassIndexToItemType: breaking at index %d", usLoop ) );			
		//	break;
		//}

		if (Item[usLoop].ubClassIndex == usMagIndex && Item[usLoop].usItemClass == IC_AMMO )
		{
			DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String( "MagazineClassIndexToItemType: return %d", usLoop ) );
			return( usLoop );
		}
	}

	DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String( "MagazineClassIndexToItemType: return none, got to %d", usLoop ) );
	return(NONE);*/
}

// sevenfm: r8312
// Code change: When loading gun/looking for a default maazine for a gun while no mag with fitting magsize exists,
// instead take a magazine of same calibre and ammotype that has magsize bigger (but close to) the originally requested magsize.

UINT16 DefaultMagazine( UINT16 usItem )
{
	WEAPONTYPE*	pWeapon = &(Weapon[usItem]);
	UINT16		usLoop  = 0;
	UINT16		usDefault = NOTHING;
	UINT16		bestfoundsize = 9999;

	DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("DefaultMagazine: item = %d",usItem));
	if (!(Item[usItem].usItemClass & IC_GUN))
	{
		return(0);
	}

	//while (Magazine[usLoop].ubCalibre != NOAMMO)
	while(usLoop < gMAXMAGAZINES_READ)
	{
		// Flugente: problems arise if we cannot find a mag that fits exactly. Instead of not using a magazine at all, find a mag of same calibre that has a size as close to what is requested as possible
		if ( Magazine[usLoop].ubCalibre == pWeapon->ubCalibre && 
			AmmoTypes[Magazine[usLoop].ubAmmoType].standardIssue )
		{
			// Flugente: forbid ammo with tracer effects to be used on singleshot-only guns (snipers wouldn't use ammo that marks their position, would they?)
			if ( !pWeapon->ubShotsPerBurst && !pWeapon->bAutofireShotsPerFiveAP && AmmoTypes[ Magazine[usLoop].ubAmmoType ].tracerEffect )
			{
				// don't use this one...
				++usLoop;
				continue;
			}

			if ( Magazine[usLoop].ubMagSize == pWeapon->ubMagSize )
			{
				DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String( "DefaultMagazine: found at index %d", usLoop ) );
				return(MagazineClassIndexToItemType( usLoop ));
			}
			// as a fallback solution, look for a mag of same calibre and ammotype, but bigger size (as close to requested size as possible)
			else if ( Magazine[usLoop].ubMagSize > pWeapon->ubMagSize && Magazine[usLoop].ubMagSize < bestfoundsize )
			{
				// store this one to use if all else fails
				usDefault = MagazineClassIndexToItemType( usLoop );

				bestfoundsize = Magazine[Item[usDefault].ubClassIndex].ubMagSize;
			}
		}

		++usLoop;
	}

	DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("DefaultMagazine: can't find exact match, use approximation"));
	return usDefault;
}

UINT16 FindReplacementMagazine( UINT8 ubCalibre, UINT16 ubMagSize, UINT8 ubAmmoType )
{
	UINT16 usLoop = 0;
	UINT16 usDefault = NOTHING;
	UINT16 bestfoundsize = 9999;

	DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("FindReplacementMagazine: calibre = %d, Mag size = %d, ammo type = %d",ubCalibre,ubMagSize,ubAmmoType));

	//while (Magazine[usLoop].ubCalibre != NOAMMO)
	while (usLoop < gMAXMAGAZINES_READ)
	{
		// Flugente: problems arise if we cannot find a mag that fits exactly. Vanilla code then compromises on ammotype - which leads to loading from a AP belt resulting in HP mags.
		// As that's rather silly, we instead compromise on mag size
		if ( Magazine[usLoop].ubCalibre == ubCalibre && Magazine[usLoop].ubAmmoType == ubAmmoType )
		{
			if ( Magazine[usLoop].ubMagSize == ubMagSize )
			{
				DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("FindReplacementMagazine: returning item = %d",MagazineClassIndexToItemType( usLoop )));
				return( MagazineClassIndexToItemType( usLoop ) );
			}
			// as a fallback solution, look for a mag of same calibre and ammotype, but bigger size (as close to requested size as possible)
			else if ( Magazine[usLoop].ubMagSize > ubMagSize && Magazine[usLoop].ubMagSize < bestfoundsize )
			{
				// store this one to use if all else fails
				usDefault = MagazineClassIndexToItemType( usLoop );

				bestfoundsize = Magazine[Item[usDefault].ubClassIndex].ubMagSize;
			}
		}

		++usLoop;
	}

	DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("FindReplacementMagazine: returning default item = %d",usDefault));
	return( usDefault );
}

UINT16 FindReplacementMagazineIfNecessary( UINT16 usOldGun, UINT16 usOldAmmo, UINT16 usNewGun )
{
	UINT16 usNewAmmo = NOTHING;

	if ( (Magazine[ Item[ usOldAmmo ].ubClassIndex ].ubCalibre == Weapon[ usOldGun ].ubCalibre) &&
			 (Magazine[ Item[ usOldAmmo ].ubClassIndex ].ubMagSize == Weapon[ usOldGun ].ubMagSize) )
	{
		// must replace this!
		usNewAmmo = FindReplacementMagazine( Weapon[ usNewGun ].ubCalibre, Weapon[ usNewGun ].ubMagSize, Magazine[ Item[ usOldAmmo ].ubClassIndex ].ubAmmoType );
	}

	return( usNewAmmo );
}

// increase this if any gun can have more types that this
#define MAX_AMMO_TYPES_PER_GUN		64  // MADD MARKER

UINT16 RandomMagazine( UINT16 usItem, UINT8 ubPercentStandard, UINT8 maxCoolness, INT8 bSoldierClass )
{
	// Note: if any ammo items in the item table are separated from the main group,
	// this function will have to be rewritten to scan the item table for an item
	// with item class ammo, which has class index ubLoop
	DebugMsg (TOPIC_JA2,DBG_LEVEL_3,String("RandomMagazine (by index)"));

	// Flugente: if accessing with wrong soldier class, or not using different selection choices, take default one
	if ( bSoldierClass >= SOLDIER_GUN_CHOICE_SELECTIONS || bSoldierClass < SOLDIER_CLASS_NONE || !gGameExternalOptions.fSoldierClassSpecificItemTables )
		bSoldierClass = SOLDIER_CLASS_NONE;

	WEAPONTYPE *	pWeapon;
	UINT16			usLoop;
	UINT16			loopItem;
	UINT16			usPossibleMagIndex[ MAX_AMMO_TYPES_PER_GUN ];
	UINT16			usPossibleMagCnt = 0;
	UINT8			ubMagChosen;

	if (!(Item[usItem].usItemClass & IC_GUN))
	{
		return( 0 );
	}

	pWeapon = &(Weapon[usItem]);

	// find & store all possible mag types that fit this gun
	usLoop = 0;

	//while (Magazine[usLoop].ubCalibre != NOAMMO)
	while (usLoop < gMAXMAGAZINES_READ)
	{
		if (Magazine[usLoop].ubCalibre == pWeapon->ubCalibre &&
			Magazine[usLoop].ubMagSize == pWeapon->ubMagSize)
		{
			loopItem = MagazineClassIndexToItemType(usLoop);

			if (ItemIsLegal(loopItem) && maxCoolness >= Item[loopItem].ubCoolness)
			{
				// store it! (make sure array is big enough)
				Assert(usPossibleMagCnt < MAX_AMMO_TYPES_PER_GUN);
				// Madd: check to see if allowed by army
				if (gArmyItemChoices[bSoldierClass][ENEMYAMMOTYPES].ubChoices > 0)
				{
					for (int i = 0; i < gArmyItemChoices[bSoldierClass][ENEMYAMMOTYPES].ubChoices; i++)
					{
						if (gArmyItemChoices[bSoldierClass][ENEMYAMMOTYPES].bItemNo[i] == Magazine[usLoop].ubAmmoType)
						{
							usPossibleMagIndex[usPossibleMagCnt++] = usLoop;
							break;
						}
					}
				}
				else
				{
					usPossibleMagIndex[usPossibleMagCnt++] = usLoop;
				}
			}
		}

		usLoop++;
	}

	// no matches?
	if (usPossibleMagCnt == 0)
	{
		return( 0 );
	}
	else
	// only one match?
	if (usPossibleMagCnt == 1)
	{
		// use that, no choice
		return(MagazineClassIndexToItemType( usPossibleMagIndex[ 0 ] ));
	}
	else	// multiple choices
	{
		// Pick one at random, using supplied probability to pick the default
		if (Random(100) < ubPercentStandard)
		{
			ubMagChosen = 0;
			DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("RandomMagazine: ubMagChosen = %d",ubMagChosen ));
			return( DefaultMagazine(usItem) );
		}
		else
		{
			// pick a non-standard type instead
			ubMagChosen = ( UINT8 ) (1 + Random(( UINT32 ) ( usPossibleMagCnt - 1 )));
		}

		DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("RandomMagazine: ubMagChosen = %d",ubMagChosen ));
		return( MagazineClassIndexToItemType(usPossibleMagIndex[ ubMagChosen ] ) );
	}
}

UINT16 RandomMagazine( OBJECTTYPE * pGun, UINT8 ubPercentStandard, UINT8 maxCoolness, INT8 bSoldierClass )
{
	// Note: if any ammo items in the item table are separated from the main group,
	// this function will have to be rewritten to scan the item table for an item
	// with item class ammo, which has class index ubLoop
	DebugMsg (TOPIC_JA2,DBG_LEVEL_3,String("RandomMagazine"));

	// Flugente: if accessing with wrong soldier class, or not using different selection choices, take default one
	if ( bSoldierClass >= SOLDIER_GUN_CHOICE_SELECTIONS || bSoldierClass < SOLDIER_CLASS_NONE || !gGameExternalOptions.fSoldierClassSpecificItemTables )
		bSoldierClass = SOLDIER_CLASS_NONE;

	WEAPONTYPE *	pWeapon;
	UINT16			usLoop;
	UINT16			loopItem;
	UINT16			usPossibleMagIndex[ MAX_AMMO_TYPES_PER_GUN ];
	UINT16			usPossibleMagCnt = 0;
	UINT8			ubMagChosen;

	if (!(Item[pGun->usItem].usItemClass & IC_GUN))
	{
		return( 0 );
	}

	pWeapon = &(Weapon[pGun->usItem]);

	// find & store all possible mag types that fit this gun
	usLoop = 0;

	//while (Magazine[usLoop].ubCalibre != NOAMMO)
	while (usLoop < gMAXMAGAZINES_READ)
	{
		if (Magazine[usLoop].ubCalibre == pWeapon->ubCalibre &&
			Magazine[usLoop].ubMagSize == GetMagSize(pGun))
		{
			loopItem = MagazineClassIndexToItemType(usLoop);

			if (ItemIsLegal(loopItem) && maxCoolness >= Item[loopItem].ubCoolness)
			{
				// store it! (make sure array is big enough)
				Assert(usPossibleMagCnt < MAX_AMMO_TYPES_PER_GUN);
				// Madd: check to see if allowed by army
				if (gArmyItemChoices[bSoldierClass][ENEMYAMMOTYPES].ubChoices > 0)
				{
					for (int i = 0; i < gArmyItemChoices[bSoldierClass][ENEMYAMMOTYPES].ubChoices; i++)
					{
						if (gArmyItemChoices[bSoldierClass][ENEMYAMMOTYPES].bItemNo[i] == Magazine[usLoop].ubAmmoType)
						{
							// Flugente: forbid ammo with tracer effects to be used on singleshot-only guns (snipers wouldn't use ammo that marks their position, would they?)
							if (!pWeapon->ubShotsPerBurst && !pWeapon->bAutofireShotsPerFiveAP && AmmoTypes[Magazine[usLoop].ubAmmoType].tracerEffect)
							{
								// don't use this one...
								continue;
							}

							usPossibleMagIndex[usPossibleMagCnt++] = usLoop;
							break;
						}
					}
				}
				else
				{
					usPossibleMagIndex[usPossibleMagCnt++] = usLoop;
				}
			}
		}		

		usLoop++;
	}

	// no matches?
	if (usPossibleMagCnt == 0)
	{
		return( 0 );
	}
	else
	// only one match?
	if (usPossibleMagCnt == 1)
	{
		// use that, no choice
		return(MagazineClassIndexToItemType(usPossibleMagIndex[ 0 ] ));
	}
	else	// multiple choices
	{
		// Pick one at random, using supplied probability to pick the default
		if (Random(100) < ubPercentStandard)
		{
			ubMagChosen = 0;
			DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("RandomMagazine: ubMagChosen = %d",ubMagChosen ));
			return( DefaultMagazine(pGun->usItem) );
		}
		else
		{
			// pick a non-standard type instead
			ubMagChosen = ( UINT8 ) (1 + Random(( UINT32 ) ( usPossibleMagCnt - 1 )));
		}

		DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("RandomMagazine: ubMagChosen = %d",ubMagChosen ));
		return( MagazineClassIndexToItemType(usPossibleMagIndex[ ubMagChosen ] ) );
	}
}

BOOLEAN CreateGun( UINT16 usItem, INT16 bStatus, OBJECTTYPE * pObj )
{
	UINT16 usAmmo;
	DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("CreateGun: usItem = %d",usItem));

	Assert( pObj != NULL);
	if ( pObj == NULL )
	{
		return( FALSE );
	}

	pObj->initialize();
	pObj->usItem = usItem;
	pObj->ubNumberOfObjects = 1;
	//pObj->objectStack.resize(1);//not necessary due to init, here for code commenting
	StackedObjectData* pStackedObject = (*pObj)[0];
	pStackedObject->data.gun.bGunStatus = bStatus;
	pStackedObject->data.ubImprintID = NO_PROFILE;

	// Flugente: temperature on creation is 0
	pStackedObject->data.bTemperature = 0.0f;
	
	pStackedObject->data.sRepairThreshold = (200 + bStatus)/3;	// arbitrary threshold
	pStackedObject->data.dDirtLevel = 0.0f;
	pStackedObject->data.sObjectFlag = 0;

	if (Weapon[ usItem ].ubWeaponClass == MONSTERCLASS)
	{
		pStackedObject->data.gun.ubGunShotsLeft = GetMagSize(pObj);
		pStackedObject->data.gun.ubGunAmmoType = AMMO_MONSTER;
		pStackedObject->data.gun.ubGunState |= GS_CARTRIDGE_IN_CHAMBER; // 0verhaul:  Monsters don't have to reload!
	}
	else if ( EXPLOSIVE_GUN( usItem ) )
	{
		if ( Item[usItem].singleshotrocketlauncher )
		{
			pStackedObject->data.gun.ubGunShotsLeft = 1;
		}
		else
		{
			// cannon
			pStackedObject->data.gun.ubGunShotsLeft = 0;
		}
		pStackedObject->data.gun.bGunAmmoStatus = 100;
		pStackedObject->data.gun.ubGunAmmoType = 0;
	}
	else
	{
		usAmmo = DefaultMagazine( usItem );
		//CHRISL: Why do we have an assert here when the very next condition says to return a FALSE if usAmmo is 0?
		//Assert( usAmmo != 0 );
		if (usAmmo == 0)
		{
			// item's calibre & mag size not found in magazine list!
			DebugStartingGear(String("CreateGun: failed to find default magazine for item %d max magazines %d", usItem, gMAXMAGAZINES_READ));
			return( FALSE );
		}
		else
		{
			pStackedObject->data.gun.usGunAmmoItem = usAmmo;
			pStackedObject->data.gun.ubGunAmmoType = Magazine[ Item[ usAmmo ].ubClassIndex].ubAmmoType;
			pStackedObject->data.gun.bGunAmmoStatus = 100;
			pStackedObject->data.gun.ubGunShotsLeft = Magazine[ Item[ usAmmo ].ubClassIndex ].ubMagSize;
			if (Weapon[usItem].APsToReloadManually == 0)
				pStackedObject->data.gun.ubGunState |= GS_CARTRIDGE_IN_CHAMBER; // Madd: new guns should have cartridge in chamber
		}
	}

	//WarmSteel - Init item slots.
	if(UsingNewAttachmentSystem()==true)
		InitItemAttachments(pObj);

	//ADB ubWeight has been removed, see comments in OBJECTTYPE
	//pObj->ubWeight = CalculateObjectWeight( pObj );
	DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("CreateGun: Done"));
	// successful
	return( TRUE );
}

BOOLEAN CreateAmmo( UINT16 usItem, OBJECTTYPE * pObj, UINT16 ubShotsLeft )
{
	CHECKF(pObj);

	pObj->initialize();
	pObj->usItem = usItem;
	pObj->ubNumberOfObjects = 1;

	(*pObj)[0]->data.ubShotsLeft = ubShotsLeft;

	//WarmSteel - Init attachment slots.
	if(UsingNewAttachmentSystem()==true)
		InitItemAttachments(pObj);

	//ADB ubWeight has been removed, see comments in OBJECTTYPE
	//pObj->ubWeight = CalculateObjectWeight( pObj );
	DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("CreateAmmo: done"));

	return( TRUE );
}

BOOLEAN CreateItem( UINT16 usItem, INT16 bStatus, OBJECTTYPE * pObj )
{
	DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("CreateItem: usItem = %d",usItem));
	BOOLEAN fRet;

	if (usItem >= gMAXITEMS_READ)
	{
		DebugBreakpoint();
		return( FALSE );
	}

	// Flugente: random items
	UINT16 newitemfromrandom = 0;
	if (GetItemFromRandomItem(usItem, &newitemfromrandom))
	{
		usItem = newitemfromrandom;		
	}

#ifdef JA2EDITOR

	// Buggler: usItem == 0 is technically an item too, so need to run code to delete existing item in editor/merc inventory mode

#else //non-editor version

	if(usItem == 0 || Item[usItem].randomitem > 0)//dnl ch79 291113 don't recreate random item from game as GetItemFromRandomItem obviously fail to create real item
	{
		DebugBreakpoint();
		return(FALSE);
	}

#endif

	if (Item[ usItem ].usItemClass == IC_GUN)
	{
		fRet = CreateGun( usItem, bStatus, pObj );
	}
	else if (Item[ usItem ].usItemClass == IC_AMMO)
	{
		fRet = CreateAmmo( usItem, pObj, Magazine[Item[usItem].ubClassIndex].ubMagSize );
	}
	else
	{
		pObj->initialize();
		pObj->usItem = usItem;
		pObj->ubNumberOfObjects = 1;
		//pObj->objectStack.resize(1);//not necessary due to init, here for code commenting
		if (usItem == MONEY || Item[usItem].usItemClass == IC_MONEY )
		{
			// special case... always set status to 100 when creating
			// and use status value to determine amount!
			(*pObj)[0]->data.objectStatus = 100;
			(*pObj)[0]->data.money.uiMoneyAmount = bStatus * 50;
		}
		else
		{
			(*pObj)[0]->data.objectStatus = bStatus;
		}

		// Flugente: the temperature variable determines the quality of the food, begin with being fresh
		if ( Item[usItem].foodtype > 0 )
		{
			(*pObj)[0]->data.bTemperature = OVERHEATING_MAX_TEMPERATURE;
		}

		// Flugente: certain items get the maximum temperature when they are created
		if ( HasItemFlag(usItem, POWER_PACK) )
		{
			(*pObj)[0]->data.bTemperature = (FLOAT)(OVERHEATING_MAX_TEMPERATURE);
		}

		//ADB ubWeight has been removed, see comments in OBJECTTYPE
		//pObj->ubWeight = CalculateObjectWeight( pObj );
		fRet = TRUE;
		//WarmSteel - Init attachment slots.
		if(UsingNewAttachmentSystem()==true)
			InitItemAttachments(pObj);

		if (Item[ usItem ].usItemClass == IC_ARMOUR)
			(*pObj)[0]->data.sRepairThreshold = (200 + bStatus)/3;	// arbitrary threshold
		else
			(*pObj)[0]->data.sRepairThreshold = 100;

		(*pObj)[0]->data.dDirtLevel = 0.0f;
	}
	if (fRet)
	{
//		if (Item[ usItem ].fFlags & ITEM_DEFAULT_UNDROPPABLE)
		if (Item[ usItem ].defaultundroppable )
		{
			(*pObj).fFlags |= OBJECT_UNDROPPABLE;
		}
		AttachDefaultAttachments(pObj);//dnl ch75 261013
	}

	DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("CreateItem: return %d",fRet));
	return( fRet );
}

BOOLEAN CreateItems( UINT16 usItem, INT16 bStatus, UINT8 ubNumber, OBJECTTYPE * pObj )
{
	BOOLEAN fOk;
	fOk = CreateItem( usItem, bStatus, pObj );
	if (fOk)
	{
		if (ubNumber > 1) {
			//no need to recalc weight if just 1, CreateItem did that.
			for (int x = 1; x < ubNumber; ++x) {
				pObj->objectStack.push_back(pObj->objectStack.front());
			}
			pObj->ubNumberOfObjects = ubNumber;
			//ADB ubWeight has been removed, see comments in OBJECTTYPE
			//pObj->ubWeight = CalculateObjectWeight(pObj);
		}
		return( TRUE );
	}
	return( FALSE );
}

BOOLEAN CreateMoney( UINT32 uiMoney, OBJECTTYPE * pObj )
{
	BOOLEAN fOk;

	fOk = CreateItem( MONEY, 100, pObj );
	if (fOk)
	{
		(*pObj)[0]->data.money.uiMoneyAmount = uiMoney;
	}
	return( fOk );
}

BOOLEAN ArmBomb( OBJECTTYPE * pObj, INT8 bSetting )
{
	BOOLEAN fRemote = FALSE;
	BOOLEAN fPressure = FALSE;
	BOOLEAN fTimed = FALSE;
	BOOLEAN	fSwitch = FALSE;
	BOOLEAN fDefuse = FALSE;		// bomb can be defused remotely	

	if ( HasAttachmentOfClass( pObj, AC_DEFUSE) )
	{
		fDefuse = TRUE;
	}

	if (pObj->usItem == ACTION_ITEM)
	{
		switch( (*pObj)[0]->data.misc.bActionValue )
		{
			case ACTION_ITEM_SMALL_PIT:
			case ACTION_ITEM_LARGE_PIT:
				fPressure = TRUE;
				break;
			default:
				fRemote = TRUE;
				break;

		}
	}
	else if ( HasAttachmentOfClass( pObj, AC_DETONATOR ) )
	{
		fTimed = TRUE;
	}
	else if ( HasAttachmentOfClass( pObj, (AC_REMOTEDET | AC_DEFUSE) ) || (pObj->usItem == ACTION_ITEM) )
	{
		fRemote = TRUE;
	}
	else if ( Item[pObj->usItem].mine || pObj->usItem == TRIP_FLARE || pObj->usItem == TRIP_KLAXON || pObj->usItem == ACTION_ITEM )
	{
		fPressure = TRUE;
	}
	else if ( pObj->usItem == SWITCH )
	{
		// this makes a remote detonator into a pressure-sensitive trigger
		if ( bSetting == PANIC_FREQUENCY )
		{
			// panic trigger is only activated by expending APs, not by
			// stepping on it... so don't define a detonator type
			fSwitch = TRUE;
		}
		else
		{
			fPressure = TRUE;
		}
	}
	else
	{
		// no sorta bomb at all!
		return( FALSE );
	}

	// Flugente: decide how to interpret the bSetting we just got.
	// Due to limitations in the message system, we only receive a single value to interpret, as we currently can't have a message box return 2 values
	// It might be possible to have proper checkboxes, but I'll rather not research this right now.
	// The remote defuse complicates things, as you'll see:
	//
	// If we are placing a bomb or mine (so no tripwire), consider the following: 
	// a) if we have a timed detonator: time in turns until she blows': 1-4
	// b) if we have a remote detonator: frequency on which the bomb will blow: 1-4
	// c) if we have a timed detonator plus a remote defuse: time in turns until she blows plus frequency to defuse: 1-16
	// d) if we have a remote detonator plus a remote defuse: frequency on which the bomb will blow plus frequency to defuse: 1-16
	//
	// I we are placing tripwire, consider this:
	// e) if we place tripwire: the tripwire network plus the hierachy in that network: 1-16
	//
	// It is clear that we only have to reinterpret the values if a defuse is equiped, or we are placing tripwire
	INT8 detonatesetting = bSetting;
	INT8 defusesetting	 = bSetting;

	if ( fDefuse && bSetting > 0 && bSetting < 17 )	// checks for safety
	{
		// the bSetting consists of the 4 * (detonation frequency - 1) + defuse frequency
		detonatesetting = 1;
		defusesetting = bSetting % 4;
		if ( defusesetting == 0 )
			defusesetting = 4;

		INT8 tmp = bSetting - defusesetting;		// now 0, 4, 8 or 12
		if ( tmp > 0 )	++detonatesetting;
		if ( tmp > 4 )	++detonatesetting;
		if ( tmp > 8 )	++detonatesetting;			// defusesetting is now in 1-4
	}

	// tripwires
	UINT32 ubWireNetworkFlag = 0;
	if ( Item[pObj->usItem].tripwire == 1 && bSetting > 0 && bSetting < 17 ) // checks for safety
	{
		// we are placing it, so it's ours
		ubWireNetworkFlag |= TRIPWIRE_NETWORK_OWNER_PLAYER;
				
		// the bSetting consists of the network number + 4 * (network hierarchy - 1)
		INT8 netnr = bSetting % 4;
		INT8 hierarchytimesfour = bSetting - netnr;

		if ( 1 == netnr )
			ubWireNetworkFlag |= TRIPWIRE_NETWORK_NET_1;
		else if ( 2 == netnr )
			ubWireNetworkFlag |= TRIPWIRE_NETWORK_NET_2;
		else if ( 3 == netnr )
			ubWireNetworkFlag |= TRIPWIRE_NETWORK_NET_3;
		else
			ubWireNetworkFlag |= TRIPWIRE_NETWORK_NET_4;

		// sevenfm: fixed incorrect hierarchy calculation
		if(netnr == 0)
			hierarchytimesfour -= 4;
		if ( 0 == hierarchytimesfour )
			ubWireNetworkFlag |= TRIPWIRE_NETWORK_LVL_1;
		else if ( 4 == hierarchytimesfour )
			ubWireNetworkFlag |= TRIPWIRE_NETWORK_LVL_2;
		else if ( 8 == hierarchytimesfour )
			ubWireNetworkFlag |= TRIPWIRE_NETWORK_LVL_3;
		else
			ubWireNetworkFlag |= TRIPWIRE_NETWORK_LVL_4;

		// TOOD: once tripwire can be placed in the editor, this has to be altered
	}

	if (fDefuse)	// TODO: doesn't work this way if both a detonator and a remote defuse is attached...
	{
		(*pObj)[0]->data.misc.bDetonatorType = BOMB_REMOTE;
		(*pObj)[0]->data.bDefuseFrequency = defusesetting;
	}

	if (fRemote)
	{
		(*pObj)[0]->data.misc.bDetonatorType = BOMB_REMOTE;
		(*pObj)[0]->data.misc.bFrequency = detonatesetting;
	}
	else if (fPressure)
	{
		(*pObj)[0]->data.misc.bDetonatorType = BOMB_PRESSURE;
		(*pObj)[0]->data.misc.bFrequency = 0;
		(*pObj)[0]->data.ubWireNetworkFlag = ubWireNetworkFlag;
	}
	else if (fTimed)
	{
		(*pObj)[0]->data.misc.bDetonatorType = BOMB_TIMED;
		// In realtime the player could choose to put down a bomb right before a turn expires, SO
		// add 1 to the setting in RT
		(*pObj)[0]->data.misc.bDelay = detonatesetting;
		if ( !(gTacticalStatus.uiFlags & TURNBASED && gTacticalStatus.uiFlags & INCOMBAT) )
		{
			(*pObj)[0]->data.misc.bDelay++;
		}
	}
	else if (fSwitch)
	{
		(*pObj)[0]->data.misc.bDetonatorType = BOMB_SWITCH;
		(*pObj)[0]->data.misc.bFrequency = detonatesetting;
	}
	else
	{
		return( FALSE );
	}

	// for safety, weird things happen
	if ( (*pObj).fFlags & OBJECT_DISABLED_BOMB )
		(*pObj).fFlags &= ~(OBJECT_DISABLED_BOMB);

	(*pObj).fFlags |= OBJECT_ARMED_BOMB;
	
	// sevenfm: r7811 fix
	(*pObj).fFlags |= OBJECT_KNOWN_TO_BE_TRAPPED;

	(*pObj)[0]->data.misc.usBombItem = pObj->usItem;
	return( TRUE );
}

/*BOOLEAN ArmTimedBomb(OBJECTTYPE *pObj, INT8 bDelay)
{
	CHECKF(pObj);

	(*pObj)[0]->data.misc.bDetonatorType = BOMB_TIMED;
	(*pObj)[0]->data.misc.bDelay = bDelay;

	(*pObj)[0]->data.misc.bFrequency = 0;
	(*pObj)[0]->data.bDefuseFrequency = 0;
	(*pObj)[0]->data.ubWireNetworkFlag = 0;

	if ((*pObj).fFlags & OBJECT_DISABLED_BOMB)
		(*pObj).fFlags &= ~(OBJECT_DISABLED_BOMB);

	(*pObj).fFlags |= OBJECT_ARMED_BOMB;
	(*pObj).fFlags |= OBJECT_KNOWN_TO_BE_TRAPPED;
	(*pObj)[0]->data.misc.usBombItem = pObj->usItem;

	return TRUE;
}*/

// sevenfm: r8404
// pAttachment will be filled with garbage data, pass a new OBJECTTYPE as pNewObj to get the removed attachment or pass NULL to indicate the attachment is to be deleted
BOOLEAN OBJECTTYPE::RemoveAttachment( OBJECTTYPE * pAttachment, OBJECTTYPE * pNewObj, UINT8 subObject, SOLDIERTYPE * pSoldier, BOOLEAN fForceInseperable, BOOLEAN fRemoveProhibited )
//BOOLEAN OBJECTTYPE::RemoveAttachment( OBJECTTYPE* pAttachment, OBJECTTYPE * pNewObj, UINT8 subObject, SOLDIERTYPE * pSoldier, BOOLEAN fForceInseperable, BOOLEAN fRemoveProhibited )
{
	BOOLEAN		objDeleted = FALSE;
	std::vector<UINT16> usAttachmentSlotIndexVector;
	std::vector<UINT16> usRemAttachmentSlotIndexVector;
	UINT16		oldMagSize = 0;
	OBJECTTYPE removedAttachment; // Madd: added to fix global attachment removal breaking attachments with their own attachments

	if ( pAttachment->exists() == false || this->exists() == false)
	{
		return( FALSE );
	}

	//Sometimes we have to force inseparable items off, sadly.
	if ( Item[ pAttachment->usItem ].inseparable && !fForceInseperable )
	{
		return( FALSE );
	}

	//CHRISL: This is so we can try to determine if the removed attachment altered our mag size.
	if(Item[this->usItem].usItemClass == IC_GUN)
		oldMagSize = GetMagSize(this);

	//CHRISL: I know this FOR loop is basically redundant to what the remove() function already does, but
	//	this setup includes a failsafe.  Now we'll only copy the attachment to our cursor (pNewObj) if
	//	iter and pAttachment are the same.  This should stop attachment duplication though it doesn't resolve
	//	the initial cause of the attachment duplication issue.

	//First look for attachment with the exact same memory address.
	//This is so that you can click to remove an attachment and it will take off THAT attachment and not the first one that just "looks" like it.
	for (std::list<OBJECTTYPE>::iterator iter = (*this)[subObject]->attachments.begin(); iter != (*this)[subObject]->attachments.end(); ++iter)
	{
		if (iter->exists())
		{
			//Compare the address
			if (&(*iter) == pAttachment)
			{
				removedAttachment = *pAttachment;
				iter = (*this)[subObject]->RemoveAttachmentAtIter(iter);
				pAttachment = NULL;
				objDeleted = TRUE;
				break;
			}
			/*
			//Compare the address
			if(&(*iter) == pAttachment)
			{
				removedAttachment = *pAttachment;
				iter = (*this)[subObject]->RemoveAttachmentAtIter(iter);
				pAttachment = NULL;
				objDeleted = TRUE;
				break;
			}*/
		}
	}

	//It is possible that the previous loop did not find the EXACT attachment we wanted to delete, look if there is one that is at least equal in data.
	if(!objDeleted)
	{
		for (std::list<OBJECTTYPE>::iterator iter = (*this)[subObject]->attachments.begin(); iter != (*this)[subObject]->attachments.end(); ++iter)
		{
				//This compares the internal data of the objects.
			if (iter->exists() && *iter == *pAttachment){
				removedAttachment = *pAttachment;
				iter = (*this)[subObject]->RemoveAttachmentAtIter(iter);
				pAttachment = NULL;
				objDeleted = TRUE;
				break;
			}
		}
	}

	if(!objDeleted)
		return( FALSE );

	//After removing an attachment, the ammo capacity might have changed.
	if ( Item[this->usItem].usItemClass == IC_GUN && (*this)[subObject]->data.gun.usGunAmmoItem != NONE && (*this)[subObject]->data.gun.ubGunShotsLeft > 0 && oldMagSize != GetMagSize(this, subObject) )
	{
		if ( (*this)[subObject]->data.gun.ubGunShotsLeft > GetMagSize(this, subObject) )
		{ //Too many rounds, eject ammo
			EjectAmmoAndPlace(pSoldier, this, subObject);
		}
		//CHRISL: We should update the usGunAmmoItem if we've changed the ammo capacity
		if((*this)[subObject]->data.gun.usGunAmmoItem != NONE){
			UINT16 usNewAmmoItem;
			usNewAmmoItem = FindReplacementMagazine(Weapon[this->usItem].ubCalibre ,GetMagSize(this, subObject),Magazine[Item[(*this)[subObject]->data.gun.usGunAmmoItem].ubClassIndex].ubAmmoType);
			(*this)[subObject]->data.gun.usGunAmmoItem = usNewAmmoItem;
		}
	}

	if (pSoldier != NULL)
	{
		// if in attached weapon mode and don't have weapon with GL attached in hand, reset weapon mode
		if ( ( (pSoldier->bWeaponMode == WM_ATTACHED_GL || pSoldier->bWeaponMode == WM_ATTACHED_GL_BURST || pSoldier->bWeaponMode == WM_ATTACHED_GL_AUTO ) && !IsGrenadeLauncherAttached( &(pSoldier->inv[ HANDPOS ] ) ) ) ||
			 ( (pSoldier->bWeaponMode == WM_ATTACHED_UB || pSoldier->bWeaponMode == WM_ATTACHED_UB_BURST || pSoldier->bWeaponMode == WM_ATTACHED_UB_AUTO ) && !IsWeaponAttached( &(pSoldier->inv[ HANDPOS ]), IC_GUN   ) ) ||
			 ( (pSoldier->bWeaponMode == WM_ATTACHED_BAYONET )																							   && !IsWeaponAttached( &(pSoldier->inv[ HANDPOS ]), IC_BLADE ) ) ) 
		{
			if ( !Weapon[pSoldier->inv[ HANDPOS ].usItem].NoSemiAuto )
			{
				pSoldier->bWeaponMode = WM_NORMAL;
				pSoldier->bDoBurst = FALSE;
				pSoldier->bDoAutofire = 0;
			}
			else
			{
				pSoldier->bWeaponMode = WM_AUTOFIRE;
				pSoldier->bDoBurst = TRUE;
				pSoldier->bDoAutofire = 1;
			}

			pSoldier->FixScopeMode();
		}
	}

	if(Item[this->usItem].usItemClass == IC_GUN && oldMagSize != GetMagSize(this, subObject)){
		fInterfacePanelDirty = DIRTYLEVEL2;
		RenderBulletIcon(this, subObject);
	}

	//CHRISL: We need to know if the removed attachment could have altered the base items potential attachments
	BOOLEAN	removeAttachments = TRUE, cleanAttachments = FALSE;
	INT8	loopCount = 0;

	while(removeAttachments){
		usRemAttachmentSlotIndexVector = GetItemSlots(&removedAttachment);
		if(usRemAttachmentSlotIndexVector.empty()){
			removeAttachments = FALSE;
		} else {
			cleanAttachments = TRUE;
			usAttachmentSlotIndexVector = GetItemSlots(this, subObject);
			for(attachmentList::iterator iter = (*this)[subObject]->attachments.begin(); iter != (*this)[subObject]->attachments.end(); ++iter)
			{
				removeAttachments = FALSE;
				if(iter->exists())
				{
					if(!ValidItemAttachment(this, iter->usItem, FALSE, FALSE, subObject, usAttachmentSlotIndexVector))
					{	//attachment is no longer valid
						removeAttachments = TRUE;
						OBJECTTYPE	remObj;
						remObj = *iter;
						if(ValidItemAttachment(&removedAttachment, iter->usItem, FALSE, FALSE, 0, usRemAttachmentSlotIndexVector)){
							removedAttachment[0]->attachments.push_back((*iter));
							iter = (*this)[subObject]->RemoveAttachmentAtIter(iter);
							continue;
						}
						if(loopCount > 5){	//try moving attachments to the removed attachment 5 times before we drop anything
							if ( pSoldier && AutoPlaceObject( pSoldier, &remObj, FALSE ) )
							{
								iter = (*this)[subObject]->RemoveAttachmentAtIter(iter);
							}
							else 
							{	// put it on the ground
								INT8 pathing = (pSoldier?pSoldier->pathing.bLevel:0);
								INT32 sGridNo = (pSoldier?pSoldier->sGridNo:0);
								if( AutoPlaceObjectToWorld(pSoldier, &remObj) )
									iter = (*this)[subObject]->RemoveAttachmentAtIter(iter);
							}
						}
					}
				}
			}
		}
		loopCount++;
	}
	if(cleanAttachments){
		RemoveProhibitedAttachments(pSoldier, &removedAttachment, removedAttachment.usItem);
		RemoveProhibitedAttachments(pSoldier, this, this->usItem);
	}

	if(pNewObj != NULL){
		*pNewObj = removedAttachment;
	}

	if (pNewObj->exists() && Item[pNewObj->usItem].grenadelauncher )//UNDER_GLAUNCHER)
	{
		// look for any grenade; if it exists, we must make it an
		// attachment of the grenade launcher
		OBJECTTYPE* pGrenade = FindAttachmentByClass( this, IC_GRENADE );
		if (pGrenade->exists())
		{
			// we might have to do it in this order, because if we attach first,
			// the object is pretty much gone and RemoveAttachment won't work (returns right away)
			OBJECTTYPE tmp;
			this->RemoveAttachment(pGrenade, &tmp, 0, NULL, 1, 0);
			pNewObj->AttachObject(NULL, &tmp, FALSE, 0, -1, 0);
		}
	}

	//Removing an attachment can alter slots, check them.
	if(UsingNewAttachmentSystem()==true && fRemoveProhibited){
		RemoveProhibitedAttachments(pSoldier, this, this->usItem);
	}

	return( TRUE );
}

void SetNewItem( SOLDIERTYPE *pSoldier, UINT8 ubInvPos, BOOLEAN fNewItem )
{
	if( fNewItem )
	{
		pSoldier->inv.bNewItemCount[ ubInvPos ]						 = -1;
		pSoldier->inv.bNewItemCycleCount[ ubInvPos ]			 = NEW_ITEM_CYCLE_COUNT;
		pSoldier->flags.fCheckForNewlyAddedItems             = TRUE;
	}
}


BOOLEAN PlaceObjectInSoldierProfile( UINT8 ubProfile, OBJECTTYPE *pObject )
{
	INT8				bLoop;
	SOLDIERTYPE *pSoldier;
	UINT16			usItem;
	INT16				bStatus;
	BOOLEAN			fReturnVal = FALSE;

	usItem	= pObject->usItem;
	Assert(pObject->ubNumberOfObjects == 1);
	bStatus = (*pObject)[0]->data.objectStatus;
	pSoldier = FindSoldierByProfileID( ubProfile, FALSE );

	if ( Item[ usItem ].usItemClass == IC_MONEY && gMercProfiles[ ubProfile ].uiMoney > 0 )
	{
		gMercProfiles[ ubProfile ].uiMoney += (*pObject)[0]->data.money.uiMoneyAmount;
		SetMoneyInSoldierProfile( ubProfile, gMercProfiles[ ubProfile ].uiMoney );
		return( TRUE );
	}

	// CHRISL:
	for (bLoop = BIGPOCKSTART; bLoop < NUM_INV_SLOTS; bLoop++)
	{
		if ( gMercProfiles[ ubProfile ].bInvNumber[ bLoop ] == 0 && (pSoldier == NULL || pSoldier->inv[ bLoop ].exists() == false ) )
		{

			// CJC: Deal with money by putting money into # stored in profile
			if ( Item[ usItem ].usItemClass == IC_MONEY )
			{
				gMercProfiles[ ubProfile ].uiMoney += (*pObject)[0]->data.money.uiMoneyAmount;
				// change any gold/silver to money
				usItem = MONEY;
			}
			else
			{
				gMercProfiles[ ubProfile ].inv[ bLoop ] = usItem;
				gMercProfiles[ ubProfile ].bInvStatus[ bLoop ] = bStatus;
				gMercProfiles[ ubProfile ].bInvNumber[ bLoop ] = pObject->ubNumberOfObjects;
			}

			fReturnVal = TRUE;
			break;
		}
	}

	//uiMoneyAmount
	if ( fReturnVal )
	{
		// ATE: Manage soldier pointer as well....
		//pSoldier = FindSoldierByProfileID( ubProfile, FALSE );

		// Do we have a valid profile?
		if ( pSoldier != NULL )
		{
			// OK, place in soldier...
			if ( usItem == MONEY )
			{
				CreateMoney( gMercProfiles[ ubProfile ].uiMoney, &(pSoldier->inv[ bLoop ] ) );
			}
			else
			{
				if ( pSoldier->ubProfile == MADLAB )
				{
					// remove ammo and drop
					pSoldier->pTempObject = new OBJECTTYPE;
					EmptyWeaponMagazine( pObject, pSoldier->pTempObject );
					AddItemToPool( pSoldier->sGridNo, pSoldier->pTempObject, 1, 0, 0, 0 );
					pSoldier->pTempObject = NULL;

					// remove attachments and drop them
					/*for (attachmentList::iterator iter = (*pObject)[0]->attachments.begin(); iter != (*pObject)[0]->attachments.end();) 
					{
						//CHRISL: Because MADLAB needs to remove all attachments, even inseparable ones, we need to temporarily make all attachments removable.
						if(!iter->exists())
						{
							++iter;
							continue;
						}

						BOOLEAN	old_inseparable = FALSE;
						UINT32	old_item = iter->usItem;
						// drop it in Madlab's tile
						AddItemToPool( pSoldier->sGridNo, &(*iter), 1, 0, 0, 0 );
						old_inseparable = Item[old_item].inseparable;
						Item[old_item].inseparable = FALSE;
						pObject->RemoveAttachment(&(*iter));
						Item[old_item].inseparable = old_inseparable;

						if ((*pObject)[0]->AttachmentListSize() == 0)
							break;
						else
							iter = (*pObject)[0]->attachments.begin();
					}*/

					// sevenfm: don't remove inseparable attachments
					attachmentList::iterator iter = (*pObject)[0]->attachments.begin();

					while ((*pObject)[0]->AttachmentListSize() > 0 && iter != (*pObject)[0]->attachments.end())
					{
						if (!iter->exists() || Item[iter->usItem].inseparable)
						{
							++iter;
							continue;
						}

						// drop attachment to the ground
						AddItemToPool(pSoldier->sGridNo, &(*iter), 1, 0, 0, 0);
						pObject->RemoveAttachment(&(*iter));
						// start from beginning
						iter = (*pObject)[0]->attachments.begin();
					} 
				}

				CreateItem( usItem, bStatus, &(pSoldier->inv[ bLoop ] ) );
			}
		}
	}

	return( fReturnVal );
}

BOOLEAN RemoveObjectFromSoldierProfile( UINT8 ubProfile, UINT16 usItem )
{
	SOLDIERTYPE *pSoldier;
	BOOLEAN	fReturnVal = FALSE;

	if ( usItem == NOTHING )
	{
		return( TRUE );
	}
	MERCPROFILESTRUCT* pProfile = &gMercProfiles[ ubProfile ];
	for (size_t bLoop = 0, invsize = pProfile->inv.size(); bLoop < invsize; ++bLoop)
	{
		if ( pProfile->inv[ bLoop ] == usItem )
		{
			pProfile->inv[ bLoop ] = NOTHING;
			pProfile->bInvStatus[ bLoop ] = 0;
			pProfile->bInvNumber[ bLoop ] = 0;

			fReturnVal = TRUE;
			break;
		}
	}

	// ATE: Manage soldier pointer as well....
	pSoldier = FindSoldierByProfileID( ubProfile, FALSE );

	// Do we have a valid profile?
	if ( pSoldier != NULL )
	{
		// Remove item...
		RemoveInvObject( pSoldier, usItem );
	}

	return( fReturnVal );
}


void SetMoneyInSoldierProfile( UINT8 ubProfile, UINT32 uiMoney )
{
	BOOLEAN					fRet;

	// remove all money from soldier
	do
	{
		fRet = RemoveObjectFromSoldierProfile( ubProfile, MONEY );
	}
	while ( fRet == TRUE );

	gMercProfiles[ ubProfile ].uiMoney = 0;

	if (uiMoney > 0)
	{
		// now add the amount specified
		CreateMoney( uiMoney, &gTempObject );
		PlaceObjectInSoldierProfile( ubProfile, &gTempObject );
	}
}

INT8 FindObjectInSoldierProfile( UINT8 ubProfile, UINT16 usItem )
{
	MERCPROFILESTRUCT* pProfile = &gMercProfiles[ ubProfile ];
	INT8 invsize = (INT8)pProfile->inv.size();
	for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if ( pProfile->bInvNumber[ bLoop ] > 0 )
		{
			if ( pProfile->inv[ bLoop ] == usItem )
			{
				return( bLoop );
			}
		}
	}
	return( NO_SLOT );
}

BOOLEAN ObjectExistsInSoldierProfile( UINT8 ubProfile, UINT16 usItem )
{
	INT8	bSlot;

	bSlot = FindObjectInSoldierProfile( ubProfile, usItem );
	return( bSlot != NO_SLOT );
}

void RemoveInvObject( SOLDIERTYPE *pSoldier, UINT16 usItem )
{
	INT8 bInvPos;

	// find object
	bInvPos = FindObj( pSoldier, usItem );
	if (bInvPos != NO_SLOT)
	{
		// Erase!
		DeleteObj(&pSoldier->inv[ bInvPos ]);

		//Dirty!
		DirtyMercPanelInterface( pSoldier, DIRTYLEVEL2 );
	}

}

INT8 CheckItemForDamage( UINT16 usItem, INT32 iMaxDamage )
{
	INT8	bDamage = 0;

	// if the item is protective armour, reduce the amount of damage
	// by its armour value
	if (Item[usItem].usItemClass == IC_ARMOUR)
	{
		iMaxDamage -= (iMaxDamage * Armour[Item[usItem].ubClassIndex].ubProtection) / 100;
	}
	// metal items are tough and will be damaged less
//	if (Item[usItem].fFlags & ITEM_METAL)
	if (Item[usItem].metal )
	{
		iMaxDamage /= 2;
	}
	else if ( usItem == BLOODCAT_PELT )
	{
		iMaxDamage *= 2;
	}
	if (iMaxDamage > 0)
	{
		bDamage = (INT8) PreRandom( iMaxDamage );
	}
	return( bDamage );
}

// sevenfm: calculate total average volatility of item+attachments
UINT16 CalcTotalVolatility(OBJECTTYPE * pObj)
{
	attachmentList::iterator iterend;
	attachmentList::iterator iter;
	UINT16 totalVolatility = 0;
	UINT8 num = 1;
	UINT16 usItem;
	UINT16 classIndex;
	UINT16 usBuddyItem;

	if(pObj==NULL)
		return 0;

	usItem = pObj->usItem;
	classIndex = Item[ usItem ].ubClassIndex;

	// search for attached explosives
	iterend = (*pObj)[0]->attachments.end();
	for (iter = (*pObj)[0]->attachments.begin(); iter != iterend; ++iter) 
	{
		if ( iter->exists() && Item[iter->usItem].usItemClass & (IC_GRENADE|IC_BOMB) )
		{ 			
			if( Explosive[Item[iter->usItem].ubClassIndex].ubVolatility > 0 )
			{
				totalVolatility += Explosive[Item[iter->usItem].ubClassIndex].ubVolatility;
				num++;
			}
			else
			{
				// add volatility from chain of buddy items
				usBuddyItem = Item[iter->usItem].usBuddyItem;

				while ( usBuddyItem != 0 )
				{
					if( Explosive[Item[usBuddyItem].ubClassIndex].ubVolatility > 0 )
					{
						totalVolatility += Explosive[Item[usBuddyItem].ubClassIndex].ubVolatility;
						num++;
					}

					usBuddyItem = Item[usBuddyItem].usBuddyItem;
				}
			}
		}
	}

	return Explosive[ classIndex ].ubVolatility + totalVolatility / num;
}

BOOLEAN CheckForChainReaction( OBJECTTYPE * pObj , INT16 bStatus, INT16 bDamage, BOOLEAN fOnGround )
{
	INT32 iChance;

	if(pObj == NULL)
		return FALSE;

	// no chain reaction for tripwire items
	if(Item[pObj->usItem].tripwire)
	{
		return FALSE;
	}

	// sevenfm: calculate total volatility as item volatility + average volatility of explosive attachments
	iChance = CalcTotalVolatility( pObj );

	if (iChance >= 10 || fOnGround && iChance > 0)
	{
		// improve chance to make it practical to blow up explosives on the ground
		if (fOnGround)
		{			
			iChance = 30 + (iChance - 1) * 10;
		}

		// Scale the base chance by the damage caused to the item (bigger the shock, bigger chance) 
		// and the condition of the item after being hit!
		iChance = iChance * ( 100 + ( (100 - bStatus) + bDamage ) / 2 ) / 100;

		if (Chance(iChance))
		{
			return( TRUE );
		}
	}

	return( FALSE );
}

// this checks specifically for action items which are just the trigger for the real item, for example a mine
BOOLEAN CheckForChainReactionActionItem( OBJECTTYPE * pObj, INT16 bDamage)
{
	INT32 iChance;
	UINT16 usItem;

	// we shouldn't be here
	if(pObj == NULL || !( pObj->usItem == ACTION_ITEM && Item[ (*pObj)[0]->data.misc.usBombItem ].usItemClass & IC_EXPLOSV ) )
		return FALSE;

	usItem = (*pObj)[0]->data.misc.usBombItem;

	iChance = Explosive[Item[usItem].ubClassIndex].ubVolatility;
	if (iChance > 0)
	{

		// Scale the base chance by the damage caused to the item
		// (bigger the shock, bigger chance) and the condition of
		// the item after being hit!
		iChance = 50 + (iChance - 1) * 10;

		iChance = iChance * ( 100 + ( (100 - (*pObj)[0]->data.objectStatus) + bDamage ) / 2 ) / 100;
		if ((INT32) PreRandom( 100 ) < iChance)
		{
			return( TRUE );
		}
	}
	return( FALSE );
}

// sevenfm: r7745
BOOLEAN DamageItem( OBJECTTYPE * pObject, INT32 iDamage, BOOLEAN fOnGround, INT32 sGridNo = -1, INT8 bLevel = 0 )
{
	INT8		bLoop;
	INT16		bDamage;

	if ( pObject->exists() && (Item[pObject->usItem].damageable  || Item[ pObject->usItem ].usItemClass == IC_AMMO) )
	//if ( (Item[pObject->usItem].damageable  || Item[ pObject->usItem ].usItemClass == IC_AMMO) && pObject->exists() == true)
	{

		for (bLoop = 0; bLoop < pObject->ubNumberOfObjects; bLoop++)
		{
			bool removed = false;
			// if the status of the item is negative then it's trapped/jammed;
			// leave it alone
			if (pObject->exists() == true && (*pObject)[bLoop]->data.objectStatus > 0)
			{
				bDamage = CheckItemForDamage( pObject->usItem, iDamage );
				switch( pObject->usItem )
				{
					case JAR_CREATURE_BLOOD:
					case JAR:
					case JAR_HUMAN_BLOOD:
					case JAR_ELIXIR:
						if ( PreRandom( bDamage ) > 5 )
						{
							// smash!
							bDamage = (*pObject)[bLoop]->data.objectStatus;
						}
						break;
					default:
						break;
				}
				if ( Item[ pObject->usItem ].usItemClass == IC_AMMO  )
				{
					if ( PreRandom( 100 ) < (UINT32) bDamage )
					{
						// destroy clip completely
						(*pObject)[bLoop]->data.objectStatus = 1;
					}
				}
				else
				{
					(*pObject)[bLoop]->data.sRepairThreshold = max(1, (*pObject)[bLoop]->data.sRepairThreshold - bDamage/3);

					(*pObject)[bLoop]->data.objectStatus -= bDamage;
					if ((*pObject)[bLoop]->data.objectStatus < 1)
					{
						(*pObject)[bLoop]->data.objectStatus = 1;
					}
				}
				// I don't think we increase viewrange based on items any more
				// FUN STUFF!  Check for explosives going off as a result!
				if (Item[pObject->usItem].usItemClass & IC_EXPLOSV)
				{
                                        if (CheckForChainReaction( pObject, (*pObject)[bLoop]->data.objectStatus, bDamage, fOnGround ))
					{
						return( TRUE );
					}
				}

				// remove item from index AFTER checking explosions because need item data for explosion!
				if ( (*pObject)[bLoop]->data.objectStatus == 1 )
				{
					if ( pObject->ubNumberOfObjects > 1 )
					{
						removed = true;
						pObject->RemoveObjectAtIndex( bLoop );
						// since an item was just removed, the items above the current were all shifted down one;
						// to process them properly, we have to back up 1 in the counter
						--bLoop;
					}
				}
			}

			if (removed == false) 
			{
				for (attachmentList::iterator iter = (*pObject)[bLoop]->attachments.begin(); iter != (*pObject)[bLoop]->attachments.end();) 
				{
					DamageItem(&(*iter), iDamage, fOnGround);
					//could have removed the object at iter
					if (iter->exists()) 
					{
						++iter;
					}
					else 
					{
						iter = (*pObject)[bLoop]->RemoveAttachmentAtIter(iter);

						if(UsingNewAttachmentSystem()==true)
							++iter;
					}
				}
			}
		}
	}
	// silversurfer: Action item bombs can now be blown up as well. Happy mine field clearing! ;)
	else if ( pObject->exists() && ( pObject->usItem == ACTION_ITEM && (*pObject)[0]->data.misc.bActionValue == ACTION_ITEM_BLOW_UP ))
	{
		bDamage = CheckItemForDamage( (*pObject)[0]->data.misc.usBombItem, iDamage );

		if ( CheckForChainReactionActionItem( pObject, bDamage) )
		{
			UINT32	uiTimeStamp= GetJA2Clock();
			for (UINT32 uiWorldBombIndex = 0; uiWorldBombIndex < guiNumWorldBombs; uiWorldBombIndex++)
			{
				if ( gWorldBombs[uiWorldBombIndex].fExists && gWorldItems[ gWorldBombs[uiWorldBombIndex].iItemIndex ].sGridNo == sGridNo && gWorldItems[ gWorldBombs[uiWorldBombIndex].iItemIndex ].ubLevel == bLevel )
					AddBombToQueue( uiWorldBombIndex, uiTimeStamp );
			}
		}
	}

	return( FALSE );
}

void CheckEquipmentForDamage( SOLDIERTYPE *pSoldier, INT32 iDamage )
{
	//DebugMiscInfo(String("CheckEquipmentForDamage: [%d] iDamage %d", pSoldier->ubID, iDamage));

	BOOLEAN			fBlowsUp;
	UINT8				ubNumberOfObjects;

	if ( TANK( pSoldier ) )
	{
		return;
	}

	UINT8 invsize = pSoldier->inv.size();
	for (UINT8 bSlot = 0; bSlot < invsize; ++bSlot)
	{
		if (pSoldier->inv[bSlot].exists() == false) {
			continue;
		}
		ubNumberOfObjects = pSoldier->inv[bSlot].ubNumberOfObjects;
		fBlowsUp = DamageItem( &(pSoldier->inv[bSlot]), iDamage, FALSE );
		if (fBlowsUp)
		{
			// blow it up!
			if ( gTacticalStatus.ubAttackBusyCount )
			{
				IgniteExplosion( pSoldier->ubAttackerID, CenterX( pSoldier->sGridNo ), CenterY( pSoldier->sGridNo ), 0, pSoldier->sGridNo, pSoldier->inv[ bSlot ].usItem, pSoldier->pathing.bLevel, pSoldier->ubDirection, &pSoldier->inv[ bSlot ] );
			}
			else
			{
				IgniteExplosion( pSoldier->ubID, CenterX( pSoldier->sGridNo ), CenterY( pSoldier->sGridNo ), 0, pSoldier->sGridNo, pSoldier->inv[ bSlot ].usItem, pSoldier->pathing.bLevel, pSoldier->ubDirection, &pSoldier->inv[ bSlot ] );
			}

			//ADB when something in a stack blows up the whole stack goes, so no need to worry about number of items
			// Remove item!
			DeleteObj( &(pSoldier->inv[ bSlot ]) );

			DirtyMercPanelInterface( pSoldier, DIRTYLEVEL2 );
		}
		else if ( ubNumberOfObjects != pSoldier->inv[bSlot].ubNumberOfObjects )
		{
			DirtyMercPanelInterface( pSoldier, DIRTYLEVEL2 );
		}
	}
}

void CheckEquipmentForFragileItemDamage( SOLDIERTYPE *pSoldier, INT32 iDamage )
{
	// glass jars etc can be damaged by falling over
	UINT8				ubNumberOfObjects;
	BOOLEAN			fPlayedGlassBreak = FALSE;

	UINT8 invsize = pSoldier->inv.size();
	for (UINT8 bSlot = 0; bSlot < invsize; ++bSlot)
	{
		if (pSoldier->inv[bSlot].exists() == false) {
			continue;
		}
		switch( pSoldier->inv[bSlot].usItem )
		{
			case JAR_CREATURE_BLOOD:
			case JAR:
			case JAR_HUMAN_BLOOD:
			case JAR_ELIXIR:
				ubNumberOfObjects = pSoldier->inv[bSlot].ubNumberOfObjects;
				DamageItem( &(pSoldier->inv[bSlot]), iDamage, FALSE );
				if ( !fPlayedGlassBreak && (ubNumberOfObjects != pSoldier->inv[bSlot].ubNumberOfObjects) )
				{
					PlayJA2Sample( GLASS_CRACK, RATE_11025, SoundVolume( MIDVOLUME, pSoldier->sGridNo ), 1, SoundDir( pSoldier->sGridNo ) );
					fPlayedGlassBreak = TRUE;
					// only dirty once
					DirtyMercPanelInterface( pSoldier, DIRTYLEVEL2 );
				}
				break;
			default:
				break;
		}
	}
}


BOOLEAN DamageItemOnGround( OBJECTTYPE * pObject, INT32 sGridNo, INT8 bLevel, INT32 iDamage, UINT8 ubOwner )
{
#ifdef JA2BETAVERSION
	CHAR tmpMPDbgString[512];
	sprintf(tmpMPDbgString,"DamageItemOnGround ( usItem : %i , sGridNo : %i , bLevel : %i , iDamage : %i , ubOwner : %i )\n",pObject->usItem, sGridNo , bLevel , iDamage , ubOwner );
	MPDebugMsg(tmpMPDbgString);
#endif

	BOOLEAN			fBlowsUp;

	fBlowsUp = DamageItem( pObject, iDamage, TRUE );
	if ( fBlowsUp )
	{
		// OK, Ignite this explosion!
		IgniteExplosion( ubOwner, CenterX( sGridNo ), CenterY( sGridNo ), 0, sGridNo, pObject->usItem, bLevel, DIRECTION_IRRELEVANT, pObject );

		// SANDRO - merc records
		if ( (pObject->fFlags & OBJECT_ARMED_BOMB) && ((*pObject)[0]->data.misc.ubBombOwner > 1) )
		{
			if ( MercPtrs[ ((*pObject)[0]->data.misc.ubBombOwner - 2) ]->ubProfile != NO_PROFILE && MercPtrs[ ((*pObject)[0]->data.misc.ubBombOwner - 2) ]->bTeam == gbPlayerNum ) 
				gMercProfiles[ MercPtrs[ ((*pObject)[0]->data.misc.ubBombOwner - 2) ]->ubProfile ].records.usExpDetonated++;
		}

		// Remove item!
		return( TRUE );
	}
	else if ( pObject->exists() == true && (pObject->ubNumberOfObjects < 2) && ((*pObject)[0]->data.objectStatus < USABLE) )
	{
		return( TRUE );
	}
	else
	{
		return( FALSE );
	}
}

// is the item a medical kit/first aid kit item?
INT8 IsMedicalKitItem( OBJECTTYPE *pObject )
{
	// check item id against current medical kits
	if ( Item[pObject->usItem].medicalkit && pObject->exists() == true)
		return 1;

	//switch( pObject->usItem )
	//{
	//	case( MEDICKIT ):
	//		// medical bag, return 1
	//		return ( 1 );
	//	break;
	//}

	return( 0 );
}

void SwapHandItems( SOLDIERTYPE * pSoldier )
{
	BOOLEAN		fOk;

	CHECKV( pSoldier );

	// stop spotting
	pSoldier->StopSpotting();
	// stop focusing
	pSoldier->StopFocusing();

	if (pSoldier->inv[HANDPOS].exists() == false || pSoldier->inv[SECONDHANDPOS].exists() == false)
	{
		// whatever is in the second hand can be swapped to the main hand!
		SwapObjs( pSoldier, HANDPOS, SECONDHANDPOS, TRUE );
		DirtyMercPanelInterface( pSoldier, DIRTYLEVEL2 );
	}
	else
	{
		if (TwoHandedItem( pSoldier->inv[SECONDHANDPOS].usItem ) )
		{
			// must move the item in the main hand elsewhere in the inventory
			fOk = AutoPlaceObject( pSoldier, &(pSoldier->inv[HANDPOS]), FALSE, HANDPOS );
			if (!fOk)
			{
				return;
			}
			// the main hand is now empty so a swap is going to work...
		}
		SwapObjs( pSoldier, HANDPOS, SECONDHANDPOS, TRUE );
		DirtyMercPanelInterface( pSoldier, DIRTYLEVEL2 );
	}

	// Flugente: we have to recheck our flashlights
	pSoldier->HandleFlashLights();

	// sevenfm: refresh sight
	HandleSight(pSoldier, SIGHT_LOOK);
}

void SwapOutHandItem( SOLDIERTYPE * pSoldier )
{
	BOOLEAN			fOk;

	CHECKV( pSoldier );

	// puts away the item in the main hand
	if (pSoldier->inv[HANDPOS].exists() == true )
	{
		if (pSoldier->inv[SECONDHANDPOS].exists() == false)
		{
			// just swap the hand item to the second hand
			SwapObjs( pSoldier, HANDPOS, SECONDHANDPOS, TRUE );
			DirtyMercPanelInterface( pSoldier, DIRTYLEVEL2 );
			return;
		}
		else
		{
			// try placing it somewhere else in our inventory
			fOk = AutoPlaceObject( pSoldier, &(pSoldier->inv[HANDPOS]), FALSE );
			if (fOk)
			{
				DirtyMercPanelInterface( pSoldier, DIRTYLEVEL2 );
			}
			// otherwise there's no room for the item anywhere!
		}
	}

	// Flugente: we have to recheck our flashlights
	pSoldier->HandleFlashLights();

	// sevenfm: refresh sight
	HandleSight(pSoldier, SIGHT_LOOK);
}

void WaterDamage( SOLDIERTYPE *pSoldier )
{
	// damage guy's equipment and camouflage due to water
	INT8		bLoop, bDamage, bDieSize;
	UINT32	uiRoll;

	if ( pSoldier->MercInDeepWater( ) )
	{
		INT8 invsize = (INT8) pSoldier->inv.size();
		for ( bLoop = 0; bLoop < invsize; ++bLoop )
		{
			if (pSoldier->inv[bLoop].exists() == false) {
				continue;
			}
			// if there's an item here that can get water damaged...
//			if (pSoldier->inv[ bLoop ].usItem && Item[pSoldier->inv[ bLoop ].usItem].fFlags & ITEM_WATER_DAMAGES)
			if (pSoldier->inv[ bLoop ].usItem && Item[pSoldier->inv[ bLoop ].usItem].waterdamages )
			{
				// roll the 'ol 100-sided dice
				uiRoll = PreRandom(100);

				// 10% chance of getting damage!
				if (uiRoll < 10)
				{
					// lose between 1 and 10 status points each time
					bDamage = (INT8) (10 - uiRoll);

					// but don't let anything drop lower than 1%
					pSoldier->inv[bLoop][0]->data.objectStatus -= bDamage;
					if (pSoldier->inv[bLoop][0]->data.objectStatus < 1)
					{
						pSoldier->inv[bLoop][0]->data.objectStatus = 1;
					}

					if ( Random(100) < Item[pSoldier->inv[ bLoop ].usItem].usDamageChance )
					{
						pSoldier->inv[bLoop][0]->data.sRepairThreshold = max(1, pSoldier->inv[bLoop][0]->data.sRepairThreshold - 1);
					}
				}
			}
		}
	}
	BOOLEAN camoWoreOff = FALSE;

	/////////////////////////////////////////////////////////////////////////////////////
	// ADDED BY SANDRO - Ranger trait makes camouflage reduction lesser
	// I've messed this part a little to be more clean, the different camouflaged traits have been merged into one
	if ( gGameOptions.fNewTraitSystem)
	{
		// silversurfer: We only allow painting the face and hands now so in medium water camo shouldn't wear off anymore.
		// reduce camouflage by 1% //2% per tile of deep water
		// and 0% //1% for medium water
		if (pSoldier->bCamo > 0 && !ProfileHasCamouflagedTrait(pSoldier->ubProfile))
		{
			if ( HAS_SKILL_TRAIT( pSoldier, RANGER_NT ) )
			{
				if ( pSoldier->MercInDeepWater( ) )
				{
					pSoldier->bCamo -= (Chance(__max(0, 100 - gSkillTraitValues.ubRACamoWornountSpeedReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ))) ? 1 : 0 );
					//pSoldier->bCamo -= (Chance(__max(0, 100 - gSkillTraitValues.ubRACamoWornountSpeedReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ))) ? 1 : 0 );
				}
/*				else
				{
					pSoldier->bCamo -= (Chance(__max(0, 100 - gSkillTraitValues.ubRACamoWornountSpeedReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ))) ? 1 : 0 );
				}*/
			}
			else
			{
				if ( pSoldier->MercInDeepWater( ) )
					pSoldier->bCamo = __max( 0, pSoldier->bCamo - 1 );	// 2
				//else
				//	pSoldier->bCamo = __max( 0, pSoldier->bCamo - 1 );
			}

			if ( pSoldier->bCamo <= 0 )
			{
				pSoldier->bCamo = 0;
				camoWoreOff = TRUE;
				// ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, Message[STR_CAMMO_WASHED_OFF], pSoldier->GetName() );
				ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, Message[STR_JUNGLE_WASHED_OFF], pSoldier->GetName() );
			}
		}
		if (pSoldier->urbanCamo > 0)
		{
			if ( HAS_SKILL_TRAIT( pSoldier, RANGER_NT ) )
			{
				if ( pSoldier->MercInDeepWater( ) )
				{
					pSoldier->urbanCamo -= (Chance(__max(0, 100 - gSkillTraitValues.ubRACamoWornountSpeedReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ))) ? 1 : 0 );
					//pSoldier->urbanCamo -= (Chance(__max(0, 100 - gSkillTraitValues.ubRACamoWornountSpeedReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ))) ? 1 : 0 );
				}
/*				else
				{
					pSoldier->urbanCamo -= (Chance(__max(0, 100 - gSkillTraitValues.ubRACamoWornountSpeedReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ))) ? 1 : 0 );
				}*/
			}
			else
			{
				if ( pSoldier->MercInDeepWater( ) )
					pSoldier->urbanCamo = __max( 0, pSoldier->urbanCamo - 1);	// 2
				//else
				//	pSoldier->urbanCamo = __max( 0, pSoldier->urbanCamo - 1);
			}

			if ( pSoldier->urbanCamo <= 0 )
			{
				pSoldier->urbanCamo = 0;
				camoWoreOff = TRUE;
				ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, Message[STR_URBAN_WASHED_OFF], pSoldier->GetName() );
			}
		}
		if ( pSoldier->desertCamo > 0)
		{
			if ( HAS_SKILL_TRAIT( pSoldier, RANGER_NT ) )
			{
				if ( pSoldier->MercInDeepWater( ) )
				{
					pSoldier->desertCamo -= (Chance(__max(0, 100 - gSkillTraitValues.ubRACamoWornountSpeedReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ))) ? 1 : 0 );
					//pSoldier->desertCamo -= (Chance(__max(0, 100 - gSkillTraitValues.ubRACamoWornountSpeedReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ))) ? 1 : 0 );
				}
/*				else
				{
					pSoldier->desertCamo -= (Chance(__max(0, 100 - gSkillTraitValues.ubRACamoWornountSpeedReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ))) ? 1 : 0 );
				}*/
			}
			else
			{
				if ( pSoldier->MercInDeepWater( ) )
					pSoldier->desertCamo = __max( 0, pSoldier->desertCamo - 1);	// 2
				//else
				//	pSoldier->desertCamo = __max( 0, pSoldier->desertCamo - 1);
			}

			if ( pSoldier->desertCamo <= 0 )
			{
				pSoldier->desertCamo = 0;
				camoWoreOff = TRUE;
				ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, Message[STR_DESERT_WASHED_OFF], pSoldier->GetName() );
			}
		}
		if ( pSoldier->snowCamo > 0)
		{
			if ( HAS_SKILL_TRAIT( pSoldier, RANGER_NT ) )
			{
				if ( pSoldier->MercInDeepWater( ) )
				{
					pSoldier->snowCamo -= (Chance(__max(0, 100 - gSkillTraitValues.ubRACamoWornountSpeedReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ))) ? 1 : 0 );
					//pSoldier->snowCamo -= (Chance(__max(0, 100 - gSkillTraitValues.ubRACamoWornountSpeedReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ))) ? 1 : 0 );
				}
/*				else
				{
					pSoldier->snowCamo -= (Chance(__max(0, 100 - gSkillTraitValues.ubRACamoWornountSpeedReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ))) ? 1 : 0 );
				}*/
			}
			else
			{
				if ( pSoldier->MercInDeepWater( ) )
					pSoldier->snowCamo = __max( 0, pSoldier->snowCamo - 1);	// 2
				//else
				//	pSoldier->snowCamo = __max( 0, pSoldier->snowCamo - 1);
			}

			if ( pSoldier->snowCamo <= 0 )
			{
				pSoldier->snowCamo = 0;
				camoWoreOff = TRUE;
				ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, Message[STR_SNOW_WASHED_OFF], pSoldier->GetName() );
			}
		}
	}
	else if ( !HAS_SKILL_TRAIT( pSoldier, CAMOUFLAGED_OT ) ) // Old Camouflaged trait
	{
		// reduce camouflage by 2% per tile of deep water
		// and 1% for medium water
		if ( pSoldier->bCamo > 0 )
		{
			if ( pSoldier->MercInDeepWater( ) )
				pSoldier->bCamo = __max( 0, pSoldier->bCamo - 1 );	// 2
			//else
			//	pSoldier->bCamo = __max( 0, pSoldier->bCamo - 1 );

			if ( (pSoldier->bCamo)== 0)
			{
				camoWoreOff = TRUE;
				// ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, Message[STR_CAMMO_WASHED_OFF], pSoldier->GetName() );
				ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, Message[STR_JUNGLE_WASHED_OFF], pSoldier->GetName() );
			}
		}
		if ( pSoldier->urbanCamo > 0 )
		{
			if ( pSoldier->MercInDeepWater( ) )
				pSoldier->urbanCamo = __max( 0, pSoldier->urbanCamo - 1);	// 2
			//else
			//	pSoldier->urbanCamo = __max( 0, pSoldier->urbanCamo - 1);
			
			if ( (pSoldier->urbanCamo)== 0)
			{
				camoWoreOff = TRUE;
				ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, Message[STR_URBAN_WASHED_OFF], pSoldier->GetName() );
			}
		}
		if ( pSoldier->desertCamo > 0 )
		{
			if ( pSoldier->MercInDeepWater( ) )
				pSoldier->desertCamo = __max( 0, pSoldier->desertCamo - 1);	// 2
			//else
			//	pSoldier->desertCamo = __max( 0, pSoldier->desertCamo - 1);
			
			if ( (pSoldier->desertCamo)== 0)
			{
				camoWoreOff = TRUE;
				ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, Message[STR_DESERT_WASHED_OFF], pSoldier->GetName() );
			}
		}
		if ( pSoldier->snowCamo > 0 )
		{
			if ( pSoldier->MercInDeepWater( ) )
				pSoldier->snowCamo = __max( 0, pSoldier->snowCamo - 1);	// 2
			//else
			//	pSoldier->snowCamo = __max( 0, pSoldier->snowCamo - 1);
			
			if ( (pSoldier->snowCamo)== 0)
			{
				camoWoreOff = TRUE;
				ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, Message[STR_SNOW_WASHED_OFF], pSoldier->GetName() );
			}
		}	
	}
	/////////////////////////////////////////////////////////////////////////////////////

	if ( camoWoreOff )
	{
		// Reload palettes....
		if ( pSoldier->bInSector )
		{
			pSoldier->CreateSoldierPalettes( );
		}
	}

	if ( pSoldier->bTeam == gbPlayerNum && pSoldier->aiData.bMonsterSmell > 0 )
	{
		if ( pSoldier->MercInDeepWater( ) )
		{
			bDieSize = 10;
		}
		else
		{
			bDieSize = 20;
		}
		if ( Random( bDieSize ) == 0 )
		{
			pSoldier->aiData.bMonsterSmell--;
		}
	}

	DirtyMercPanelInterface( pSoldier, DIRTYLEVEL2 );
}

BOOLEAN ApplyCammo( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj, BOOLEAN *pfGoodAPs, BOOLEAN fUseAPs )
{
	// Added - SANDRO
	INT8	bPointsToUse;
	UINT16	usTotalKitPoints;
	UINT16	iRemainingCamoAfterRemoving; 

	(*pfGoodAPs) = TRUE;

	if (pObj->exists() == false)
		return( FALSE );

	//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"apply camo %d to %s", pObj->usItem, pSoldier->GetName());

	// sevenfm: cannot apply camo to certain mercs
	/*if( ProfileHasCamouflagedTrait( pSoldier->ubProfile ) )
	{
		return FALSE;
	}*/

	// certain profiles (Spooky for example) don't allow to change camo
	if (pSoldier->HasBackgroundFlag(BACKGROUND_PERMANENT_CAMO))
		return FALSE;

	//////////////////////////////////////////////////////////////////////////////
	// added possibility to remove all camo by using a rag on self - SANDRO
	if (HasItemFlag(pObj->usItem, CAMO_REMOVAL))
	{
		if ( fUseAPs && !EnoughPoints( pSoldier, (APBPConstants[AP_CAMOFLAGE]/2), 0, TRUE ) )
		{
			(*pfGoodAPs) = FALSE;
			return( TRUE );
		}
		// 100 should be enough. The third value "0" means we will remove all types of camo
		ReduceCamoFromSoldier( pSoldier, 100, 0 );

		// damage the rag :) - actually you would need to flag it damagable in the items.XML
		DamageItem( pObj, 22, FALSE );

		if ( fUseAPs )
			DeductPoints( pSoldier, (APBPConstants[AP_CAMOFLAGE] / 2), 0 );

		// Reload palettes....
		if ( pSoldier->bInSector )
		{
			pSoldier->CreateSoldierPalettes( );
		}
		return( TRUE );
	}
	else if ( !Item[pObj->usItem].camouflagekit )
	{
		return( FALSE );
	}
	//////////////////////////////////////////////////////////////////////////////

	if ( fUseAPs && !EnoughPoints( pSoldier, APBPConstants[AP_CAMOFLAGE], 0, TRUE ) )
	{
		(*pfGoodAPs) = FALSE;
		return( TRUE );
	}

	usTotalKitPoints = TotalPoints( pObj );
	if (usTotalKitPoints == 0)
	{
		// HUH???
		return( FALSE );
	}

	// sevenfm: check if we have any camo bonus on kit
	if (Item[pObj->usItem].camobonus <= 0 &&
		Item[pObj->usItem].urbanCamobonus <= 0 &&
		Item[pObj->usItem].desertCamobonus <= 0 &&
		Item[pObj->usItem].snowCamobonus <= 0)
	{
		return(FALSE);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// SANDRO - Added a feature to determine which type of camo we use								//
	// and procedure able to reduce different type of camo											//
	// ******************************************************************************************** //

	int iJungleCamoTotal = pSoldier->bCamo + pSoldier->wornCamo;
	int iUrbanCamoTotal = pSoldier->urbanCamo + pSoldier->wornUrbanCamo;
	int iDesertCamoTotal = pSoldier->desertCamo + pSoldier->wornDesertCamo;
	int iSnowCamoTotal = pSoldier->snowCamo + pSoldier->wornSnowCamo;

	int totalCamo = iJungleCamoTotal + iUrbanCamoTotal + iDesertCamoTotal + iSnowCamoTotal;
	int iTotalKitCamo = pSoldier->bCamo + pSoldier->urbanCamo + pSoldier->desertCamo + pSoldier->snowCamo;
	int iTotalWornCamo = pSoldier->wornCamo + pSoldier->wornUrbanCamo + pSoldier->wornDesertCamo + pSoldier->wornSnowCamo;
	int iMaxKitCamo = gGameExternalOptions.bCamoKitArea;	

	/*int iTotalKitBonus = Item[pObj->usItem].camobonus + Item[pObj->usItem].urbanCamobonus + Item[pObj->usItem].desertCamobonus + Item[pObj->usItem].snowCamobonus;

	// determine percent of kit to use
	int iJungleKitUse = Item[pObj->usItem].camobonus * Item[pObj->usItem].camobonus / iTotalKitBonus;
	int iUrbanKitUse = Item[pObj->usItem].urbanCamobonus * Item[pObj->usItem].urbanCamobonus / iTotalKitBonus;
	int iDesertKitUse = Item[pObj->usItem].desertCamobonus * Item[pObj->usItem].desertCamobonus / iTotalKitBonus;
	int iSnowKitUse = Item[pObj->usItem].snowCamobonus * Item[pObj->usItem].snowCamobonus / iTotalKitBonus;*/

	//ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"iJungleCamoTotal %d totalCamo %d iTotalKitCamo %d iTotalWornCamo %d iMaxKitCamo %d", iJungleCamoTotal, totalCamo, iTotalKitCamo, iTotalWornCamo, iMaxKitCamo);

	// First, check if we have an item with major JUNGLE camobonus
	if ((Item[pObj->usItem].camobonus >= Item[pObj->usItem].urbanCamobonus) &&
		(Item[pObj->usItem].camobonus >= Item[pObj->usItem].desertCamobonus) &&
		(Item[pObj->usItem].camobonus >= Item[pObj->usItem].snowCamobonus))
	{
		if (iJungleCamoTotal >= 100)
			return(FALSE);

		// sevenfm: also limit max camo from kit
		if (pSoldier->bCamo >= iMaxKitCamo)
			return(FALSE);

		// determine how much we can add
		bPointsToUse = 100 - iJungleCamoTotal;
		bPointsToUse = min(bPointsToUse, usTotalKitPoints);

		// determine how much we will add
		int iJungleCamoAdded = (int)(Item[pObj->usItem].camobonus * bPointsToUse / 100);

		// sevenfm: limit how much we can add
		iJungleCamoAdded = min(iJungleCamoAdded, iMaxKitCamo - pSoldier->bCamo);

		// sevenfm: remove extra camo
		int iCamoToRemove = iJungleCamoAdded + iTotalKitCamo - iMaxKitCamo;

		// if we have already too much different camo on ourselves, reduce some or all
		if (iCamoToRemove > 0)
		{
			// reduce camo.. IMPORTANT - third value is type of camo to skip (0=skip-none, 1=jungle, 2=urban, 3=desert,4=snow)
			iRemainingCamoAfterRemoving = ReduceCamoFromSoldier(pSoldier, iCamoToRemove, 1); // "1" - we want to keep the JUNGLE camo

			// if some camo haven't been reduced from us, the camo to add will be lessened by that value
			if (iRemainingCamoAfterRemoving > 0)
				iJungleCamoAdded = max(0, (iJungleCamoAdded - iRemainingCamoAfterRemoving));

			if (iJungleCamoAdded <= 0) // if we have nothing to add now, return
				return(FALSE);
			else // otherwise apply the camo
			{
				pSoldier->bCamo = min(iMaxKitCamo, pSoldier->bCamo + iJungleCamoAdded);
			}
		}
		else // everything's fine, apply!!
		{
			pSoldier->bCamo = min(iMaxKitCamo, pSoldier->bCamo + iJungleCamoAdded);
		}
		// update with amount that we really used
		bPointsToUse = min(bPointsToUse, iJungleCamoAdded);
	}
	// Second, check if we have an item with major URBAN camobonus
	else if ((Item[pObj->usItem].urbanCamobonus >= Item[pObj->usItem].camobonus) &&
		(Item[pObj->usItem].urbanCamobonus >= Item[pObj->usItem].desertCamobonus) &&
		(Item[pObj->usItem].urbanCamobonus >= Item[pObj->usItem].snowCamobonus))
	{
		if (iUrbanCamoTotal >= 100)
			return(FALSE);

		// sevenfm: also limit max camo from kit
		if (pSoldier->urbanCamo >= iMaxKitCamo)
			return(FALSE);

		// determine how much we can add
		bPointsToUse = 100 - iUrbanCamoTotal;
		bPointsToUse = min(bPointsToUse, usTotalKitPoints);

		// determine how much we will add
		int iUrbanCamoAdded = (int)(Item[pObj->usItem].urbanCamobonus * bPointsToUse / 100);

		// sevenfm: limit how much we can add
		iUrbanCamoAdded = min(iUrbanCamoAdded, iMaxKitCamo - pSoldier->urbanCamo);

		// sevenfm: remove extra camo
		int iCamoToRemove = iUrbanCamoAdded + iTotalKitCamo - iMaxKitCamo;

		// if we have already too much different camo on ourselves, reduce some or all
		if (iCamoToRemove > 0)
		{
			// reduce camo.. IMPORTANT - third value is type of camo to skip (0=skip-none, 1=jungle, 2=urban, 3=desert,4=snow)
			iRemainingCamoAfterRemoving = ReduceCamoFromSoldier(pSoldier, iCamoToRemove, 2); // "2" - we want to keep the URBAN camo

			// if some camo haven't been reduced from us, the camo to add will be lessened by that value
			if (iRemainingCamoAfterRemoving > 0)
				iUrbanCamoAdded = max(0, (iUrbanCamoAdded - iRemainingCamoAfterRemoving));

			if (iUrbanCamoAdded <= 0) // if we have nothing to add now, return
				return(FALSE);
			else // otherwise apply the camo
			{
				pSoldier->urbanCamo = min(iMaxKitCamo, pSoldier->urbanCamo + iUrbanCamoAdded);
			}
		}
		else // everything's fine, apply!!
		{
			pSoldier->urbanCamo = min(iMaxKitCamo, pSoldier->urbanCamo + iUrbanCamoAdded);
		}
		// update with amount that we really used
		bPointsToUse = min(bPointsToUse, iUrbanCamoAdded);
	}
	// Third, check if we have an item with major DESERT camobonus
	else if ((Item[pObj->usItem].desertCamobonus >= Item[pObj->usItem].camobonus) &&
		(Item[pObj->usItem].desertCamobonus >= Item[pObj->usItem].urbanCamobonus) &&
		(Item[pObj->usItem].desertCamobonus >= Item[pObj->usItem].snowCamobonus))
	{
		if (iDesertCamoTotal >= 100)
			return(FALSE);

		// sevenfm: also limit max camo from kit
		if (pSoldier->desertCamo >= iMaxKitCamo)
			return(FALSE);

		// determine how much we can add
		bPointsToUse = 100 - iDesertCamoTotal;
		bPointsToUse = min(bPointsToUse, usTotalKitPoints);

		// determine how much we will add
		int iDesertCamoAdded = (int)(Item[pObj->usItem].desertCamobonus * bPointsToUse / 100);

		// sevenfm: limit how much we can add
		iDesertCamoAdded = min(iDesertCamoAdded, iMaxKitCamo - pSoldier->desertCamo);

		// sevenfm: remove extra camo
		int iCamoToRemove = iDesertCamoAdded + iTotalKitCamo - iMaxKitCamo;

		// if we have already too much different camo on ourselves, reduce some or all
		if (iCamoToRemove > 0)
		{
			// reduce camo.. IMPORTANT - third value is type of camo to skip (0=skip-none, 1=jungle, 2=urban, 3=desert,4=snow)
			iRemainingCamoAfterRemoving = ReduceCamoFromSoldier(pSoldier, iCamoToRemove, 3); // "3" - we want to keep the DESERT camo

			// if some camo haven't been reduced from us, the camo to add will be lessened by that value
			if (iRemainingCamoAfterRemoving > 0)
				iDesertCamoAdded = max(0, (iDesertCamoAdded - iRemainingCamoAfterRemoving));

			if (iDesertCamoAdded <= 0) // if we have nothing to add now, return
				return(FALSE);
			else // otherwise apply the camo
			{
				pSoldier->desertCamo = min(iMaxKitCamo, pSoldier->desertCamo + iDesertCamoAdded);
			}
		}
		else // everything's fine, apply!!
		{
			pSoldier->desertCamo = min(iMaxKitCamo, pSoldier->desertCamo + iDesertCamoAdded);
		}
		// update with amount that we really used
		bPointsToUse = min(bPointsToUse, iDesertCamoAdded);
	}
	// Fourth, check if we have an item with major SNOW camobonus
	else if ((Item[pObj->usItem].snowCamobonus >= Item[pObj->usItem].camobonus) &&
		(Item[pObj->usItem].snowCamobonus >= Item[pObj->usItem].urbanCamobonus) &&
		(Item[pObj->usItem].snowCamobonus >= Item[pObj->usItem].desertCamobonus))
	{
		if (iSnowCamoTotal >= 100)
			return(FALSE);

		// sevenfm: also limit max camo from kit
		if (pSoldier->snowCamo >= iMaxKitCamo)
			return(FALSE);

		// determine how much we can add
		bPointsToUse = 100 - iSnowCamoTotal;
		bPointsToUse = min(bPointsToUse, usTotalKitPoints);

		// determine how much we will add
		int iSnowCamoAdded = (int)(Item[pObj->usItem].snowCamobonus * bPointsToUse / 100);

		// sevenfm: limit how much we can add
		iSnowCamoAdded = min(iSnowCamoAdded, iMaxKitCamo - pSoldier->snowCamo);

		// sevenfm: remove extra camo
		int iCamoToRemove = iSnowCamoAdded + iTotalKitCamo - iMaxKitCamo;

		// if we have already too much different camo on ourselves, reduce some or all
		if (iCamoToRemove > 0)
		{
			// reduce camo.. IMPORTANT - third value is type of camo to skip (0=skip-none, 1=jungle, 2=urban, 3=desert,4=snow)
			iRemainingCamoAfterRemoving = ReduceCamoFromSoldier(pSoldier, iCamoToRemove, 4); // "4" - we want to keep the SNOW camo

			// if some camo haven't been reduced from us, the camo to add will be lessened by that value
			if (iRemainingCamoAfterRemoving > 0)
				iSnowCamoAdded = max(0, (iSnowCamoAdded - iRemainingCamoAfterRemoving));

			if (iSnowCamoAdded <= 0) // if we have nothing to add now, return
				return(FALSE);
			else // otherwise apply the camo
			{
				pSoldier->snowCamo = min(iMaxKitCamo, pSoldier->snowCamo + iSnowCamoAdded);
			}
		}
		else // everything's fine, apply!!
		{
			pSoldier->snowCamo = min(iMaxKitCamo, pSoldier->snowCamo + iSnowCamoAdded);
		}
		// update with amount that we really used
		bPointsToUse = min(bPointsToUse, iSnowCamoAdded);
	}
	else // the item has no major camo, return
		return(FALSE);

	// ****************************************************************************************** //
	////////////////////////////////////////////////////////////////////////////////////////////////

	UseKitPoints( pObj, bPointsToUse, pSoldier );

	if ( fUseAPs )
		DeductPoints( pSoldier, APBPConstants[AP_CAMOFLAGE], 0 );

	// Reload palettes....
	if ( pSoldier->bInSector )
	{
		pSoldier->CreateSoldierPalettes( );
	}

	return( TRUE );
}

// Flugente: apply clothes, and eventually disguise
BOOLEAN ApplyClothes( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj, BOOLEAN fUseAPs )
{
	// this will only work with the new trait system
	if (!gGameOptions.fNewTraitSystem)
	{
		ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, szCovertTextStr[STR_COVERT_ERROR_OLDTRAITS] );
		return FALSE;
	}

	if ( !pSoldier || pObj->exists() == false )
		return( FALSE );

	UINT8 skilllevel = NUM_SKILL_TRAITS( pSoldier, COVERT_NT );

	INT16 apcost = (APBPConstants[AP_DISGUISE] * ( 100 - gSkillTraitValues.sCODisguiseAPReduction * skilllevel))/100;
	if ( fUseAPs && !EnoughPoints( pSoldier, apcost, 0, TRUE ) )
	{
		ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, szCovertTextStr[STR_COVERT_NOT_ENOUGH_APS] );
		return( FALSE );
	}

	// determine clothes type
	UINT32 clothestype = Item[pObj->usItem].clothestype;

	// if not a clothes item, nothing to see here
	if ( clothestype == 0 || clothestype > CLOTHES_MAX )
	{
		// silversurfer: This produces an annoying message whenever any invalid item is used on the char so I removed it.
		//ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, szCovertTextStr[STR_COVERT_NO_UNIFORM_FOUND] );
		return( FALSE);
	}
	
	UINT8 filler = 0;
	// if it has a vest, wear it
	bool newvest  = FALSE;	
	if ( GetPaletteRepIndexFromID(Clothes[clothestype].vest, &filler) )
		newvest = TRUE;

	// if it has pants, wear them
	bool newpants = FALSE;
	if ( GetPaletteRepIndexFromID(Clothes[clothestype].pants, &filler) )
		newpants = TRUE;
	
	if ( newvest || newpants )
	{
		// sevenfm: loose disguise every time we apply clothes
		pSoldier->LooseDisguise();
		// also stop auto disguise
		pSoldier->usSoldierFlagMask2 &= ~SOLDIER_COVERT_AUTO_DISGUISE;

		UINT16 usPaletteAnimSurface = LoadSoldierAnimationSurface( pSoldier, pSoldier->usAnimState );

		if ( usPaletteAnimSurface != INVALID_ANIMATION_SURFACE )
		{
			if ( newvest )
			{
				// if we are already wearing a vest, give us back that item
				if ( !(pSoldier->usSoldierFlagMask & SOLDIER_DAMAGED_VEST) )
				{
					UINT16 vestitem = 0;
					if ( GetFirstClothesItemWithSpecificData(&vestitem, pSoldier->VestPal, "blank")  )
					{
						CreateItem( vestitem, 100, &gTempObject );
						if ( !AutoPlaceObject( pSoldier, &gTempObject, FALSE ) )
							AddItemToPool( pSoldier->sGridNo, &gTempObject, 1, 0, 0, -1 );
					}
					else
					{
						// sevenfm: removed to allow using new camo palette without defining all new clothes in XML
						//ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, szCovertTextStr[STR_COVERT_NO_CLOTHES_ITEM] );
					}
				}

				SET_PALETTEREP_ID( pSoldier->VestPal, Clothes[clothestype].vest );
				
				// this vest is not damaged, so remove the damaged vest flag
				pSoldier->usSoldierFlagMask &= ~SOLDIER_DAMAGED_VEST;
			}

			if ( newpants )
			{
				// if we are already wearing pants, give us back that item
				if ( !(pSoldier->usSoldierFlagMask & SOLDIER_DAMAGED_PANTS) )
				{
					UINT16 pantsitem = 0;
					if ( GetFirstClothesItemWithSpecificData(&pantsitem, "blank", pSoldier->PantsPal)  )
					{
						CreateItem( pantsitem, 100, &gTempObject );
						if ( !AutoPlaceObject( pSoldier, &gTempObject, FALSE ) )
							AddItemToPool( pSoldier->sGridNo, &gTempObject, 1, 0, 0, -1 );
					}
					else
					{
						// sevenfm: removed to allow using new camo palette without defining all new clothes in XML
						//ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, szCovertTextStr[STR_COVERT_NO_CLOTHES_ITEM] );
					}
				}

				SET_PALETTEREP_ID( pSoldier->PantsPal, Clothes[clothestype].pants );

				// these pants are not damaged, so remove the damaged pants flag
				pSoldier->usSoldierFlagMask &= ~SOLDIER_DAMAGED_PANTS;
			}

			// Use palette from HVOBJECT, then use substitution for pants, etc
			memcpy( pSoldier->p8BPPPalette, gAnimSurfaceDatabase[ usPaletteAnimSurface ].hVideoObject->pPaletteEntry, sizeof( pSoldier->p8BPPPalette ) * 256 );

			SetPaletteReplacement( pSoldier->p8BPPPalette, pSoldier->HeadPal );
			SetPaletteReplacement( pSoldier->p8BPPPalette, pSoldier->VestPal );
			SetPaletteReplacement( pSoldier->p8BPPPalette, pSoldier->PantsPal );
			SetPaletteReplacement( pSoldier->p8BPPPalette, pSoldier->SkinPal );

			pSoldier->CreateSoldierPalettes();

			UseKitPoints( pObj, 100, pSoldier );

			if ( fUseAPs )
				DeductPoints( pSoldier, apcost, 0 );
		}
	}
		
	return( TRUE );
}

BOOLEAN ApplyCanteen(SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj, BOOLEAN *pfGoodAPs, BOOLEAN fUseAPs, BOOLEAN fHydrationBladder)
{
	INT16	sPointsToUse;
	INT16	sPointsToRestore;
	UINT16	usTotalKitPoints;
	INT16 sDrinkAP = fHydrationBladder ? APBPConstants[AP_DRINK] / 2 : APBPConstants[AP_DRINK];

	(*pfGoodAPs) = TRUE;

	if (!Item[pObj->usItem].canteen || pObj->exists() == false)
	{
		return( FALSE );
	}

	usTotalKitPoints = TotalPoints( pObj );
	// sevenfm: canteen with status = 1 is considered empty
	if (usTotalKitPoints <= 1)
	{
		// HUH???
		return( FALSE );
	}

	// sevenfm: don't allow more than one canteen use/turn
	/*if ((pSoldier->usSoldierFlagMask2 & SOLDIER_USED_CANTEEN))
	{
		return FALSE;
	}*/

	// sevenfm: display message when using APs only for player mercs
	if (fUseAPs && !EnoughPoints(pSoldier, sDrinkAP, 0, pSoldier->IsSoldierPC()))
	{
		(*pfGoodAPs) = FALSE;
		return( TRUE );
	}

	if (pSoldier->bTeam == gbPlayerNum)
	{
		if (gMercProfiles[pSoldier->ubProfile].bSex == MALE)
			PlayJA2Sample(DRINK_CANTEEN_MALE, RATE_11025, LOWVOLUME, 1, MIDDLEPAN);
		else
			PlayJA2Sample(DRINK_CANTEEN_FEMALE, RATE_11025, LOWVOLUME, 1, MIDDLEPAN);
	}
	else
	{
		if (pSoldier->ubBodyType < REGFEMALE)
			PlayJA2Sample(DRINK_CANTEEN_MALE, RATE_11025, SoundVolume(LOWVOLUME, pSoldier->sGridNo), 1, SoundDir(pSoldier->sGridNo));
		else if (pSoldier->ubBodyType == REGFEMALE)
			PlayJA2Sample(DRINK_CANTEEN_FEMALE, RATE_11025, SoundVolume(LOWVOLUME, pSoldier->sGridNo), 1, SoundDir(pSoldier->sGridNo));

		if (pSoldier->bVisible == TRUE)
			ShowRadioLocator(pSoldier->ubID, SHOW_LOCATOR_NOISE);
	}

	UINT32 foodtype = Item[pObj->usItem].foodtype;
	if (foodtype == 0 || foodtype > FOOD_TYPE_MAX)
		sPointsToUse = 20;	// default value for canteens
	else
		sPointsToUse = max(1, Food[foodtype].ubPortionSize);

	sPointsToUse = min(sPointsToUse, usTotalKitPoints);	

	// sevenfm: always restore same amount of breath when using any canteen
	sPointsToRestore = 20;

	// sevenfm: second use of canteen is less effective
	//if ((pSoldier->usSoldierFlagMask2 & SOLDIER_USED_CANTEEN))
		//sPointsToRestore = sPointsToRestore / 2;

	// CJC Feb 9.  Canteens don't seem effective enough, so doubled return from them
	//DeductPoints( pSoldier, 0, (INT16)(2 * usPointsToUse * -(100 - pSoldier->bBreath)) );
	// sevenfm: less penalty for breath restoring with high breath level
	if (fUseAPs)
		DeductPoints(pSoldier, sDrinkAP, (INT16)(2 * sPointsToRestore * -(100 - pSoldier->bBreath / 2)));
	else
		DeductPoints(pSoldier, 0, (INT16)(2 * sPointsToRestore * -(100 - pSoldier->bBreath / 2)));

	UseKitPoints( pObj, sPointsToUse, pSoldier );

	// remember we used canteen this turn
	pSoldier->usSoldierFlagMask2 |= SOLDIER_USED_CANTEEN;

	return TRUE;
}

#define MAX_HUMAN_CREATURE_SMELL (NORMAL_HUMAN_SMELL_STRENGTH - 1)

BOOLEAN ApplyElixir( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj, BOOLEAN *pfGoodAPs )
{
	INT16		sPointsToUse;
	UINT16	usTotalKitPoints;

	(*pfGoodAPs) = TRUE;

	if (pObj->usItem != JAR_ELIXIR || pObj->exists() == false)
	{
		return( FALSE );
	}

	usTotalKitPoints = TotalPoints( pObj );
	if (usTotalKitPoints == 0)
	{
		// HUH???
		return( FALSE );
	}

	if (!EnoughPoints( pSoldier, APBPConstants[AP_CAMOFLAGE], 0, TRUE ) )
	{
    (*pfGoodAPs) = FALSE;
		return( TRUE );
	}

	DeductPoints( pSoldier, APBPConstants[AP_CAMOFLAGE], 0 );

	sPointsToUse = ( MAX_HUMAN_CREATURE_SMELL - pSoldier->aiData.bMonsterSmell ) * 2;
	sPointsToUse = __min( sPointsToUse, usTotalKitPoints );

	UseKitPoints( pObj, sPointsToUse, pSoldier );

	pSoldier->aiData.bMonsterSmell += sPointsToUse / 2;

	return( TRUE );
}

UINT32 ConvertProfileMoneyValueToObjectTypeMoneyValue( UINT8 ubStatus )
{
	return( ubStatus * 50 );
}

UINT8 ConvertObjectTypeMoneyValueToProfileMoneyValue( UINT32 uiMoneyAmount )
{
	return( (UINT8)( uiMoneyAmount / 50 ) );
}

BOOLEAN ItemIsCool( OBJECTTYPE * pObj )
{
	if (pObj->exists() == false) {
		return FALSE;
	}
	if ((*pObj)[0]->data.objectStatus < 60)
	{
		return( FALSE );
	}
	if ( Item[ pObj->usItem ].usItemClass & IC_WEAPON )
	{
		if ( Weapon[ pObj->usItem ].ubDeadliness >= 30 )
		{
			return( TRUE );
		}
	}
	else if ( Item[ pObj->usItem ].usItemClass & IC_ARMOUR )
	{
		if ( Armour[ Item[ pObj->usItem ].ubClassIndex ].ubProtection >= 20 )
		{
			return( TRUE );
		}
	}

	return( FALSE );
}

void ActivateXRayDevice( SOLDIERTYPE * pSoldier )
{
	SOLDIERTYPE *	pSoldier2;
	UINT32				uiSlot;

	if ( Item[pSoldier->inv[ HANDPOS ].usItem].needsbatteries && pSoldier->inv[ HANDPOS ].exists() == true)
	{
		// check for batteries
		OBJECTTYPE* pBatteries = FindAttachedBatteries( &(pSoldier->inv[HANDPOS]) );
		if ( pBatteries == 0 )
		{
			// doesn't work without batteries!
			pSoldier->DoMercBattleSound( BATTLE_SOUND_CURSE1 );
			return;
		}

		// use up 8-12 percent of batteries
		if ( Item[pBatteries->usItem].percentstatusdrainreduction > 0 )
			(*pBatteries)[0]->data.objectStatus -= (INT8)( (8 + Random( 5 )) * (100 - Item[(*pBatteries)[0]->data.objectStatus].percentstatusdrainreduction)/100 );
		else
			(*pBatteries)[0]->data.objectStatus -= (INT8)( (8 + Random( 5 )) );

		if ( (*pBatteries)[0]->data.objectStatus <= 0 )
		{
			// destroy batteries
			pBatteries->RemoveObjectsFromStack(1);
			if (pBatteries->exists() == false) {
				pSoldier->inv[HANDPOS].RemoveAttachment(pBatteries);
			}
		}
	}

	// sevenfm: moved here from HandleItem, play only if device is activated (has battery)
	PlayJA2Sample( USE_X_RAY_MACHINE, RATE_11025, SoundVolume( HIGHVOLUME, pSoldier->sGridNo ), 1, SoundDir( pSoldier->sGridNo ) );

	// sevenfm: alternative way of using XRay device - show locator
	UINT8 ubDirection;
	for ( uiSlot = 0; uiSlot < guiNumMercSlots; uiSlot++ )
	{
		pSoldier2 = MercSlots[ uiSlot ];
		if ( pSoldier2 )
		{
			if( pSoldier2->bActive &&
				pSoldier2->stats.bLife >= OKLIFE &&
				PythSpacesAway( pSoldier->sGridNo, pSoldier2->sGridNo ) <= TACTICAL_RANGE / 2 )
			{
				// should be looking in this direction or CD or CCD direction
				ubDirection = atan8(CenterX(pSoldier->sGridNo),CenterY(pSoldier->sGridNo),CenterX(pSoldier2->sGridNo),CenterY(pSoldier2->sGridNo));

				if( pSoldier->ubDirection == ubDirection ||
					pSoldier->ubDirection == gOneCDirection[ubDirection] ||
					pSoldier->ubDirection == gOneCCDirection[ubDirection] )
				{
					/*if (ValidOpponent(pSoldier, pSoldier2) &&
						PersonalKnowledge(pSoldier, pSoldier2->ubID) != SEEN_CURRENTLY && 
						PublicKnowledge(pSoldier->bTeam, pSoldier2->ubID) != SEEN_CURRENTLY)
					{
						pSoldier->aiData.bOppList[pSoldier2->ubID] = HEARD_THIS_TURN;
						gsLastKnownOppLoc[pSoldier->ubID][pSoldier2->ubID] = pSoldier2->sGridNo;
						gbLastKnownOppLevel[pSoldier->ubID][pSoldier2->ubID] = pSoldier2->pathing.bLevel;
					}*/
					ShowRadioLocator( pSoldier2->ubID, SHOW_LOCATOR_NORMAL );
				}					
			}
		}
	}
	// X-Ray is dangerous device, can cause health loss
	pSoldier->stats.bLife = max(OKLIFE, pSoldier->stats.bLife - PreRandom(3));
	
	// first, scan through all mercs and turn off xrayed flag for anyone
	// previously xrayed by this guy
	/*for ( uiSlot = 0; uiSlot < guiNumMercSlots; uiSlot++ )
	{
		pSoldier2 = MercSlots[ uiSlot ];
		if ( pSoldier2 )
		{
			if ( (pSoldier2->ubMiscSoldierFlags & SOLDIER_MISC_XRAYED) && (pSoldier2->aiData.ubXRayedBy == pSoldier->ubID) )
			{
				pSoldier2->ubMiscSoldierFlags &= (~SOLDIER_MISC_XRAYED);
				pSoldier2->aiData.ubXRayedBy = NOBODY;
			}
		}
	}
	// now turn on xray for anyone within range
	for ( uiSlot = 0; uiSlot < guiNumMercSlots; uiSlot++ )
	{
		pSoldier2 = MercSlots[ uiSlot ];
		if ( pSoldier2 )
		{
			if ( pSoldier2->bTeam != pSoldier->bTeam && PythSpacesAway( pSoldier->sGridNo, pSoldier2->sGridNo ) < XRAY_RANGE )
			{
				pSoldier2->ubMiscSoldierFlags |= SOLDIER_MISC_XRAYED;
				pSoldier2->aiData.ubXRayedBy = pSoldier->ubID;
			}
		}
	}
	pSoldier->uiXRayActivatedTime = GetWorldTotalSeconds();*/
	
}

void TurnOnXRayEffects( SOLDIERTYPE * pSoldier )
{
	SOLDIERTYPE *	pSoldier2;
	UINT32				uiSlot;

	// first, scan through all mercs and turn off xrayed flag for anyone
	// previously xrayed by this guy
	for ( uiSlot = 0; uiSlot < guiNumMercSlots; ++uiSlot )
	{
		pSoldier2 = MercSlots[ uiSlot ];
		if ( pSoldier2 )
		{
			if ( (pSoldier2->ubMiscSoldierFlags & SOLDIER_MISC_XRAYED) && (pSoldier2->aiData.ubXRayedBy == pSoldier->ubID) )
			{
				pSoldier2->ubMiscSoldierFlags &= (~SOLDIER_MISC_XRAYED);
				pSoldier2->aiData.ubXRayedBy = NOBODY;
			}
		}
	}
	// now turn on xray for anyone within range
	for ( uiSlot = 0; uiSlot < guiNumMercSlots; ++uiSlot )
	{
		pSoldier2 = MercSlots[ uiSlot ];
		if ( pSoldier2 )
		{
			if ( pSoldier2->bTeam != pSoldier->bTeam && PythSpacesAway( pSoldier->sGridNo, pSoldier2->sGridNo ) < XRAY_RANGE )
			{
				pSoldier2->ubMiscSoldierFlags |= SOLDIER_MISC_XRAYED;
				pSoldier2->aiData.ubXRayedBy = pSoldier->ubID;
			}
		}
	}
	pSoldier->uiXRayActivatedTime = GetWorldTotalSeconds();
}

void TurnOffXRayEffects( SOLDIERTYPE * pSoldier )
{
	SOLDIERTYPE *	pSoldier2;
	UINT32				uiSlot;

	if ( !pSoldier->uiXRayActivatedTime )
	{
		return;
	}

	// scan through all mercs and turn off xrayed flag for anyone
	// xrayed by this guy
	for ( uiSlot = 0; uiSlot < guiNumMercSlots; uiSlot++ )
	{
		pSoldier2 = MercSlots[ uiSlot ];
		if ( pSoldier2 )
		{
			if ( (pSoldier2->ubMiscSoldierFlags & SOLDIER_MISC_XRAYED) && (pSoldier2->aiData.ubXRayedBy == pSoldier->ubID) )
			{
				pSoldier2->ubMiscSoldierFlags &= (~SOLDIER_MISC_XRAYED);
				pSoldier2->aiData.ubXRayedBy = NOBODY;
			}
		}
	}
	pSoldier->uiXRayActivatedTime = 0;
}



#ifdef JA2TESTVERSION
void DumpItemsList( void )
{
  CHAR8 zPrintFileName[60];
  FILE *FDump;
	UINT16 usItem;
	INVTYPE *pItem;

  // open output file
 	strcpy(zPrintFileName, "ItemDump.txt");
  FDump = fopen(zPrintFileName, "wt");

  if (FDump == NULL)
    return;

	// print headings
	fprintf(FDump, "            ITEM              COOLNESS  VALUE\n");
	fprintf(FDump, "============================  ========  =====\n");

	for( usItem = 0; usItem < MAXITEMS; usItem++ )
	{
		pItem= &( Item[ usItem ] );

		if (pItem->ubCoolness > 0 )
		{
			fprintf(FDump, "%28ls     %2d     $%4d\n", ItemNames[ usItem ], pItem->ubCoolness, pItem->usPrice );
		}
	}

  fclose(FDump);
}
#endif // JA2TESTVERSION


// Snap: status modifiers for various item bonuses:

// In JA Gold status above this limit does not affect item performance (for some items)
const INT8 STANDARD_STATUS_CUTOFF = 85;

// Scale bonus with item status
INT16 BonusReduce( INT16 bonus, INT16 status, INT16 statusCutoff = STANDARD_STATUS_CUTOFF )
{
	if ( bonus > 0 && status < statusCutoff && statusCutoff > 0 && statusCutoff <= 100 )
		return ( ( status * 100 ) / statusCutoff * bonus ) / 100;
	else // A penalty can't be reduced by status!
		return bonus;
}

// Scale bonus with item status. Status < 50% creates a penalty!
INT16 BonusReduceMore( INT16 bonus, INT16 status, INT16 statusCutoff = 100 )
{
	if ( bonus > 0 && status < statusCutoff && statusCutoff > 0 && statusCutoff <= 100 )
		return ( ( ( status * 100 ) / statusCutoff - 50 ) * bonus ) / 50;
	else // A penalty can't be reduced by status!
		return bonus;
}

// Scale bonus with item status. Status < 50% creates a penalty!
FLOAT BonusReduceMoreFloat( FLOAT bonus, INT16 status, INT16 statusCutoff = 100 )
{
	if ( bonus > 0 && status < statusCutoff && statusCutoff > 0 && statusCutoff <= 100 )
		return ( ( ( (FLOAT)status * 100.0f ) / (FLOAT)statusCutoff - 50.0f ) * bonus ) / 50.0f;
	else // A penalty can't be reduced by status!
		return bonus;
}

// Some items either work or they don't...
INT16 BonusOnOff( INT16 bonus, INT16 status )
{
	if ( bonus > 0 )
		return (status >= 50) ? bonus : 0;
	else // A penalty can't be reduced by status!
		return bonus;
}

// HEADROCK HAM 4: Scopes now determined not by an aim bonus but by a magnification factor.
BOOLEAN NCTHIsScoped( OBJECTTYPE * pObj )
{
	if (pObj->exists() == true && UsingNewCTHSystem() == true) {
		if ( Item[pObj->usItem].scopemagfactor > 1.0 || Item[(*pObj)[0]->data.gun.usGunAmmoItem].scopemagfactor > 1.0 )
			return TRUE;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && Item[iter->usItem].scopemagfactor > 1.0)
				return TRUE;
		}
	}

	return FALSE;
}

// Snap: a fast aimbonus check for AI
BOOLEAN IsScoped( OBJECTTYPE * pObj )
{
	if(UsingNewCTHSystem() == true)
		return NCTHIsScoped(pObj);

	if (pObj->exists() == true) {
		if ( Item[pObj->usItem].aimbonus > 0 || Item[(*pObj)[0]->data.gun.usGunAmmoItem].aimbonus > 0 )
			return TRUE;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && Item[iter->usItem].aimbonus > 0)
				return TRUE;
		}
	}

	return FALSE;
}

// Snap: get aim bonus for a single item
INT16 GetItemAimBonus( const INVTYPE* pItem, INT32 iRange, INT16 ubAimTime )
{
	if (iRange >= pItem->minrangeforaimbonus || pItem->minrangeforaimbonus <= 0)
		return pItem->aimbonus;
	else
		return pItem->aimbonus * iRange * iRange / (pItem->minrangeforaimbonus * pItem->minrangeforaimbonus);

	/*if ( iRange <= pItem->minrangeforaimbonus )	return 0;

	// reduce effective sight range by aimbonus% per extra aiming time AP
	// of the distance beyond minrangeforaimbonus.
	if ( pItem->aimbonus < 0)
		ubAimTime = 1;

	//CHRISL: The old system basically allowed scopes to reduce sight range by AimBonus%/click.  So a scope with an
	//	AimBonus 20, and 8 clicks, would reduce sight range by 160%.  In other words, it would effectively reduce sight
	//	range to 0 just for using 8 APs to aim.
	//return ( pItem->aimbonus * ubAimTime	* ( iRange - pItem->minrangeforaimbonus ) ) / 100;
	//	Instead, let's have the bonus reduce each click separately and base the percentage on the new range.
	INT16 bonus = 0, stepBonus = 0;
	for( UINT8 step = 0; step < ubAimTime; step++)
	{
		stepBonus = (pItem->aimbonus * iRange)/100;
		iRange -= stepBonus;
		bonus += stepBonus;
		if(iRange < pItem->minrangeforaimbonus)
		{
			stepBonus = pItem->minrangeforaimbonus - iRange;
			bonus -= stepBonus;
			break;
		}
	}
	return(bonus);*/
}

INT16 GetAimBonus( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj, INT32 iRange, INT16 ubAimTime )
{
	INT16 bonus = 0;
	BOOLEAN fUseCowitnessSight = FALSE;

	if (pObj->exists() == true) 
	{
		// bonus from item
		bonus = BonusReduceMore(GetItemAimBonus(&Item[pObj->usItem], iRange, ubAimTime), (*pObj)[0]->data.objectStatus);

		// bonus from active scope
		if ( gGameExternalOptions.fScopeModes && pSoldier && Item[pObj->usItem].usItemClass == IC_GUN )
		{
			std::map<INT8, OBJECTTYPE*> ObjList;
			GetScopeLists(pSoldier, pObj, ObjList);
		
			// only use scope mode if gun is in hand, otherwise an error might occur!
			if ((&pSoldier->inv[HANDPOS]) == pObj && ObjList[pSoldier->bScopeMode] != NULL && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD)
			{
				bonus = BonusReduceMore(GetItemAimBonus(&Item[ObjList[pSoldier->bScopeMode]->usItem], iRange, ubAimTime), (*ObjList[pSoldier->bScopeMode])[0]->data.objectStatus);

				if (IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT) || IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE))
					fUseCowitnessSight = TRUE;
			}
		}

		// bonus from ammo
		bonus += GetItemAimBonus( &Item[(*pObj)[0]->data.gun.usGunAmmoItem], iRange, ubAimTime );

		// bonus from attachments except scopes
		attachmentList::iterator iterend = (*pObj)[0]->attachments.end();
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != iterend; ++iter) 
		{
			// sevenfm: disabled bonus/penalty from attached face items
			if (iter->exists() && 
				Item[iter->usItem].usItemClass != IC_FACE && 
				(!gGameExternalOptions.fScopeModes || !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) && (!IsAttachmentClass(iter->usItem, AC_EXTERNAL) || fUseCowitnessSight)))
			{
				bonus += BonusReduceMore( GetItemAimBonus( &Item[iter->usItem], iRange, ubAimTime ), (*iter)[0]->data.objectStatus );
			}
		}
	}

	return( bonus );
}

//WarmSteel - This is used to determine the base aim bonus of the scope on this gun.
//It searched for the most powerful scope that is not targeting under it's minimum range.
INT16 GetBaseScopeAimBonus( OBJECTTYPE * pObj, INT32 iRange )
{
	INT16 bonus = 0;

	//Valid integrated scope?
	if(Item[pObj->usItem].aimbonus > bonus && iRange >= Item[pObj->usItem].minrangeforaimbonus ){
		bonus = Item[pObj->usItem].aimbonus;
	}

	//Search for the most powerful scope we can use.
	for(attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); iter++)
	{
		if (iter->exists() && Item[iter->usItem].aimbonus > bonus && iRange >= Item[iter->usItem].minrangeforaimbonus)
		{
			bonus = Item[iter->usItem].aimbonus;
		}
	}
	return( bonus );
}

// Madd: check equipment for aim bonus (penalties)
INT16 GetGearAimBonus( SOLDIERTYPE * pSoldier, INT32 iRange, INT16 sAimTime  )
{
	INT16 bonus=0;

	for (int j = HELMETPOS; j <= HEAD2POS; j++)
	{
		OBJECTTYPE* pObj = &pSoldier->inv[j];
		if (pObj->exists() == true) 
		{
			bonus += GetItemAimBonus( &Item[pSoldier->inv[j].usItem], iRange, sAimTime );
			for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
			{
				// sevenfm: ignore face items attached to other items
				if (iter->exists() && Item[iter->usItem].usItemClass != IC_FACE)
				{
					bonus += GetItemAimBonus(&Item[iter->usItem], iRange, sAimTime);
				}
			}
		}
	}

	return( bonus );
}

// Madd: check equipment for to hit bonus (penalties)
INT16 GetGearToHitBonus( SOLDIERTYPE * pSoldier )
{
	INT16 bonus=0;

	for (int j = HELMETPOS; j <= HEAD2POS; j++)
	{
		OBJECTTYPE* pObj = &pSoldier->inv[j];
		if (pObj->exists() == true) 
		{
			bonus += Item[pSoldier->inv[j].usItem].tohitbonus;
			for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
			{
				// sevenfm: ignore face items attached to other items
				if (iter->exists() && Item[iter->usItem].usItemClass != IC_FACE)
				{
					bonus += Item[iter->usItem].tohitbonus;
				}
			}
		}
	}

	return( bonus );
}

// Madd: check equipment for AP bonus (penalties)
INT16 GetGearAPBonus( SOLDIERTYPE * pSoldier )
{
	INT16 bonus=0;

	for (int j = HELMETPOS; j <= HEAD2POS; ++j)
	{
		if (pSoldier->inv[j].exists() == true) 
		{
			OBJECTTYPE* pObj = &pSoldier->inv[j];

			for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
			{
				// sevenfm: ignore face items attached to other items
				if (iter->exists() && Item[iter->usItem].usItemClass != IC_FACE)
				{
					bonus += Item[iter->usItem].APBonus;
				}
			}
		}
	}

	return( bonus );
}

UINT32 FindRangeBonusAttachment( OBJECTTYPE * pObj )
{
	if (pObj->exists() == true) {
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) {
			if (iter->exists() && Item[iter->usItem].rangebonus > 0)
			{
				return( Item[iter->usItem].uiIndex );
			}
		}
	}
	return( NONE );
}

INT16 GetRangeBonus( OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	if (pObj->exists() == true) 
	{
		if(Item[pObj->usItem].usItemClass == IC_AMMO)
			return( Item[pObj->usItem].rangebonus );

		bonus = BonusReduce( Item[pObj->usItem].rangebonus, (*pObj)[0]->data.objectStatus );

		if ( (*pObj)[0]->data.gun.ubGunShotsLeft > 0 )
			bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].rangebonus;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && (!Item[iter->usItem].duckbill || (Item[iter->usItem].duckbill && (*pObj)[0]->data.gun.ubGunAmmoType == AMMO_BUCKSHOT)))
				bonus += BonusReduce( Item[iter->usItem].rangebonus, (*iter)[0]->data.objectStatus );
		}
	}
	return( bonus );
}

INT16 LaserBonus(SOLDIERTYPE *pSoldier, OBJECTTYPE *pObj, INT32 iRange, UINT8 bLightLevel, UINT8 ubAimTime, UINT8& ubLaserType, INT16 &sFullLaserBonus, BOOLEAN &fLaserSight)
{
	INT16 sMaxBonus = 0;
	INT16 sMaxRange = 0;
	UINT16 usMaxItem = 0;
	std::map<INT8, OBJECTTYPE*> ObjList;
	BOOLEAN fUseAttachmentLaser = FALSE;
	//BOOLEAN fLaserSight = FALSE;

	// initialize value
	ubLaserType = 0;
	sFullLaserBonus = 0;
	fLaserSight = FALSE;

	// first check laser sights
	// if item is a gun and scope mode enabled, add bonus from scope used
	if (gGameExternalOptions.fScopeModes && pSoldier && Item[pObj->usItem].usItemClass == IC_GUN)
	{
		GetScopeLists(pSoldier, pObj, ObjList);

		// laser bonus from attachments doesn't work when using reflex sight or scope, unless this sight or scope is active
		if (pSoldier->bScopeMode == USE_ALT_WEAPON_HOLD ||
			ObjList[pSoldier->bScopeMode] != NULL && IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_IRONSIGHT) ||
			Item[pObj->usItem].bestlaserrange > 0)
		{
			fUseAttachmentLaser = TRUE;
		}

		// only use scope mode if gun is in hand, otherwise an error might occur!
		if ((&pSoldier->inv[HANDPOS]) == pObj && 
			ObjList[pSoldier->bScopeMode] != NULL && 
			pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD &&
			Item[ObjList[pSoldier->bScopeMode]->usItem].bestlaserrange > 0 && 
			Item[ObjList[pSoldier->bScopeMode]->usItem].tohitbonus > 0)
		{
			usMaxItem = ObjList[pSoldier->bScopeMode]->usItem;
			sMaxRange = Item[usMaxItem].bestlaserrange;
			sMaxBonus = Item[usMaxItem].tohitbonus;

			//if (IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_IRONSIGHT))
			fLaserSight = TRUE;
		}
	}

	// only check item and attachments if using hip firing mode or iron sight	
	if (usMaxItem == 0 && (!gGameExternalOptions.fScopeModes || fUseAttachmentLaser))
	{
		// add bonus for item
		if (Item[pObj->usItem].bestlaserrange > sMaxRange && Item[pObj->usItem].tohitbonus > 0)
		{
			usMaxItem = pObj->usItem;
			sMaxRange = Item[usMaxItem].bestlaserrange;
			sMaxBonus = Item[usMaxItem].tohitbonus;
		}

		// check the rest of attachments except sights, find attachment with max laser range
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			// exclude possible scopes from search as activated scope is already used before
			if (iter->exists() &&
				Item[iter->usItem].bestlaserrange > sMaxRange && 
				Item[iter->usItem].tohitbonus > 0 &&
				(!gGameExternalOptions.fScopeModes || 
				!IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) || 
				pSoldier->bScopeMode == USE_ALT_WEAPON_HOLD && IsAttachmentClass(iter->usItem, AC_IRONSIGHT)))
			{
				if (Item[iter->usItem].bestlaserrange > sMaxRange && Item[iter->usItem].tohitbonus > 0)
				{
					usMaxItem = iter->usItem;
					sMaxRange = Item[usMaxItem].bestlaserrange;
					sMaxBonus = Item[usMaxItem].tohitbonus;
				}				
			}
		}
	}

	// now calculate actual laser bonus from ToHit value, distance and light
	if (usMaxItem > 0)
	{
		// Figure out max. visible distance for the laser dot:
		// day: 1.5 * bestlaserrange, night: 2.5 * bestlaserrange
		INT32 iBestLaserRange = Item[usMaxItem].bestlaserrange;
		INT32 iOptimalLaserRange;
		INT32 iMaxLaserRange;
		INT16 sBonus = 0;

		//iMaxLaserRange = (iBestLaserRange * (2 * bLightLevel + 3 * NORMAL_LIGHTLEVEL_NIGHT - 5 * NORMAL_LIGHTLEVEL_DAY)) / (2 * (NORMAL_LIGHTLEVEL_NIGHT - NORMAL_LIGHTLEVEL_DAY));

		if (Item[usMaxItem].ubLaserType == LASER_TYPE_GREEN)
			// full at day
			iOptimalLaserRange = iBestLaserRange * max(NORMAL_LIGHTLEVEL_NIGHT, bLightLevel) / NORMAL_LIGHTLEVEL_NIGHT;
		else if (Item[usMaxItem].ubLaserType == LASER_TYPE_WHITE)
			// full at night, zero at day
			iOptimalLaserRange = iBestLaserRange * (max(NORMAL_LIGHTLEVEL_DAY, bLightLevel) - NORMAL_LIGHTLEVEL_DAY) / (NORMAL_LIGHTLEVEL_NIGHT - NORMAL_LIGHTLEVEL_DAY);
		else if (Item[usMaxItem].ubLaserType == LASER_TYPE_IR)
			// full at night, 12% at day
			iOptimalLaserRange = iBestLaserRange * bLightLevel * min(NORMAL_LIGHTLEVEL_NIGHT, bLightLevel) / (NORMAL_LIGHTLEVEL_NIGHT * NORMAL_LIGHTLEVEL_NIGHT);
		else
			// full at night, 25% at day
			iOptimalLaserRange = iBestLaserRange * bLightLevel / NORMAL_LIGHTLEVEL_NIGHT;

		iMaxLaserRange = iOptimalLaserRange * 3 / 2;
		//iMaxLaserRange = iOptimalLaserRange + iOptimalLaserRange * bLightLevel / NORMAL_LIGHTLEVEL_NIGHT;

		if (iRange > iMaxLaserRange)
			sBonus = 0;
		else if (iRange < iOptimalLaserRange)
			sBonus = Item[usMaxItem].tohitbonus;
		else
			sBonus = Item[usMaxItem].tohitbonus * (iMaxLaserRange - iRange) / (iMaxLaserRange - iOptimalLaserRange);

		/*if (iRange > iMaxLaserRange)
			sBonus = 0;
		else if (iRange < iBestLaserRange)
			sBonus = Item[usMaxItem].tohitbonus;
		else
			sBonus = Item[usMaxItem].tohitbonus * (iMaxLaserRange - iRange) / (iMaxLaserRange - iBestLaserRange);*/

		// set full laser bonus before light/aiming modifications
		sFullLaserBonus = sBonus;

		// IR lasers require NVG to work
		if (Item[usMaxItem].ubLaserType == LASER_TYPE_IR && !pSoldier->WearsNVG())
			sBonus = 0;

		// reduce bonus if not in the dark
		if (bLightLevel < NORMAL_LIGHTLEVEL_DAY)
		{
			sBonus = 0;
		}
		else if (bLightLevel < NORMAL_LIGHTLEVEL_NIGHT)
		{
			if (Item[usMaxItem].ubLaserType == LASER_TYPE_GREEN)
				// 80% at day, 100% at night
				sBonus = sBonus * (3 * NORMAL_LIGHTLEVEL_NIGHT + bLightLevel) / (4 * NORMAL_LIGHTLEVEL_NIGHT);
			else if (Item[usMaxItem].ubLaserType == LASER_TYPE_WHITE)
				// zero at daylight, 100% at night light
				sBonus = sBonus * (bLightLevel - NORMAL_LIGHTLEVEL_DAY) / (NORMAL_LIGHTLEVEL_NIGHT - NORMAL_LIGHTLEVEL_DAY);
			else
				// 34% at day, 100% at night
				sBonus = sBonus * (NORMAL_LIGHTLEVEL_NIGHT + 7 * bLightLevel) / (8 * NORMAL_LIGHTLEVEL_NIGHT);
		}

		// penalty if cannot see dot while trying to aim by laser
		//if ((fLaserSight || pSoldier->bScopeMode == USE_ALT_WEAPON_HOLD) &&
		if (fLaserSight && sMaxBonus > 0 && sBonus <= 0)
		{
			sBonus -= 50;
		}

		/*if (fLaserSight && sMaxBonus > 0 && sBonus < sMaxBonus)
		{
			sBonus -= 50 * (sMaxBonus - sBonus) * (sMaxBonus - sBonus) / (sMaxBonus * sMaxBonus);
		}*/

		// min dot brightness to see
		//INT16 sMinBonus = sBonus * 2;

		// apply penalty based on how bad we see laser
		/*if ((fLaserSight || pSoldier->bScopeMode == USE_ALT_WEAPON_HOLD) && 
			sMaxRange > 0 &&
			sMaxBonus > 0 && 
			sMaxBonus > sMinBonus)
		{
			if (Item[usMaxItem].ubLaserType == LASER_TYPE_WHITE)
				sBonus -= min(50, 50 * (sMaxBonus - sMinBonus) / sMaxBonus);
			else
				sBonus -= min(50, 50 * (sMaxBonus - sMinBonus) * (sMaxBonus - sMinBonus) / (sMaxBonus * sMaxBonus));
		}*/

		// reduce laser bonus when not using laser as main aiming sight
		if (!fLaserSight && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD)
		{
			sBonus = 2 * sBonus / (2 + ubAimTime);
		}

		ubLaserType = Item[usMaxItem].ubLaserType;

		return sBonus;
	}
	
	return 0;
}

// apply usBestToHitRange modifier
INT16 ApplyBestRange(INT16 sValue, INT32 iRange, INT16 sBestRange)
{
	if (sBestRange == 0 || iRange <= sBestRange)
		return sValue;

	INT16 sMaxRange = sBestRange * 3 / 2;

	if (iRange <= sMaxRange)
		return sValue * (sMaxRange - iRange) / (sMaxRange - sBestRange);

	return 0;
}

INT16 GetToHitBonus( SOLDIERTYPE *pSoldier, OBJECTTYPE *pObj, INT32 iRange, UINT8 bLightLevel, BOOLEAN fProneStance, UINT8 ubAimTime )
{
	///////////////////////////////////////////////////////////////////////////
	// sevenfm: added scope mode support
	///////////////////////////////////////////////////////////////////////////
	INT16 sBonus = 0;
	INT16 sValue = 0;
	UINT8 ubLaserType;
	INT16 sFullLaserBonus;
	BOOLEAN fUseCowitnessSight = FALSE;
	BOOLEAN fLaserSight;

	if (pObj->exists() == true) 
	{
		// add bonus from laser
		if (pSoldier)
		{
			sBonus += BonusReduceMore(LaserBonus(pSoldier, pObj, iRange, bLightLevel, ubAimTime, ubLaserType, sFullLaserBonus, fLaserSight), (*pObj)[0]->data.objectStatus);
		}

		if (fProneStance)
			sBonus += Item[pObj->usItem].bipod;

		// add bonus for item
		if (Item[pObj->usItem].bestlaserrange == 0)
			sBonus += ApplyBestRange(BonusReduceMore(Item[pObj->usItem].tohitbonus, (*pObj)[0]->data.objectStatus), iRange, Item[pObj->usItem].usBestToHitRange);

		// if item is a gun and scope mode enabled, add bonus from scope used
		if ( gGameExternalOptions.fScopeModes && pSoldier && Item[pObj->usItem].usItemClass == IC_GUN )
		{
			std::map<INT8, OBJECTTYPE*> ObjList;
			GetScopeLists(pSoldier, pObj, ObjList);

			// only use scope mode if gun is in hand, otherwise an error might occur!
			if ((&pSoldier->inv[HANDPOS]) == pObj && ObjList[pSoldier->bScopeMode] != NULL && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD && 
				Item[ObjList[pSoldier->bScopeMode]->usItem].bestlaserrange == 0)
			{
				sBonus += ApplyBestRange(BonusReduceMore(Item[ObjList[pSoldier->bScopeMode]->usItem].tohitbonus, (*ObjList[pSoldier->bScopeMode])[0]->data.objectStatus), iRange, Item[ObjList[pSoldier->bScopeMode]->usItem].usBestToHitRange);

				if (IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT) || IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE))
					fUseCowitnessSight = TRUE;
			}
		}

		// bonus from ammo
		sBonus += ApplyBestRange(Item[(*pObj)[0]->data.gun.usGunAmmoItem].tohitbonus, iRange, Item[(*pObj)[0]->data.gun.usGunAmmoItem].usBestToHitRange);

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			// sevenfm: disabled bonus/penalty from attached face items
			// exclude possible scopes from search as activated scope is already used before
			if (iter->exists() && 
				Item[iter->usItem].usItemClass != IC_FACE &&
				Item[iter->usItem].bestlaserrange == 0 &&
				(!gGameExternalOptions.fScopeModes || !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) && (!IsAttachmentClass(iter->usItem, AC_EXTERNAL) || fUseCowitnessSight)) &&
				AttachmentHandlingAllowed(pSoldier, pObj, iter->usItem))
			{
				if ( fProneStance )
					sBonus += Item[iter->usItem].bipod;

				sBonus += ApplyBestRange(BonusReduceMore(Item[iter->usItem].tohitbonus, (*iter)[0]->data.objectStatus), iRange, Item[iter->usItem].usBestToHitRange);
			}
		}
	}

	return sBonus;
}

BOOLEAN AttachmentHandlingAllowed(SOLDIERTYPE *pSoldier, OBJECTTYPE *pObj, UINT16 usItem)
{
	CHECKF(pSoldier);
	CHECKF(pObj);

	// we need to know weapon handling mode after raising
	//gbForceWeaponReady = true;
	UINT8 ubWeaponHandling = pSoldier->DetermineHandlingMode(pObj);
	//gbForceWeaponReady = false;

	if (IsAttachmentClass(usItem, AC_SLING) &&
		//Item[usItem].attachmentclass == AC_SLING && 
		Item[usItem].tohitbonus > 0 &&
		ubWeaponHandling != WEAPON_HANDLING_SLING)
		return FALSE;

	// don't allow ready AP bonus from sling when prone
	if (IsAttachmentClass(usItem, AC_SLING) &&
		//Item[usItem].attachmentclass == AC_SLING &&
		Item[usItem].tohitbonus == 0 &&
		pSoldier->AnimEndHeight() == ANIM_PRONE)
		return FALSE;

	if (IsAttachmentClass(usItem, AC_FOREGRIP) &&
		//Item[usItem].attachmentclass == AC_FOREGRIP && 
		ubWeaponHandling != WEAPON_HANDLING_FOREGRIP &&
		!(Item[usItem].bipod > 0 && ubWeaponHandling == WEAPON_HANDLING_BIPOD))
		return FALSE;

	return TRUE;
}

// HEADROCK HAM 4: The following functions return the value of new NCTH-related modifiers from an item and all its
// attachments. They are stance-based, meaning that the soldier's stance determines which modifier is referenced.
// For a "default" value, feed the function a value of ubStance=ANIM_STAND.


INT32 GetItemModifier(OBJECTTYPE* pObj, UINT8 ubRef, UINT8 usType)
{
	INT32 iModifier = 0;

	switch( usType )
	{
	case ITEMMODIFIER_FLATBASE:
		iModifier += BonusReduceMore( Item[pObj->usItem].flatbasemodifier[ubRef], (*pObj)[0]->data.objectStatus );
		break;
	case ITEMMODIFIER_PERCENTBASE:
		iModifier += BonusReduceMore( Item[pObj->usItem].percentbasemodifier[ubRef], (*pObj)[0]->data.objectStatus );
		break;
	case ITEMMODIFIER_FLATAIM:
		iModifier += BonusReduceMore( Item[pObj->usItem].flataimmodifier[ubRef], (*pObj)[0]->data.objectStatus );
		break;
	case ITEMMODIFIER_PERCENTAIM:
		iModifier += BonusReduceMore( Item[pObj->usItem].percentaimmodifier[ubRef], (*pObj)[0]->data.objectStatus );
		break;
	case ITEMMODIFIER_PERCENTCAP:
		iModifier += BonusReduceMore( Item[pObj->usItem].percentcapmodifier[ubRef], (*pObj)[0]->data.objectStatus );
		break;
	case ITEMMODIFIER_PERCENTHANDLING:
		iModifier += BonusReduceMore( Item[pObj->usItem].percenthandlingmodifier[ubRef], (*pObj)[0]->data.objectStatus );
		break;
	case ITEMMODIFIER_DROPCOMPENSATION:
		iModifier += BonusReduceMore( Item[pObj->usItem].percentdropcompensationmodifier[ubRef], (*pObj)[0]->data.objectStatus );
		break;
	case ITEMMODIFIER_COUNTERFORCEMAX:
		iModifier += BonusReduceMore( Item[pObj->usItem].maxcounterforcemodifier[ubRef], (*pObj)[0]->data.objectStatus );
		break;
	case ITEMMODIFIER_COUNTERFORCEACCURACY:
		iModifier += BonusReduceMore( Item[pObj->usItem].counterforceaccuracymodifier[ubRef], (*pObj)[0]->data.objectStatus );
		break;
	case ITEMMODIFIER_TRACKING:
		iModifier += BonusReduceMore( Item[pObj->usItem].targettrackingmodifier[ubRef], (*pObj)[0]->data.objectStatus );
		break;
	case ITEMMODIFIER_AIMLEVELS:
		// silversurfer: NCTH system only
		if ( UsingNewCTHSystem() )
			iModifier += Item[pObj->usItem].aimlevelsmodifier[ubRef];
		break;

	case ITEMMODIFIER_SPOTTER:
		iModifier += BonusReduceMore( Item[pObj->usItem].usSpotting, (*pObj)[0]->data.objectStatus );
		break;
	}

	return iModifier;
}

// Flugente: unified function (no need to have 12 functions that all do the same thing and clutter the code)
INT32 GetObjectModifier(SOLDIERTYPE* pSoldier, OBJECTTYPE *pObj, UINT8 ubStance, UINT8 usType)
{
	INT32 iModifier = 0;
	BOOLEAN fUseCowitnessSight = FALSE;
	UINT8 ubRef = GetStanceModifierRef(ubStance);

	if (pObj->exists())
	{
		// simply add the object modifier
		iModifier += GetItemModifier(pObj, ubRef, usType);

		// silversurfer: add stance based max counter force modifier here and not in function GetItemModifier() because 
		// that function is called for everything including attachments multiple times giving an insane bonus to max counter force modifier.
		// An attachment that provides such bonus is not affected by this change. This stance based modifier only applies to guns.
		if (usType == ITEMMODIFIER_COUNTERFORCEMAX && Item[pObj->usItem].usItemClass & IC_GUN)
		{
			if (ubRef == 1)
				iModifier += (INT32)gGameCTHConstants.RECOIL_MAX_COUNTER_CROUCH;
			else if (ubRef == 2)
				iModifier += (INT32)gGameCTHConstants.RECOIL_MAX_COUNTER_PRONE;
		}

		// Flugente::if we are a soldier and are using a gun, we might be checking for scope modes
		if (gGameExternalOptions.fScopeModes && pSoldier && Item[pObj->usItem].usItemClass & IC_GUN)
		{
			// only apply boni if we are not hip-firing
			if (pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD)
			{
				std::map<INT8, OBJECTTYPE*> ObjList;
				GetScopeLists(pSoldier, pObj, ObjList);

				// only use scope mode if gun is in hand, otherwise an error might occur!
				if ((&pSoldier->inv[HANDPOS]) == pObj && ObjList[pSoldier->bScopeMode] != NULL)
				{
					// Do not apply weapon bonus/penalty because this will be added one step below. We don't want to apply it twice.
					if (pObj->usItem != ObjList[pSoldier->bScopeMode]->usItem)
						iModifier += GetItemModifier(ObjList[pSoldier->bScopeMode], ubRef, usType);

					if (IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT) || IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE))
						fUseCowitnessSight = TRUE;
				}
			}
		}

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists())
			{
				// Flugente: if we use scope modes, are a soldier, this is a gun and the attachment a scope/sight, ignore it for the moment
				//if (gGameExternalOptions.fScopeModes && pSoldier && Item[pObj->usItem].usItemClass & IC_GUN && IsAttachmentClass(iter->usItem, (AC_SCOPE_MODE)))
				if (gGameExternalOptions.fScopeModes && pSoldier && Item[pObj->usItem].usItemClass & IC_GUN && 
					(IsAttachmentClass(iter->usItem, (AC_SCOPE_MODE)) || IsAttachmentClass(iter->usItem, (AC_EXTERNAL)) && !fUseCowitnessSight))
					continue;

				// sevenfm: check if this attachment allowed in current handling mode
				if (!AttachmentHandlingAllowed(pSoldier, pObj, iter->usItem))
					continue;

				iModifier += GetItemModifier((&(*iter)), ubRef, usType);
			}
		}		

		// sevenfm: bonus for mounted weapon (without bipod)
		if (usType == ITEMMODIFIER_PERCENTHANDLING && pSoldier->DetermineHandlingMode(pObj) == WEAPON_HANDLING_NORMAL && pSoldier->IsWeaponMounted() && pSoldier->AnimEndHeight() == ANIM_PRONE)
			iModifier += -20;
	}

	iModifier = max(-100, iModifier);

	return (iModifier);
}

// sevenfm: r7864 fix
INT32 GetAimLevelsTraitModifier( SOLDIERTYPE *pSoldier, OBJECTTYPE *pObj )
{
	INT8 ubSkillModifier = 0;
	INT8 iCTHSystem = UsingNewCTHSystem() ? -1 : 1;	// silversurfer: NCTH benefits from reduced levels, OCTH benefits from increased levels

	if( gGameOptions.fNewTraitSystem )
	{
		if ( Weapon[Item[pObj->usItem].ubClassIndex].ubWeaponType == GUN_PISTOL || Weapon[Item[pObj->usItem].ubClassIndex].ubWeaponType == GUN_M_PISTOL )
			ubSkillModifier += gSkillTraitValues.ubGSAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, GUNSLINGER_NT ) * iCTHSystem;
		else if ( Weapon[Item[pObj->usItem].ubClassIndex].ubWeaponType == GUN_SHOTGUN )
			ubSkillModifier += gSkillTraitValues.ubRAAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ) * iCTHSystem;
		else if ( Weapon[Item[pObj->usItem].ubClassIndex].ubWeaponType == GUN_RIFLE )
			ubSkillModifier += max(gSkillTraitValues.ubRAAimClicksAdded * NUM_SKILL_TRAITS(pSoldier, RANGER_NT) / 2, gSkillTraitValues.ubSNAimClicksAdded * NUM_SKILL_TRAITS(pSoldier, SNIPER_NT)) * iCTHSystem;
		else if ( Weapon[Item[pObj->usItem].ubClassIndex].ubWeaponType == GUN_SN_RIFLE )
			ubSkillModifier += gSkillTraitValues.ubSNAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, SNIPER_NT ) * iCTHSystem;
	}
	else
	{
		ubSkillModifier += NUM_SKILL_TRAITS( pSoldier, PROF_SNIPER_OT ) * iCTHSystem;
	}

	return (INT32)ubSkillModifier;
}
// end fix

INT16 GetBurstToHitBonus( SOLDIERTYPE *pSoldier, OBJECTTYPE * pObj, BOOLEAN fProneStance )
{
	///////////////////////////////////////////////////////////////////
	// sevenfm: added scope mode support
	///////////////////////////////////////////////////////////////////
	INT16 bonus = 0;
	BOOLEAN fUseCowitnessSight = FALSE;

	if (pObj->exists() == true) 
	{
		if( fProneStance )
			bonus += BonusReduceMore( Item[pObj->usItem].bipod, (*pObj)[0]->data.objectStatus );
		else
			bonus += BonusReduceMore( Item[pObj->usItem].bursttohitbonus, (*pObj)[0]->data.objectStatus );
		
		// sevenfm: tracers remove additional penalty from distance instead
		if (!gGameExternalOptions.fOCTHTracerCode)
			bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].bursttohitbonus ;

		// Flugente: check for scope mode
		if ( gGameExternalOptions.fScopeModes && pSoldier && Item[pObj->usItem].usItemClass == IC_GUN )
		{
			std::map<INT8, OBJECTTYPE*> ObjList;
			GetScopeLists(pSoldier, pObj, ObjList);

			// only use scope mode if gun is in hand, otherwise an error might occur!
			if ( (&pSoldier->inv[HANDPOS]) == pObj && ObjList[pSoldier->bScopeMode] != NULL && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD)
			{
				bonus += BonusReduceMore( Item[ObjList[pSoldier->bScopeMode]->usItem].bursttohitbonus, (*pObj)[0]->data.objectStatus );

				if (IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT) || IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE))
					fUseCowitnessSight = TRUE;
			}
		}

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && 
				(!gGameExternalOptions.fScopeModes || !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) && (!IsAttachmentClass(iter->usItem, AC_EXTERNAL) || fUseCowitnessSight)) &&
				AttachmentHandlingAllowed(pSoldier, pObj, iter->usItem))
			{
				if ( fProneStance )
					bonus += BonusReduceMore( Item[iter->usItem].bipod, (*iter)[0]->data.objectStatus );
				else
					bonus += BonusReduceMore( Item[iter->usItem].bursttohitbonus, (*iter)[0]->data.objectStatus );
			}
		}
	}

	return( bonus );
}

void GetRecoil( SOLDIERTYPE *pSoldier, OBJECTTYPE *pObj, FLOAT *bRecoilX, FLOAT *bRecoilY, UINT8 ubNumBullet )
{
	*bRecoilX = 0;
	*bRecoilY = 0;

	// Flugente: get weapon actually used (might be an underbarrel shotgun)
	OBJECTTYPE* pObjUsed = pSoldier->GetUsedWeapon(pObj);
	OBJECTTYPE* pObjUsedInHand = pSoldier->GetUsedWeapon( &(pSoldier->inv[HANDPOS]) );
 
	// silversurfer: The first bullet should never return recoil. If function CalcPreRecoilOffset( ) in LOS.cpp wants to know
	// what recoil to expect for the second bullet in the volley it should ask for the second bullet and does so from now on.
	// ubRecoilDelay is 0 for almost all weapons so we need to have a failsafe here for the first bullet.
	if (ubNumBullet < 2 || 
		pSoldier->bWeaponMode == WM_BURST && ubNumBullet <= Weapon[pObjUsed->usItem].ubRecoilDelay ||
		pSoldier->bWeaponMode == WM_AUTOFIRE && ubNumBullet <= Weapon[pObjUsed->usItem].ubRecoilDelay && Weapon[pObjUsed->usItem].ubRecoilDelay == 2)
	{
		// The first bullet in a volley never has recoil - it hasn't "set in" yet. Only the second+ bullets
		// will have any recoil.
		*bRecoilX = 0;
		*bRecoilY = 0;
		return;
	}

	*bRecoilX = Weapon[pObjUsed->usItem].bRecoilX;
	*bRecoilY = Weapon[pObjUsed->usItem].bRecoilY;

	// modify by ini values
	if ( Item[ pObj->usItem ].usItemClass == IC_GUN )
	{
		*bRecoilX *= gItemSettings.fRecoilXModifierGun[ Weapon[ pObj->usItem ].ubWeaponType ];
		*bRecoilY *= gItemSettings.fRecoilYModifierGun[ Weapon[ pObj->usItem ].ubWeaponType ];
	}
	// Apply a percentage-based modifier. This can increase or decrease BOTH axes. At most, it can eliminate
	// recoil on the gun.

	//INT16 sPercentRecoilModifier = GetPercentRecoilModifier( pObj );
	INT16 sPercentRecoilModifier = __max(-100, (GetBasePercentRecoilModifier( pObjUsed ) + GetAttachmentPercentRecoilModifier( pObjUsedInHand ) ));


	*bRecoilX *= ( (FLOAT)( 100 + sPercentRecoilModifier ) / 100 );
	*bRecoilY *= ( (FLOAT)( 100 + sPercentRecoilModifier ) / 100 );

	// Apply a flat modifier. This acts on either axis, and if powerful enough can "reverse polarity" of either
	// axis recoil. For instance, it can make a gun that normally pulls LEFT start pulling RIGHT instead.

	FLOAT bRecoilAdjustX = 0;
	FLOAT bRecoilAdjustY = 0;

	//GetFlatRecoilModifier( pObj, &bRecoilAdjustX, &bRecoilAdjustY );
	GetBaseFlatRecoilModifier( pObj, &bRecoilAdjustX, &bRecoilAdjustY );
	GetAttachmentFlatRecoilModifier( &pSoldier->inv[HANDPOS], &bRecoilAdjustX, &bRecoilAdjustY);
	//JMich TODO: Currently no check for dual wielding
	// silversurfer: I don't think that we need a check for dual wielding here. This function is supposed to return weapon recoil value.
	// It doesn't matter if this gun is wielded by someone else, with right, left hand or with the toes. The gun always produces the same recoil.
	// Controlling recoil when firing two guns is more difficult but this should be checked outside this function.

	*bRecoilX = __max(0, *bRecoilX + bRecoilAdjustX);
	*bRecoilY = __max(0, *bRecoilY + bRecoilAdjustY);

	return;
}

///////////////////////////////////////////////////////
// HEADROCK HAM 4: This function calculates the flat recoil adjustment for a gun. Flat adjustment increases
// or decreases recoil by a specific number of points in either the vertical or horizontal axes (or both).
// It can potentially cause a weapon it reverse its recoil direction.
void GetBaseFlatRecoilModifier( OBJECTTYPE *pObj, FLOAT *bRecoilModifierX, FLOAT *bRecoilModifierY )
{
	FLOAT bRecoilAdjustX = 0;
	FLOAT bRecoilAdjustY = 0;
	if (pObj->exists() == true && UsingNewCTHSystem() == true)
	{
		// Inherent item modifiers
		bRecoilAdjustX += BonusReduceMoreFloat( Item[pObj->usItem].RecoilModifierX, (*pObj)[0]->data.objectStatus );
		bRecoilAdjustY += BonusReduceMoreFloat( Item[pObj->usItem].RecoilModifierY, (*pObj)[0]->data.objectStatus );

		// Ammo item modifiers
		bRecoilAdjustX += Item[(*pObj)[0]->data.gun.usGunAmmoItem].RecoilModifierX;
		bRecoilAdjustY += Item[(*pObj)[0]->data.gun.usGunAmmoItem].RecoilModifierY;
	}

	*bRecoilModifierX = bRecoilAdjustX;
	*bRecoilModifierY = bRecoilAdjustY;
}
void GetAttachmentFlatRecoilModifier( OBJECTTYPE *pObj, FLOAT *bRecoilModifierX, FLOAT *bRecoilModifierY )
{
	FLOAT bRecoilAdjustX = 0;
	FLOAT bRecoilAdjustY = 0;
	if (pObj->exists() == true && UsingNewCTHSystem() == true)
	{
		// Attachment item modifiers
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists())
			{
				bRecoilAdjustX += BonusReduceMoreFloat( Item[iter->usItem].RecoilModifierX, (*iter)[0]->data.objectStatus );
				bRecoilAdjustY += BonusReduceMoreFloat( Item[iter->usItem].RecoilModifierY, (*iter)[0]->data.objectStatus );
			}
		}
	}

	*bRecoilModifierX += bRecoilAdjustX;
	*bRecoilModifierY += bRecoilAdjustY;
}
void GetFlatRecoilModifier( OBJECTTYPE *pObj, FLOAT *bRecoilModifierX, FLOAT *bRecoilModifierY )
{

	FLOAT bRecoilAdjustX = 0;
	FLOAT bRecoilAdjustY = 0;

	if (pObj->exists() == true && UsingNewCTHSystem() == true)
	{
		// Inherent item modifiers
		bRecoilAdjustX += BonusReduceMoreFloat( Item[pObj->usItem].RecoilModifierX, (*pObj)[0]->data.objectStatus );
		bRecoilAdjustY += BonusReduceMoreFloat( Item[pObj->usItem].RecoilModifierY, (*pObj)[0]->data.objectStatus );

		// Ammo item modifiers
		bRecoilAdjustX += Item[(*pObj)[0]->data.gun.usGunAmmoItem].RecoilModifierX;
		bRecoilAdjustY += Item[(*pObj)[0]->data.gun.usGunAmmoItem].RecoilModifierY;

		// Attachment item modifiers
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists())
			{
				bRecoilAdjustX += BonusReduceMoreFloat( Item[iter->usItem].RecoilModifierX, (*iter)[0]->data.objectStatus );
				bRecoilAdjustY += BonusReduceMoreFloat( Item[iter->usItem].RecoilModifierY, (*iter)[0]->data.objectStatus );
			}
		}
	}

	*bRecoilModifierX = bRecoilAdjustX;
	*bRecoilModifierY = bRecoilAdjustY;
}

///////////////////////////////////////////////////////////////////
// HEADROCK HAM 4: This calculates the percentile recoil adjustment of a gun.
// This adjustment either increases or decreases the gun's vertical and horizontal recoil at the same time. Due to
// the percentage-based nature of this modifier, it cannot cause a gun to reverse its recoil - only diminish it to
// zero.
INT16 GetBasePercentRecoilModifier( OBJECTTYPE *pObj)
{
	INT16 sRecoilAdjust = 0;

	if (pObj->exists() == true && UsingNewCTHSystem() == true)
	{
		// Inherent item modifiers
		sRecoilAdjust += BonusReduceMore( Item[pObj->usItem].PercentRecoilModifier, (*pObj)[0]->data.objectStatus );

		// Ammo item modifiers
		sRecoilAdjust += Item[(*pObj)[0]->data.gun.usGunAmmoItem].PercentRecoilModifier;
	}

	return (sRecoilAdjust);
}
INT16 GetAttachmentPercentRecoilModifier( OBJECTTYPE *pObj)
{
	INT16 sRecoilAdjust = 0;

	if (pObj->exists() == true && UsingNewCTHSystem() == true)
	{
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists())
			{
				sRecoilAdjust += BonusReduceMore( Item[iter->usItem].PercentRecoilModifier, (*iter)[0]->data.objectStatus );
			}
		}
	}

	return (sRecoilAdjust);

}
INT16 GetPercentRecoilModifier( OBJECTTYPE *pObj )
{
	INT16 sRecoilAdjust = 0;

	if (pObj->exists() == true && UsingNewCTHSystem() == true)
	{
		// Inherent item modifiers
		sRecoilAdjust += BonusReduceMore( Item[pObj->usItem].PercentRecoilModifier, (*pObj)[0]->data.objectStatus );

		// Ammo item modifiers
		sRecoilAdjust += Item[(*pObj)[0]->data.gun.usGunAmmoItem].PercentRecoilModifier;

		// Attachment item modifiers
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists())
			{
				sRecoilAdjust += BonusReduceMore( Item[iter->usItem].PercentRecoilModifier, (*iter)[0]->data.objectStatus );
			}
		}
	}

	sRecoilAdjust = __max(-100, sRecoilAdjust);
	return (sRecoilAdjust);
}

// HEADROCK HAM 4: This is used by functions that get stance-based modifiers from weapons. It turns a ubEndHeight variable
// as either 0, 1 or 2.
UINT8 GetStanceModifierRef( INT8 ubStance )
{
	switch (ubStance)
	{
		case ANIM_PRONE:
			return(2);
		case ANIM_CROUCH:
			return(1);
		default:
			return(0);
	}
}

INT16 GetDamageBonus( OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	if (pObj->exists() == true) {
		bonus = BonusReduce( Item[pObj->usItem].damagebonus, (*pObj)[0]->data.objectStatus );

		if ( (*pObj)[0]->data.gun.ubGunShotsLeft > 0 )
			bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].damagebonus ;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if(iter->exists())
			{
				bonus += BonusReduce( Item[iter->usItem].damagebonus, (*iter)[0]->data.objectStatus );
			}
		}
	}

	return( bonus );
}


INT16 GetMeleeDamageBonus( OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	if (pObj->exists() == true) {
		bonus = BonusReduce( Item[pObj->usItem].meleedamagebonus, (*pObj)[0]->data.objectStatus);
		bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].meleedamagebonus ;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if(iter->exists())
			{
				bonus += BonusReduce( Item[iter->usItem].meleedamagebonus, (*iter)[0]->data.objectStatus );
			}
		}
	}
	return( bonus );
}


INT16 GetPercentAPReduction( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	BOOLEAN fUseCowitnessSight = FALSE;

	if (pObj->exists() == true) 
	{
		if ( gGameExternalOptions.fScopeModes && pSoldier )
		{
			bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].percentapreduction;

			// Flugente: check for scope mode
			if (Item[pObj->usItem].usItemClass == IC_GUN)
			{
				std::map<INT8, OBJECTTYPE*> ObjList;
				GetScopeLists(pSoldier, pObj, ObjList);

				// only use scope mode if gun is in hand, otherwise an error might occur!
				if ((&pSoldier->inv[HANDPOS]) == pObj  && ObjList[pSoldier->bScopeMode] != NULL && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD)
				{
					bonus += BonusReduceMore(Item[ObjList[pSoldier->bScopeMode]->usItem].percentapreduction, (*ObjList[pSoldier->bScopeMode])[0]->data.objectStatus);

					if (IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT) || IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE))
						fUseCowitnessSight = TRUE;
				}
			}

			for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
			{
				if (iter->exists() && (!IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) && (!IsAttachmentClass(iter->usItem, AC_EXTERNAL) || fUseCowitnessSight)))
				{
					bonus += BonusReduceMore( Item[iter->usItem].percentapreduction, (*iter)[0]->data.objectStatus );
				}
			}
		}
		else
		{
			bonus = BonusReduceMore(Item[pObj->usItem].percentapreduction, (*pObj)[0]->data.objectStatus);
			bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].percentapreduction;

			for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
			{
				if(iter->exists())
				{
					bonus += BonusReduceMore( Item[iter->usItem].percentapreduction, (*iter)[0]->data.objectStatus );
				}
			}
		}
	}

	return bonus;
}

INT16 GetMagSizeBonus( OBJECTTYPE * pObj, UINT8 subObject )
{
	INT16 bonus = 0;
	if (pObj->exists() == true) {
		bonus = BonusOnOff( Item[pObj->usItem].magsizebonus, (*pObj)[subObject]->data.objectStatus );

		for (attachmentList::iterator iter = (*pObj)[subObject]->attachments.begin(); iter != (*pObj)[subObject]->attachments.end(); ++iter) 
		{
			if(iter->exists())
			{
				bonus += BonusOnOff( Item[iter->usItem].magsizebonus, (*iter)[0]->data.objectStatus );
			}
		}
	}
	return( bonus );
}

INT16 GetBurstSizeBonus( OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	if (pObj->exists() == true) {
		bonus = BonusOnOff( Item[pObj->usItem].burstsizebonus, (*pObj)[0]->data.objectStatus );
		bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].burstsizebonus ;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if(iter->exists())
			{
				bonus += BonusOnOff( Item[iter->usItem].burstsizebonus, (*iter)[0]->data.objectStatus );
			}
		}
	}

	return( bonus );
}

INT16 GetRateOfFireBonus( OBJECTTYPE * pObj )
{
	INT16 bonus=0;

	if (pObj->exists() == true) {
		if ((gMAXITEMS_READ <= pObj->usItem) || (gMAXITEMS_READ <= (*pObj)[0]->data.gun.usGunAmmoItem))
		{
			DebugMsg(TOPIC_JA2, DBG_LEVEL_1, String("GetRateOfFireBonus would crash: pObj->usItem=%d or (*pObj)[0]->data.gun.usGunAmmoItem=%d ist higher than max %d", pObj->usItem, (*pObj)[0]->data.gun.usGunAmmoItem, gMAXITEMS_READ));
			ScreenMsg(MSG_FONT_RED, MSG_DEBUG, L"GetRateOfFireBonus would crash: pObj->usItem=%d or (*pObj)[0]->data.gun.usGunAmmoItem=%d ist higher than max %d", pObj->usItem, (*pObj)[0]->data.gun.usGunAmmoItem, gMAXITEMS_READ);
			AssertMsg( 0, "GetRateOfFireBonus would crash" );
			return 0; /* cannot calculate Bonus, this only happens sometimes in FULLSCREEN */
		}

		bonus = BonusReduceMore( Item[pObj->usItem].rateoffirebonus, (*pObj)[0]->data.objectStatus );
		bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].rateoffirebonus ;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if(iter->exists())
			{
				bonus += BonusReduceMore( Item[iter->usItem].rateoffirebonus, (*iter)[0]->data.objectStatus );
			}
		}
	}
	return( bonus );
}

INT16 GetAutoToHitBonus( SOLDIERTYPE *pSoldier, OBJECTTYPE * pObj, BOOLEAN fProneStance )
{
	///////////////////////////////////////////////////////////////////
	// sevenfm: added scope mode support
	///////////////////////////////////////////////////////////////////
	INT16 bonus = 0;
	BOOLEAN fUseCowitnessSight = FALSE;

	if (pObj->exists() == true) 
	{
		if( fProneStance )
			bonus += BonusReduceMore( Item[pObj->usItem].bipod, (*pObj)[0]->data.objectStatus );
		else
			bonus += BonusReduceMore( Item[pObj->usItem].autofiretohitbonus, (*pObj)[0]->data.objectStatus );

		// sevenfm: tracers remove additional penalty from distance instead
		if (!gGameExternalOptions.fOCTHTracerCode)
			bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].autofiretohitbonus;

		// Flugente: check for scope mode
		if ( gGameExternalOptions.fScopeModes && pSoldier && Item[pObj->usItem].usItemClass == IC_GUN )
		{
			std::map<INT8, OBJECTTYPE*> ObjList;
			GetScopeLists(pSoldier, pObj, ObjList);

			// only use scope mode if gun is in hand, otherwise an error might occur!
			if ( (&pSoldier->inv[HANDPOS]) == pObj && ObjList[pSoldier->bScopeMode] != NULL && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD)
			{
				bonus += BonusReduceMore( Item[ObjList[pSoldier->bScopeMode]->usItem].autofiretohitbonus, (*pObj)[0]->data.objectStatus );

				if (IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT) || IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE))
					fUseCowitnessSight = TRUE;
			}
		}

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() &&
				(!gGameExternalOptions.fScopeModes || !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) && (!IsAttachmentClass(iter->usItem, AC_EXTERNAL) || fUseCowitnessSight)) &&
				AttachmentHandlingAllowed(pSoldier, pObj, iter->usItem))
			{
				if ( fProneStance )
					bonus += BonusReduceMore( Item[iter->usItem].bipod, (*iter)[0]->data.objectStatus );
				else
					bonus += BonusReduceMore( Item[iter->usItem].autofiretohitbonus, (*iter)[0]->data.objectStatus );
			}
		}
	}

	return( bonus );
}

/*INT16 GetPercentReadyTimeAPReduction(SOLDIERTYPE *pSoldier, OBJECTTYPE * pObj)
{
	INT16 bonus = 0;

	if (pObj->exists() == true)
	{
		// base item bonus
		bonus = BonusReduceMore(Item[pObj->usItem].percentreadytimeapreduction, (*pObj)[0]->data.objectStatus);
		// ammo bonus
		bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].percentreadytimeapreduction;

		if (gGameExternalOptions.fScopeModes && pSoldier)
		{
			// check for scope mode
			if (Item[pObj->usItem].usItemClass == IC_GUN)
			{
				std::map<INT8, OBJECTTYPE*> ObjList;
				GetScopeLists(pSoldier, pObj, ObjList);

				// only use scope mode if gun is in hand, otherwise an error might occur!
				if ((&pSoldier->inv[HANDPOS]) == pObj  && ObjList[pSoldier->bScopeMode] != NULL && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD)
					bonus += BonusReduceMore(Item[ObjList[pSoldier->bScopeMode]->usItem].percentreadytimeapreduction, (*ObjList[pSoldier->bScopeMode])[0]->data.objectStatus);
			}

			for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
			{
				if (iter->exists() && !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE))
				{
					bonus += BonusReduceMore(Item[iter->usItem].percentreadytimeapreduction, (*iter)[0]->data.objectStatus);
				}
			}
		}
		else
		{
			for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
			{
				if (iter->exists())
				{
					bonus += BonusReduceMore(Item[iter->usItem].percentreadytimeapreduction, (*iter)[0]->data.objectStatus);
				}
			}
		}
	}

	return bonus;
}*/

INT16 GetPercentReadyTimeAPReduction(SOLDIERTYPE *pSoldier, OBJECTTYPE * pObj)
{
	INT16 bonus = 0;
	if (pObj->exists() == true)
	{
		bonus = BonusReduceMore(Item[pObj->usItem].percentreadytimeapreduction, (*pObj)[0]->data.objectStatus);
		bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].percentreadytimeapreduction;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists() &&
				AttachmentHandlingAllowed(pSoldier, pObj, iter->usItem))
			{
				bonus += BonusReduceMore(Item[iter->usItem].percentreadytimeapreduction, (*iter)[0]->data.objectStatus);
			}
		}
	}

	return(bonus);
}

INT16 GetPercentAutofireAPReduction( OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	if (pObj->exists() == true) {
		bonus = BonusReduceMore( Item[pObj->usItem].percentautofireapreduction, (*pObj)[0]->data.objectStatus );
		bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].percentautofireapreduction;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if(iter->exists())
			{
				bonus += BonusReduceMore( Item[iter->usItem].percentautofireapreduction, (*iter)[0]->data.objectStatus );
			}
		}
	}

	return( bonus );
}

INT16 GetPercentReloadTimeAPReduction( OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	if (pObj->exists() == true) {
		bonus = BonusReduceMore( Item[pObj->usItem].percentreloadtimeapreduction, (*pObj)[0]->data.objectStatus );
		bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].percentreloadtimeapreduction;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if(iter->exists())
			{
				bonus += BonusReduceMore( Item[iter->usItem].percentreloadtimeapreduction, (*iter)[0]->data.objectStatus );
			}
		}
	}
	return( bonus );
}

INT16 GetPercentBurstFireAPReduction( OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	if (pObj->exists() == true) {
		bonus = BonusReduceMore( Item[pObj->usItem].percentburstfireapreduction, (*pObj)[0]->data.objectStatus );
		bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].percentburstfireapreduction;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if(iter->exists())
			{
				bonus += BonusReduceMore( Item[iter->usItem].percentburstfireapreduction, (*iter)[0]->data.objectStatus );
			}
		}
	}
	return( bonus );
}


INT16 GetVisionRangeBonus(SOLDIERTYPE * pSoldier, INT32 sSpot, INT8 bLevel)
{
	Assert(pSoldier);

	INT16	sBonus = 0;
	INT16	sItemBonus;
	OBJECTTYPE *pObj;
	UINT16	usItem;
	INVTYPE *pItem;

	INT16 sScopebonus = 0;
	INT16 sHandBonus = 0;

	BOOLEAN fScope = FALSE;
	BOOLEAN fBinocs = FALSE;
	BOOLEAN fUseCowitnessSight = FALSE;

	//INT16 sBaseDistance = TACTICAL_RANGE / 2;
	//INT16 sDistance = TileIsOutOfBounds(sSpot) ? sBaseDistance : max(sBaseDistance, PythSpacesAway(pSoldier->sGridNo, sSpot));
	//UINT8 ubWatchTurns = sDistance / sBaseDistance;
	//UINT8 ubWatchTurns = 1;
	//UINT8 ubWatchPoints = TileIsOutOfBounds(sSpot) ? 0 : GetWatchedLocPoints(pSoldier->ubID, sSpot, bLevel);
	BOOLEAN fWatchedBonus = FALSE;	// !pSoldier->IsFocusing() && ubWatchPoints >= ubWatchTurns

	// scope bonus (only when focusing or checking for max vision or OCTH scope bonus)
	if (WeaponReady(pSoldier) && (gbForceWeaponReady || pSoldier->IsFocusing(sSpot, bLevel) || fWatchedBonus))
	{
		// SANDRO - added scouting check		
		pObj = &(pSoldier->inv[HANDPOS]);

		if (pObj->exists())
		{
			if (gGameExternalOptions.fScopeModes)
			{
				if (CanUseScopeMode(pObj->usItem))
				{
					// Flugente: check for scope mode
					std::map<INT8, OBJECTTYPE*> ObjList;
					GetScopeLists(pSoldier, pObj, ObjList);

					// only use scope mode if gun is in hand, otherwise an error might occur!
					if (pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD && ObjList[pSoldier->bScopeMode] != NULL)
					{
						// now apply the bonus from the scope we use
						sScopebonus += BonusReduceMore(Item[ObjList[pSoldier->bScopeMode]->usItem].visionrangebonus, (*ObjList[pSoldier->bScopeMode])[0]->data.objectStatus);
						fScope = TRUE;

						if (IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT) || IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE))
							fUseCowitnessSight = TRUE;
					}
				}

				for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
				{
					// add boni only from non-scope items
					if (iter->exists() && !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) && (!IsAttachmentClass(iter->usItem, AC_EXTERNAL) || fUseCowitnessSight))
					{
						sScopebonus += BonusReduceMore(Item[iter->usItem].visionrangebonus, (*iter)[0]->data.objectStatus);
					}
				}

			}
			else
			{
				for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
				{
					if (iter->exists())
					{
						sScopebonus += BonusReduceMore(Item[iter->usItem].visionrangebonus, (*iter)[0]->data.objectStatus);
						if (IsAttachmentClass(iter->usItem, AC_SCOPE_MODE))
							fScope = TRUE;
					}
				}
			}

			if (sScopebonus > 0 && HAS_SKILL_TRAIT(pSoldier, SCOUTING_NT) && gGameOptions.fNewTraitSystem)
			{
				sScopebonus += gSkillTraitValues.ubSCSightRangebonusWithScopes;
			}
			sBonus += sScopebonus;
		}		
	}

	// binocs/scope in hand
	if (gbForceBinocsReady || pSoldier->IsSpotting(sSpot, bLevel))
	{
		pObj = &(pSoldier->inv[HANDPOS]);

		if (pObj->exists())
		{
			usItem = pObj->usItem;
			pItem = &(Item[usItem]);

			// Snap (TODO): binoculars and such should not be active by default
			if ((pItem->usItemClass & IC_MISC) &&
				!(pItem->usItemClass & IC_ARMOUR || pItem->usItemClass & IC_FACE) &&
				!IsWeapon(usItem) &&
				(!Item[usItem].needsbatteries || FindAttachedBatteries(pObj)))
			{
				sItemBonus = BonusReduceMore(pItem->visionrangebonus, (*pObj)[0]->data.objectStatus);

				sHandBonus += sItemBonus;
				if (pItem->visionrangebonus > 0 || pItem->dayvisionrangebonus > 0 || pItem->brightlightvisionrangebonus > 0 || pItem->nightvisionrangebonus > 0 || pItem->cavevisionrangebonus > 0)
					fBinocs = TRUE;
			}			
		}

		sBonus += sHandBonus;
	}

	// gear bonus
	for (int i = HELMETPOS; i <= HEAD2POS; ++i)
	{
		if (i == VESTPOS || i == LEGPOS)
			continue;

		pObj = &(pSoldier->inv[i]);

		if (pObj->exists() && !IsWeapon(pObj->usItem))
		{
			usItem = pObj->usItem;
			pItem = &(Item[usItem]);

			// sevenfm: check batteries
			if (Item[usItem].needsbatteries && !FindAttachedBatteries(pObj))
				continue;

			sItemBonus = BonusReduceMore(pItem->visionrangebonus, (*pObj)[0]->data.objectStatus);

			// sevenfm: apply gear bonus only if not using scope/binocs
			if (!fScope && !fBinocs && !gbForceWeaponReady && !gbForceWeaponNotReady && !gbForceBinocsReady ||
				pItem->nightvisionrangebonus <= 0 && pItem->cavevisionrangebonus <= 0)
				sBonus += sItemBonus;
		}
	}

	return sBonus;
}

// Snap: Scale night vision bonus with light level
INT16 NightBonusScale( INT16 bonus, UINT8 bLightLevel )
{
	// sevenfm: full night vision bonus in darkness
	if (bLightLevel > NORMAL_LIGHTLEVEL_NIGHT)
		return bonus;
	else if (bLightLevel > NORMAL_LIGHTLEVEL_DAY)
		return idiv(bonus * (bLightLevel - NORMAL_LIGHTLEVEL_DAY), NORMAL_LIGHTLEVEL_NIGHT - NORMAL_LIGHTLEVEL_DAY);
	else 
		return 0;

	/*if (bLightLevel > NORMAL_LIGHTLEVEL_NIGHT)
		return idiv(bonus * (SHADE_MIN - bLightLevel), SHADE_MIN - NORMAL_LIGHTLEVEL_NIGHT);
	else if (bLightLevel > NORMAL_LIGHTLEVEL_DAY)
		return idiv(bonus * (bLightLevel - NORMAL_LIGHTLEVEL_DAY), NORMAL_LIGHTLEVEL_NIGHT - NORMAL_LIGHTLEVEL_DAY);*/	
}

// sevenfm: scale day vision bonus with light level
INT16 DayBonusScale(INT16 bonus, UINT8 bLightLevel)
{
	// sevenfm: full day vision bonus in bright light
	if (bLightLevel < NORMAL_LIGHTLEVEL_DAY)
		return bonus;
	else if (bLightLevel < NORMAL_LIGHTLEVEL_NIGHT)
		return idiv(bonus * (NORMAL_LIGHTLEVEL_NIGHT - bLightLevel), NORMAL_LIGHTLEVEL_NIGHT - NORMAL_LIGHTLEVEL_DAY);
	else
		return 0;
}

INT16 GetNightVisionRangeBonus(SOLDIERTYPE * pSoldier, UINT8 bLightLevel, INT32 sSpot, INT8 bLevel)
{
	Assert(pSoldier);

	INT16	sBonus = 0;
	INT16	sItemBonus;
	OBJECTTYPE *pObj;
	UINT16	usItem;
	INVTYPE *pItem;

	INT16 sScopebonus = 0;
	INT16 sHandBonus = 0;

	BOOLEAN fScope = FALSE;
	BOOLEAN fBinocs = FALSE;
	BOOLEAN fUseCowitnessSight = FALSE;

	//INT16 sBaseDistance = TACTICAL_RANGE / 2;
	//INT16 sDistance = TileIsOutOfBounds(sSpot) ? sBaseDistance : max(sBaseDistance, PythSpacesAway(pSoldier->sGridNo, sSpot));
	//UINT8 ubWatchTurns = sDistance / sBaseDistance;
	//UINT8 ubWatchTurns = 1;
	//UINT8 ubWatchPoints = TileIsOutOfBounds(sSpot) ? 0 : GetWatchedLocPoints(pSoldier->ubID, sSpot, bLevel);
	BOOLEAN fWatchedBonus = FALSE;	// !pSoldier->IsFocusing() && ubWatchPoints >= ubWatchTurns

	// scope bonus (only when focusing or checking for max vision or OCTH scope bonus)
	if (WeaponReady(pSoldier) && (gbForceWeaponReady ||	pSoldier->IsFocusing(sSpot, bLevel) || fWatchedBonus))
	{
		// SANDRO - added scouting check		
		pObj = &(pSoldier->inv[HANDPOS]);

		if (pObj->exists())
		{
			if (gGameExternalOptions.fScopeModes)
			{				
				if (CanUseScopeMode(pObj->usItem))
				{
					// Flugente: check for scope mode
					std::map<INT8, OBJECTTYPE*> ObjList;
					GetScopeLists(pSoldier, pObj, ObjList);

					// only use scope mode if gun is in hand, otherwise an error might occur!
					if (pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD && ObjList[pSoldier->bScopeMode] != NULL)
					{
						sScopebonus += BonusReduceMore(NightBonusScale(Item[ObjList[pSoldier->bScopeMode]->usItem].nightvisionrangebonus, bLightLevel), (*ObjList[pSoldier->bScopeMode])[0]->data.objectStatus);
						fScope = TRUE;
						if (IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT) || IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE))
							fUseCowitnessSight = TRUE;
					}
				}

				for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
				{
					// add boni only from non-scope items
					if (iter->exists() && !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) && (!IsAttachmentClass(iter->usItem, AC_EXTERNAL) || fUseCowitnessSight))
					{
						sScopebonus += BonusReduceMore(NightBonusScale(Item[iter->usItem].nightvisionrangebonus, bLightLevel), (*iter)[0]->data.objectStatus);
					}
				}
			}
			else
			{
				for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
				{
					if (iter->exists())
					{
						sScopebonus += BonusReduceMore(NightBonusScale(Item[iter->usItem].nightvisionrangebonus, bLightLevel), (*iter)[0]->data.objectStatus);
						if (IsAttachmentClass(iter->usItem, AC_SCOPE_MODE))
							fScope = TRUE;
					}
				}
			}

			if (sScopebonus > 0 && gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, SCOUTING_NT))
			{
				sScopebonus += gSkillTraitValues.ubSCSightRangebonusWithScopes;
			}
			sBonus += sScopebonus;
		}
	}

	// binocs/scope in hand
	if (gbForceBinocsReady || pSoldier->IsSpotting(sSpot, bLevel))
	{
		pObj = &(pSoldier->inv[HANDPOS]);

		if (pObj->exists())
		{
			usItem = pObj->usItem;
			pItem = &(Item[usItem]);

			// armour/head items don't work from hands
			if ((pItem->usItemClass & IC_MISC) &&
				!(pItem->usItemClass & IC_ARMOUR || pItem->usItemClass & IC_FACE) &&
				!IsWeapon(usItem) &&
				(!Item[usItem].needsbatteries || FindAttachedBatteries(pObj)))
			{
				sItemBonus = BonusReduceMore(NightBonusScale(pItem->nightvisionrangebonus, bLightLevel), (*pObj)[0]->data.objectStatus);

				sHandBonus += sItemBonus;
				if (pItem->visionrangebonus > 0 || pItem->dayvisionrangebonus > 0 || pItem->brightlightvisionrangebonus > 0 || pItem->nightvisionrangebonus > 0 || pItem->cavevisionrangebonus > 0)
					fBinocs = TRUE;
			}			
		}

		sBonus += sHandBonus;
	}

	// gear bonus
	for (int i = HELMETPOS; i <= HEAD2POS; ++i)
	{
		if (i == VESTPOS || i == LEGPOS)
			continue;

		// More optimization
		pObj = &( pSoldier->inv[i]);
		if (pObj->exists() && !IsWeapon(pObj->usItem))
		{
			usItem = pObj->usItem;
			pItem = &(Item[usItem]);

			// sevenfm: check batteries
			if (Item[usItem].needsbatteries && !FindAttachedBatteries(pObj))
				continue;
			
			sItemBonus = BonusReduceMore(NightBonusScale(pItem->nightvisionrangebonus, bLightLevel), (*pObj)[0]->data.objectStatus);
			// sevenfm: apply gear bonus only if not using scope/binocs
			if (!fScope && !fBinocs && !gbForceWeaponReady && !gbForceWeaponNotReady && !gbForceBinocsReady ||
				pItem->nightvisionrangebonus <= 0 && pItem->cavevisionrangebonus <= 0)
				sBonus += sItemBonus;
		}
	}

	return( sBonus );
}

INT16 GetCaveVisionRangeBonus(SOLDIERTYPE * pSoldier, UINT8 bLightLevel, INT32 sSpot, INT8 bLevel)
{
	Assert(pSoldier);

	INT16	sBonus = 0;
	INT16	sItemBonus;
	OBJECTTYPE *pObj;
	UINT16	usItem;
	INVTYPE *pItem;

	INT16 sScopebonus = 0;
	INT16 sHandBonus = 0;

	BOOLEAN fScope = FALSE;
	BOOLEAN fBinocs = FALSE;
	BOOLEAN fUseCowitnessSight = FALSE;

	//INT16 sBaseDistance = TACTICAL_RANGE / 2;
	//INT16 sDistance = TileIsOutOfBounds(sSpot) ? sBaseDistance : max(sBaseDistance, PythSpacesAway(pSoldier->sGridNo, sSpot));
	//UINT8 ubWatchTurns = sDistance / sBaseDistance;
	//UINT8 ubWatchTurns = 1;
	//UINT8 ubWatchPoints = TileIsOutOfBounds(sSpot) ? 0 : GetWatchedLocPoints(pSoldier->ubID, sSpot, bLevel);
	BOOLEAN fWatchedBonus = FALSE;	// !pSoldier->IsFocusing() && ubWatchPoints >= ubWatchTurns	

	// scope bonus (only when focusing or checking for max vision or OCTH scope bonus)
	if (WeaponReady(pSoldier) && (gbForceWeaponReady || pSoldier->IsFocusing(sSpot, bLevel) || fWatchedBonus))
	{
		// SANDRO - added scouting check		
		pObj = &(pSoldier->inv[HANDPOS]);

		if (pObj->exists())
		{
			if (gGameExternalOptions.fScopeModes)
			{
				if (CanUseScopeMode(pObj->usItem))
				{
					// Flugente: check for scope mode
					std::map<INT8, OBJECTTYPE*> ObjList;
					GetScopeLists(pSoldier, pObj, ObjList);

					// only use scope mode if gun is in hand, otherwise an error might occur!
					if (pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD && ObjList[pSoldier->bScopeMode] != NULL)
					{
						sScopebonus += BonusReduceMore(NightBonusScale(Item[ObjList[pSoldier->bScopeMode]->usItem].cavevisionrangebonus, bLightLevel), (*ObjList[pSoldier->bScopeMode])[0]->data.objectStatus);
						fScope = TRUE;
						if (IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT) || IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE))
							fUseCowitnessSight = TRUE;
					}
				}

				for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
				{
					// add boni only from non-scope items
					if (iter->exists() && !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) && (!IsAttachmentClass(iter->usItem, AC_EXTERNAL) || fUseCowitnessSight))
					{
						sScopebonus += BonusReduceMore(NightBonusScale(Item[iter->usItem].cavevisionrangebonus, bLightLevel), (*iter)[0]->data.objectStatus);
					}
				}
			}
			else
			{
				for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
				{
					if (iter->exists())
					{
						sScopebonus += BonusReduceMore(NightBonusScale(Item[iter->usItem].cavevisionrangebonus, bLightLevel), (*iter)[0]->data.objectStatus);
						if (IsAttachmentClass(iter->usItem, AC_SCOPE_MODE))
							fScope = TRUE;
					}
				}
			}

			if (sScopebonus > 0 && HAS_SKILL_TRAIT(pSoldier, SCOUTING_NT) && gGameOptions.fNewTraitSystem)
			{
				sScopebonus += gSkillTraitValues.ubSCSightRangebonusWithScopes;
			}
			sBonus += sScopebonus;
		}
	}

	// binocs/scope in hand
	if (gbForceBinocsReady || pSoldier->IsSpotting(sSpot, bLevel))
	{
		pObj = &(pSoldier->inv[HANDPOS]);

		if (pObj->exists())
		{
			usItem = pObj->usItem;
			pItem = &(Item[usItem]);

			if ((pItem->usItemClass & IC_MISC) &&
				!(pItem->usItemClass & IC_ARMOUR || pItem->usItemClass & IC_FACE) &&
				!IsWeapon(usItem) &&
				(!Item[usItem].needsbatteries || FindAttachedBatteries(pObj)))
			{
				sItemBonus = BonusReduceMore(NightBonusScale(pItem->cavevisionrangebonus, bLightLevel), (*pObj)[0]->data.objectStatus);

				sHandBonus += sItemBonus;
				if (pItem->visionrangebonus > 0 || pItem->dayvisionrangebonus > 0 || pItem->brightlightvisionrangebonus > 0 || pItem->nightvisionrangebonus > 0 || pItem->cavevisionrangebonus > 0)
					fBinocs = TRUE;
			}
		}

		sBonus += sHandBonus;
	}
	
	// gear bonus
	for (int i = HELMETPOS; i <= HEAD2POS; ++i)
	{
		if (i == VESTPOS || i == LEGPOS)
			continue;

		// More optimization
		pObj = &(pSoldier->inv[i]);

		if (pObj->exists() && !IsWeapon(pObj->usItem))
		{
			usItem = pObj->usItem;
			pItem = &(Item[usItem]);

			// sevenfm: check batteries
			if (Item[usItem].needsbatteries && !FindAttachedBatteries(pObj))
				continue;

			sItemBonus = BonusReduceMore(NightBonusScale( pItem->cavevisionrangebonus, bLightLevel ), (*pObj)[0]->data.objectStatus );

			// sevenfm: apply gear bonus only if not using scope/binocs
			if (!fScope && !fBinocs && !gbForceWeaponReady && !gbForceWeaponNotReady && !gbForceBinocsReady ||
				pItem->nightvisionrangebonus <= 0 && pItem->cavevisionrangebonus <= 0)
				sBonus += sItemBonus;
		}
	}

	return( sBonus );
}

INT16 GetDayVisionRangeBonus(SOLDIERTYPE * pSoldier, UINT8 bLightLevel, INT32 sSpot, INT8 bLevel)
{
	Assert(pSoldier);

	INT16	sBonus = 0;
	INT16	sItemBonus;
	OBJECTTYPE *pObj;
	UINT16	usItem;
	INVTYPE *pItem;
		
	INT16 sScopebonus = 0;
	INT16 sHandBonus = 0;

	BOOLEAN fScope = FALSE;
	BOOLEAN fBinocs = FALSE;
	BOOLEAN fUseCowitnessSight = FALSE;

	//INT16 sBaseDistance = TACTICAL_RANGE / 2;
	//INT16 sDistance = TileIsOutOfBounds(sSpot) ? sBaseDistance : max(sBaseDistance, PythSpacesAway(pSoldier->sGridNo, sSpot));
	//UINT8 ubWatchTurns = sDistance / sBaseDistance;
	//UINT8 ubWatchTurns = 1;
	//UINT8 ubWatchPoints = TileIsOutOfBounds(sSpot) ? 0 : GetWatchedLocPoints(pSoldier->ubID, sSpot, bLevel);
	BOOLEAN fWatchedBonus = FALSE;	// !pSoldier->IsFocusing() && ubWatchPoints >= ubWatchTurns

	int lightlevelmultiplier;
	int lightleveldivisor;
	// Flugente 2013-06-20: determine the lightlevel modifier, according to ChrisL:
	//CHRISL: Since this is a daytime calculation, I think we want the difference between NORMAL_LIGHTLEVEL_NIGHT and
	//	NORMAL_LIGHTLEVEL_DAY.  To just use NORMAL_LIGHTLEVEL_NIGHT is the same as basing the calculation off of the
	//	difference between NORMAL_LIGHTLEVEL_NIGHT and 0, which represent bright light.
	if (bLightLevel >= NORMAL_LIGHTLEVEL_DAY)
	{
		lightlevelmultiplier = NORMAL_LIGHTLEVEL_NIGHT - bLightLevel;
		lightleveldivisor = NORMAL_LIGHTLEVEL_NIGHT - NORMAL_LIGHTLEVEL_DAY;
	}
	else
	{
		lightlevelmultiplier = bLightLevel;
		lightleveldivisor = NORMAL_LIGHTLEVEL_DAY;
	}

	// scope bonus (only when focusing or checking for max vision or OCTH scope bonus)
	if (WeaponReady(pSoldier) && (gbForceWeaponReady || pSoldier->IsFocusing(sSpot, bLevel) || fWatchedBonus))
	{
		// SANDRO - added scouting check		
		pObj = &(pSoldier->inv[HANDPOS]);

		if (pObj->exists())
		{
			if (gGameExternalOptions.fScopeModes)
			{
				if (CanUseScopeMode(pObj->usItem))
				{
					// Flugente: check for scope mode
					std::map<INT8, OBJECTTYPE*> ObjList;
					GetScopeLists(pSoldier, pObj, ObjList);

					// only use scope mode if gun is in hand, otherwise an error might occur!
					if (pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD && ObjList[pSoldier->bScopeMode] != NULL)
					{
						// now apply the bonus from the scope we use
						sScopebonus += BonusReduceMore(idiv(Item[ObjList[pSoldier->bScopeMode]->usItem].dayvisionrangebonus	* lightlevelmultiplier, lightleveldivisor), (*ObjList[pSoldier->bScopeMode])[0]->data.objectStatus);
						fScope = TRUE;
						if (IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT) || IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE))
							fUseCowitnessSight = TRUE;
					}
				}

				for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
				{
					// add boni only from non-scope items
					if (iter->exists() && !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) && (!IsAttachmentClass(iter->usItem, AC_EXTERNAL) || fUseCowitnessSight))
					{
						sScopebonus += BonusReduceMore(idiv(Item[iter->usItem].dayvisionrangebonus * lightlevelmultiplier, lightleveldivisor), (*iter)[0]->data.objectStatus);
					}
				}
			}
			else
			{
				for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
				{
					if (iter->exists())
					{
						sScopebonus += BonusReduceMore(idiv(Item[iter->usItem].dayvisionrangebonus * lightlevelmultiplier, lightleveldivisor), (*iter)[0]->data.objectStatus);
						if (IsAttachmentClass(iter->usItem, AC_SCOPE_MODE))
							fScope = TRUE;
					}
				}
			}

			if (sScopebonus > 0 && HAS_SKILL_TRAIT(pSoldier, SCOUTING_NT) && gGameOptions.fNewTraitSystem)
			{
				sScopebonus += gSkillTraitValues.ubSCSightRangebonusWithScopes;
			}
			sBonus += sScopebonus;
		}
	}

	// binocs/scope in hand
	if (gbForceBinocsReady || pSoldier->IsSpotting(sSpot, bLevel))
	{
		pObj = &(pSoldier->inv[HANDPOS]);

		if (pObj->exists())
		{
			usItem = pObj->usItem;
			pItem = &(Item[usItem]);

			if ((pItem->usItemClass & IC_MISC) &&
				!(pItem->usItemClass & IC_ARMOUR || pItem->usItemClass & IC_FACE) &&
				!IsWeapon(usItem) &&
				(!Item[usItem].needsbatteries || FindAttachedBatteries(pObj)))
			{
				sItemBonus = BonusReduceMore(idiv(pItem->dayvisionrangebonus * lightlevelmultiplier, lightleveldivisor), (*pObj)[0]->data.objectStatus);

				// sevenfm: bonus for scouts using binocs
				if (gGameOptions.fNewTraitSystem &&
					HAS_SKILL_TRAIT(pSoldier, SCOUTING_NT) &&
					pSoldier->bSectorZ == 0 &&
					pItem->dayvisionrangebonus > 0)
				{
					sItemBonus += sItemBonus * gSkillTraitValues.usSCSightRangebonusWithBinoculars / 100;
				}

				sHandBonus = sItemBonus;
				if (pItem->visionrangebonus > 0 || pItem->dayvisionrangebonus > 0 || pItem->brightlightvisionrangebonus > 0 || pItem->nightvisionrangebonus > 0 || pItem->cavevisionrangebonus > 0)
					fBinocs = TRUE;
			}
		}

		sBonus += sHandBonus;
	}

	// gear bonus
	for (int i = HELMETPOS; i <= HEAD2POS; ++i)
	{
		if (i == VESTPOS || i == LEGPOS)
			continue;

		// More optimization
		pObj = &( pSoldier->inv[i]);

		if (pObj->exists() && !IsWeapon(pObj->usItem))
		{
			usItem = pObj->usItem;
			pItem = &(Item[usItem]);

			// sevenfm: check batteries
			if (Item[usItem].needsbatteries && !FindAttachedBatteries(pObj))
				continue;

			sItemBonus = BonusReduceMore(idiv(pItem->dayvisionrangebonus * lightlevelmultiplier, lightleveldivisor), (*pObj)[0]->data.objectStatus);

			// sevenfm: apply gear bonus only if not using scope/binocs
			if (!fScope && !fBinocs && !gbForceWeaponReady && !gbForceWeaponNotReady && !gbForceBinocsReady ||
				pItem->nightvisionrangebonus <= 0 && pItem->cavevisionrangebonus <= 0)
				sBonus += sItemBonus;
		}
	}

	return( sBonus );
}

INT16 GetBrightLightVisionRangeBonus(SOLDIERTYPE * pSoldier, UINT8 bLightLevel, INT32 sSpot, INT8 bLevel)
{
	Assert(pSoldier);

	INT16	sBonus = 0;
	OBJECTTYPE *pObj;
	UINT16	usItem;
	INVTYPE *pItem;
	INT16	sItemBonus;

	INT16 sScopebonus = 0;
	INT16 sHandBonus = 0;

	BOOLEAN fScope = FALSE;
	BOOLEAN fBinocs = FALSE;
	BOOLEAN fUseCowitnessSight = FALSE;

	//INT16 sBaseDistance = TACTICAL_RANGE / 2;
	//INT16 sDistance = TileIsOutOfBounds(sSpot) ? sBaseDistance : max(sBaseDistance, PythSpacesAway(pSoldier->sGridNo, sSpot));
	//UINT8 ubWatchTurns = sDistance / sBaseDistance;
	//UINT8 ubWatchTurns = 1;
	//UINT8 ubWatchPoints = TileIsOutOfBounds(sSpot) ? 0 : GetWatchedLocPoints(pSoldier->ubID, sSpot, bLevel);
	BOOLEAN fWatchedBonus = FALSE;	// !pSoldier->IsFocusing() && ubWatchPoints >= ubWatchTurns

	int lightlevelmultiplier = NORMAL_LIGHTLEVEL_DAY - bLightLevel;
	int lightleveldivisor = NORMAL_LIGHTLEVEL_DAY;

	// scope bonus (only when focusing or checking for max vision or OCTH scope bonus)
	if (WeaponReady(pSoldier) && (gbForceWeaponReady || pSoldier->IsFocusing(sSpot, bLevel) || fWatchedBonus))
	{
		// SANDRO - added scouting check
		pObj = &(pSoldier->inv[HANDPOS]);

		if (pObj->exists())
		{
			if (gGameExternalOptions.fScopeModes)
			{
				if (CanUseScopeMode(pObj->usItem))
				{
					// Flugente: check for scope mode
					std::map<INT8, OBJECTTYPE*> ObjList;
					GetScopeLists(pSoldier, pObj, ObjList);

					// only use scope mode if gun is in hand, otherwise an error might occur!
					if (pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD && ObjList[pSoldier->bScopeMode] != NULL)
					{
						// now apply the bonus from the scope we use
						sScopebonus += BonusReduceMore(idiv(Item[ObjList[pSoldier->bScopeMode]->usItem].brightlightvisionrangebonus	* lightlevelmultiplier, lightleveldivisor), (*ObjList[pSoldier->bScopeMode])[0]->data.objectStatus);
						fScope = TRUE;
						if (IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT) || IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE))
							fUseCowitnessSight = TRUE;
					}
				}

				for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
				{
					// add boni only from non-scope items
					if (iter->exists() && !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) && (!IsAttachmentClass(iter->usItem, AC_EXTERNAL) || fUseCowitnessSight))
					{
						sScopebonus += BonusReduceMore(idiv(Item[iter->usItem].brightlightvisionrangebonus * lightlevelmultiplier, lightleveldivisor), (*iter)[0]->data.objectStatus);
					}
				}
			}
			else
			{
				for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
				{
					if (iter->exists())
					{
						sScopebonus += BonusReduceMore(idiv(Item[iter->usItem].brightlightvisionrangebonus * lightlevelmultiplier, lightleveldivisor), (*iter)[0]->data.objectStatus);
						if (IsAttachmentClass(iter->usItem, AC_SCOPE_MODE))
							fScope = TRUE;
					}
				}
			}

			if (sScopebonus > 0 && HAS_SKILL_TRAIT(pSoldier, SCOUTING_NT) && gGameOptions.fNewTraitSystem)
			{
				sScopebonus += gSkillTraitValues.ubSCSightRangebonusWithScopes;
			}
			sBonus += sScopebonus;
		}		
	}

	// binocs/scope in hand
	if (gbForceBinocsReady || pSoldier->IsSpotting(sSpot, bLevel))
	{
		// More optimization
		pObj = &(pSoldier->inv[HANDPOS]);

		if (pObj->exists())
		{
			usItem = pObj->usItem;
			pItem = &(Item[usItem]);

			if ((pItem->usItemClass & IC_MISC) &&
				!(pItem->usItemClass & IC_ARMOUR || pItem->usItemClass & IC_FACE) &&
				!IsWeapon(usItem) &&
				(!Item[usItem].needsbatteries || FindAttachedBatteries(pObj)))
			{
				sItemBonus = BonusReduceMore(idiv(pItem->brightlightvisionrangebonus * lightlevelmultiplier, lightleveldivisor), (*pObj)[0]->data.objectStatus);

				// sevenfm: bonus for scouts using binocs
				if (gGameOptions.fNewTraitSystem &&
					HAS_SKILL_TRAIT(pSoldier, SCOUTING_NT) &&
					pSoldier->bSectorZ == 0 &&
					pItem->brightlightvisionrangebonus > 0)
				{
					sItemBonus += sItemBonus * gSkillTraitValues.usSCSightRangebonusWithBinoculars / 100;
				}

				sHandBonus = sItemBonus;

				if (pItem->visionrangebonus > 0 || pItem->dayvisionrangebonus > 0 || pItem->brightlightvisionrangebonus > 0 || pItem->nightvisionrangebonus > 0 || pItem->cavevisionrangebonus > 0)
					fBinocs = TRUE;
			}
		}

		sBonus += sHandBonus;
	}

	// gear bonus
	for (int i = HELMETPOS; i <= HEAD2POS; ++i)
	{
		if (i == VESTPOS || i == LEGPOS)
			continue;

		// More optimization
		pObj = &( pSoldier->inv[i]);

		if (pObj->exists() && !IsWeapon(pObj->usItem))
		{
			usItem = pObj->usItem;
			pItem = &(Item[usItem]);
			
			// sevenfm: check batteries
			if (Item[usItem].needsbatteries && !FindAttachedBatteries(pObj))
				continue;

			sItemBonus = BonusReduceMore(idiv(pItem->brightlightvisionrangebonus * lightlevelmultiplier, lightleveldivisor), (*pObj)[0]->data.objectStatus);

			// sevenfm: apply gear bonus only if not using scope
			if (!fScope && !fBinocs && !gbForceWeaponReady && !gbForceWeaponNotReady && !gbForceBinocsReady ||
				pItem->nightvisionrangebonus <= 0 && pItem->cavevisionrangebonus <= 0)
				sBonus += sItemBonus;
		}
	}

	return( sBonus );
}

bool operator<(const VisionCacheInfo& first, const VisionCacheInfo& second)
{
	if (first.bLightLevel < second.bLightLevel)
		return true;
	else if (first.bLightLevel > second.bLightLevel)
		return false;

	if (first.usItem < second.usItem)
		return true;
	else if (first.usItem > second.usItem)
		return false;

	if (first.bScopeMode < second.bScopeMode)
		return true;
	else if (first.bScopeMode > second.bScopeMode)
		return false;

	if (first.bShock < second.bShock)
		return true;
	else if (first.bShock > second.bShock)
		return false;

	if (first.bForceWeaponReady < second.bForceWeaponReady)
		return true;
	else if (first.bForceWeaponReady > second.bForceWeaponReady)
		return false;

	if (first.bForceWeaponNotReady < second.bForceWeaponNotReady)
		return true;
	else if (first.bForceWeaponNotReady > second.bForceWeaponNotReady)
		return false;

	if (first.bForceBinocsReady < second.bForceBinocsReady)
		return true;
	else if (first.bForceBinocsReady > second.bForceBinocsReady)
		return false;

	if (first.fIsFocusing < second.fIsFocusing)
		return true;
	else if (first.fIsFocusing > second.fIsFocusing)
		return false;

	if (first.fIsSpotting < second.fIsSpotting)
		return true;
	else if (first.fIsSpotting > second.fIsSpotting)
		return false;

	return false;
}

// GetVisionRangeBonus applies to all light levels
// GetNightVisionRangeBonus: bLightLevel > NORMAL_LIGHTLEVEL_DAY, above ground only
// GetCaveVisionRangeBonus: bLightLevel > NORMAL_LIGHTLEVEL_DAY, underground only
// GetDayVisionRangeBonus: NORMAL_LIGHTLEVEL_DAY <= bLightLevel < NORMAL_LIGHTLEVEL_NIGHT
// GetBrightLightVisionRangeBonus: bLightLevel < NORMAL_LIGHTLEVEL_DAY
// (remember: darker is higher!)

INT16 GetTotalVisionRangeBonus(SOLDIERTYPE *pSoldier, UINT8 bLightLevel, INT32 sSpot, INT8 bLevel)
{
	Assert(pSoldier);

	VisionCacheInfo visionInfo;

	visionInfo.bLightLevel = bLightLevel;
	visionInfo.usItem = pSoldier->inv[HANDPOS].exists() ? pSoldier->inv[HANDPOS].usItem : 0;
	visionInfo.bScopeMode = gGameExternalOptions.fScopeModes ? pSoldier->bScopeMode : 0;
	visionInfo.bShock = pSoldier->aiData.bShock;
	visionInfo.bForceWeaponReady = gbForceWeaponReady;
	visionInfo.bForceWeaponNotReady = gbForceWeaponNotReady;
	visionInfo.bForceBinocsReady = gbForceBinocsReady;
	visionInfo.fIsFocusing = pSoldier->IsFocusing(sSpot, bLevel);
	visionInfo.fIsSpotting = pSoldier->IsSpotting(sSpot, bLevel);

	std::map<VisionCacheInfo, INT16, std::less<>>::iterator cache_iter = pSoldier->mapVisionRangeCache.find(visionInfo);

	if (cache_iter != pSoldier->mapVisionRangeCache.end())
	{
		return cache_iter->second;
	}

	INT16 sBonus = GetVisionRangeBonus(pSoldier, sSpot, bLevel);

	if (bLightLevel > NORMAL_LIGHTLEVEL_DAY)
	{
		if (pSoldier->bSectorZ == 0)
			sBonus += GetNightVisionRangeBonus(pSoldier, bLightLevel, sSpot, bLevel);
		else
			sBonus += GetCaveVisionRangeBonus(pSoldier, bLightLevel, sSpot, bLevel);
	}
	else if (bLightLevel < NORMAL_LIGHTLEVEL_DAY)
	{
		sBonus += GetBrightLightVisionRangeBonus(pSoldier, bLightLevel, sSpot, bLevel);
	}

	// sevenfm: at bright light, combine day and bright light bonus
	//if (bLightLevel >= NORMAL_LIGHTLEVEL_DAY && bLightLevel < NORMAL_LIGHTLEVEL_NIGHT)
	if (bLightLevel < NORMAL_LIGHTLEVEL_NIGHT)
	{
		sBonus += GetDayVisionRangeBonus(pSoldier, bLightLevel, sSpot, bLevel);
	}

	// Flugente: drugs can alter our sight
	if ( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_VISION ] )
		sBonus += 10;
	else if ( pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_VISION ] )
		sBonus -= 10;

	// Flugente: add sight range bonus due to disabilities, traits etc. (not equipment)
	sBonus += pSoldier->GetSightRangeBonus();

	pSoldier->mapVisionRangeCache[visionInfo] = sBonus;

	return sBonus;
}

UINT8 GetPercentTunnelVision( SOLDIERTYPE * pSoldier )
{
	Assert(pSoldier);

	VisionCacheInfo visionInfo;

	visionInfo.bLightLevel = 0;
	visionInfo.usItem = pSoldier->inv[HANDPOS].exists() ? pSoldier->inv[HANDPOS].usItem : 0;
	visionInfo.bScopeMode = gGameExternalOptions.fScopeModes ? pSoldier->bScopeMode : 0;
	visionInfo.bShock = pSoldier->aiData.bShock;
	visionInfo.bForceWeaponReady = gbForceWeaponReady;
	visionInfo.bForceWeaponNotReady = gbForceWeaponNotReady;
	visionInfo.bForceBinocsReady = gbForceBinocsReady;
	visionInfo.fIsFocusing = pSoldier->IsFocusing();
	visionInfo.fIsSpotting = pSoldier->IsSpotting();

	std::map<VisionCacheInfo, UINT8, std::less<>>::iterator cache_iter = pSoldier->mapTunnelVisionCache.find(visionInfo);

	if (cache_iter != pSoldier->mapTunnelVisionCache.end())
	{
		return cache_iter->second;
	}

	UINT8 bonus = 0;
	UINT8 bonus_body = 0;
	UINT8 bonus_gun = 0;
	UINT8 bonus_binocs = 0;
	UINT16 usItem;
	INVTYPE *pItem;
	OBJECTTYPE *pObj;
	BOOLEAN fUseCowitnessSight = FALSE;

	// sevenfm: tunnel vision for tanks
	if (pSoldier->IsTank())
	{
		bonus = 60;
		pSoldier->mapTunnelVisionCache[visionInfo] = bonus;
		return bonus;
	}

	// body equipment
	for (int i = HELMETPOS; i <= HEAD2POS; i++)
	{
		if (i == VESTPOS || i == LEGPOS)
			continue;

		// Okay, it's time for some optimization here
		if (pSoldier->inv[i].exists()) 
		{
			usItem = pSoldier->inv[i].usItem;
			pItem = &(Item[usItem]);

			if (!IsWeapon(usItem))
			{
				bonus_body = max(bonus_body, pItem->percenttunnelvision);
			}
		}
	}

	// weapon scopes
	if (WeaponReady(pSoldier))
	{
		// sevenfm: only apply weapon tunnel vision bonus when focusing
		if (pSoldier->IsFocusing() && !gbForceWeaponReady && !gbForceWeaponNotReady)
		{
			pObj = &(pSoldier->inv[HANDPOS]);

			if (pObj->exists())
			{
				usItem = pObj->usItem;
				pItem = &(Item[usItem]);

				if (IsWeapon(usItem))
				{
					bonus_gun = max(Item[usItem].percenttunnelvision, bonus_gun);
				}

				if (gGameExternalOptions.fScopeModes)
				{
					if (CanUseScopeMode(pObj->usItem))
					{
						// Flugente: check for scope mode
						std::map<INT8, OBJECTTYPE*> ObjList;
						GetScopeLists(pSoldier, pObj, ObjList);

						// only use scope mode if gun is in hand, otherwise an error might occur!
						if (pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD && ObjList[pSoldier->bScopeMode] != NULL)
						{
							// now apply the bonus from the scope we use
							bonus_gun = max(Item[ObjList[pSoldier->bScopeMode]->usItem].percenttunnelvision, bonus_gun);

							if (IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT) || IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE))
								fUseCowitnessSight = TRUE;
						}
					}

					for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
					{
						// add boni only from non-scope items
						if (iter->exists() && !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) && (!IsAttachmentClass(iter->usItem, AC_EXTERNAL) || fUseCowitnessSight))
						{
							bonus_gun = max(Item[iter->usItem].percenttunnelvision, bonus_gun);
						}
					}

				}
				else
				{
					for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
					{
						if (iter->exists())
						{
							bonus_gun = max(Item[iter->usItem].percenttunnelvision, bonus_gun);
						}
					}
				}
			}

			// sevenfm: small bonus when focusing
			bonus_gun = max(30, bonus_gun);
		}

		// sevenfm: always apply small tunnel vision bonus if weapon is raised
		bonus_gun = max(15, bonus_gun);
	}
	
	// binocs/scope in hand
	if (pSoldier->IsSpotting() && !gbForceBinocsReady)
	{
		// More optimization
		pObj = &(pSoldier->inv[HANDPOS]);

		if (pObj->exists())
		{
			usItem = pObj->usItem;
			pItem = &(Item[usItem]);

			if ((pItem->usItemClass & IC_MISC) &&
				!(pItem->usItemClass & IC_ARMOUR || pItem->usItemClass & IC_FACE) &&
				!IsWeapon(usItem))
			{
				bonus_binocs = max(bonus_binocs, pItem->percenttunnelvision);
			}
		}

		// SANDRO - STOMP traits - Scouting tunnel vision reduction with binoculars and similar
		if (gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, SCOUTING_NT) && pSoldier->bSectorZ == 0)
		{
			bonus_binocs = max(0, (bonus_binocs - gSkillTraitValues.ubSCTunnelVisionReducedWithBinoculars));
		}
	}	

	// Flugente: it would be unreasonable to apply helmet penalties if we already look through a scope, so have those separated
	bonus = max(max(bonus_binocs, bonus_gun), bonus_body);

	// Madd: adjust tunnel vision by difficulty level
	if (!PTR_OURTEAM && gGameOptions.ubDifficultyLevel > 0)
		bonus = bonus * 4 / (4 + gGameOptions.ubDifficultyLevel);

	// Flugente: drugs can alter our vision
	if (pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_TUNNELVISION])
	{
		bonus = min(100, bonus + 25);
	}

	// HEADROCK HAM 3.2: Further increase tunnel vision for cowering characters.
	// SANDRO - this calls many sub-functions over and over, we should at least skip this for civilians and such  
	if (gGameExternalOptions.ubMaxSuppressionShock > 0 &&
		(gGameExternalOptions.ubShockReducesSightRange == 1 || gGameExternalOptions.ubShockReducesSightRange == 3) &&
		pSoldier->IsHumanBodyType() &&
		bonus < 100)
	{
		// Make sure character is cowering.
		if (pSoldier->IsCowering())
		{
			// sevenfm: always use max tunnel percentage when cowering
			bonus = max(bonus, 40);
		}
		else if (pSoldier->aiData.bShock > 0)
		{
			// At shock 0, tunnel vision remains unchanged. At full shock, tunnel vision is full (100%)
			bonus = max(bonus, (80 * pSoldier->aiData.bShock) / gGameExternalOptions.ubMaxSuppressionShock);
		}
	}

	bonus = min(100, bonus);

	pSoldier->mapTunnelVisionCache[visionInfo] = bonus;

	return bonus;
}

BOOLEAN HasThermalOptics(SOLDIERTYPE * pSoldier, INT32 sSpot, INT8 bLevel)
{
	Assert(pSoldier);

	VisionCacheInfo visionInfo;

	visionInfo.bLightLevel = 0;
	visionInfo.usItem = pSoldier->inv[HANDPOS].exists() ? pSoldier->inv[HANDPOS].usItem : 0;
	visionInfo.bScopeMode = gGameExternalOptions.fScopeModes ? pSoldier->bScopeMode : 0;
	visionInfo.bShock = pSoldier->aiData.bShock;
	visionInfo.bForceWeaponReady = gbForceWeaponReady;
	visionInfo.bForceWeaponNotReady = gbForceWeaponNotReady;
	visionInfo.bForceBinocsReady = gbForceBinocsReady;
	visionInfo.fIsFocusing = pSoldier->IsFocusing(sSpot, bLevel);
	visionInfo.fIsSpotting = pSoldier->IsSpotting(sSpot, bLevel);

	std::map<VisionCacheInfo, BOOLEAN, std::less<>>::iterator cache_iter = pSoldier->mapThermalOpticsCache.find(visionInfo);

	if (cache_iter != pSoldier->mapThermalOpticsCache.end())
	{
		return cache_iter->second;
	}

	OBJECTTYPE *pObj;
	UINT16	usItem;
	INVTYPE *pItem;
	BOOLEAN fUseCowitnessSight = FALSE;

	//INT16 sBaseDistance = TACTICAL_RANGE / 2;
	//INT16 sDistance = TileIsOutOfBounds(sSpot) ? sBaseDistance : max(sBaseDistance, PythSpacesAway(pSoldier->sGridNo, sSpot));
	//UINT8 ubWatchTurns = sDistance / sBaseDistance;
	//UINT8 ubWatchTurns = 1;
	//UINT8 ubWatchPoints = TileIsOutOfBounds(sSpot) ? 0 : GetWatchedLocPoints(pSoldier->ubID, sSpot, bLevel);

	// scope bonus
	if (WeaponReady(pSoldier))
	{
		// SANDRO - added scouting check		
		pObj = &(pSoldier->inv[HANDPOS]);

		if (pObj->exists())
		{
			if (gGameExternalOptions.fScopeModes)
			{
				/*for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
				{
					// add boni only from non-scope items
					if (iter->exists() && !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) && Item[iter->usItem].thermaloptics)
					{
						if (gbForceWeaponReady ||
							pSoldier->IsFocusing(sSpot))
						{
							pSoldier->mapThermalOpticsCache[visionInfo] = TRUE;
							return TRUE;
						}
					}
				}*/

				if (CanUseScopeMode(pObj->usItem))
				{
					// Flugente: check for scope mode
					std::map<INT8, OBJECTTYPE*> ObjList;
					GetScopeLists(pSoldier, pObj, ObjList);

					// only use scope mode if gun is in hand, otherwise an error might occur!
					if (pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD && ObjList[pSoldier->bScopeMode] != NULL &&
						Item[ObjList[pSoldier->bScopeMode]->usItem].thermaloptics)
					{
						// now apply the bonus from the scope we use
						if (gbForceWeaponReady ||
							pSoldier->IsFocusing(sSpot, bLevel))
						{
							pSoldier->mapThermalOpticsCache[visionInfo] = TRUE;
							return TRUE;
						}
					}
				}
			}
			else
			{
				for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
				{
					if (iter->exists() && Item[iter->usItem].thermaloptics)
					{
						pSoldier->mapThermalOpticsCache[visionInfo] = TRUE;
						return TRUE;
					}
				}
			}
		}
	}

	// binocs/scope in hand
	if (gbForceBinocsReady || pSoldier->IsSpotting(sSpot, bLevel))
	{
		pObj = &(pSoldier->inv[HANDPOS]);

		if (pObj->exists())
		{
			usItem = pObj->usItem;
			pItem = &(Item[usItem]);

			// Snap (TODO): binoculars and such should not be active by default
			if ((pItem->usItemClass & IC_MISC) &&
				!(pItem->usItemClass & IC_ARMOUR || pItem->usItemClass & IC_FACE) &&
				!IsWeapon(usItem) &&
				Item[usItem].thermaloptics)
			{
				pSoldier->mapThermalOpticsCache[visionInfo] = TRUE;
				return TRUE;
			}
		}
	}

	// gear bonus
	for (int i = HELMETPOS; i <= HEAD2POS; ++i)
	{
		if (i == VESTPOS || i == LEGPOS)
			continue;

		// More optimization
		pObj = &(pSoldier->inv[i]);

		if (pObj->exists())
		{
			// Flugente: weapons are checked later on...
			if (!IsWeapon(pObj->usItem) &&
				Item[pObj->usItem].thermaloptics)
			{
				pSoldier->mapThermalOpticsCache[visionInfo] = TRUE;
				return TRUE;
			}
		}
	}

	pSoldier->mapThermalOpticsCache[visionInfo] = FALSE;

	return FALSE;
}

INT8 FindWirecutters(SOLDIERTYPE * pSoldier)
{
	Assert(pSoldier);

	for (INT8 bLoop = 0; bLoop < NUM_INV_SLOTS; bLoop++)
	{
		if (pSoldier->inv[bLoop].exists() == true) 
		{
			if ((Item[pSoldier->inv[bLoop].usItem].usItemClass & IC_MISC) && Item[pSoldier->inv[bLoop].usItem].wirecutters)
			{
				return(bLoop);
			}
		}
	}

	return NO_SLOT;
}

INT8 FindTNT(SOLDIERTYPE * pSoldier)
{
	Assert(pSoldier);

	for (INT8 bLoop = 0; bLoop < NUM_INV_SLOTS; bLoop++)
	{
		if (pSoldier->inv[bLoop].exists() && ItemIsTNT(pSoldier->inv[bLoop].usItem))
		{
			return(bLoop);
		}
	}

	return NO_SLOT;
}

INT16 GetHearingRangeBonus(SOLDIERTYPE * pSoldier, INT32 sSpot)
{
	INT16 sTotalBonus = 0;
	INT16 sBonus;
	BOOLEAN fExtendedEar = FALSE;
	BOOLEAN fHearingAid = FALSE;

	if (FindHearingAid(pSoldier, FALSE) != NO_SLOT)
		fHearingAid = TRUE;

	Assert(pSoldier);
	CHECKF(pSoldier);

	for (int i = BODYPOSSTART; i < BODYPOSFINAL; i++)
	{
		if (pSoldier->inv[i].exists() == true) 
		{
			if ( (i == HANDPOS || i == SECONDHANDPOS) &&
				   (Item[pSoldier->inv[i].usItem].usItemClass & IC_ARMOUR || Item[pSoldier->inv[i].usItem].usItemClass & IC_FACE ))
			{
				continue;
			}

			fExtendedEar = FALSE;

			if ((i== HEAD1POS || i== HEAD2POS) && (pSoldier->inv[i].usItem == EXTENDEDEAR || Item[pSoldier->inv[i].usItem].hearingrangebonus > 0 && Item[pSoldier->inv[i].usItem].electronic))
				fExtendedEar = TRUE;

			sBonus = BonusReduceMore(Item[pSoldier->inv[i].usItem].hearingrangebonus, pSoldier->inv[i][0]->data.objectStatus);

			// sevenfm: when not using extended ear, apply bonus from all items
			// when using extended ear, apply positive bonus only from extended ear, apply all negative bonuses
			if (!fHearingAid || sBonus < 0 || 
				fExtendedEar && 
				(!Item[pSoldier->inv[i].usItem].needsbatteries || FindAttachedBatteries(&(pSoldier->inv[i]))) //&&
				//!TileIsOutOfBounds(sSpot) &&
				//(pSoldier->ubDirection == AIDirection(pSoldier->sGridNo, sSpot) ||
				//gOneCDirection[pSoldier->ubDirection] == AIDirection(pSoldier->sGridNo, sSpot) ||
				//gOneCCDirection[pSoldier->ubDirection] == AIDirection(pSoldier->sGridNo, sSpot))
				)
				sTotalBonus += sBonus;
		}
	}

	return sTotalBonus;
}


BOOLEAN IsDuckbill( OBJECTTYPE * pObj )
{
	if (pObj->exists() == true) {
		if (Item[pObj->usItem].duckbill || Item[(*pObj)[0]->data.gun.usGunAmmoItem].duckbill )
			return TRUE;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && Item[iter->usItem].duckbill)
			{
				return( TRUE );
			}
		}
	}
	return( FALSE );
}


INT16 GetPercentNoiseVolume( OBJECTTYPE * pObj )
{
	INT16 sMod = 0;
	if (pObj->exists() == true) {
		sMod = 100 - BonusReduce( Item[pObj->usItem].percentnoisereduction, (*pObj)[0]->data.objectStatus );
		sMod = sMod * ( 100 - Item[(*pObj)[0]->data.gun.usGunAmmoItem].percentnoisereduction ) / 100;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists())
			{
				sMod = sMod * (100 - BonusReduce(Item[iter->usItem].percentnoisereduction, (*iter)[0]->data.objectStatus)) / 100;
			}
		}
	}
	sMod = max(sMod, 0);
	sMod = min(sMod, 100);

	return sMod;
}


INT8 FindGasMask( SOLDIERTYPE * pSoldier )
{
	INT8 bLoop;

	for (bLoop = 0; bLoop < NUM_INV_SLOTS; bLoop++)//dnl ch40 041009
	{
		if (pSoldier->inv[bLoop].exists() == true) 
		{
			if ( Item[pSoldier->inv[bLoop].usItem].gasmask )
			{
				return( bLoop );
			}
		}
	}
	return( NO_SLOT );
}

// sevenfm: check if soldier has usable binocs in inventory
BOOLEAN HasBinocs(SOLDIERTYPE * pSoldier)
{
	INT8 bLoop;

	for (bLoop = 0; bLoop < NUM_INV_SLOTS; bLoop++)
	{
		if (pSoldier->inv[bLoop].exists() &&
			(Item[(&(pSoldier->inv[bLoop]))->usItem].usItemClass & IC_MISC) &&
			GetObjectModifier(pSoldier, &(pSoldier->inv[bLoop]), ANIM_STAND, ITEMMODIFIER_SPOTTER) > 0 &&
			pSoldier->inv[bLoop][0]->data.objectStatus >= USABLE)
		{
			return TRUE;
		}
	}
	return FALSE;
}

// sevenfm: check if soldier has usable binocs in hands
BOOLEAN HasBinocsInHands(SOLDIERTYPE * pSoldier)
{
	INT8 bLoop;

	for (bLoop = HANDPOS; bLoop <= SECONDHANDPOS; bLoop++)
	{
		if (pSoldier->inv[bLoop].exists() &&
			(Item[(&(pSoldier->inv[bLoop]))->usItem].usItemClass & IC_MISC) &&
			GetObjectModifier(pSoldier, &(pSoldier->inv[bLoop]), ANIM_STAND, ITEMMODIFIER_SPOTTER) > 0 &&
			pSoldier->inv[bLoop][0]->data.objectStatus >= USABLE)
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOLEAN HasActiveBinocsInHands(SOLDIERTYPE * pSoldier)
{
	if (DayTime() && gbCurrentRainIntensity == 0 && HasBinocsInHands(pSoldier))
		return TRUE;

	return FALSE;
}

BOOLEAN IsDetonatorAttached( OBJECTTYPE * pObj )
{
	if (pObj->exists() == true) {
		//if (Item[pObj->usItem].detonator)
		//	return TRUE;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && Item[iter->usItem].detonator)
			{
				return( TRUE );
			}
		}
	}
	return( FALSE );
}

BOOLEAN IsRemoteDetonatorAttached( OBJECTTYPE * pObj )
{
	if (pObj->exists() == true) {
		//if (Item[pObj->usItem].remotedetonator)
		//	return TRUE;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && Item[iter->usItem].remotedetonator)
			{
				return( TRUE );
			}
		}
	}
	return( FALSE );
}

BOOLEAN IsFlashSuppressor(OBJECTTYPE * pObj, SOLDIERTYPE * pSoldier)
{
	if (pObj->exists())
	{
		//Madd: tracers automatically negate any muzzle flash suppression due to inherent lighting effects
		if (Item[pObj->usItem].usItemClass == IC_GUN && AmmoTypes[(*pObj)[0]->data.gun.ubGunAmmoType].tracerEffect)
			return FALSE;

		// sevenfm: always activate muzzle flash code when using flashlight
		if (pSoldier && pSoldier->GetBestEquippedFlashLightRange() > 0)
			return FALSE;

		if (Item[pObj->usItem].hidemuzzleflash && (*pObj)[0]->data.objectStatus >= USABLE)
			return TRUE;

		if (Item[(*pObj)[0]->data.gun.usGunAmmoItem].hidemuzzleflash)
			return TRUE;

		attachmentList::iterator iter = (*pObj)[0]->attachments.begin();
		attachmentList::iterator iterend = (*pObj)[0]->attachments.end();
		for (; iter != iterend; ++iter)
		{
			if (iter->exists() && Item[iter->usItem].hidemuzzleflash && (*iter)[0]->data.objectStatus >= USABLE)
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

INT16 GetFlashSuppressorStatus( OBJECTTYPE * pObj )
{
	INT16 p=100;
	if (pObj->exists() == true) {
		if (Item[pObj->usItem].hidemuzzleflash )
			p=(*pObj)[0]->data.objectStatus;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && Item[iter->usItem].hidemuzzleflash)
			{
				p =p+ (*iter)[0]->data.objectStatus;
			}
		}
	}
	p = min(p,100);
	return p;
}

BOOLEAN IsGrenadeLauncherAttached( OBJECTTYPE * pObj, UINT8 subObject )
{
	if (pObj->exists() == true)
	{
		if (Item[pObj->usItem].grenadelauncher )
			return TRUE;

		for (attachmentList::iterator iter = (*pObj)[subObject]->attachments.begin(); iter != (*pObj)[subObject]->attachments.end(); ++iter)
		{
			if (iter->exists() && (Item[iter->usItem].grenadelauncher || IsAttachmentClass( iter->usItem, AC_RIFLEGRENADE ) ) )
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

OBJECTTYPE* FindAttachment_GrenadeLauncher( OBJECTTYPE * pObj )
{
	if (pObj->exists() == true) 
	{
		// Flugente: as it is now possible to have both a grenade launcher and a rifle grenade attached to a gun, the rifle grenade has priority. We thus have to search for it first
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists() )
			{
				if ( IsAttachmentClass(iter->usItem, AC_RIFLEGRENADE) )
				{
					return &(*iter);
				}
			}
		}

		if (Item[pObj->usItem].grenadelauncher )
			return pObj;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists() && Item[iter->usItem].grenadelauncher )
			{
				return( &(*iter) );
			}
		}
	}
	return( 0 );
}

INT16 GetGrenadeLauncherStatus( OBJECTTYPE * pObj )
{
	if (pObj->exists() == true)
	{
		// Flugente: as it is now possible to have both a grenade launcher and a rifle grenade attached to a gun, the rifle grenade has priority. We thus have to search for it first
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists() )
			{
				if ( IsAttachmentClass(iter->usItem, AC_RIFLEGRENADE) )
				{
					return( (*iter)[0]->data.objectStatus );
				}
			}
		}

		if (Item[pObj->usItem].grenadelauncher  )
			return (*pObj)[0]->data.objectStatus;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists() && Item[iter->usItem].grenadelauncher )
			{
				return( (*iter)[0]->data.objectStatus );
			}
		}
	}
	return( ITEM_NOT_FOUND );
}

UINT16 GetAttachedGrenadeLauncher( OBJECTTYPE * pObj )
{
	if (pObj->exists() == true)
	{
		// Flugente: as it is now possible to have both a grenade launcher and a rifle grenade attached to a gun, the rifle grenade has priority. We thus have to search for it first
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists() )
			{
				if ( IsAttachmentClass(iter->usItem, AC_RIFLEGRENADE) )
				{
					return( (UINT16) Item[iter->usItem].uiIndex );
				}
			}
		}

		if (Item[pObj->usItem].grenadelauncher  )
			return pObj->usItem;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists() && Item[iter->usItem].grenadelauncher )
			{
				return( (UINT16) Item[iter->usItem].uiIndex );
			}
		}
	}
	return( NONE );
}

INT16 GetUnderBarrelStatus( OBJECTTYPE * pObj, UINT32 usFlag )
{
	if (pObj->exists() == true) {

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists() && Item[iter->usItem].usItemClass & usFlag )
			{
				return( (*iter)[0]->data.objectStatus );
			}
		}
	}
	return( ITEM_NOT_FOUND );
}

BOOLEAN IsWeaponAttached( OBJECTTYPE * pObj, UINT32 usFlag, UINT8 subObject )
{
	if (pObj->exists() == true) {

		for (attachmentList::iterator iter = (*pObj)[subObject]->attachments.begin(); iter != (*pObj)[subObject]->attachments.end(); ++iter)
		{
			if (iter->exists() && Item[iter->usItem].usItemClass & usFlag )
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

OBJECTTYPE* FindAttachedWeapon( OBJECTTYPE * pObj, UINT32 usFlag )
{
	if (pObj->exists() == true) {

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists() && Item[iter->usItem].usItemClass & usFlag )
			{
				return( &(*iter) );
			}
		}
	}
	return( NULL );
}

UINT16 GetAttachedWeapon( OBJECTTYPE * pObj, UINT32 usFlag )
{
	if (pObj->exists() == true) {

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists() && (Item[iter->usItem].usItemClass & usFlag ) )
			{
				return( (UINT16) Item[iter->usItem].uiIndex );
			}
		}
	}
	return( NONE );
}

INT16 GetAttachedArmourBonus( OBJECTTYPE * pObj )
{
	INT16 bonus=0;

	if (pObj->exists() == true) {
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if(iter->exists())
			{
				bonus += BonusReduce( Armour[Item[iter->usItem].ubClassIndex].ubProtection,
					(*iter)[0]->data.objectStatus );
			}
		}
	}
	return( bonus );
}


INT16 GetBulletSpeedBonus( OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	if (pObj->exists() == true) {
		bonus = Item[pObj->usItem].bulletspeedbonus ;
		bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].bulletspeedbonus ;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if(iter->exists())
				bonus = bonus +Item[iter->usItem].bulletspeedbonus  ;
		}
	}
	return( bonus );
}

BOOLEAN EXPLOSIVE_GUN ( UINT16 x)
{
	 //DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("EXPLOSIVE_GUN x = %d",x));

	if ( Item[x].rocketlauncher || Item[x].cannon )
		return TRUE;
	else
		return FALSE;
}

INT8 FindRocketLauncherOrCannon( SOLDIERTYPE * pSoldier )
{
	INT8 invsize = (INT8)pSoldier->inv.size();
	for ( INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true) 
		{
			if (Item[pSoldier->inv[bLoop].usItem].rocketlauncher || Item[pSoldier->inv[bLoop].usItem].cannon )
			{
				return( bLoop );
			}
		}
	}
	return( NO_SLOT );
}

INT8 FindRocketLauncher( SOLDIERTYPE * pSoldier )
{
	INT8 invsize = (INT8)pSoldier->inv.size();
	for ( INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true) 
		{
			if (Item[pSoldier->inv[bLoop].usItem].rocketlauncher )
			{
				return( bLoop );
			}
		}
	}
	return( NO_SLOT );
}

INT8 FindCannon( SOLDIERTYPE * pSoldier )
{
	INT8 invsize = (INT8)pSoldier->inv.size();
	for ( INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true) 
		{
			if ( Item[pSoldier->inv[bLoop].usItem].cannon )
			{
				return( bLoop );
			}
		}
	}
	return( NO_SLOT );
}

INT8 FindUsableCrowbar( SOLDIERTYPE * pSoldier )
{
	//JMich_SkillModifiers: Adding a bonus check, to return the best crowbar, and modifying the return value.
	INT8 bonus, FoundCrowbar;
	FoundCrowbar = NO_SLOT;
	bonus = -101;

	INT8 invsize = (INT8)pSoldier->inv.size();
	for ( INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true) 
		{
			if ( Item[pSoldier->inv[bLoop].usItem].crowbar && pSoldier->inv[bLoop][0]->data.objectStatus >= USABLE && Item[pSoldier->inv[bLoop].usItem].CrowbarModifier > bonus)
			{
				bonus = Item[pSoldier->inv[bLoop].usItem].CrowbarModifier;
				FoundCrowbar = bLoop;
			}
		}
	}
	return( FoundCrowbar );
}

OBJECTTYPE* FindAttachedBatteries( OBJECTTYPE * pObj )
{
	if (pObj->exists() == true) 
	{
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && Item[iter->usItem].batteries)
			{
				return( &(*iter) );
			}
		}
	}
	return( 0 );
}

OBJECTTYPE* FindAttachedCleaningKit(OBJECTTYPE * pObj, UINT8 subObject)
{
	if (!UsingNewAttachmentSystem() || pObj->exists() == false)
		return 0;

	for (attachmentList::iterator iter = (*pObj)[subObject]->attachments.begin(); iter != (*pObj)[subObject]->attachments.end(); ++iter)
	{
		if (iter->exists() && HasItemFlag(iter->usItem, CLEANING_KIT))
		{
			return(&(*iter));
		}
	}

	return 0;
}

INT8 FindToolkit( SOLDIERTYPE * pSoldier )
{
	INT8 invsize = (INT8)pSoldier->inv.size();
	for ( INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true) 
		{
			if (Item[pSoldier->inv[bLoop].usItem].toolkit )
			{
				return( bLoop );
			}
		}
	}
	return( NO_SLOT );
}

INT8 FindMedKit( SOLDIERTYPE * pSoldier )
{
	INT8 invsize = (INT8)pSoldier->inv.size();

	// sevenfm: first try to find kit without poison damage
	for ( INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true) 
		{
			if (Item[pSoldier->inv[bLoop].usItem].medicalkit && !Item[pSoldier->inv[bLoop].usItem].bPoisonPercentage)
			{
				return bLoop;
			}
		}
	}
	for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true)
		{
			if (Item[pSoldier->inv[bLoop].usItem].medicalkit)
			{
				return bLoop;
			}
		}
	}

	return NO_SLOT;
}

INT8 FindFirstAidKit( SOLDIERTYPE * pSoldier )
{
	INT8 invsize = (INT8)pSoldier->inv.size();

	// sevenfm: first try to find kit without poison damage
	for ( INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true) 
		{
			if (Item[pSoldier->inv[bLoop].usItem].firstaidkit && !Item[pSoldier->inv[bLoop].usItem].bPoisonPercentage)
			{
				return bLoop;
			}
		}
	}
	for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true)
		{
			if (Item[pSoldier->inv[bLoop].usItem].firstaidkit)
			{
				return bLoop;
			}
		}
	}

	return NO_SLOT;
}

INT8 FindCamoKit( SOLDIERTYPE * pSoldier )
{
	INT8 invsize = (INT8)pSoldier->inv.size();
	for ( INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true) {
			if (Item[pSoldier->inv[bLoop].usItem].camouflagekit   )
			{
				return( bLoop );
			}
		}
	}
	return( NO_SLOT );
}

//JMich_SkillModifiers: Adding a function to see if we have an item with disarm bonus
INT8 FindDisarmKit( SOLDIERTYPE * pSoldier )
{
	INT8 bonus, FoundKit;
	FoundKit = NO_SLOT;
	bonus = 0;

	INT8 invsize = (INT8)pSoldier->inv.size();
	for ( INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true) 
		{
			if ( ( ( Item[pSoldier->inv[bLoop].usItem].DisarmModifier * pSoldier->inv[bLoop][0]->data.objectStatus ) / 100 ) > bonus )
			{
				bonus = Item[pSoldier->inv[bLoop].usItem].DisarmModifier * pSoldier->inv[bLoop][0]->data.objectStatus / 100;;
				FoundKit = bLoop;
			}
		}
	}
	return( FoundKit );
}

INT8 FindLocksmithKit( SOLDIERTYPE * pSoldier )
{
	//JMich_SkillModifiers: Adding a bonus check, to return the best LocksmithKit, and modifying the return value.
	INT8 bonus, FoundKit;
	FoundKit = NO_SLOT;
	bonus = -101;

	INT8 invsize = (INT8)pSoldier->inv.size();
	for ( INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true)
		{
			if (Item[pSoldier->inv[bLoop].usItem].locksmithkit )
			{
				//JMich_SkillModifiers: If the locksmith kit has a bonus, reduce it based on the status, so we use the best bonus.
				if (Item[pSoldier->inv[bLoop].usItem].LockPickModifier > 0 )
				{	
					if ( ( Item[pSoldier->inv[bLoop].usItem].LockPickModifier * pSoldier->inv[bLoop][0]->data.objectStatus / 100 ) > bonus  ) 
					{
						bonus = ( Item[pSoldier->inv[bLoop].usItem].LockPickModifier * pSoldier->inv[bLoop][0]->data.objectStatus / 100 );
						FoundKit = bLoop;
					}

				}
				//JMich_SkillModifiers: If on the other hand the locksmith is a shoddy one, keep that penalty regardless of status.
				else
				{
					if ( Item[pSoldier->inv[bLoop].usItem].LockPickModifier > bonus  ) 
					{
						bonus = Item[pSoldier->inv[bLoop].usItem].LockPickModifier;
						FoundKit = bLoop;
					}
				}
			}
		}
	}
	return( FoundKit );
}

INT8 FindWalkman(SOLDIERTYPE * pSoldier, BOOLEAN fBatteries)
{
	Assert(pSoldier);

	// sevenfm: walkman only works from head slot
	for (INT8 bLoop = HEAD1POS; bLoop <= HEAD2POS; bLoop++)
	{
		if (pSoldier->inv[bLoop].exists() && 
			Item[pSoldier->inv[bLoop].usItem].walkman &&
			(!fBatteries || !Item[pSoldier->inv[bLoop].usItem].needsbatteries || FindAttachedBatteries(&(pSoldier->inv[bLoop]))))
		{
			return(bLoop);
		}
	}
	return NO_SLOT;
}

INT8 FindHearingAid(SOLDIERTYPE * pSoldier, BOOLEAN fBatteries)
{
	Assert(pSoldier);
	if (!pSoldier)
	{
		return NO_SLOT;
	}

	// sevenfm: extended ear works only from head slot
	for (INT8 bLoop = HEAD1POS; bLoop <= HEAD2POS; bLoop++)
	{
		if (pSoldier->inv[bLoop].exists() &&
			(!fBatteries || !Item[pSoldier->inv[bLoop].usItem].needsbatteries || FindAttachedBatteries(&(pSoldier->inv[bLoop]))))
		{
			// sevenfm: use hardcoded value or check that item is electronic and has hearing bonus
			if (pSoldier->inv[bLoop].usItem == EXTENDEDEAR ||
				Item[pSoldier->inv[bLoop].usItem].hearingrangebonus > 0 && Item[pSoldier->inv[bLoop].usItem].electronic)
			{
				return(bLoop);
			}
		}
	}

	return(NO_SLOT);
}

INT8 FindTrigger( SOLDIERTYPE * pSoldier )
{
	INT8 invsize = (INT8)pSoldier->inv.size();
	for ( INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true)
		{
			if (Item[pSoldier->inv[bLoop].usItem].remotetrigger )
			{
				return( bLoop );
			}
		}
	}
	return( NO_SLOT );
}

INT8 FindZippo(SOLDIERTYPE * pSoldier)
{
	INT8 invsize = (INT8)pSoldier->inv.size();
	for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true)
		{
			if (Item[pSoldier->inv[bLoop].usItem].zippo && pSoldier->inv[bLoop][0]->data.objectStatus > 1)
			{
				return(bLoop);
			}
		}
	}
	return NO_SLOT;
}

INT8 FindRemoteControl( SOLDIERTYPE * pSoldier )
{
	for (INT8 bLoop = BODYPOSSTART; bLoop < BODYPOSFINAL; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() == true) {
			if (Item[pSoldier->inv[bLoop].usItem].robotremotecontrol    )
			{
				return( bLoop );
			}
		}
	}
	return( NO_SLOT );
}

UINT16 LowestLaunchableCoolness(UINT16 launcherIndex)
{
	UINT16 lowestCoolness = 999;

	for (UINT16 i = 0; i < gMAXITEMS_READ; ++i)
	{
		if ( Item[i].usItemClass  == 0 )
			break;

		if( ValidLaunchable( i, launcherIndex ) && ItemIsLegal(i) && Item[i].ubCoolness <= lowestCoolness )
		{
			lowestCoolness = Item[i].ubCoolness;
		}
	}
	return lowestCoolness;
}

UINT16 PickARandomLaunchable(UINT16 itemIndex)
{
	UINT16 usNumMatches = 0;
	UINT16 usRandom = 0;
	UINT16 lowestCoolness = LowestLaunchableCoolness(itemIndex);

	// Flugente: the above code is highly dubious.. why do we loop over all items 2 times, and why that obscure usRandom--; business? This can cause an underflow!
	BOOLEAN isnight = NightTime();
	UINT16 maxcoolness = max( 1 + HighestPlayerProgressPercentage() / 10, lowestCoolness );

	std::vector<UINT16> legalvec;
	for (UINT16 i = 0; i < gMAXITEMS_READ; ++i)
	{
		if ( Item[i].usItemClass == 0 )
			break;

		//Madd: quickfix: make it not choose best grenades right away.
		if ( Item[i].ubCoolness <= maxcoolness && ItemIsLegal( i ) && ValidLaunchable( i, itemIndex ) )
		{
			// Flugente: ignore this item if we aren't allowed to pick it at this time of day
			if ( (isnight && Item[i].usItemChoiceTimeSetting == 1) || (!isnight && Item[i].usItemChoiceTimeSetting == 2) )
				continue;

			legalvec.push_back(i);
			++usNumMatches;
		}
	}

	if ( !legalvec.empty() )
	{
		usRandom = (UINT16)Random( legalvec.size() );

		return legalvec[usRandom];
	}

	return 0;
}
INT16 GetCamoBonus( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	//BOOLEAN fWeaponReady = FALSE;

	if ( pObj->exists() == true ) 
	{
		/*if( pSoldier && 
			Item[pObj->usItem].usItemClass == IC_GUN && 
			((&pSoldier->inv[HANDPOS]) == pObj || (&pSoldier->inv[SECONDHANDPOS]) == pObj && pSoldier->IsValidSecondHandShot()) &&
			(WeaponReady(pSoldier) || (gAnimControl[ pSoldier->usAnimState ].uiFlags & ANIM_ALT_WEAPON_HOLDING )) )
		{
			fWeaponReady = TRUE;
		}*/

		bonus = (Item[pObj->usItem].camobonus);// * (WEAPON_STATUS_MOD((*pObj)[0]->data.objectStatus) / 100)) ;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && !Item[iter->usItem].camouflagekit)
			{
				//if( Item[iter->usItem].bestlaserrange == 0 || fWeaponReady )
				if (Item[iter->usItem].bestlaserrange == 0)
				{
					bonus += (INT16) (Item[iter->usItem].camobonus);// * (WEAPON_STATUS_MOD((*iter)[0]->data.objectStatus) / 100));
				}				
			}
		}
	}
	return( bonus );
}
INT16 GetUrbanCamoBonus( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	//BOOLEAN fWeaponReady = FALSE;

	if ( pObj->exists() == true ) 
	{
		/*if( pSoldier && 
			Item[pObj->usItem].usItemClass == IC_GUN && 
			((&pSoldier->inv[HANDPOS]) == pObj || (&pSoldier->inv[SECONDHANDPOS]) == pObj && pSoldier->IsValidSecondHandShot()) &&
			(WeaponReady(pSoldier) || (gAnimControl[ pSoldier->usAnimState ].uiFlags & ANIM_ALT_WEAPON_HOLDING )) )
		{
			fWeaponReady = TRUE;
		}*/

		bonus = (Item[pObj->usItem].urbanCamobonus);// * (WEAPON_STATUS_MOD((*pObj)[0]->data.objectStatus) / 100)) ;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && !Item[iter->usItem].camouflagekit)
			{
				//if( Item[iter->usItem].bestlaserrange == 0 || fWeaponReady )
				if (Item[iter->usItem].bestlaserrange == 0)
				{
					bonus += (INT16) (Item[iter->usItem].urbanCamobonus);// * (WEAPON_STATUS_MOD((*iter)[0]->data.objectStatus) / 100));
				}
			}
		}
	}
	return( bonus );
}
INT16 GetDesertCamoBonus( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	//BOOLEAN fWeaponReady = FALSE;

	if ( pObj->exists() == true ) 
	{
		/*if( pSoldier && 
			Item[pObj->usItem].usItemClass == IC_GUN && 
			((&pSoldier->inv[HANDPOS]) == pObj || (&pSoldier->inv[SECONDHANDPOS]) == pObj && pSoldier->IsValidSecondHandShot()) &&
			(WeaponReady(pSoldier) || (gAnimControl[ pSoldier->usAnimState ].uiFlags & ANIM_ALT_WEAPON_HOLDING )) )
		{
			fWeaponReady = TRUE;
		}*/

		bonus = (Item[pObj->usItem].desertCamobonus);// * (WEAPON_STATUS_MOD((*pObj)[0]->data.objectStatus) / 100)) ;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && !Item[iter->usItem].camouflagekit)
			{
				//if( Item[iter->usItem].bestlaserrange == 0 || fWeaponReady )
				if (Item[iter->usItem].bestlaserrange == 0)
				{
					bonus += (INT16) (Item[iter->usItem].desertCamobonus);// * (WEAPON_STATUS_MOD((*iter)[0]->data.objectStatus) / 100));
				}				
			}
		}
	}
	return( bonus );
}
INT16 GetSnowCamoBonus( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	//BOOLEAN fWeaponReady = FALSE;

	if ( pObj->exists() == true ) 
	{
		/*if( pSoldier && 
			Item[pObj->usItem].usItemClass == IC_GUN && 
			((&pSoldier->inv[HANDPOS]) == pObj || (&pSoldier->inv[SECONDHANDPOS]) == pObj && pSoldier->IsValidSecondHandShot()) &&
			(WeaponReady(pSoldier) || (gAnimControl[ pSoldier->usAnimState ].uiFlags & ANIM_ALT_WEAPON_HOLDING )) )
		{
			fWeaponReady = TRUE;
		}*/

		bonus = (Item[pObj->usItem].snowCamobonus);// * (WEAPON_STATUS_MOD((*pObj)[0]->data.objectStatus) / 100)) ;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && !Item[iter->usItem].camouflagekit)
			{
				//if( Item[iter->usItem].bestlaserrange == 0 || fWeaponReady )
				if (Item[iter->usItem].bestlaserrange == 0)
				{
					bonus += (INT16) (Item[iter->usItem].snowCamobonus);// * (WEAPON_STATUS_MOD((*iter)[0]->data.objectStatus) / 100));
				}				
			}
		}
	}
	return( bonus );
}
INT16 GetWornCamo( SOLDIERTYPE * pSoldier )
{
	INT8	bLoop;
	INT16	ttl=0;

	for (bLoop = HELMETPOS; bLoop <= LEGPOS; bLoop++)
	{
		if ( pSoldier->inv[bLoop].exists() == true )
		{
			ttl += GetCamoBonus(pSoldier, &pSoldier->inv[bLoop]);
			if ( UsingNewInventorySystem() )
			{
				if ( bLoop == VESTPOS && pSoldier->inv[VESTPOCKPOS].exists() )
				{
					// silversurfer: Using LBE vest. Only apply partial bonus from armor vest.
					ttl -= (INT16)( ( 1.0 - gItemSettings.fCamoLBEoverVestModifier ) * Item[ pSoldier->inv[bLoop].usItem ].camobonus );
				}
				else if ( bLoop == LEGPOS )
				{
					FLOAT fLegPenalty = 0;
					// depending on how many leg LBE we use the camo effect of the armor pants is higher or lower
					fLegPenalty += pSoldier->inv[LTHIGHPOCKPOS].exists();
					fLegPenalty += pSoldier->inv[RTHIGHPOCKPOS].exists();
					// Using thigh rigs. Only apply partial bonus from armor pants.
					if ( fLegPenalty > 0 )
						ttl -= (INT16)( ( 1.0 - gItemSettings.fCamoLBEoverPantsModifier ) * Item[ pSoldier->inv[bLoop].usItem ].camobonus * fLegPenalty / 2 );
				}
			}
		}
	}

	// CHRISL: Add additional loop for LBE items while using new inventory system
	if((UsingNewInventorySystem() == true))
	{
		for (bLoop = VESTPOCKPOS; bLoop <= BPACKPOCKPOS; bLoop++)
		{
			if ( pSoldier->inv[bLoop].exists() == true )
				ttl += GetCamoBonus(pSoldier, &pSoldier->inv[bLoop]);
		}

		//tais: guns can be camouflaged, this will make gun camo have effect when in main/second hand or on gunsling, did a check for guns and nothing else, hope that's enough.
		if (pSoldier->inv[HANDPOS].exists() == true && Item[pSoldier->inv[HANDPOS].usItem].usItemClass & IC_WEAPON)
			ttl += GetCamoBonus(pSoldier, &pSoldier->inv[HANDPOS]);
		if (pSoldier->inv[SECONDHANDPOS].exists() == true && Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass & IC_WEAPON)
			ttl += GetCamoBonus(pSoldier, &pSoldier->inv[SECONDHANDPOS]);
		if (pSoldier->inv[GUNSLINGPOCKPOS].exists() == true && Item[pSoldier->inv[GUNSLINGPOCKPOS].usItem].usItemClass & IC_WEAPON)
			ttl += GetCamoBonus(pSoldier, &pSoldier->inv[GUNSLINGPOCKPOS]);
	}

	// sevenfm: cannot have negative camo
	ttl = max(ttl, 0);

	return min( ttl, 100 );
}
INT16 GetWornUrbanCamo( SOLDIERTYPE * pSoldier )
{
	INT8	bLoop;
	INT16	ttl=0;

	for (bLoop = HELMETPOS; bLoop <= LEGPOS; bLoop++)
	{
		if ( pSoldier->inv[bLoop].exists() == true )
		{
			ttl += GetUrbanCamoBonus(pSoldier, &pSoldier->inv[bLoop]);
			if ( UsingNewInventorySystem() )
				if ( bLoop == VESTPOS && pSoldier->inv[VESTPOCKPOS].exists() )
				{
					// silversurfer: Using LBE vest. Only apply partial bonus from armor vest.
					ttl -= (INT16)( ( 1.0 - gItemSettings.fCamoLBEoverVestModifier ) * Item[ pSoldier->inv[bLoop].usItem ].urbanCamobonus );
				}
				else if ( bLoop == LEGPOS )
				{
					FLOAT fLegPenalty = 0;
					// depending on how many leg LBE we use the camo effect of the armor pants is higher or lower
					fLegPenalty += pSoldier->inv[LTHIGHPOCKPOS].exists();
					fLegPenalty += pSoldier->inv[RTHIGHPOCKPOS].exists();
					// Using thigh rigs. Only apply partial bonus from armor pants.
					if ( fLegPenalty > 0 )
						ttl -= (INT16)( ( 1.0 - gItemSettings.fCamoLBEoverPantsModifier ) * Item[ pSoldier->inv[bLoop].usItem ].urbanCamobonus * fLegPenalty / 2 );
				}
		}
	}

	// CHRISL: Add additional loop for LBE items while using new inventory system
	if((UsingNewInventorySystem() == true))
	{
		for (bLoop = VESTPOCKPOS; bLoop <= BPACKPOCKPOS; bLoop++)
		{
			if ( pSoldier->inv[bLoop].exists() == true )
				ttl += GetUrbanCamoBonus(pSoldier, &pSoldier->inv[bLoop]);
		}

		//tais: guns can be camouflaged, this will make gun camo have effect when in main/second hand or on gunsling, did a check for guns and nothing else, hope that's enough.
		if (pSoldier->inv[HANDPOS].exists() == true && Item[pSoldier->inv[HANDPOS].usItem].usItemClass & IC_WEAPON)
			ttl += GetUrbanCamoBonus(pSoldier, &pSoldier->inv[HANDPOS]);
		if (pSoldier->inv[SECONDHANDPOS].exists() == true && Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass & IC_WEAPON)
			ttl += GetUrbanCamoBonus(pSoldier, &pSoldier->inv[SECONDHANDPOS]);
		if (pSoldier->inv[GUNSLINGPOCKPOS].exists() == true && Item[pSoldier->inv[GUNSLINGPOCKPOS].usItem].usItemClass & IC_WEAPON)
			ttl += GetUrbanCamoBonus(pSoldier, &pSoldier->inv[GUNSLINGPOCKPOS]);
	}

	// sevenfm: cannot have negative camo
	ttl = max(ttl, 0);

	return min( ttl, 100 );
}
INT16 GetWornDesertCamo( SOLDIERTYPE * pSoldier )
{
	INT8	bLoop;
	INT16 ttl=0;

	for (bLoop = HELMETPOS; bLoop <= LEGPOS; bLoop++)
	{
		if ( pSoldier->inv[bLoop].exists() == true )
		{
			ttl += GetDesertCamoBonus(pSoldier, &pSoldier->inv[bLoop]);
			if ( UsingNewInventorySystem() )
				if ( bLoop == VESTPOS && pSoldier->inv[VESTPOCKPOS].exists() )
				{
					// silversurfer: Using LBE vest. Only apply partial bonus from armor vest.
					ttl -= (INT16)( ( 1.0 - gItemSettings.fCamoLBEoverVestModifier ) * Item[ pSoldier->inv[bLoop].usItem ].desertCamobonus );
				}
				else if ( bLoop == LEGPOS )
				{
					FLOAT fLegPenalty = 0;
					// depending on how many leg LBE we use the camo effect of the armor pants is higher or lower
					fLegPenalty += pSoldier->inv[LTHIGHPOCKPOS].exists();
					fLegPenalty += pSoldier->inv[RTHIGHPOCKPOS].exists();
					// Using thigh rigs. Only apply partial bonus from armor pants.
					if ( fLegPenalty > 0 )
						ttl -= (INT16)( ( 1.0 - gItemSettings.fCamoLBEoverPantsModifier ) * Item[ pSoldier->inv[bLoop].usItem ].desertCamobonus * fLegPenalty / 2 );
				}
		}
	}

	// CHRISL: Add additional loop for LBE items while using new inventory system
	if((UsingNewInventorySystem() == true))
	{
		for (bLoop = VESTPOCKPOS; bLoop <= BPACKPOCKPOS; bLoop++)
		{
			if ( pSoldier->inv[bLoop].exists() == true )
				ttl += GetDesertCamoBonus(pSoldier, &pSoldier->inv[bLoop]);
		}

		//tais: guns can be camouflaged, this will make gun camo have effect when in main/second hand or on gunsling, did a check for guns and nothing else, hope that's enough.
		if (pSoldier->inv[HANDPOS].exists() == true && Item[pSoldier->inv[HANDPOS].usItem].usItemClass & IC_WEAPON)
			ttl += GetDesertCamoBonus(pSoldier, &pSoldier->inv[HANDPOS]);
		if (pSoldier->inv[SECONDHANDPOS].exists() == true && Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass & IC_WEAPON)
			ttl += GetDesertCamoBonus(pSoldier, &pSoldier->inv[SECONDHANDPOS]);
		if (pSoldier->inv[GUNSLINGPOCKPOS].exists() == true && Item[pSoldier->inv[GUNSLINGPOCKPOS].usItem].usItemClass & IC_WEAPON)
			ttl += GetDesertCamoBonus(pSoldier, &pSoldier->inv[GUNSLINGPOCKPOS]);
	}

	// sevenfm: cannot have negative camo
	ttl = max(ttl, 0);

	return min( ttl, 100 );
}
INT16 GetWornSnowCamo( SOLDIERTYPE * pSoldier )
{
	INT8	bLoop;
	INT16 ttl=0;

	for (bLoop = HELMETPOS; bLoop <= LEGPOS; bLoop++)
	{
		if ( pSoldier->inv[bLoop].exists() == true )
		{
			ttl += GetSnowCamoBonus(pSoldier, &pSoldier->inv[bLoop]);
			if ( UsingNewInventorySystem() )
				if ( bLoop == VESTPOS && pSoldier->inv[VESTPOCKPOS].exists() )
				{
					// silversurfer: Using LBE vest. Only apply partial bonus from armor vest.
					ttl -= (INT16)( ( 1.0 - gItemSettings.fCamoLBEoverVestModifier ) * Item[ pSoldier->inv[bLoop].usItem ].snowCamobonus );
				}
				else if ( bLoop == LEGPOS )
				{
					FLOAT fLegPenalty = 0;
					// depending on how many leg LBE we use the camo effect of the armor pants is higher or lower
					fLegPenalty += pSoldier->inv[LTHIGHPOCKPOS].exists();
					fLegPenalty += pSoldier->inv[RTHIGHPOCKPOS].exists();
					// Using thigh rigs. Only apply partial bonus from armor pants.
					if ( fLegPenalty > 0 )
						ttl -= (INT16)( ( 1.0 - gItemSettings.fCamoLBEoverPantsModifier ) * Item[ pSoldier->inv[bLoop].usItem ].snowCamobonus * fLegPenalty / 2 );
				}
		}
	}

	// CHRISL: Add additional loop for LBE items while using new inventory system
	if((UsingNewInventorySystem() == true))
	{
		for (bLoop = VESTPOCKPOS; bLoop <= BPACKPOCKPOS; bLoop++)
		{
			if ( pSoldier->inv[bLoop].exists() == true )
				ttl += GetSnowCamoBonus(pSoldier, &pSoldier->inv[bLoop]);
		}

		//tais: guns can be camouflaged, this will make gun camo have effect when in main/second hand or on gunsling, did a check for guns and nothing else, hope that's enough.
		if (pSoldier->inv[HANDPOS].exists() == true && Item[pSoldier->inv[HANDPOS].usItem].usItemClass & IC_WEAPON)
			ttl += GetSnowCamoBonus(pSoldier, &pSoldier->inv[HANDPOS]);
		if (pSoldier->inv[SECONDHANDPOS].exists() == true && Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass & IC_WEAPON)
			ttl += GetSnowCamoBonus(pSoldier, &pSoldier->inv[SECONDHANDPOS]);
		if (pSoldier->inv[GUNSLINGPOCKPOS].exists() == true && Item[pSoldier->inv[GUNSLINGPOCKPOS].usItem].usItemClass & IC_WEAPON)
			ttl += GetSnowCamoBonus(pSoldier, &pSoldier->inv[GUNSLINGPOCKPOS]);
	}

	// sevenfm: cannot have negative camo
	ttl = max(ttl, 0);

	return min( ttl, 100 );
}

void ApplyEquipmentBonuses(SOLDIERTYPE * pSoldier)
{
	if ( pSoldier == NULL) 
		return;

	// sevenfm: only for player mercs. AI soldiers get camo at creation time
	if( !(pSoldier->flags.uiStatusFlags & SOLDIER_PC) )
	{
		return;
	}

	// sevenfm: clear vision range cache
	pSoldier->ResetVisionRangeCache();

	INT16 newCamo = GetWornCamo ( pSoldier );
	INT16 oldCamo = pSoldier->wornCamo;
	if ( oldCamo != newCamo )
		pSoldier->wornCamo = (INT8)newCamo;

	INT16 newUrbanCamo = GetWornUrbanCamo ( pSoldier );
	INT16 oldUrbanCamo = pSoldier->wornUrbanCamo;
	if ( oldUrbanCamo != newUrbanCamo )
		pSoldier->wornUrbanCamo = (INT8)newUrbanCamo;

	INT16 newDesertCamo = GetWornDesertCamo ( pSoldier );
	INT16 oldDesertCamo = pSoldier->wornDesertCamo;
	if ( oldDesertCamo != newDesertCamo )
		pSoldier->wornDesertCamo = (INT8)newDesertCamo;

	INT16 newSnowCamo = GetWornSnowCamo ( pSoldier );
	INT16 oldSnowCamo = pSoldier->wornSnowCamo;
	if ( oldSnowCamo != newSnowCamo )
		pSoldier->wornSnowCamo = (INT8)newSnowCamo;

	if ( (newCamo > oldCamo || newUrbanCamo > oldUrbanCamo || newDesertCamo > oldDesertCamo || newSnowCamo > oldSnowCamo )&& pSoldier->bTeam == OUR_TEAM )
	{
		//CHRISL: This sound interferes with some RPC hiring in NewInv because of the camo bonus some LBE Vests give
		if(UsingNewInventorySystem() == false)
			pSoldier->DoMercBattleSound( BATTLE_SOUND_COOL1 );

		// WANNE: Only call the method if oldCame != newCamo
		if ( pSoldier->bInSector)
			pSoldier->CreateSoldierPalettes( );
	}
	else if ( (newCamo < oldCamo || newUrbanCamo < oldUrbanCamo || newDesertCamo < oldDesertCamo || newSnowCamo < oldSnowCamo )&& pSoldier->bTeam == OUR_TEAM )
	{
		// WANNE: Only call the method if oldCame != newCamo
		if ( pSoldier->bInSector)
			pSoldier->CreateSoldierPalettes( );
	}

	BOOLEAN fUpdateSight = FALSE;
	// sevenfm: check if can still spot
	if (pSoldier->IsSpotting() && !pSoldier->CanUseSkill(SKILLS_SPOTTER, FALSE, pSoldier->sFocusGridNo, pSoldier->bFocusLevel))
	{
		pSoldier->StopSpotting();
		fUpdateSight = TRUE;
	}
	// check if still can focus
	if(pSoldier->IsFocusing() && !pSoldier->CanUseSkill(SKILLS_FOCUS, FALSE, pSoldier->sFocusGridNo, pSoldier->bFocusLevel))
	{
		pSoldier->StopFocusing();
		fUpdateSight = TRUE;
	}

	if(fUpdateSight)
	{
		HandleSight(pSoldier, SIGHT_LOOK);
	}

	fInterfacePanelDirty = DIRTYLEVEL2;
}

UINT16 GetFirstExplosiveOfType(UINT16 expType)
{
	for (UINT16 i = 0; i<gMAXITEMS_READ; i++)
	{
		if ((Item[i].usItemClass == IC_EXPLOSV || Item[i].usItemClass == IC_GRENADE) && Explosive[Item[i].ubClassIndex].ubType == expType)
			return i;
	}

	return 0;
}

// sevenfm: same as GetFirstExplosiveOfType, but check only hand grenades
UINT16 GetFirstHandGrenadeOfType(UINT16 expType)
{
	for (UINT16 i = 0; i < gMAXITEMS_READ; i++)
	{
		if (ItemIsHandGrenade(i) && 
			Item[i].ubCoolness > 0 && 
			Explosive[Item[i].ubClassIndex].ubType == expType)
			return i;
	}

	return 0;
}

// sevenfm: use to substitute default item with first matching if needed
UINT16 GetHandGrenadeOfType(UINT16 usDefaultItem, UINT16 usType)
{
	UINT16 usSmokeItem = usDefaultItem;

	if (usSmokeItem > 0 &&
		ItemIsHandGrenade(usSmokeItem) &&
		Item[usSmokeItem].ubCoolness > 0 &&
		Explosive[Item[usSmokeItem].ubClassIndex].ubType == usType)
		return usSmokeItem;

	usSmokeItem = GetFirstHandGrenadeOfType(EXPLOSV_SMOKE);

	if (usSmokeItem > 0 &&
		ItemIsHandGrenade(usSmokeItem) &&
		Item[usSmokeItem].ubCoolness > 0 &&
		Explosive[Item[usSmokeItem].ubClassIndex].ubType == usType)
		return usSmokeItem;

	return 0;
}

UINT16 GetMortarShellOfType(UINT16 usType)
{
	for (UINT16 i = 0; i < gMAXITEMS_READ; i++)
	{
		if (Item[i].ubCoolness > 0 &&
			ItemIsMortarShell(i) &&
			(usType == EXPLOSV_ANY_TYPE || Explosive[Item[i].ubClassIndex].ubType == usType))
		{
			return i;
		}
	}

	return 0;
}

UINT16 GetRandomDrug(void)
{
	std::vector<UINT16>	ItemList;

	for (UINT16 i = 0; i < gMAXITEMS_READ; i++)
	{
		if (Item[i].ubCoolness > 0 &&
			Item[i].usItemClass & IC_MISC &&
			ItemIsLegal(i) &&
			Item[i].drugtype &&
			Item[i].usPrice < 500)
		{
			ItemList.push_back(i);
		}
	}

	if (ItemList.size() > 0)
	{
		return ItemList[Random(ItemList.size())];
	}

	return 0;
}

UINT16 GetTNT(UINT16 usDefaultItem)
{
	if (usDefaultItem > 0 && 
		Item[usDefaultItem].ubCoolness > 0 &&
		ItemIsTNT(usDefaultItem))
		return usDefaultItem;

	for (UINT16 i = 0; i < gMAXITEMS_READ; i++)
	{
		if (Item[i].ubCoolness > 0 &&
			ItemIsTNT(i))
			return i;
	}

	return 0;
}

UINT16 GetWirecutters(UINT16 usDefaultItem)
{
	if (usDefaultItem > 0 &&
		Item[usDefaultItem].ubCoolness > 0 &&
		(Item[usDefaultItem].usItemClass & IC_MISC) &&
		Item[usDefaultItem].wirecutters)
		return usDefaultItem;

	for (UINT16 i = 0; i < (INT32)gMAXITEMS_READ; i++)
	{
		if (Item[i].ubCoolness > 0 &&
			(Item[i].usItemClass & IC_MISC) &&
			Item[i].wirecutters)
			return i;
	}

	return 0;
}

UINT16 GetFirstAidKit(UINT16 usDefaultItem)
{
	if (usDefaultItem > 0 &&
		Item[usDefaultItem].ubCoolness > 0 &&
		(Item[usDefaultItem].usItemClass & IC_MEDKIT) &&
		Item[usDefaultItem].firstaidkit)
		return usDefaultItem;

	for (UINT16 i = 0; i < (INT32)gMAXITEMS_READ; i++)
	{
		if (Item[i].ubCoolness > 0 &&
			(Item[i].usItemClass & IC_MEDKIT) &&
			Item[i].firstaidkit)
			return i;
	}

	return 0;
}

UINT16 GetTraumaPack(void)
{
	for (UINT16 i = 0; i < (INT32)gMAXITEMS_READ; i++)
	{
		if (Item[i].ubCoolness > 0 &&
			(Item[i].usItemClass & IC_MEDKIT) &&
			Item[i].firstaidkit &&
			!Item[i].bPoisonPercentage &&
			Item[i].percentstatusdrainreduction <= -100)
			return i;
	}

	return 0;
}

// sevenfm: check if item is valid hand grenade
BOOLEAN ItemIsHandGrenade(UINT16 usItem)
{
	if (usItem > 0 &&
		Item[usItem].usItemClass == IC_GRENADE &&
		Item[usItem].ubCursor == TOSSCURS)
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN ItemIsGLGrenade(UINT16 usItem)
{
	if (usItem > 0 &&
		Item[usItem].usItemClass == IC_GRENADE &&
		Item[usItem].glgrenade)
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN ItemIsMortarShell(UINT16 usItem)
{
	UINT16 usLauncherItem;
	
	if (usItem > 0 &&
		Item[usItem].usItemClass == IC_BOMB &&
		//Item[usItem].attachment &&
		!Item[usItem].mine)
	{
		usLauncherItem = GetLauncherFromLaunchable(usItem);
		if (usLauncherItem != NOTHING && Item[usLauncherItem].mortar)
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN ItemIsRocket(UINT16 usItem)
{
	UINT16 usLauncherItem;

	if (usItem > 0 &&
		Item[usItem].usItemClass == IC_GRENADE &&
		Item[usItem].electronic)
	{
		usLauncherItem = GetLauncherFromLaunchable(usItem);
		if (usLauncherItem != NOTHING && (Item[usLauncherItem].rocketlauncher || Item[usLauncherItem].singleshotrocketlauncher))
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN ItemIsTNT(UINT16 usItem)
{
	if (Item[usItem].usItemClass == IC_BOMB &&
		!Item[usItem].mine &&
		!Item[usItem].tripwire &&
		//Item[usItem].ubCursor != BOMBCURS &&
		Explosive[Item[usItem].ubClassIndex].ubType == EXPLOSV_NORMAL &&
		Explosive[Item[usItem].ubClassIndex].ubDamage > 40 &&
		GetLauncherFromLaunchable(usItem) == NOTHING)
		return TRUE;

	return FALSE;
}

BOOLEAN ItemIsBearTrap(UINT16 usItem)
{
	if (Item[usItem].usItemClass == IC_BOMB &&
		Explosive[Item[usItem].ubClassIndex].ubType == EXPLOSV_BEARTRAP)
		return TRUE;

	return FALSE;
}

// WDS - Smart goggle switching
OBJECTTYPE* FindSunGogglesInInv( SOLDIERTYPE * pSoldier, INT8 * bSlot, BOOLEAN * isAttach, BOOLEAN searchAllInventory )
{
	INT8	bLoop;
	INT16	bonusToBeat = 0;
	OBJECTTYPE*	pGoggles = 0;
	// CHRISL:
	// silversurfer: use the sum of day vision bonus and bright light bonus because both help at daytime
	// but make sure that day vision bonus is > 0 because we are not always looking at bright lights
	for (bLoop = (searchAllInventory ? HELMETPOS : HANDPOS); bLoop < NUM_INV_SLOTS; bLoop++) 
	{
		if (pSoldier->inv[bLoop].exists())
		{
			if ( //Item[pSoldier->inv[bLoop].usItem].dayvisionrangebonus > 0 && 
					( Item[pSoldier->inv[bLoop].usItem].dayvisionrangebonus + Item[pSoldier->inv[bLoop].usItem].brightlightvisionrangebonus ) > bonusToBeat && 
					  Item[pSoldier->inv[bLoop].usItem].usItemClass == IC_FACE ) 
			{
				pGoggles = &(pSoldier->inv[bLoop]);
				*bSlot = bLoop;
				*isAttach = FALSE;
				bonusToBeat = Item[pSoldier->inv[bLoop].usItem].dayvisionrangebonus + Item[pSoldier->inv[bLoop].usItem].brightlightvisionrangebonus;
			}
			if (searchAllInventory) 
			{
				for (UINT8 loop = 0; loop < pSoldier->inv[bLoop].ubNumberOfObjects; loop ++)
				{
					for (attachmentList::iterator iter = pSoldier->inv[bLoop][loop]->attachments.begin(); iter != pSoldier->inv[bLoop][loop]->attachments.end(); ++iter) 
					{
						if ( iter->exists() && 
							//Item[ iter->usItem ].dayvisionrangebonus > 0 &&
							( Item[ iter->usItem ].dayvisionrangebonus + Item[ iter->usItem ].brightlightvisionrangebonus ) > bonusToBeat &&
							   Item[ iter->usItem ].usItemClass == IC_FACE ) 
						{
							pGoggles = &(*iter);
							*bSlot = bLoop;
							*isAttach = TRUE;
							bonusToBeat = Item[ iter->usItem ].dayvisionrangebonus + Item[ iter->usItem ].brightlightvisionrangebonus;
						}
					}
				}
			}
		}
	}
	return( pGoggles );
}

OBJECTTYPE* FindNightGogglesInInv( SOLDIERTYPE * pSoldier, INT8 * bSlot, BOOLEAN * isAttach, BOOLEAN searchAllInventory )
{
	INT8	bLoop;
	INT16	bonusToBeat = 0;
	OBJECTTYPE*	pGoggles = 0;
	// CHRISL:
	// silversurfer: check if we are above ground, night vision is only useful there
	if ( pSoldier->bSectorZ == 0 )
	{
		for (bLoop = (searchAllInventory ? HELMETPOS : HANDPOS); bLoop < NUM_INV_SLOTS; bLoop++) {
			if ( pSoldier->inv[bLoop].exists() == true ) {
				if (Item[pSoldier->inv[bLoop].usItem].nightvisionrangebonus > bonusToBeat && Item[pSoldier->inv[bLoop].usItem].usItemClass == IC_FACE ) {
					pGoggles = &(pSoldier->inv[bLoop]);
					*bSlot = bLoop;
					*isAttach = FALSE;
					bonusToBeat = Item[pSoldier->inv[bLoop].usItem].nightvisionrangebonus;
				}
				if (searchAllInventory) {
					for (UINT8 loop = 0; loop < pSoldier->inv[bLoop].ubNumberOfObjects; loop ++){
						for (attachmentList::iterator iter = pSoldier->inv[bLoop][loop]->attachments.begin(); iter != pSoldier->inv[bLoop][loop]->attachments.end(); ++iter) 
						{
							if ( iter->exists() && Item[ iter->usItem ].nightvisionrangebonus > bonusToBeat && Item[ iter->usItem ].usItemClass == IC_FACE ) 
							{
								pGoggles = &(*iter);
								*bSlot = bLoop;
								*isAttach = TRUE;
								bonusToBeat = Item[ iter->usItem ].nightvisionrangebonus;
							}
						}
					}
				}
			}
		}
	}
	// below ground we need cave vision
	else
	{
		for (bLoop = (searchAllInventory ? HELMETPOS : HANDPOS); bLoop < NUM_INV_SLOTS; bLoop++) 
		{
			if ( pSoldier->inv[bLoop].exists() == true ) {
				if (Item[pSoldier->inv[bLoop].usItem].cavevisionrangebonus > bonusToBeat && Item[pSoldier->inv[bLoop].usItem].usItemClass == IC_FACE ) 
				{
					pGoggles = &(pSoldier->inv[bLoop]);
					*bSlot = bLoop;
					*isAttach = FALSE;
					bonusToBeat = Item[pSoldier->inv[bLoop].usItem].cavevisionrangebonus;
				}
				if (searchAllInventory) 
				{
					for (UINT8 loop = 0; loop < pSoldier->inv[bLoop].ubNumberOfObjects; loop ++)
					{
						for (attachmentList::iterator iter = pSoldier->inv[bLoop][loop]->attachments.begin(); iter != pSoldier->inv[bLoop][loop]->attachments.end(); ++iter) 
						{
							if (iter->exists() && Item[iter->usItem].cavevisionrangebonus > bonusToBeat && Item[iter->usItem].usItemClass == IC_FACE)
							{
								pGoggles = &(*iter);
								*bSlot = bLoop;
								*isAttach = TRUE;
								bonusToBeat = Item[ iter->usItem ].cavevisionrangebonus;
							}
						}
					}
				}
			}
		}
	}

	return( pGoggles );
}

FLOAT GetHighestScopeMagnificationFactor( OBJECTTYPE * pObj )
{
	FLOAT BestFactor = 1.0;

	if ( pObj->exists() == true && UsingNewCTHSystem() == true ) {
		BestFactor = Item[pObj->usItem].scopemagfactor;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists())
			{
				BestFactor = __max(BestFactor, Item[iter->usItem].scopemagfactor);
			}
		}
	}

	return( BestFactor );
}

INT16 GetMinRangeForAimBonus( SOLDIERTYPE* pSoldier, OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	BOOLEAN fUseCowitnessSight = FALSE;

	if ( pObj->exists() == true ) 
	{
		// use item
		bonus = Item[pObj->usItem].minrangeforaimbonus;

		// use active scope
		if ( gGameExternalOptions.fScopeModes && pSoldier && pSoldier->bTeam == gbPlayerNum && Item[pObj->usItem].usItemClass == IC_GUN )
		{
			std::map<INT8, OBJECTTYPE*> ObjList;
			GetScopeLists(pSoldier, pObj, ObjList);
		
			// only use scope mode if gun is in hand, otherwise an error might occur!
			if ((&pSoldier->inv[HANDPOS]) == pObj  && ObjList[pSoldier->bScopeMode] != NULL && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD)
			{
				bonus = max(bonus, Item[ObjList[pSoldier->bScopeMode]->usItem].minrangeforaimbonus);
				if (IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT) || IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SCOPE))
					fUseCowitnessSight = TRUE;
			}
		}

		//bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].minrangeforaimbonus;

		// add bonus from attachments except scopes
		attachmentList::iterator iterend = (*pObj)[0]->attachments.end();
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != iterend; ++iter) 
		{
			if (!gGameExternalOptions.fScopeModes || 
				iter->exists() && !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) && (!IsAttachmentClass(iter->usItem, AC_EXTERNAL) || 
				fUseCowitnessSight))
				bonus = max(bonus, Item[iter->usItem].minrangeforaimbonus);
		}
	}

	return max(0, bonus);
}

// altered by Flugente: uiRange is no more considered
FLOAT GetScopeMagnificationFactor( SOLDIERTYPE *pSoldier, OBJECTTYPE * pObj, FLOAT uiRange )
{
	FLOAT BestFactor = 1.0;
	FLOAT CurrentFactor = 0.0;
	FLOAT TargetMagFactor = __max(1.0f,(FLOAT)uiRange / (FLOAT)gGameCTHConstants.NORMAL_SHOOTING_DISTANCE);
	FLOAT rangeModifier = gGameCTHConstants.SCOPE_RANGE_MULTIPLIER;

	TargetMagFactor = TargetMagFactor / rangeModifier;

	// Flugente: if scope modes are allowed, use them
	if ( gGameExternalOptions.fScopeModes && pSoldier && pObj->exists() && Item[pObj->usItem].usItemClass == IC_GUN)
	{
		// Flugente: check for scope mode
		std::map<INT8, OBJECTTYPE*> ObjList;
		GetScopeLists(pSoldier, pObj, ObjList);
		
		// only use scope mode if gun is in hand, otherwise an error might occur!
		if ((&pSoldier->inv[HANDPOS]) == pObj  && ObjList[pSoldier->bScopeMode] != NULL && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD)
		{
			// now apply the bonus from the scope we use
			BestFactor = Item[ObjList[pSoldier->bScopeMode]->usItem].scopemagfactor;

			// sevenfm: bonus for holo sight from magnifier
			if (BestFactor <= 1.0f && IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT))
			{
				attachmentList::iterator iterend = (*pObj)[0]->attachments.end();
				for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != iterend; ++iter)
				{
					if (iter->exists() &&
						IsAttachmentClass(iter->usItem, AC_EXTERNAL) &&
						Item[iter->usItem].scopemagfactor > 1.0f)
					{
						BestFactor = Item[iter->usItem].scopemagfactor;
					}
				}
			}
		}

		return max(1.0f, BestFactor);
	}		
	
	if(pObj->exists() == true && UsingNewCTHSystem() == true)
	{
		BestFactor = __max(1.0f, Item[pObj->usItem].scopemagfactor);

		attachmentList::iterator iterend = (*pObj)[0]->attachments.end();
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != iterend; ++iter) 
		{
			if (iter->exists())
			{
				if(BestFactor == 1.0f)
					BestFactor = __max(BestFactor, Item[iter->usItem].scopemagfactor);
				else if(Item[iter->usItem].scopemagfactor >= TargetMagFactor)
					BestFactor = Item[iter->usItem].scopemagfactor;
			}						
		}
	}

	return __max(1.0f, BestFactor);
}

FLOAT GetBestScopeMagnificationFactor( SOLDIERTYPE *pSoldier, OBJECTTYPE * pObj, FLOAT uiRange )
{
	Assert(pSoldier);
	if(!pSoldier)
	{
		return 1.0f;
	}

	// Flugente: if this weapon is an underbarrel weapon, use the 'carrier' weapon instead
	OBJECTTYPE* pObjUsed = pObj;
	if ( pObj == pSoldier->GetUsedWeapon( &pSoldier->inv[pSoldier->ubAttackingHand] ) )
	{
		pObjUsed = &pSoldier->inv[pSoldier->ubAttackingHand];
	}
		
	FLOAT BestFactor = 1.0;
	FLOAT TargetMagFactor = __max(1.0f,uiRange / (FLOAT)gGameCTHConstants.NORMAL_SHOOTING_DISTANCE);
	FLOAT CurrentFactor = 0.0;
	FLOAT ActualCurrentFactor = 0.0;
	INT32 iCurrentTotalPenalty = 0;
	INT32 iBestTotalPenalty = 0;
	FLOAT rangeModifier = GetScopeRangeMultiplier(pSoldier, pObjUsed, uiRange);

	// Flugente: if scope modes are allowed, use them
	if ( gGameExternalOptions.fScopeModes && pSoldier && pObjUsed->exists() && Item[pObjUsed->usItem].usItemClass == IC_GUN )
	{
		// Flugente: check for scope mode
		std::map<INT8, OBJECTTYPE*> ObjList;
		GetScopeLists(pSoldier, pObjUsed, ObjList);
		
		// only use scope mode if gun is in hand, otherwise an error might occur!
		if ((&pSoldier->inv[HANDPOS]) == pObjUsed  && ObjList[pSoldier->bScopeMode] != NULL && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD)
		{
			// now apply the bonus from the scope we use
			CurrentFactor = max(1.0f, Item[ObjList[pSoldier->bScopeMode]->usItem].scopemagfactor);

			// sevenfm: bonus for holo sight from magnifier
			if (CurrentFactor <= 1.0f && IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_SIGHT))
			{
				attachmentList::iterator iterend = (*pObj)[0]->attachments.end();
				for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != iterend; ++iter)
				{
					if (iter->exists() &&
						IsAttachmentClass(iter->usItem, AC_EXTERNAL) &&
						Item[iter->usItem].scopemagfactor > 1.0f)
					{
						CurrentFactor = Item[iter->usItem].scopemagfactor;
					}
				}
			}
		}
		else
			CurrentFactor = 1.0f;

		// with scope modes we just return the scope factor of the selected optics
		return( __max(1.0f, CurrentFactor) );
	}
	// if not using scope modes find the best scope
	else
	{
		if (TargetMagFactor <= 1.0f)
		{
			// Target is at Iron Sights range. No scope is required.
			return 1.0f;
		}

		if ( pObj->exists() == true && UsingNewCTHSystem() == true )
		{
			// Real Scope Magnification Factor from the item
			CurrentFactor = __max(1.0f, Item[pObjUsed->usItem].scopemagfactor);

			if (CurrentFactor > 1.0f)
			{
				// Actual Scope Mag Factor is what we get at the distance the target's at.
				ActualCurrentFactor = __min(CurrentFactor, (TargetMagFactor/rangeModifier));

				if (ActualCurrentFactor >= CurrentFactor * gGameCTHConstants.AIM_TOO_CLOSE_THRESHOLD)
				{
					// This scope gives no penalty. Record this as the best factor found so far.
					BestFactor = CurrentFactor;
					iBestTotalPenalty = 0;
				}
				else
				{
					// This scopes gives a penalty for shooting under its range.
					FLOAT dScopePenaltyRatio = (CurrentFactor * gGameCTHConstants.AIM_TOO_CLOSE_THRESHOLD * rangeModifier / TargetMagFactor);
					INT32 iScopePenalty = (INT32)((dScopePenaltyRatio * gGameCTHConstants.AIM_TOO_CLOSE_SCOPE) * (CurrentFactor / 2));

					// There's no previous scope to compare with so record this as the best factor for now.
					BestFactor = CurrentFactor;
					iBestTotalPenalty = iScopePenalty;
				}
			}
		
			// Now perform the same process for each scope installed on the item. The difference is, we also compare to 
			// BestTotalPenalty to find the scope that gives the least penalty compared to its bonus.
			for (attachmentList::iterator iter = (*pObjUsed)[0]->attachments.begin(); iter != (*pObjUsed)[0]->attachments.end(); ++iter) 
			{
				if (iter->exists() && Item[iter->usItem].scopemagfactor > 1.0f)
				{
					// Real Scope Magnification Factor from the item
					CurrentFactor = __max(1.0f, Item[iter->usItem].scopemagfactor);

					// Actual Scope Mag Factor is what we get at the distance the target's at.
					ActualCurrentFactor = __min(CurrentFactor, (TargetMagFactor/rangeModifier));

					if (ActualCurrentFactor >= CurrentFactor * gGameCTHConstants.AIM_TOO_CLOSE_THRESHOLD)
					{
						// This scope gives no penalty. Is it any better than the ones we've already processed?
						if (iBestTotalPenalty <= 0 && CurrentFactor > BestFactor)
						{
							// This is the best scope we've found so far. Record it.
							BestFactor = CurrentFactor;
							iBestTotalPenalty = 0;
						}
					}
					else
					{
						// This scope will give a penalty if used. Is it worth using compared to other scopes found?
						FLOAT dScopePenaltyRatio = (CurrentFactor * gGameCTHConstants.AIM_TOO_CLOSE_THRESHOLD * rangeModifier / TargetMagFactor);
						INT32 iScopePenalty = (INT32)((dScopePenaltyRatio * gGameCTHConstants.AIM_TOO_CLOSE_SCOPE) * (CurrentFactor / 2));

						// Is this scope any better than the ones we've already processed?
						// this new formula takes gaps between different scopes into account because
						// even with a penalty a higher power scope is not necessarily a bad choice
						// 10x sniper scopes still suck though, at the moment they are useless because of the big penalty
						if (dScopePenaltyRatio <= CurrentFactor / (( CurrentFactor + BestFactor ) /2 -1 )  && CurrentFactor > BestFactor)
						{
							// This is the best scope we've found so far. Record it.
							BestFactor = CurrentFactor;
							iBestTotalPenalty = iScopePenalty;
						}
					}
				}
			}
		}
	}
	return( __max(1.0f, BestFactor) );
}

FLOAT GetScopeRangeMultiplier( SOLDIERTYPE *pSoldier, OBJECTTYPE *pObj, FLOAT d2DDistance )
{
	FLOAT	iScopeFactor = 0;
	FLOAT	rangeModifier = gGameCTHConstants.SCOPE_RANGE_MULTIPLIER;

	iScopeFactor = GetScopeMagnificationFactor( pSoldier, pObj, d2DDistance );

	// if we are not using a scope we will not apply "SCOPE_RANGE_MULTIPLIER"
	if ( iScopeFactor <= 1.0 )
		rangeModifier = 1.0;

	if( gGameOptions.fNewTraitSystem )
	{
		if(iScopeFactor > 5.0f)
			rangeModifier -= (NUM_SKILL_TRAITS( pSoldier, SNIPER_NT ) * 0.05f);
		else
			rangeModifier -= (NUM_SKILL_TRAITS( pSoldier, RANGER_NT ) * 0.05f);
	}
	else
		rangeModifier -= (NUM_SKILL_TRAITS( pSoldier, PROF_SNIPER_OT ) * 0.05f);

	return rangeModifier;
}

UINT8 AllowedAimingLevelsNCTH( SOLDIERTYPE *pSoldier, INT32 sGridNo )
{
	INT8 aimLevels = 4;
	FLOAT iScopeMagFactor = 0.0, rangeMultiplier = 0.0;
	BOOLEAN allowed = TRUE;
	UINT16 weaponRange;
	UINT8 weaponType;
	BOOLEAN fTwoHanded, fUsingBipod;

	Assert(pSoldier);
	if (!pSoldier)
	{
		return 1;
	}

	if (pSoldier->bBlindedCounter > 0)
	{
		return 1;
	}

	// sevenfm: check if we have correct sGridNo
	//Assert(!TileIsOutOfBounds(sGridNo));
	UINT32 uiRange;
	if (TileIsOutOfBounds(sGridNo))
		uiRange = TACTICAL_RANGE * CELL_X_SIZE;
	else
		uiRange = GetRangeInCellCoordsFromGridNoDiff(pSoldier->sGridNo, sGridNo);

	rangeMultiplier = GetScopeRangeMultiplier(pSoldier, &pSoldier->inv[pSoldier->ubAttackingHand], (FLOAT)uiRange);

	// HEADROCK HAM 4: This function has been radically altered AGAIN for the NCTH project.
	// Weapons can now have a tag that defines how many aim clicks they should have. Under the NCTH
	// system, weapons with FEWER aiming clicks are faster to aim without being any less accurate.
	//
	// If the weapon lacks a ubAimLevels tag, the program uses the old HAM 3 algorithm to figure
	// out how much it should have using its class and range.

	// Read from item
	aimLevels = Weapon[pSoldier->inv[pSoldier->ubAttackingHand].usItem].ubAimLevels;
	fTwoHanded = Item[pSoldier->inv[pSoldier->ubAttackingHand].usItem].twohanded;
	weaponRange = ( Weapon[pSoldier->inv[pSoldier->ubAttackingHand].usItem].usRange * GetPercentRangeBonus(&pSoldier->inv[pSoldier->ubAttackingHand]) ) / 10000;
	weaponRange += GetRangeBonus(&pSoldier->inv[pSoldier->ubAttackingHand]);
	weaponType = Weapon[pSoldier->inv[pSoldier->ubAttackingHand].usItem].ubWeaponType;
	fUsingBipod = FALSE;

	// SANDRO - throwing knives are a special case, allow 2 aiming clicks for them
	if( weaponType == NOT_GUN ) 
	{
		aimLevels = 2;

		if ( Item[pSoldier->inv[pSoldier->ubAttackingHand].usItem].usItemClass == IC_THROWING_KNIFE )
		{
			// minus the bonus for Throwing trait
			if ( gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( pSoldier, THROWING_NT ) )
			{
				aimLevels -= gSkillTraitValues.ubTHBladesAimClicksAdded;
			}
		}
		return max(1, aimLevels);
	}

	// If outside limits...
	if (aimLevels < 1 || aimLevels > 8 )
	{
		// Probably uninitialized. Run an algorithm instead.
		
		// Read weapon data
		
		// Define basic (no attachments), and absolute maximums
		if (weaponType == GUN_PISTOL || weaponType == GUN_M_PISTOL || fTwoHanded == 0)
		{
			aimLevels = 2;
 		}
		else if (weaponType == GUN_SHOTGUN || weaponType == GUN_LMG || weaponType == GUN_SMG)
 		{
			aimLevels = 3;
 		}
		else if ((weaponType == GUN_AS_RIFLE || weaponType == GUN_RIFLE ) && weaponRange <= 500)
		{
			aimLevels = 4;
		}
		else if (((weaponType == GUN_AS_RIFLE || weaponType == GUN_RIFLE) && weaponRange > 500) || 
						(weaponType == GUN_SN_RIFLE && weaponRange <= 500))
		{
			aimLevels = 6;
		}
		else if (weaponType == GUN_SN_RIFLE && weaponRange > 500)
		{
			aimLevels = 8;
		}
		else
		{
			aimLevels = 4;
		}
 	}

	// SANDRO - scopes are not gonna give us any aim levels when firing from hip etc.
	if ( pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD ) 
	{
		// HEADROCK HAM 4: This modifier from the weapon and its attachments replaces the generic bipod bonus.
		UINT8 stance = gAnimControl[ pSoldier->usAnimState ].ubEndHeight;

		// Flugente: new feature: if the next tile in our sight direction has a height so that we could rest our weapon on it, we do that, thereby gaining the prone boni instead. This includes bipods
		if ( gGameExternalOptions.fWeaponResting && pSoldier->IsWeaponMounted() )
			stance = ANIM_PRONE;

		INT32 moda = GetObjectModifier( pSoldier, &pSoldier->inv[pSoldier->ubAttackingHand], stance, ITEMMODIFIER_AIMLEVELS );
		INT32 modb = GetObjectModifier( pSoldier, &pSoldier->inv[pSoldier->ubAttackingHand], gAnimControl[ pSoldier->usAnimState ].ubEndHeight, ITEMMODIFIER_AIMLEVELS );
		aimLevels += (INT32) ((gGameExternalOptions.ubProneModifierPercentage * moda + (100 - gGameExternalOptions.ubProneModifierPercentage) * modb)/100); 
	}

	aimLevels += GetAimLevelsTraitModifier( pSoldier, &pSoldier->inv[pSoldier->ubAttackingHand]);

	// however, the alternative stance, reduces number of aim levels on its own
	if ( pSoldier->bScopeMode == USE_ALT_WEAPON_HOLD ) 
	{
		aimLevels = (aimLevels * (100 - gGameExternalOptions.ubAltWeaponHoldingAimLevelsReduced) + 50) / 100; // round up
	}

	aimLevels = max(1, aimLevels);
	aimLevels = min(8, aimLevels);

 	return ((UINT8)aimLevels);
}

UINT8 AllowedAimingLevels(SOLDIERTYPE * pSoldier, INT32 sGridNo)
{
	 if(UsingNewCTHSystem() == true)
		 return AllowedAimingLevelsNCTH(pSoldier, sGridNo);

	 Assert(pSoldier);
	 if(!pSoldier)
	 {
		 return 1;
	 }

	 if(pSoldier->bBlindedCounter > 0)
	 {
		 return 1;
	 }

	 // sevenfm: check if we have correct sGridNo
	 //Assert(!TileIsOutOfBounds(sGridNo));
	 UINT32 uiRange;
	 INT16 sNormalDistance = TACTICAL_RANGE / 2;
	 INT16 sShootingDistance;
	 if (TileIsOutOfBounds(sGridNo))
	 {
		 uiRange = TACTICAL_RANGE * CELL_X_SIZE;
		 sShootingDistance = TACTICAL_RANGE;
	 }
	 else
	 {
		 uiRange = GetRangeInCellCoordsFromGridNoDiff(pSoldier->sGridNo, sGridNo);
		 sShootingDistance = PythSpacesAway(pSoldier->sGridNo, sGridNo);
	 }

	 UINT16 usItemUsed = pSoldier->GetUsedWeaponNumber(&(pSoldier->inv[pSoldier->ubAttackingHand]));
	 OBJECTTYPE * pInHand = pSoldier->GetUsedWeapon(&(pSoldier->inv[pSoldier->ubAttackingHand]));

	 if (usItemUsed == FLAMETHROWER || (AmmoTypes[(*pInHand)[0]->data.gun.ubGunAmmoType].ammoflag & AMMOFLAG_FLAME))
	 {
		 return 2;
	 }

	 UINT8 aimLevels = 4;
	 UINT16 sScopeBonus = 0;
	 BOOLEAN allowed = TRUE;
	 UINT8 weaponType;	 

	 weaponType = Weapon[pSoldier->inv[pSoldier->ubAttackingHand].usItem].ubWeaponType;

	 if ( gGameExternalOptions.fAimLevelRestriction ) // Extra aiming on/off 
	 {
		 // HEADROCK HAM B2.6: Dynamic aiming level restrictions based on gun type and attachments.
		 // HEADROCK HAM 3.5: Revamped this - it was illogically constructed.
		 if ( gGameExternalOptions.fDynamicAimingTime )
		 {
			 // SANDRO - throwing knives are a special case, allow two aiming clicks for them
			 if( weaponType == NOT_GUN ) 
			 {
				 if ( Item[pSoldier->inv[pSoldier->ubAttackingHand].usItem].usItemClass == IC_THROWING_KNIFE )
				 {
					 aimLevels = 2;
					 if ( gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( pSoldier, THROWING_NT ) )
					 {
						 aimLevels += gSkillTraitValues.ubTHBladesAimClicksAdded;
					 }
					 return ( min(6, aimLevels) );
				 }
				 else
				 {
					 return ( 2 );
				 }
			 }

			 UINT16 weaponRange;
			 UINT8 maxAimForType, maxAimWithoutBipod;
			 BOOLEAN fTwoHanded, fUsingBipod;

			 // Read weapon data
			 fTwoHanded = Item[pSoldier->inv[pSoldier->ubAttackingHand].usItem].twohanded;

			 UINT16 usRange = GetModifiedGunRange(pSoldier->inv[pSoldier->ubAttackingHand].usItem);

			 weaponRange = ( usRange * GetPercentRangeBonus(&pSoldier->inv[pSoldier->ubAttackingHand]) ) / 10000;
			 weaponRange += GetRangeBonus(&pSoldier->inv[pSoldier->ubAttackingHand]);
			 fUsingBipod = FALSE;

			 maxAimWithoutBipod = 4;

			 // sevenfm: change aim levels:
			 // pistols 2-4
			 // MP, shotguns: 3-4
			 // Rifles, AR, LMG: 4-6
			 // Sniper rifles: 4-8

			 // Define basic (no attachments), and absolute maximums
			 if (weaponType == GUN_PISTOL || weaponType == GUN_M_PISTOL )
			 {
				 maxAimForType = 4;
				 aimLevels = 2;
				 maxAimWithoutBipod = 4;

				 // SANDRO - STOMP traits - Gunslinger bonus aim clicks
				 if ( gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( pSoldier, GUNSLINGER_NT ) )
				 {
					 //maxAimForType += (gSkillTraitValues.ubGSAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, GUNSLINGER_NT ));
					 aimLevels += (gSkillTraitValues.ubGSAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, GUNSLINGER_NT ));
					 //maxAimWithoutBipod += (gSkillTraitValues.ubGSAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, GUNSLINGER_NT ));
				 }
			 }
			 //else if ((weaponType == GUN_SMG && fTwoHanded == 0) || fTwoHanded == 0)
			 else if (!fTwoHanded)
			 {
				 maxAimForType = 4;
				 aimLevels = 2;
				 maxAimWithoutBipod = 4;
			 }
			 else if (weaponType == GUN_SHOTGUN)
			 {
				 maxAimForType = 4;
				 aimLevels = 3;
				 maxAimWithoutBipod = 4;

				 // silversurfer added this - STOMP traits - Ranger bonus aim clicks
				 if ( gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( pSoldier, RANGER_NT ) )
				 {
					 //maxAimForType += (gSkillTraitValues.ubRAAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ));
					 aimLevels += (gSkillTraitValues.ubRAAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ));
					 //maxAimWithoutBipod += (gSkillTraitValues.ubRAAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ));
				 }
			 }
			 //else if (weaponType == GUN_SMG && fTwoHanded)
			 else if (weaponType == GUN_SMG)
			 {
				 maxAimForType = 4;
				 aimLevels = 3;
				 maxAimWithoutBipod = 4;
			 }
			 else if (weaponType == GUN_LMG)
			 {
				 maxAimForType = 6;
				 aimLevels = 4;
				 maxAimWithoutBipod = 4;
			 }
			 else if (weaponType == GUN_AS_RIFLE || weaponType == GUN_RIFLE )
			 {
				 maxAimForType = 6;
				 aimLevels = 4;
				 maxAimWithoutBipod = 4;

				 // SANDRO - STOMP traits - Sniper bonus aim clicks
				 if ( gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( pSoldier, SNIPER_NT ) )
				 {
					 //maxAimForType += (gSkillTraitValues.ubSNAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, SNIPER_NT ));
					 aimLevels += (gSkillTraitValues.ubSNAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, SNIPER_NT ));
					 //maxAimWithoutBipod += (gSkillTraitValues.ubSNAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, SNIPER_NT ));
				 }
			 }
			 else if (weaponType == GUN_SN_RIFLE)
			 {
				 maxAimForType = 8;
				 aimLevels = 4;
				 maxAimWithoutBipod = 4;

				 // SANDRO - STOMP traits - Sniper bonus aim clicks
				 if ( gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( pSoldier, SNIPER_NT ) )
				 {
					 //maxAimForType += (gSkillTraitValues.ubSNAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, SNIPER_NT ));
					 aimLevels += (gSkillTraitValues.ubSNAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, SNIPER_NT ));
					 //maxAimWithoutBipod += (gSkillTraitValues.ubSNAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, SNIPER_NT ));
				 }
			 }
			 else
			 {
				 // unknown gun type
				 return 4;
			 }

			 // Determine whether a bipod is being used (prone)
			 UINT8 stance = gAnimControl[ pSoldier->usAnimState ].ubEndHeight;

			 // Flugente: new feature: if the next tile in our sight direction has a height so that we could rest our weapon on it, we do that, thereby gaining the prone boni instead. This includes bipods
			 if ( gGameExternalOptions.fWeaponResting && pSoldier->IsWeaponMounted() )
				 stance = ANIM_PRONE;

			 if (GetBipodBonus(&pSoldier->inv[pSoldier->ubAttackingHand])>0 && stance == ANIM_PRONE )
			 {
				 fUsingBipod = TRUE;
			 }
			 // SANDRO - scopes are not gonna give us any aim levels when firing from hip etc.
			 if ( pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD )
			 {
				 // don't break compatibility, let the users choose
				 if (gGameExternalOptions.iAimLevelsCompatibilityOption != 0)
				 {
					 sScopeBonus = OldWayOfCalculatingScopeBonus(pSoldier);
				 }
				 //WarmSteel - Using scope aimbonus instead, as it is used elsewhere like this too.
				 //Also, you won't get extra aimclicks anymore if you're too close to use your scope.
				 //I've externalized the scope types.
				 else if ( gGameExternalOptions.fAimLevelsDependOnDistance )
				 {
					 if ( gGameExternalOptions.fScopeModes && pSoldier && (&pSoldier->inv[pSoldier->ubAttackingHand])->exists() == true && Item[(&pSoldier->inv[pSoldier->ubAttackingHand])->usItem].usItemClass == IC_GUN)
					 {
						 // Flugente: check for scope mode
						 std::map<INT8, OBJECTTYPE*> ObjList;
						 GetScopeLists(pSoldier, &pSoldier->inv[pSoldier->ubAttackingHand], ObjList);

						 // only use scope mode if gun is in hand, otherwise an error might occur!
						 if ( (&pSoldier->inv[HANDPOS]) == &pSoldier->inv[pSoldier->ubAttackingHand] && ObjList[pSoldier->bScopeMode] != NULL && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD )
							 sScopeBonus = Item[ObjList[pSoldier->bScopeMode]->usItem].aimbonus;
					 }
					 else
						 sScopeBonus = GetBaseScopeAimBonus( &pSoldier->inv[pSoldier->ubAttackingHand], uiRange );
				 }
				 else
				 {
					 sScopeBonus = GetMinRangeForAimBonus( pSoldier, &pSoldier->inv[pSoldier->ubAttackingHand]);
				 }

				 if ( sScopeBonus >= gGameExternalOptions.sVeryHighPowerScope ) 
				 {
					 //aimLevels *= 2;
					 aimLevels += 4;
				 }
				 else if ( sScopeBonus >= gGameExternalOptions.sHighPowerScope ) 
				 {
					 //aimLevels = (UINT8)((float)(aimLevels+1) * (float)1.5);
					 aimLevels += 3;
				 }
				 else if ( sScopeBonus >= gGameExternalOptions.sMediumPowerScope ) 
				 {
					 //aimLevels = (UINT8)((float)(aimLevels+1) * (float)1.3);
					 aimLevels += 2;
				 }
				 // Smaller scopes increase by one.
				 else if ( sScopeBonus > 0 )
				 {
					 //aimLevels++;
					 aimLevels += 1;
				 }
			 }
			 // SANDRO - if using alternative weapon holding, we reduce the aim levels available
			 else
			 {
				 aimLevels = (aimLevels * (100 - gGameExternalOptions.ubAltWeaponHoldingAimLevelsReduced) + 50) / 100; // round up
			 }

			 // sevenfm: reduce when firing with two pistols
			 if (pSoldier->IsValidSecondHandShot() && aimLevels > 1)
			 {
				 aimLevels -= 1;
			 }

			 // Make sure not over maximum allowed for weapon type.
			 if (aimLevels > maxAimForType)
			 {
				 aimLevels = maxAimForType;
			 }
			 // Make sure not over maximum allowed without a bipod.
			 if (!fUsingBipod)
			 {
				 aimLevels = min(aimLevels, maxAimWithoutBipod);
			 }
			 // sevenfm: penalty when under adrenaline
			 if (MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE))
			 {
				 aimLevels -= 2;
			 }

			 // sevenfm: limit aim levels depending on distance			 
			 
			 if( sShootingDistance <= sNormalDistance )
				 aimLevels = min(4, aimLevels);
			 else
				 aimLevels = min( 4 * sShootingDistance / sNormalDistance, aimLevels );

			 // sevenfm: limit aiming levels depending on shock
			 aimLevels = min(max(1, 8 - 12 * pSoldier->ShockLevelPercent() / 100), aimLevels);

			 // sevenfm: limit if was suppressed this turn
			 if (pSoldier->InShock())
			 {
				 UINT8 ubMaxSuppressedAimLevels = 1;
				 if (gGameOptions.fNewTraitSystem && 
					 pSoldier->ubProfile != NO_PROFILE && 
					 gMercProfiles[pSoldier->ubProfile].bCharacterTrait == CHAR_TRAIT_DAUNTLESS)
				 {
					 ubMaxSuppressedAimLevels++;
				 }
				 aimLevels = min(aimLevels, ubMaxSuppressedAimLevels);
			 }
		 }
		 else // JA2 1.13 Basic aiming restrictions (8 levels for 10x scope, 6 levels for 7x scope)
		 {
			 OBJECTTYPE* pAttackingWeapon = &pSoldier->inv[pSoldier->ubAttackingHand];
			 if ( !IsScoped( pAttackingWeapon ) )
			 {
				 // No scope. 4 Allowed.
				 aimLevels = 4;
			 }
			 // SANDRO - scopes are not gonna give us any aim levels when firing from hip etc.
			 else if ( pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD )
			 {
				 //CHRRISL: yeah, this doesn't work.  GetMinRangeForAimBonus returns a range value in units while GetBaseScopeAimBonus returns a small number.
				 //	The result is that if fAimLevelsDependOnDistance is false, all scopes are going to grant +4 aim clicks which is definitely not what
				 //	we want to happen.  What we do want is simply to know whether we should send the range or use an extreme range value to guarantee that
				 //	the scope is factored.
				 //			sScopeBonus = gGameExternalOptions.fAimLevelsDependOnDistance ?
				 //				GetBaseScopeAimBonus( pAttackingWeapon, iRange )
				 //				: GetMinRangeForAimBonus( pAttackingWeapon );
				 if (gGameExternalOptions.iAimLevelsCompatibilityOption != 0)
					 sScopeBonus = OldWayOfCalculatingScopeBonus(pSoldier);
				 else
				 {
					 if ( gGameExternalOptions.fScopeModes && pSoldier && pAttackingWeapon->exists() == true && Item[pAttackingWeapon->usItem].usItemClass == IC_GUN)
					 {
						 // Flugente: check for scope mode
						 std::map<INT8, OBJECTTYPE*> ObjList;
						 GetScopeLists(pSoldier, pAttackingWeapon, ObjList);

						 // only use scope mode if gun is in hand, otherwise an error might occur!
						 if ( (&pSoldier->inv[HANDPOS]) == pAttackingWeapon  && ObjList[pSoldier->bScopeMode] != NULL && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD )
							 sScopeBonus = Item[ObjList[pSoldier->bScopeMode]->usItem].aimbonus;
					 }
					 else
						 sScopeBonus = gGameExternalOptions.fAimLevelsDependOnDistance ? GetBaseScopeAimBonus( pAttackingWeapon, uiRange ) : GetBaseScopeAimBonus( pAttackingWeapon, 25000 );
				 }

				 if ( sScopeBonus >= gGameExternalOptions.sVeryHighPowerScope )
				 {
					 aimLevels += 2;
				 }
				 if ( sScopeBonus >= gGameExternalOptions.sHighPowerScope )
				 {
					 aimLevels += 2;
				 }

				 // SANDRO - STOMP traits - Sniper bonus aim clicks
				 if ((weaponType == GUN_AS_RIFLE || weaponType == GUN_RIFLE || weaponType == GUN_SN_RIFLE) &&
					 gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( pSoldier, SNIPER_NT ) )
				 {
					 aimLevels += (gSkillTraitValues.ubSNAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, SNIPER_NT ));
				 }
			 }
			 // SANDRO - STOMP traits - Gunslinger bonus aim clicks
			 if ((weaponType == GUN_PISTOL || weaponType == GUN_M_PISTOL) &&
				 gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( pSoldier, GUNSLINGER_NT ) )
			 {
				 aimLevels += (gSkillTraitValues.ubGSAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, GUNSLINGER_NT ));
			 }

			 // silversurfer added this - STOMP traits - Ranger bonus aim clicks
			 if ( weaponType == GUN_SHOTGUN &&
				 gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( pSoldier, RANGER_NT ) )
			 {
				 aimLevels += (gSkillTraitValues.ubRAAimClicksAdded * NUM_SKILL_TRAITS( pSoldier, RANGER_NT ));
			 }

			 // SANDRO - if using alternative weapon holding, we reduce the aim levels available
			 if ( pSoldier->bScopeMode == USE_ALT_WEAPON_HOLD && gGameExternalOptions.fScopeModes )
			 {
				 aimLevels /= 2;
			 }

			 //CHRISL: The system can't currently support more then 8 aim levels so make sure we can never have more then 8
			 aimLevels = min(8, aimLevels);
		 }
	 }

	 //CHRISL: Make sure we always limit to the proper number of aim clicks
	 aimLevels = max(1, aimLevels);
	 aimLevels = min(8, aimLevels);

	 return aimLevels;
 }

UINT8 GetAllowedAimingLevelsForItem( SOLDIERTYPE *pSoldier, OBJECTTYPE *pObj, UINT8 ubStance )
{
	if ( !(Item[pObj->usItem].usItemClass & (IC_GUN|IC_LAUNCHER|IC_THROWING_KNIFE)) )
	{
		return 1;
	}

	INT8 aimLevels = 4;

	// HEADROCK HAM B2.6: Dynamic aiming level restrictions based on gun type and attachments.
	// HEADROCK HAM 3.5: Revamped this - it was illogically constructed.
	if ( gGameExternalOptions.fDynamicAimingTime )
	{
		UINT16 weaponRange;
		UINT8 weaponType;
		BOOLEAN fTwoHanded, fUsingBipod;

		aimLevels = 0;
		
		// Read weapon data
		fTwoHanded = Item[pObj->usItem].twohanded;
		weaponRange = ( Weapon[Item[pObj->usItem].ubClassIndex].usRange * GetPercentRangeBonus(pObj) ) / 10000;
		weaponRange += GetRangeBonus(pObj);
		weaponType = Weapon[Item[pObj->usItem].ubClassIndex].ubWeaponType;
		fUsingBipod = FALSE;
		if(UsingNewCTHSystem() == true)
			aimLevels = Weapon[Item[pObj->usItem].ubClassIndex].ubAimLevels;
		
		// Only use default values if we don't find a weapon specific value.
		if(aimLevels == 0)
		{
			// Define basic (no attachments), and absolute maximums
			if (weaponType == GUN_PISTOL || weaponType == GUN_M_PISTOL || fTwoHanded == 0)
			{
				aimLevels = 2;
			}
			else if (weaponType == GUN_SHOTGUN || weaponType == GUN_LMG || weaponType == GUN_SMG)
			{
				aimLevels = 3;
			}
			else if ((weaponType == GUN_AS_RIFLE || weaponType == GUN_RIFLE ) && weaponRange <= 500)
			{
				aimLevels = 4;
			}
			else if (((weaponType == GUN_AS_RIFLE || weaponType == GUN_RIFLE) && weaponRange > 500) || 
							(weaponType == GUN_SN_RIFLE && weaponRange <= 500))
			{
				aimLevels = 6;
			}
			else if (weaponType == GUN_SN_RIFLE && weaponRange > 500)
			{
				aimLevels = 8;
			}
			else
			{
				return 4;
			}
		}

		// HEADROCK HAM 4: This modifier from the weapon and its attachments replaces the generic bipod bonus.
		aimLevels += GetObjectModifier( pSoldier, pObj, ubStance, ITEMMODIFIER_AIMLEVELS );

		aimLevels += GetAimLevelsTraitModifier( pSoldier, pObj );

		aimLevels = __max(1, aimLevels);
		aimLevels = __min(8, aimLevels);

	}
	return aimLevels;
}

//Madd: added
INT16 GetStealthBonus( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	BOOLEAN fWeaponReady = FALSE;

	if ( pObj->exists() == true ) 
	{
		if( pSoldier && 
			Item[pObj->usItem].usItemClass == IC_GUN && 
			((&pSoldier->inv[HANDPOS]) == pObj || (&pSoldier->inv[SECONDHANDPOS]) == pObj && pSoldier->IsValidSecondHandShot()) &&
			(WeaponReady(pSoldier) || (gAnimControl[ pSoldier->usAnimState ].uiFlags & ANIM_ALT_WEAPON_HOLDING )) )
		{
			fWeaponReady = TRUE;
		}

		bonus = BonusReduce(Item[pObj->usItem].stealthbonus,(*pObj)[0]->data.objectStatus);

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if(iter->exists())
			{
				if( Item[iter->usItem].bestlaserrange == 0 || fWeaponReady )
				{
					bonus += (INT16) BonusReduce(Item[iter->usItem].stealthbonus,(*iter)[0]->data.objectStatus);
				}				
			}
		}
	}
	return( bonus );
}
INT16 GetWornStealth( SOLDIERTYPE * pSoldier )
{
	CHECKF(pSoldier);

	//note: Stealth bonus is capped at 100
	//note: Stealth is not a perk! Stealth bonus only applies to equipment, and stacks with camouflage
	//note: stealth bonus is not affected by terrain like the camo bonus, otherwise they're very similar
	//note: stealth bonus also affects noise made by characters walking
	INT8	bLoop;
	INT16	ttl=0;

	for (bLoop = HELMETPOS; bLoop <= LEGPOS; ++bLoop)
	{
		if ( pSoldier->inv[bLoop].exists() == true )
			ttl += GetStealthBonus(pSoldier, &pSoldier->inv[bLoop]);
	}

	// sevenfm: apply 65 stealth for stealth palette
	if (pSoldier->IsShadowHidingActive())
	{
		//ttl = max(ttl, 65 + pSoldier->stats.bExpLevel * 3);
		//ttl += 100;
		ttl = max(65, ttl + 100);
	}

	// sevenfm: stealth for elite AI soldiers at night
	if (pSoldier->bTeam == ENEMY_TEAM && 
		NightLight() && 
		pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE &&
		pSoldier->wornCamo < 65 &&
		pSoldier->wornUrbanCamo < 65 &&
		pSoldier->wornDesertCamo < 65 &&
		pSoldier->wornSnowCamo < 65)
	{
		UINT8 ubSoldierDiff = SoldierDifficultyLevel( pSoldier );	// 0 - 4
		UINT8 ubGameDiff = gGameOptions.ubDifficultyLevel;			// 1 - 4
		UINT8 ubProgress = HighestPlayerProgressPercentage();		// 0 - 100
		UINT8 ubChance = 0;
		INT16 sStealth = 0;

		// base chance for elite soldiers 5-25%
		ubChance = 5 + ubProgress / 5;

		// determine chance for stealth
		if (pSoldier->aiData.bAttitude == CUNNINGAID ||
			pSoldier->aiData.bAttitude == CUNNINGSOLO ||
			pSoldier->aiData.bAttitude == DEFENSIVE ||
			pSoldier->aiData.bOrders == STATIONARY)
		{
			ubChance += 5 + ubProgress / 5;
		}

		//if( AICheckIsSniper(pSoldier) )
		if (pSoldier->stats.bMarksmanship > 90 &&
			gGameOptions.fNewTraitSystem &&
			HAS_SKILL_TRAIT(pSoldier, SNIPER_NT))
		{
			ubChance += 5 + ubProgress / 5;
		}						

		if( pSoldier->ubID % 10 <= ubChance / 10 )
		{
			//sStealth = ubGameDiff * 10 + ubSoldierDiff * 10 + ubProgress / 5;
			sStealth = 65 + (1 + ubSoldierDiff) * 3 + ubProgress / 5;
		}

		// finally, apply stealth bonus
		ttl = max(ttl, sStealth);
	}

	ttl = max(ttl, 0);
	ttl = min(ttl, 100);

	return ttl;
}


/////////////////////////////
// HEADROCK: Several functions created for the Enhanced Description Box project, but may generally be useful some
// day. They calculate item bonuses, ammo bonuses and attachment bonuses, without requiring a SOLDIERTYPE or any
// other variable.
////////////////////////////

//WarmSteel - Function to get the total reliability from the gun and its attachments
INT16 GetReliability( OBJECTTYPE * pObj )
{
	INT16 bonus = 0;

	bonus += Item[pObj->usItem].bReliability;

	//Ammo modifications need to be added aswell
	if ( Item[ pObj->usItem ].usItemClass == IC_GUN && (*pObj)[0]->data.gun.ubGunShotsLeft > 0 )
	{
		bonus += Item[( *pObj )[0]->data.gun.usGunAmmoItem].bReliability;
	}
	
	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
	{
		if (iter->exists() && !(Item[iter->usItem].usItemClass & IC_WEAPON))
			bonus += Item[iter->usItem].bReliability;
	}

	return( bonus );
}

UINT8 GetStraightJamChance(OBJECTTYPE *pObj, SOLDIERTYPE *pSoldier)
{
	INT16 bonus = 0;

	bonus += Item[pObj->usItem].ubJamChance;

	// if item is a gun and scope mode enabled, add bonus from scope used
	if (gGameExternalOptions.fScopeModes && pSoldier && Item[pObj->usItem].usItemClass == IC_GUN)
	{
		std::map<INT8, OBJECTTYPE*> ObjList;
		GetScopeLists(pSoldier, pObj, ObjList);

		// only use scope mode if gun is in hand, otherwise an error might occur!
		if ((&pSoldier->inv[HANDPOS]) == pObj && ObjList[pSoldier->bScopeMode] != NULL && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD)
		{
			bonus += Item[ObjList[pSoldier->bScopeMode]->usItem].ubJamChance;
		}
	}

	//Ammo modifications need to be added as well
	if (Item[pObj->usItem].usItemClass == IC_GUN && (*pObj)[0]->data.gun.ubGunShotsLeft > 0)
	{
		// from magazine item
		bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].ubJamChance;
		// from ammo type
		bonus += AmmoTypes[(*pObj)[0]->data.gun.ubGunAmmoType].ubJamChance;
	}

	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
	{
		if (iter->exists() && 
			!(Item[iter->usItem].usItemClass & IC_WEAPON) &&
			(!gGameExternalOptions.fScopeModes || !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE)))
			bonus += Item[iter->usItem].ubJamChance;
	}

	bonus = min(100, bonus);
	bonus = max(0, bonus);

	return (UINT8)bonus;
}

UINT8 GetItemDifficulty(OBJECTTYPE * pObj, SOLDIERTYPE *pSoldier)
{
	INT16 bonus = 0;

	// calculate max difficulty
	bonus = max(bonus, Item[pObj->usItem].ubDifficulty);

	// if item is a gun and scope mode enabled, add bonus from scope used
	if (gGameExternalOptions.fScopeModes && pSoldier && Item[pObj->usItem].usItemClass == IC_GUN)
	{
		std::map<INT8, OBJECTTYPE*> ObjList;
		GetScopeLists(pSoldier, pObj, ObjList);

		// only use scope mode if gun is in hand, otherwise an error might occur!
		if ((&pSoldier->inv[HANDPOS]) == pObj && ObjList[pSoldier->bScopeMode] != NULL && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD)
		{
			bonus = max(bonus, Item[ObjList[pSoldier->bScopeMode]->usItem].ubDifficulty);
		}
	}

	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
	{
		if (iter->exists() &&
			!(Item[iter->usItem].usItemClass & IC_WEAPON) &&
			(!gGameExternalOptions.fScopeModes || !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE)))
			bonus = max(bonus, Item[iter->usItem].ubDifficulty);
	}

	// apply difficulty bonus
	bonus += Item[pObj->usItem].bDifficultyBonus;

	// if item is a gun and scope mode enabled, add bonus from scope used
	if (gGameExternalOptions.fScopeModes && pSoldier && Item[pObj->usItem].usItemClass == IC_GUN)
	{
		std::map<INT8, OBJECTTYPE*> ObjList;
		GetScopeLists(pSoldier, pObj, ObjList);

		// only use scope mode if gun is in hand, otherwise an error might occur!
		if ((&pSoldier->inv[HANDPOS]) == pObj && ObjList[pSoldier->bScopeMode] != NULL && pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD)
		{
			bonus += Item[ObjList[pSoldier->bScopeMode]->usItem].bDifficultyBonus;
		}
	}

	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
	{
		if (iter->exists() &&
			!(Item[iter->usItem].usItemClass & IC_WEAPON) &&
			(!gGameExternalOptions.fScopeModes || !IsAttachmentClass(iter->usItem, AC_SCOPE_MODE)))
			bonus += Item[iter->usItem].bDifficultyBonus;
	}

	bonus = min(100, bonus);
	bonus = max(0, bonus);

	return (UINT8)bonus;
}

// HEADROCK: Function to get the total aim-bonus from the gun and its attachments
INT16 GetFlatAimBonus( OBJECTTYPE * pObj )
{
	INT16 bonus = 0;

	bonus += Item[pObj->usItem].aimbonus;
	
	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
	{
		// sevenfm: disabled bonus/penalty from attached face items
		if (iter->exists() && Item[iter->usItem].usItemClass != IC_FACE)
		{
			bonus += Item[iter->usItem].aimbonus;
		}
	}

	return( bonus );
}

// HEADROCK: Function to get the final Loudness value of a gun, after ammo and attachment reductions
INT16 GetFinalLoudness( OBJECTTYPE * pObj )
{
	INT16 loudness = 0;
	INT16 loudnessModifier = 0;

	loudness = Weapon[Item[pObj->usItem].ubClassIndex].ubAttackVolume;

	
	// WANNE: Fix by Headrock
	// It seems that the game calculates noise reduction not by adding together the reduction from ammo and all attachments, 
	// but by applying them to the gun's loudness as percentages one by one. 
	// So instead of (60+20)% it does 60% and then 20%, giving completely different results.
	loudness = ( loudness * GetPercentNoiseVolume( pObj ) ) / 100;
	
	/*
	loudnessModifier += Item[pObj->usItem].percentnoisereduction;
	
	if ( (*pObj)[0]->data.gun.ubGunShotsLeft > 0 )
		loudnessModifier += Item[(*pObj)[0]->data.gun.usGunAmmoItem].percentnoisereduction ;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			loudnessModifier += BonusReduce( Item[iter->usItem].percentnoisereduction, (*iter)[0]->data.objectStatus ) ;
		}

	loudness = loudness * ( 100 - loudnessModifier ) / 100;
	*/

	loudness = __max(loudness, 1);

	return ( loudness );
}

// HEADROCK: Function to get AP bonus from an item rather than a soldier

INT16 GetAPBonus( OBJECTTYPE * pObj )
{
	INT16 bonus=0;

	bonus += Item[ pObj->usItem ].APBonus;

	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
	{
		// sevenfm: disabled bonus/penalty from attached face items
		if (iter->exists() && Item[iter->usItem].usItemClass != IC_FACE)
		{
			bonus += Item[iter->usItem].APBonus;
		}
	}

	return( bonus );
}

// HEADROCK: Alternative function to determine flat to-hit bonus of weapon
INT16 GetFlatToHitBonus( OBJECTTYPE * pObj )
{
	INT16 bonus=0;

	bonus = Item[pObj->usItem].tohitbonus;
	bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].tohitbonus;

	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
	{
		// sevenfm: disabled bonus/penalty from attached face items
		if (iter->exists() && Item[iter->usItem].usItemClass != IC_FACE)
		{
			bonus += Item[iter->usItem].tohitbonus;
		}
	}

	return( bonus );
}

// HEADROCK: Function to get average of all "best laser range" attributes from weapon and attachments
INT16 GetAverageBestLaserRange( OBJECTTYPE * pObj )
{
	INT16 bonus=0;
	INT16 numModifiers=0;

	if (Item[pObj->usItem].bestlaserrange > 0)
	{
		numModifiers++;
		bonus += Item[pObj->usItem].bestlaserrange;
	}
	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
	{
		if (iter->exists() && Item[iter->usItem].bestlaserrange > 0)
		{
			numModifiers++;
			bonus += Item[iter->usItem].bestlaserrange;
		}
	}

	if (numModifiers>0)
	{
		bonus = bonus / numModifiers;
	}

	return (INT16)(bonus * gItemSettings.fBestLaserRangeModifier);
}

// get the best laser range from the weapon and attachments
INT16 GetBestLaserRange( OBJECTTYPE * pObj )
{
	INT16 range=0;

	if (Item[pObj->usItem].bestlaserrange > 0)
	{
		range = Item[pObj->usItem].bestlaserrange;
	}
	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
	{
		if (iter->exists() && Item[iter->usItem].bestlaserrange > range)
		{
			range = Item[iter->usItem].bestlaserrange;
		}
	}

	return (INT16)(range * gItemSettings.fBestLaserRangeModifier);
}

FLOAT IronSightAperture(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel, DOUBLE d2DDistance, INT16 sAimTime, FLOAT dMagFactor, FLOAT dBasicAperture)
{
	// sevenfm: find weapon range
	OBJECTTYPE* pWeapon = pSoldier->GetUsedWeapon(&pSoldier->inv[HANDPOS]);
	UINT16 usGunRange = GunRange(pWeapon, pSoldier);
	//UINT16 usGunRange = Weapon[Item[pObjAttHand->usItem].ubClassIndex].usRange * GetPercentRangeBonus(pObjAttHand) / 10000 + GetRangeBonus(pObjAttHand);
	//UINT16 usGunRange = GunRange(&pSoldier->inv[HANDPOS], pSoldier);
	//INT16 sMaxAim = (INT16)AllowedAimingLevelsNCTH(pSoldier, sSpot);

	// sevenfm: determine if we are using laser sight
	BOOLEAN fUsingLaser = FALSE;
	if (gGameExternalOptions.fScopeModes && Item[pSoldier->inv[HANDPOS].usItem].usItemClass == IC_GUN)
	{
		std::map<INT8, OBJECTTYPE*> ObjList;
		GetScopeLists(pSoldier, &pSoldier->inv[HANDPOS], ObjList);
		if (ObjList[pSoldier->bScopeMode] != NULL && IsAttachmentClass(ObjList[pSoldier->bScopeMode]->usItem, AC_IRONSIGHT))
		{
			if (Item[ObjList[pSoldier->bScopeMode]->usItem].bestlaserrange > 0 && Item[ObjList[pSoldier->bScopeMode]->usItem].tohitbonus > 0)
			{
				fUsingLaser = TRUE;
			}
		}
	}

	// silversurfer: New functionality for iron sights - There have been many complaints that iron sights lose their usefulness
	// very fast the farther the target is away. Setting IRON_SIGHT_PERFORMANCE_BONUS too high makes them overly powerful at
	// close range. This experimental formula implements a curve that lowers iBasicAperture the farther the target is away.
	// At 1 tile distance iBasicAperture will be the same as before. That's the common start.
	// sevenfm: apply bonus only in weapon range
	if (gGameCTHConstants.IRON_SIGHTS_MAX_APERTURE_USE_GRADIENT && 
		dMagFactor <= 1.0 && 
		//!fUsingLaser &&
		//(Weapon[pWeapon->usItem].ubWeaponClass == HANDGUNCLASS || !pSoldier->IsValidAlternativeFireMode(sAimTime, sSpot)) &&
		pSoldier->bBlindedCounter == 0)
	{
		FLOAT fApertureModifier = gGameCTHConstants.IRON_SIGHTS_MAX_APERTURE_MODIFIER;
		if (pSoldier->IsValidAlternativeFireMode(sAimTime, sSpot))
			fApertureModifier += 1.0f;
		if (d2DDistance > usGunRange)
			fApertureModifier += 1.0f;
		dBasicAperture = dBasicAperture * (1 / (FLOAT)sqrt(d2DDistance / FLOAT(CELL_X_SIZE)) / fApertureModifier + (fApertureModifier - 1) / fApertureModifier);
	}

	// iron sights can get a percentage bonus to make them overall better but only when not shooting from hip
	// sevenfm: no iron sight bonus for blind soldier
	// sevenfm: apply bonus only in weapon range
	if (dMagFactor <= 1.0 &&
		//!fUsingLaser &&
		(Weapon[pWeapon->usItem].ubWeaponClass == HANDGUNCLASS || !pSoldier->IsValidAlternativeFireMode(sAimTime, sSpot)) &&
		pSoldier->bBlindedCounter == 0 &&
		d2DDistance <= usGunRange)
	{
		// iBasicAperture = iBasicAperture * (FLOAT)( (100 - gGameCTHConstants.IRON_SIGHT_PERFORMANCE_BONUS) / 100);

		// sevenfm: iron sights receive bonus only at day vision range
		//INT16 sEffRange = CELL_X_SIZE * TACTICAL_RANGE;
		INT16 sEffRange = gGameCTHConstants.NORMAL_SHOOTING_DISTANCE;
		FLOAT dBonus = gGameCTHConstants.IRON_SIGHT_PERFORMANCE_BONUS;

		//if (pSoldier->IsValidAlternativeFireMode(sAimTime, sSpot))
			//dBonus = dBonus / 2.0f;

		if (d2DDistance <= sEffRange)
			dBasicAperture = dBasicAperture * (FLOAT)((100 - dBonus) / 100);
		else
			dBasicAperture = dBasicAperture * (FLOAT)((100 - dBonus * 100 / (d2DDistance - sEffRange + 100)) / 100);
	}

	return dBasicAperture;
}

FLOAT DistanceAperture(FLOAT iBasicAperture, DOUBLE d2DDistance)
{
	FLOAT iDistanceAperture;

	// sevenfm: use sqrt
	if (gGameExternalOptions.fNCTHSqrtDistance)
	{
		FLOAT dRatio = (FLOAT)d2DDistance / gGameCTHConstants.NORMAL_SHOOTING_DISTANCE;
		if (dRatio > 1.0f)
			iDistanceAperture = iBasicAperture * sqrt(dRatio);
		else
			iDistanceAperture = iBasicAperture * dRatio;
	}
	else
	{
		iDistanceAperture = iBasicAperture * (FLOAT)d2DDistance / (FLOAT)gGameCTHConstants.NORMAL_SHOOTING_DISTANCE;
	}

	return iDistanceAperture;
}

// HEADROCK: This function determines the bipod boni of the gun or its attachments
INT16 GetBipodBonus( OBJECTTYPE * pObj )
{
	INT16 bonus=0;

	bonus = Item[pObj->usItem].bipod;

	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter){
		if (iter->exists())
		{
			bonus += Item[iter->usItem].bipod;
		}
	}

	return( bonus );
}

// HEADROCK: Added a function to calculate the vision range bonus of an object and its attachments. Takes an argument which determines what type
// of vision range bonus we want to get:
// 0 - Regular
// 1 - Day
// 2 - Night
// 3 - Bright Light
// 4 - Cave
INT16 GetItemVisionRangeBonus( OBJECTTYPE * pObj, INT16 VisionType )
{
	INT16 bonus = 0;
		
	if (VisionType == 0)
	{
		bonus += Item[ pObj->usItem ].visionrangebonus;
		
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && !(Item[iter->usItem].usItemClass & IC_FACE))
				bonus += Item[iter->usItem].visionrangebonus;
		}		
	}
	else if (VisionType == 1)
	{
		bonus += Item[ pObj->usItem ].dayvisionrangebonus;
		
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && !(Item[iter->usItem].usItemClass & IC_FACE))
				bonus += Item[iter->usItem].dayvisionrangebonus;
		}		
	}
	else if (VisionType == 2)
	{
		bonus += Item[ pObj->usItem ].nightvisionrangebonus;
		
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && !(Item[iter->usItem].usItemClass & IC_FACE))
				bonus += Item[iter->usItem].nightvisionrangebonus;
		}		
	}
	else if (VisionType == 3)
	{
		bonus += Item[ pObj->usItem ].brightlightvisionrangebonus;
		
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && !(Item[iter->usItem].usItemClass & IC_FACE))
				bonus += Item[iter->usItem].brightlightvisionrangebonus;
		}		
	}
	else if (VisionType == 4)
	{
		bonus += Item[ pObj->usItem ].cavevisionrangebonus;
		
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && !(Item[iter->usItem].usItemClass & IC_FACE))
				bonus += Item[iter->usItem].cavevisionrangebonus;
		}		
	}

	return( bonus );

}

// HEADROCK: function to get Tunnel Vision percent from an item and its attachments

UINT8 GetItemPercentTunnelVision( OBJECTTYPE * pObj )
{
	UINT8 bonus = 0;

	bonus += Item[ pObj->usItem ].percenttunnelvision;

	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
	{
		if (iter->exists() && !(Item[iter->usItem].usItemClass & IC_FACE))
			bonus += Item[iter->usItem].percenttunnelvision;
	}

	bonus = __min(bonus, 100);

	return( bonus );
}

// HEADROCK: Function to calculate hearing range bonus without SOLDIERTYPE, from an item and its attachments
INT16 GetItemHearingRangeBonus( OBJECTTYPE * pObj )
{
	INT16 bonus = 0;

	bonus += Item[ pObj->usItem ].hearingrangebonus;

	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
	{
		if (iter->exists() && !(Item[iter->usItem].usItemClass & IC_FACE))
			bonus += Item[ iter->usItem ].hearingrangebonus;
	}

	return( bonus );
}

// HEADROCK: Flash Suppression Detector function that does not use SOLDIERTYPE. 
BOOLEAN IsFlashSuppressorAlt( OBJECTTYPE * pObj )
{
	if ( AmmoTypes[(*pObj)[0]->data.gun.ubGunAmmoType].tracerEffect )
		return FALSE;

	if ( Item[pObj->usItem].hidemuzzleflash )
		return TRUE;

	if ( Item[(*pObj)[0]->data.gun.usGunAmmoItem].hidemuzzleflash )
		return TRUE;

	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
	{
		if (iter->exists() && Item[iter->usItem].hidemuzzleflash)
		{
			return( TRUE );
		}
	}
	return( FALSE );
}

// HEADROCK: Calculate stealth not based on item status
INT16 GetBasicStealthBonus( SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj )
{
	INT16 bonus = 0;
	BOOLEAN fWeaponReady = FALSE;

	if ( pObj->exists() == true ) 
	{
		if( pSoldier && 
			Item[pObj->usItem].usItemClass == IC_GUN && 
			((&pSoldier->inv[HANDPOS]) == pObj || (&pSoldier->inv[SECONDHANDPOS]) == pObj && pSoldier->IsValidSecondHandShot()) &&
			(WeaponReady(pSoldier) || (gAnimControl[ pSoldier->usAnimState ].uiFlags & ANIM_ALT_WEAPON_HOLDING )) )
		{
			fWeaponReady = TRUE;
		}

		bonus = Item[pObj->usItem].stealthbonus;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if(iter->exists())
			{
				if( Item[iter->usItem].bestlaserrange == 0 || fWeaponReady )
				{
					bonus += (INT16) Item[iter->usItem].stealthbonus;
				}				
			}
		}
	}
	return( bonus );
}

// HEADROCK HAM 4: Calculate a gun's accuracy, including bonuses from ammo and attachments.
INT32 GetGunAccuracy( OBJECTTYPE *pObj )
{
	if (!pObj->exists())
		return 0;

	BOOLEAN fFlame = FALSE;

	if (pObj->usItem == FLAMETHROWER ||
		(AmmoTypes[(*pObj)[0]->data.gun.ubGunAmmoType].ammoflag & AMMOFLAG_FLAME))
	{
		fFlame = TRUE;
	}

	// Flugente: If overheating is allowed, an overheated gun receives a slight malus to accuracy
	FLOAT accuracyheatmultiplicator = 1.0;
	// sevenfm: no accuracy penalty from overheated flamethrower
	if (gGameExternalOptions.fWeaponOverheating && !fFlame)
	{
		FLOAT overheatdamagepercentage = GetGunOverheatDamagePercentage(pObj);	// 1.0 means 100% damage percentage
		
		//FLOAT accuracymalus = (max(1.0f, overheatdamagepercentage) - 1.0f) * 0.1f;
		//accuracyheatmultiplicator = max(0.0f, 1.0f - accuracymalus);

		// sevenfm: use square law for values < 1.0
		if (overheatdamagepercentage < 1.0f)
			overheatdamagepercentage = overheatdamagepercentage * overheatdamagepercentage;

		// sevenfm: start lowering accuracy before hitting 100% level, so at 100% overheating we'll have 50% accuracy reduction
		accuracyheatmultiplicator = accuracyheatmultiplicator * 1.0f / (1.0f + overheatdamagepercentage);
	}

	INT32 iModifier = GetAccuracyModifier(pObj);

	if (UsingNewCTHSystem())
	{
		INT32 iAccuracy = (INT32)(accuracyheatmultiplicator * Weapon[Item[pObj->usItem].uiIndex].nAccuracy);
		iAccuracy = (iAccuracy * (*pObj)[0]->data.gun.bGunStatus) / 100;

		// Accuracy works in a very different way from most modifiers. At low levels, a small change is almost completely
		// irrelevant. At high levels, every point of accuracy can potentially increase the gun's effective range by
		// a large amount. Therefore, we apply this percentage in REVERSE - the higher our accuracy, the less change
		// we receive.
		// You can look at it a different way: We're actually adding/subtracting a percentage of the distance between
		// the gun's accuracy and Max Accuracy (100).
		// Examples:
		// Modifier = +20%
		// Initial Gun Accuracy = 90, Final Gun Accuracy = 90 + (20% of the gap = 20% of 100-90 = 20% of 10 = 2) = 92.
		// Initial Gun Accuracy = 10, Final Gun Accuracy = 10 + (20% of the gap = 20% of 100-10 = 20% of 90 = 18) = 28.
		iAccuracy += ((100 - iAccuracy) * iModifier) / 100;

		iAccuracy = max(0, iAccuracy);
		iAccuracy = min(100, iAccuracy);

		return iAccuracy;
	}
	else
	{
		INT8 bAccuracy = Weapon[pObj->usItem].bAccuracy;

		// sevenfm: do not scale negative accuracy
		if (bAccuracy > 0)
		{
			/*if (iModifier > 0)
				bAccuracy += ceil(bAccuracy * iModifier / 100.0f);
			else
				bAccuracy += floor(bAccuracy * iModifier / 100.0f);*/

			if (iModifier > 0)
				bAccuracy += (INT8)(bAccuracy * iModifier / 100.0f + 0.5f);
			else
				bAccuracy += (INT8)(bAccuracy * iModifier / 100.0f - 0.5f);

			bAccuracy = (INT8)(accuracyheatmultiplicator * bAccuracy);
		}

		return min(bAccuracy, 25);
	}	
}

// Get Accuracy Modifier from an item and its attachments
INT32 GetAccuracyModifier( OBJECTTYPE *pObj )
{
	INT32 bonus = 0;
	if (pObj->exists())
	{
		bonus += Item[pObj->usItem].percentaccuracymodifier;

		if ((*pObj)[0]->data.gun.ubGunShotsLeft > 0)
			bonus += Item[(*pObj)[0]->data.gun.usGunAmmoItem].percentaccuracymodifier;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
		{
			if (iter->exists())
			{
				bonus += BonusReduceMore((INT32)Item[iter->usItem].percentaccuracymodifier, (*iter)[0]->data.objectStatus);
			}
		}
	}

	return bonus;
}

// HEADROCK HAM 3.6: This is meant to squash an exploit where a backpack can be moved to your hand to avoid AP penalties.
// CHRISL: Carrying an empty backpack in a none BACKPACKPOS location shouldn't be an issue.
INT8 FindBackpackOnSoldier( SOLDIERTYPE * pSoldier )
{
	INT8	bLoop;

	for (bLoop = 0; bLoop < NUM_INV_SLOTS ; bLoop++)
	{
		if (pSoldier->inv[bLoop].exists())
		{
			if( bLoop == BPACKPOCKPOS )
				return( bLoop );
			if (Item[pSoldier->inv[bLoop].usItem].usItemClass == IC_LBEGEAR &&
				LoadBearingEquipment[Item[pSoldier->inv[bLoop].usItem].ubClassIndex].lbeClass == BACKPACK)
			{
				for (INT8 bLoop2 = 0; bLoop2 < pSoldier->inv[bLoop].ubNumberOfObjects; bLoop2++)
				{
					if(pSoldier->inv[bLoop].IsActiveLBE(bLoop2) == true)
						return( bLoop );
				}
			}
		}
	}
	return( ITEM_NOT_FOUND );
}

// HEADROCK HAM 3.6: This applies the INI modifier to explosives
UINT8 GetModifiedExplosiveDamage( UINT16 sDamage, UINT8 ubType )
{
	if (sDamage == 0)
	{
	   return(0);
	}

	// apply ini modifiers by type
	if ( ubType == 0 )	// type 0 is health damage
		sDamage = (UINT16)(sDamage * gGameExternalOptions.iExplosivesDamageModifier * gItemSettings.fDamageHealthModifierExplosive / 100.0f);
	else if ( ubType == 1 )	// type 1 is breath damage
		sDamage = (UINT16)(sDamage * gGameExternalOptions.iExplosivesDamageModifier * gItemSettings.fDamageBreathModifierExplosive / 100.0f);
	else
		return(0);	// undefined type

	sDamage = __max(1, sDamage);
	sDamage = __min(255, sDamage);

	return (UINT8)sDamage;
}

UINT8 GetModifiedMeleeDamage( UINT16 sDamage )
{
	if (sDamage == 0)
	{
	   return(0);
	}

	sDamage = (INT16)(( sDamage * gGameExternalOptions.iMeleeDamageModifier ) / 100);
	sDamage = __max(1, sDamage);
	sDamage = __min(255, sDamage);

	return (UINT8)sDamage;
}

UINT8 GetModifiedGunDamage( UINT16 sDamage )
{
	if (sDamage == 0)
	{
	   return(0);
	}

	sDamage = (INT16)(( sDamage * gGameExternalOptions.iGunDamageModifier ) / 100);
	sDamage = __max(1, sDamage);
	sDamage = __min(255, sDamage);

	return (UINT8)sDamage;
}

UINT16 GetModifiedGunRange(UINT16 usWeaponIndex)
{
	UINT16 ubRange = Weapon[usWeaponIndex].usRange;

	if (ubRange == 0)
	{
	   return(0);
	}

	// Only apply range modifier on "real" guns!
	if (Item[Weapon[usWeaponIndex].uiIndex].usItemClass == IC_GUN )
		ubRange = (INT16)((FLOAT)( ubRange * gGameExternalOptions.iGunRangeModifier ) / 100 * gItemSettings.fRangeModifierGun[ Weapon[usWeaponIndex].ubWeaponType ] );
	else if (Item[Weapon[usWeaponIndex].uiIndex].usItemClass == IC_LAUNCHER )
		ubRange = (INT16)((FLOAT)( ubRange * gGameExternalOptions.iGunRangeModifier ) / 100 * gItemSettings.fRangeModifierLauncher );

	return (UINT16)ubRange;
}



/////////////////////////////////////////////////////////////////////////////////////////////
// SANDRO - Added a procedure to reduce camo on soldier, used when applying camo kit
INT16 ReduceCamoFromSoldier( SOLDIERTYPE * pSoldier, INT16 iCamoToRemove, INT16 iCamoToSkip )
{
	INT32 i;
	UINT16 usCamoToRemovePart = 3;

	if( iCamoToSkip > 0 ) // iCamoToSkip determines what camo type should not be reduced
		usCamoToRemovePart = 2;

	if ( (pSoldier->bCamo == 0) && (iCamoToSkip != 1) )
		usCamoToRemovePart -= 1;
	if ( (pSoldier->urbanCamo == 0) && (iCamoToSkip != 2) )
		usCamoToRemovePart -= 1;
	if ( (pSoldier->desertCamo == 0) && (iCamoToSkip != 3) )
		usCamoToRemovePart -= 1;
	if ( (pSoldier->snowCamo == 0) && (iCamoToSkip != 4) )
		usCamoToRemovePart -= 1;

	for (i = 0; i < 4; i++ ) // 4 times should be enough, a little paranoya here
	{
		// first, try to reduce jungle camo
		if ( ((iCamoToRemove / (1 + usCamoToRemovePart)) <= pSoldier->bCamo) && (pSoldier->bCamo > 0) && (iCamoToSkip != 1) )
		{
			// jungle camo enough to reduce
			pSoldier->bCamo -= max(1,(iCamoToRemove / (1 + usCamoToRemovePart)));
			iCamoToRemove -= max(1,(iCamoToRemove / (1 + usCamoToRemovePart)));
			usCamoToRemovePart = max( 0,(usCamoToRemovePart - 1));
			if( iCamoToRemove <= 0 )
				break;
		}
		else if ((pSoldier->bCamo > 0) && (iCamoToSkip != 1))
		{
			// jungle camo not enough to reduce by intended value, reduce only by what we have
			iCamoToRemove -= pSoldier->bCamo;
			pSoldier->bCamo = 0;
			usCamoToRemovePart = max( 0,(usCamoToRemovePart - 1));
			if( iCamoToRemove <= 0 )
				break;
		}
		// second, try to reduce urban camo
		if ( ((iCamoToRemove / (1 + usCamoToRemovePart)) <= pSoldier->urbanCamo) && (pSoldier->urbanCamo > 0) && (iCamoToSkip != 2) )
		{
			// urban camo enough to reduce
			pSoldier->urbanCamo -= max(1,(iCamoToRemove / (1 + usCamoToRemovePart)));
			iCamoToRemove -= max(1,(iCamoToRemove / (1 + usCamoToRemovePart)));
			usCamoToRemovePart = max( 0,(usCamoToRemovePart - 1));
			if( iCamoToRemove <= 0 )
				break;
		}
		else if ((pSoldier->urbanCamo > 0) && (iCamoToSkip != 2))
		{
			// urban camo not enough to reduce by intended value, reduce only by what we have
			iCamoToRemove -= pSoldier->urbanCamo;
			pSoldier->urbanCamo = 0;
			usCamoToRemovePart = max( 0,(usCamoToRemovePart - 1));
			if( iCamoToRemove <= 0 )
				break;
		}
		// third, try to reduce desert camo
		if ( ((iCamoToRemove / (1 + usCamoToRemovePart)) <= pSoldier->desertCamo) && (pSoldier->desertCamo > 0) && (iCamoToSkip != 3) )
		{
			// desert camo enough to reduce
			pSoldier->desertCamo -= max(1,(iCamoToRemove / (1 + usCamoToRemovePart)));
			iCamoToRemove -= max(1,(iCamoToRemove / (1 + usCamoToRemovePart)));
			usCamoToRemovePart = max( 0,(usCamoToRemovePart - 1));
			if( iCamoToRemove <= 0 )
				break;
		}
		else if ((pSoldier->desertCamo > 0) && (iCamoToSkip != 3))
		{
			// desert camo not enough to reduce by intended value, reduce only by what we have
			iCamoToRemove -= pSoldier->desertCamo;
			pSoldier->desertCamo = 0;
			usCamoToRemovePart = max( 0,(usCamoToRemovePart - 1));
			if( iCamoToRemove <= 0 )
				break;
		}
		// fourth, try to reduce snow camo
		if ( ((iCamoToRemove / (1 + usCamoToRemovePart)) <= pSoldier->snowCamo) && (pSoldier->snowCamo > 0) && (iCamoToSkip != 4) )
		{
			// snow camo enough to reduce
			pSoldier->snowCamo -= max(1,(iCamoToRemove / (1 + usCamoToRemovePart)));
			iCamoToRemove -= max(1,(iCamoToRemove / (1 + usCamoToRemovePart)));
			usCamoToRemovePart = max( 0,(usCamoToRemovePart - 1));
			if( iCamoToRemove <= 0 )
				break;
		}
		else if ((pSoldier->snowCamo > 0) && (iCamoToSkip != 4))
		{
			// snow camo not enough to reduce by intended value, reduce only by what we have
			iCamoToRemove -= pSoldier->snowCamo;
			pSoldier->snowCamo = 0;
			if( iCamoToRemove <= 0 )
				break;
		}
	}

	// return remaining value or zero
	return( max( 0, iCamoToRemove));
}

// SANDRO - added function to determine if we have Extended Ear on
BOOLEAN HasExtendedEarOn(SOLDIERTYPE * pSoldier, BOOLEAN fBatteries)
{
	INT8 bSlot = FindHearingAid(pSoldier, fBatteries);

	if( bSlot != NO_SLOT &&	(pSoldier->inv[bSlot])[0]->data.objectStatus >= USABLE)
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN HasWalkmanOn(SOLDIERTYPE * pSoldier, BOOLEAN fBatteries)
{
	INT8 bSlot = FindWalkman(pSoldier, fBatteries);

	if (bSlot != NO_SLOT && (pSoldier->inv[bSlot])[0]->data.objectStatus >= USABLE)
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN UseTotalMedicalKitPoints( SOLDIERTYPE * pSoldier, UINT16 usPointsToConsume )
{
	OBJECTTYPE * pObj;
	UINT8 ubPocket;
	INT8 bLoop;

	// add up kit points of all medkits
	// CHRISL: Changed to dynamically determine max inventory locations.
	for (ubPocket = HANDPOS; ubPocket < NUM_INV_SLOTS; ++ubPocket)
	{
		if ( IsMedicalKitItem( &( pSoldier->inv[ ubPocket ] ) ) )
		{
			pObj = &(pSoldier->inv[ ubPocket ]);
			// start consuming from the last kit in, so we end up with fewer fuller kits rather than
			// lots of half-empty ones.
			for (bLoop = pObj->ubNumberOfObjects - 1; bLoop >= 0; bLoop--)
			{
				if( (usPointsToConsume * (max( 0, (100 - Item[pObj->usItem].percentstatusdrainreduction)))/100) < (*pObj)[bLoop]->data.objectStatus )
				{
					(*pObj)[bLoop]->data.objectStatus -= (INT8)(usPointsToConsume * (max( 0, (100 - Item[pObj->usItem].percentstatusdrainreduction) ) )/100);
					usPointsToConsume = 0;
					break;
				}
				else
				{
					// consume this kit totally
					usPointsToConsume -= (((*pObj)[bLoop]->data.objectStatus) / (max( 0, (100 - Item[pObj->usItem].percentstatusdrainreduction))) /100);
					(*pObj)[bLoop]->data.objectStatus = 0;

					pObj->ubNumberOfObjects--;
				}
			}
			// check if pocket/hand emptied..update inventory, then update panel
			if( pObj->exists() == false )
			{
				// Delete object
				DeleteObj( pObj );

				// dirty interface panel
				DirtyMercPanelInterface(  pSoldier, DIRTYLEVEL2 );
			}

			if( usPointsToConsume <= 0 )
				break;
		}
	}

	if (usPointsToConsume > 0)
		return( FALSE );
	else
		return( TRUE );
}

static UINT16 OldWayOfCalculatingScopeBonus(SOLDIERTYPE *pSoldier)
{
	// Yes, this may look stupid, maybe it IS stupid, but this is purely an option
	// to use code that was checked in before.
	// Please, do not trash it again.
	return (UINT16)(GetMinRangeForAimBonus(pSoldier, &pSoldier->inv[pSoldier->ubAttackingHand]) * gGameExternalOptions.iAimLevelsCompatibilityOption / gGameExternalOptions.ubStraightSightRange);
}

// Flugente
FLOAT GetItemCooldownModificator( OBJECTTYPE * pObj )
{
	FLOAT modificator = Item[pObj->usItem].overheatCooldownModificator;

	if ( pObj->exists() == true ) 
	{
		attachmentList::iterator iterend = (*pObj)[0]->attachments.end();
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != iterend; ++iter) 
		{
			if (iter->exists())
			{
				modificator += Item[iter->usItem].overheatCooldownModificator;
			}
		}
	}

	return modificator;
}

FLOAT GetItemCooldownFactor( OBJECTTYPE * pObj )
{
	FLOAT cooldownfactor = Item[pObj->usItem].usOverheatingCooldownFactor;	// ... get item-specific cooldown factor ...
	if ( Item[pObj->usItem].usItemClass & IC_GUN )
		cooldownfactor *= gItemSettings.fOverheatCooldownModifierGun[ Weapon[ pObj->usItem ].ubWeaponType ];
	else if ( Item[pObj->usItem].usItemClass & IC_LAUNCHER )
		cooldownfactor *= gItemSettings.fOverheatCooldownModifierLauncher;

	FLOAT modificator = 1.0f + GetItemCooldownModificator( pObj );

	cooldownfactor *= modificator;

	return cooldownfactor;
}

// sevenfm: check if item can use scope mode
BOOLEAN CanUseScopeMode(UINT16 usItem)
{
	// only allow scope mode for guns
	if (Item[usItem].usItemClass & IC_GUN)
	//if ((Item[usItem].usItemClass & IC_GUN || Item[usItem].usItemClass & IC_LAUNCHER && Item[usItem].grenadelauncher) && !Item[usItem].rocketlauncher)
	{
		return TRUE;
	}

	return FALSE;
}

void  GetScopeLists(SOLDIERTYPE * pSoldier, OBJECTTYPE * pObj, std::map<INT8, OBJECTTYPE*>& arScopeMap)
{
	if(!pObj->exists())
	{
		return;
	}

	// sevenfm: only check scope lists for guns
	if (!CanUseScopeMode(pObj->usItem))
	{
		return;
	}

	// build a list of all available scopes and sights ( we always have at least one: our weapon's iron sights)
	arScopeMap[USE_BEST_SCOPE] = pObj;
	
	for (INT8 i = USE_SCOPE_2; i < NUM_SCOPE_MODES; ++i)
	{
		arScopeMap[i] = NULL;
	}

	// dual wielding only allows iron sights or similar attachments
	BOOLEAN bDualWielding = FALSE;

	if ((Item[pSoldier->inv[HANDPOS].usItem].usItemClass & IC_GUN && !Item[pSoldier->inv[HANDPOS].usItem].twohanded) &&
		(Item[pSoldier->inv[SECONDHANDPOS].usItem].usItemClass & IC_GUN && !Item[pSoldier->inv[SECONDHANDPOS].usItem].twohanded))
		bDualWielding = TRUE;

	// certain attachments prohibit the use of an iron sight once they are installed (flip-up built-in sights)
	BOOLEAN fBlockIronSights = Item[pObj->usItem].blockironsight;
		
	attachmentList::iterator iterend = (*pObj)[0]->attachments.end();

	// sevenfm: find separable iron sights
	BOOLEAN fSeparableBlockIronSightsFound = FALSE;
	BOOLEAN fNonScopeAttachmentFound = FALSE;

	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != iterend; ++iter)
	{
		if (iter->exists())
		{
			if (Item[iter->usItem].blockironsight)
				fBlockIronSights = TRUE;

			if (IsAttachmentClass(iter->usItem, AC_SCOPE_MODE) && Item[iter->usItem].blockironsight && !Item[iter->usItem].inseparable)
				//(!bDualWielding || !IsAttachmentClass(iter->usItem, AC_SCOPE)))
				fSeparableBlockIronSightsFound = TRUE;

			if (!IsAttachmentClass(iter->usItem, AC_SCOPE))
				fNonScopeAttachmentFound = TRUE;
		}
	}

	for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != iterend; ++iter) 
	{
		if ( iter->exists() )
		{
			if ( Item[iter->usItem].blockironsight == TRUE )
				fBlockIronSights = TRUE;

			if (IsAttachmentClass(iter->usItem, AC_SCOPE_MODE))
			{
				// block inseparable iron sights if found separable scope with <BlockIronSight> tag
				if (fSeparableBlockIronSightsFound &&
					IsAttachmentClass(iter->usItem, AC_IRONSIGHT) &&
					Item[iter->usItem].inseparable &&
					Item[iter->usItem].bestlaserrange == 0 &&
					ValidAttachment(iter->usItem, pObj->usItem))
				{
					continue;
				}

				// dual wielding only allows iron sights or reflex sights
				if (bDualWielding &&
					fNonScopeAttachmentFound &&
					IsAttachmentClass(iter->usItem, AC_SCOPE))
				{
					continue;
				}

				FLOAT magfactor = Item[iter->usItem].scopemagfactor;

				// fix: if there is no scopemagfactor (or suspiciously small one), assume it to be 1.0f
				if ( magfactor < 0.1f )
					magfactor = 1.0f;

				BOOLEAN isplaced = false;

				for (INT8 i = USE_BEST_SCOPE; i < NUM_SCOPE_MODES; ++i)
				{
					if ( arScopeMap[i] && magfactor > Item[arScopeMap[i]->usItem].scopemagfactor )
					{
						// fit scope into existing list: move other entries down
						for (INT8 j = NUM_SCOPE_MODES; j > i; --j)
						{
							arScopeMap[j] = arScopeMap[j-1];
						}

						arScopeMap[i]   = &(*iter);
						isplaced		= true;

						break;
					}
				}

				// we have a scope that wasn't placed anywhere, as its magfactor is worse than everything else we have. Still, we put this thing to the end of our list
				if ( !isplaced )
				{
					// determine which scope this would be and insert it
					for (INT8 i = USE_BEST_SCOPE; i < NUM_SCOPE_MODES; ++i)
					{
						if ( !arScopeMap[i] )
						{
							arScopeMap[i] = &(*iter);

							break;
						}
					}
				}
			}
		}
	}

	// attachments might block the iron sights, we might not be able to use them...
	if ( fBlockIronSights )
	{
		// if we have at least one other scope to use
		if ( arScopeMap[USE_BEST_SCOPE] != NULL && arScopeMap[USE_SCOPE_2] != NULL )
		{
			// determine position of iron sight, delete it, move up all following scopes
			for (INT8 i = USE_BEST_SCOPE; i < NUM_SCOPE_MODES; ++i)
			{
				if ( arScopeMap[i] == pObj )
				{
					arScopeMap.erase(i);

					// move other entries down
					for (INT8 j = NUM_SCOPE_MODES; j > i; --j)
					{
						arScopeMap[j] = arScopeMap[j-1];
					}

					break;
				}
			}
		}
	}

	// sevenfm: if after building scope lists, scope mode points to incorrect scope, try changing it
	/*if (arScopeMap[pSoldier->bScopeMode] == NULL)
	{
		//ChangeScopeMode(pSoldier, NOWHERE);
		pSoldier->bScopeMode = USE_BEST_SCOPE;
	}*/
}

BOOLEAN IsAttachmentClass( UINT16 usItem, UINT32 aFlag )
{
	return( (Item[usItem].attachmentclass & aFlag) != 0 );
}

BOOLEAN HasAttachmentOfClass( OBJECTTYPE * pObj, UINT32 aFlag )
{
	if ( pObj->exists() )
	{
		// check all attachments
		attachmentList::iterator iterend = (*pObj)[0]->attachments.end();
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != iterend; ++iter) 
		{
			if ( iter->exists() && IsAttachmentClass( iter->usItem, aFlag ) )
				return( TRUE );
		}
	}

	return( FALSE );
}

BOOLEAN HasAttachmentItemClass(OBJECTTYPE * pObj, UINT32 aFlag)
{
	if (pObj->exists())
	{
		// check all attachments
		attachmentList::iterator iterend = (*pObj)[0]->attachments.end();
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != iterend; ++iter)
		{
			if (iter->exists() && (Item[iter->usItem].usItemClass & aFlag))
				return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN HasAttachmentUnderArmor(OBJECTTYPE * pObj)
{
	if (pObj->exists())
	{
		// check all attachments
		attachmentList::iterator iterend = (*pObj)[0]->attachments.end();
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != iterend; ++iter)
		{
			if (iter->exists() && 
				(Item[iter->usItem].usItemClass & IC_MISC) &&
				(Item[iter->usItem].camobonus >0 || Item[iter->usItem].urbanCamobonus > 0 || Item[iter->usItem].desertCamobonus > 0 || Item[iter->usItem].snowCamobonus > 0))
				return TRUE;
		}
	}

	return FALSE;
}

//DBrot: calculate the volume already taken up by other pouches attached to this carrier
UINT8 GetVolumeAlreadyTaken(OBJECTTYPE * pObj, INT16 exceptSlot){
	UINT8 sum=0;
	if ( pObj->exists() )
	{
		std::vector<UINT16>	usAttachmentSlotIndexVector = GetItemSlots(pObj);
		OBJECTTYPE* pAttachment; 
		UINT16 slotCount;
		for (slotCount = 0; slotCount < usAttachmentSlotIndexVector.size(); slotCount++ ){
			if( slotCount == exceptSlot)
				continue;
			
			pAttachment = (*pObj)[0]->GetAttachmentAtIndex(slotCount);
			if(pAttachment->exists() && Item[pAttachment->usItem].usItemClass == IC_LBEGEAR){
				sum += LBEPocketType[GetFirstPocketOnItem(pAttachment->usItem)].pVolume;
			}
		}
	}
	return sum;
}
//DBrot: search the attachments for a pocket
UINT8 GetPocketFromAttachment(OBJECTTYPE * pObj, UINT8 pMap, int subObject)
{
	std::vector<UINT16>	usAttachmentSlotIndexVector = GetItemSlots(pObj, subObject);
	OBJECTTYPE* pAttachment;
	for (UINT16 slotCount = 0; slotCount < usAttachmentSlotIndexVector.size(); ++slotCount)
	{
		if (AttachmentSlots[usAttachmentSlotIndexVector[slotCount]].ubPocketMapping - 1 == pMap)
		{
			pAttachment = (*pObj)[subObject]->GetAttachmentAtIndex(slotCount);
			if (pAttachment->exists() && Item[pAttachment->usItem].usItemClass == IC_LBEGEAR)
			{
				return(GetFirstPocketOnItem(pAttachment->usItem));
			}
		}
	}

	return 0;
}
UINT8 GetFirstPocketOnItem(UINT16 usIndex){
	UINT8 pPocket = 0;
	for(UINT8 i = 0; i < LoadBearingEquipment[Item[usIndex].ubClassIndex].lbePocketIndex.size(); i++){
		pPocket = LoadBearingEquipment[Item[usIndex].ubClassIndex].lbePocketIndex[i];
		if(pPocket){
			return pPocket;
		}
	}
	return pPocket;
}

extern void HandleSight(SOLDIERTYPE *pSoldier, UINT8 ubSightFlags);
///////////////////////////////////////////////////////////////////////////////////////////////////
// HEADROCK HAM 5: Item Transformation.
// Item Transformation is a new way to interact with items. Where Merges combine two items into one, a Transformation
// can turn one item into another item, or into more than one item.
// To instigate a Transformation, the player uses a menu to choose the desired effect. Then the program comes here to
// see how it's done. 
// The instructions for performing a specific transformation are drawn from XML and fed into this function as a pointer. 
// They tell us what items will result from this transformation, as well as the APBP cost.

BOOLEAN OBJECTTYPE::TransformObject( SOLDIERTYPE * pSoldier, UINT8 ubStatusIndex, TransformInfoStruct * Transform, OBJECTTYPE *pParent )
{
	// The argument "Transform" is a pointer to an entry in the Transformation Data array. By looking at the pointer we
	// can determine all the data we need. Therefore, this pointer must not be null!
	AssertMsg( Transform != NULL, String( "OBJECTTYPE::TransformObject attempt with invalid Transformation data." ) );

	// Booleans to track what happened during the transformation.
	BOOLEAN fSplit = FALSE;
	BOOLEAN fDropped = FALSE;
	BOOLEAN fRemoved = FALSE;
	BOOLEAN fItemInPool = FALSE;

	// This boolean tracks whether we've managed to place an item in a soldier's inventory.
	BOOLEAN fFoundPlaceInInventory = FALSE;

	// Constants for storing details about the original item.
	OBJECTTYPE TempItem;
	UINT16 usOrigItem = this->usItem;
	UINT32 uiOrigClass = Item[this->usItem].usItemClass;
	UINT8 ubOrigNumObjects = this->ubNumberOfObjects;
	INT16 ubOrigStatus = (*this)[ubStatusIndex]->data.objectStatus; //Madd: moved this up here, since it was getting corrupted when applied to multiple results -- all statuses were coming back as 0
	
	UINT16 usAPCost = 0;
	INT32 iBPCost = 0;

	// An array to store all result items.
	UINT16 usResult[MAX_NUM_TRANSFORMATION_RESULTS];
	UINT32 iNumResults = 0;

	// Start reading transformation data with APBP costs.
	usAPCost = Transform->usAPCost;
	iBPCost = Transform->iBPCost;
	
	// sevenfm: if active soldier is vehicle, don't apply BP cost
	if( pSoldier->flags.uiStatusFlags & SOLDIER_VEHICLE )
	{
		iBPCost = 0;
	}

	// Check whether our soldier can afford this transformation!
	if (!EnoughPoints( pSoldier, (INT16)usAPCost, iBPCost, true ))
	{
		return false;
	}
	else
	{
		// Soldier can afford the transformation. Deduct APBP as necessary.
		DeductPoints(pSoldier, (INT16)usAPCost, iBPCost, DISABLED_INTERRUPT);
	}

	// Read all result items for this transformation. Record them into an array.
	for (UINT32 curResult = 0; curResult < MAX_NUM_TRANSFORMATION_RESULTS; curResult++)
	{
		if (Transform->usResult[curResult] > 0)
		{
			usResult[curResult] = Transform->usResult[curResult];

			// Flugente: resolution of random items
			UINT16 rdresult = 0;
			if ( GetItemFromRandomItem(usResult[curResult], &rdresult) )
				usResult[curResult] = rdresult;

			// Count the number of valid result items
			iNumResults++;
		}
	}

	// If there are no result items, the Transformation is illegal - we need at least one resulting item!
	AssertMsg( iNumResults, String( "OBJECTTYPE::TransformObject attempting to transform an object which has no resulting items defined!" ) );

	if (iNumResults > 1)
	{
		fSplit = TRUE;
	}

	if (HasItemFlag(usOrigItem, FULL_SANDBAG))
	{
		if (!PlayMiscSound("FillSandbag", MIDVOLUME, pSoldier->sGridNo))
			PlayJA2Sample(ATTACH_TO_GUN, RATE_11025, SoundVolume(MIDVOLUME, pSoldier->sGridNo), 1, SoundDir(pSoldier->sGridNo));
	}
	else
	{
		// Play a gun-cocking sound, it's the best one we've got ATM.
		PlayJA2Sample(ATTACH_TO_GUN, RATE_11025, SoundVolume(MIDVOLUME, pSoldier->sGridNo), 1, SoundDir(pSoldier->sGridNo));
	}

	// Before we continue, lets check whether our object is in the sector inventory.
	// Is the sector inventory open?
	if (fShowMapInventoryPool)
	{
		// Is our object currently in the pool?
		for (UINT32 x = 0; x < pInventoryPoolList.size(); x++)
		{
			if (pInventoryPoolList[x].object.exists())
			{
				if (&(pInventoryPoolList[x].object) == this)
				{
					// Aha! In that case, all transformations will be done directly at the sector pool,
					// with multiple results ending on the ground rather than in the inventory.
					fItemInPool = TRUE;
					break;
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// The Transformation itself
	////////////////////////////////////////////////////////////////////////////////////////////////////

	// We're going to do the transformation in steps, going hierarchially down from the item's own attachments,
	// to itsself, then its parents, and finally resulting items and unrelated inventory. 
	// Note that we'll need to change the item's usItem property several times during this process, so lets
	// record its original usItem now.
	UINT16 usOldItem = this->usItem;	

	// STEP 1: Attachments and subsidiary objects.
	// First of all, if the original item is a gun, we'll need to see if it can hold the same magazine after the
	// transformation. If it can't, the magazine will be ejected immediately into the soldier's inventory.	
	// Is this a gun, AND does it have any ammo loaded?
	if (Item[this->usItem].usItemClass == IC_GUN && (*this)[ubStatusIndex]->data.gun.usGunAmmoItem != NONE && (*this)[ubStatusIndex]->data.gun.ubGunShotsLeft > 0)
	{
		// Is the resulting item not a gun, OR is the resulting item of a different caliber, OR is the resulting item's magazine too small?
		if (Item[usResult[0]].usItemClass != IC_GUN ||
			Weapon[Item[usResult[0]].ubClassIndex].ubCalibre != Weapon[Item[this->usItem].ubClassIndex].ubCalibre)
			//(*this)[ubStatusIndex]->data.gun.ubGunShotsLeft > Weapon[Item[usResult[0]].ubClassIndex].ubMagSize)
		{
			// item types/calibers/magazines don't match, spit out old ammo
			EjectAmmoAndPlace(pSoldier, this);
		}
	}

	// STEP 2: Check the item's attachments to see whether they still fit.
	// We'll have to fool this function into thinking we've already changed the item.
	// Note that this function automatically drops invalid attachments to the ground.
	attachmentList unattachableList;
	this->usItem = usResult[0];
	// Repeat for each object in the stack.
	for ( UINT8 x = 0; x < this->ubNumberOfObjects; x++ )
	{
		unattachableList = ReInitMergedItem(pSoldier, this, usOldItem, x);
	}
	this->usItem = usOldItem;	

	//RemoveProhibitedAttachments(pSoldier, this, usOldItem);

	///////////////////////////////////////////////////////////////
	// TRANSFORM
	this->usItem = usResult[0];
	// Record the new itemclass
	//UINT32 uiNewClass = Item[this->usItem].usItemClass;

	// sevenfm: remove ammo if new capacity is not enough
	if (Item[this->usItem].usItemClass == IC_GUN && (*this)[ubStatusIndex]->data.gun.usGunAmmoItem != NONE && (*this)[ubStatusIndex]->data.gun.ubGunShotsLeft > 0)
	{
		if ((*this)[ubStatusIndex]->data.gun.ubGunShotsLeft > GetMagSize(this, ubStatusIndex))
		{
			// item types/calibers/magazines don't match, spit out old ammo
			EjectAmmoAndPlace(pSoldier, this);
		}
	}

	// Flugente: if the new item is a food item (and the old one wasn't), we define it to be fresh, so adjust its temperature
	if ( Item[usOldItem].foodtype == 0 &&  Item[this->usItem].foodtype > 0 )
		(*this)[0]->data.bTemperature = OVERHEATING_MAX_TEMPERATURE;

	// Make a clone of this object, so that we can point the DescBox to it later if we run into any problems.
	gCloneItemDescObject = *(this);
	///////////////////////////////////////////////////////////////

	////////////////////
	// Placement checks

	// Is the item an attachment? If so, check its parent to see if it's still compatible.
	if (pParent != NULL)
	{
		// When transforming an attachment, "THIS" is actually a copy of the object, called Object2. So transforming
		// "this" would only yield, at most, a superficial change (Description Box will change). The attachment item
		// itself will not change at all.

		// Lets change the attachment item directly, then.
		gpItemDescOrigAttachmentObject->usItem = usResult[0];
		// Make a copy of it, so we know what data it contained.
		OBJECTTYPE pClone = *gpItemDescOrigAttachmentObject;		

		// Test the parent now. See whether all attachments are still valid on it.
		ReInitMergedItem(pSoldier, pParent, pParent->usItem, ubStatusIndex);

		// sevenfm: r7855 fix
		// Flugente: sometimes gpItemDescObject gets corrupted during ReInitMergedItem(...), so we have to repair that here
		gpItemDescObject = &gCloneItemDescObject;

		gpItemDescOrigAttachmentObject = NULL;

		// After reiniting the attachments on the parent, "this" still exists, but the actual object is gone.
		// Lets look through the parent's attachments list, and see if we can find it again.
		UINT8 ubNewAttachmentsIndex = 0;
		for(attachmentList::iterator iter = (*pParent)[ubStatusIndex]->attachments.begin(); iter != (*pParent)[ubStatusIndex]->attachments.end(); iter++, ubNewAttachmentsIndex++) 
		{
			if(iter->exists())
			{
				// Compare it to the clone we made earlier.
				if (*(iter) == pClone)
				{
					// Yes, here it is!
					gpItemDescOrigAttachmentObject = (*pParent)[ubStatusIndex]->GetAttachmentAtIndex( ubNewAttachmentsIndex );
					break;
				}
			}
		}
				
		// Have we failed to find our item on the reinited parent?
		if (gpItemDescOrigAttachmentObject == NULL)
		{
			// It has been removed. Lets reset all the Description Box extra variables.
			// Note that by doing this, we actually trigger closing the description box later down the line.
			pParent = NULL;
			gfItemDescObjectIsAttachment = NULL;
			gpItemDescPrevObject = NULL;
			fRemoved = TRUE;
		}
	}

	// Our object is not an attachment. If it is on a soldier, it may still not fit in the pocket where 
	// it is currently placed, so lets see if we have to move it.
	else if (!fItemInPool)
	{
		for (INT8 bPocket = HELMETPOS; bPocket < NUM_INV_SLOTS; bPocket++)
		{
			if (&(pSoldier->inv[bPocket]) == this)
			{
				// Found our item. Does it fit in this slot?
				if (!CanItemFitInPosition(pSoldier, this, bPocket, FALSE) )
				{
					if (!AutoPlaceObject( pSoldier, this, TRUE ))
					{
						fDropped = TRUE;

						AutoPlaceObjectToWorld( pSoldier, this, TRUE );

						//Unfortunately the above function will not erase the item in tactical mode, so lets
						//double-check.
						if (pSoldier->inv[bPocket].exists() )
						{
							DeleteObj( &(pSoldier->inv[bPocket]) );
						}
					}
					// Whatever we've done with it, THIS is no longer a valid item, so lets
					// fool the desc box by switching it with the clone.
					gpItemDescObject = &gCloneItemDescObject;
				}
				break;
			}
		}
	}

	// If either the new or old items were LBEs, lets check the entire LBE inventory for item-size compatibility!
	//if (uiOrigClass & IC_LBEGEAR || uiNewClass & IC_LBEGEAR )
	if (!fItemInPool)
	{
		for (INT8 bPocket = HELMETPOS; bPocket < NUM_INV_SLOTS; bPocket++)
		{
			if (pSoldier->inv[bPocket].exists())
			{
				// Found an item. Does it still fit inside its own slot?
				if (!CanItemFitInPosition(pSoldier, &(pSoldier->inv[bPocket]), bPocket, FALSE) )
				{
					if (!AutoPlaceObject( pSoldier, &(pSoldier->inv[bPocket]), TRUE ))
					{
						fDropped = TRUE;

						AutoPlaceObjectToWorld( pSoldier, &(pSoldier->inv[bPocket]), TRUE );

						//Unfortunately the above function will not erase the item in tactical mode, so lets
						//double-check.
						if (pSoldier->inv[bPocket].exists() )
						{
							DeleteObj( &(pSoldier->inv[bPocket]) );
						}
					}
				}
			}
		}
	}

	//////////////////////////////
	// Multiple Results
	//
	// If the item has several transformation results defined, that means we're going to split it into two or
	// more items. 
	
	// We actually start with any results above the first. This occurs if the item is split into two or more other
	// items. We'll try to place them in the pSoldier's inventory if possible, otherwise they are dumped to the
	// sector pool.
	
	// Iterate through the results array we've constructed earlier.
	for (UINT32 x = 1; x < iNumResults; x++)
	{
		for (UINT32 y = 0; y < ubOrigNumObjects; y++)
		{
			// Create the result item. Set its condition to match that of the original.
			CreateItem( usResult[x], ubOrigStatus, &gTempObject );

			//Madd:  sometimes we have leftover attachments that couldn't fit on first result, so we try reattaching them here
			// I am the prince of copy pasta ;)
			for (attachmentList::iterator iter = unattachableList.begin(); iter != unattachableList.end();) 
			{
				if (iter->exists() && ValidItemAttachmentSlot(&gTempObject, iter->usItem, TRUE, FALSE, ubStatusIndex))
				{
					//This seems to be rather valid. Can't be 100% sure though.
					OBJECTTYPE tempAttachment; // Madd:  we must recreate the attachments because they may themselves have default inseparable attachments...
					CreateItem(iter->usItem, (*iter)[0]->data.objectStatus, &tempAttachment);
					if(gTempObject.AttachObject(NULL, &tempAttachment, FALSE, ubStatusIndex))
					{
						//remove this object from the list, so we don't try to attach it to the next result
						iter = unattachableList.erase(iter);
					} 
					else 
					{
						++iter;
					}
				} 
				else 
				{
					++iter;
				}
			}

			if (!fItemInPool)
			{
				// Try placing it in the soldier's invnetory.
				if (!AutoPlaceObject( pSoldier, &gTempObject, TRUE ))
				{
					// Failed to find a place in the inventory. Dump to sector pool.
					AutoPlaceObjectToWorld( pSoldier, &gTempObject, TRUE );

					fDropped = TRUE;
				}
			}
			else
			{
				AutoPlaceObjectToWorld( pSoldier, &gTempObject, TRUE );
			}

			// Cleanup after the autoplace has to occur in tactical mode.
			if (gTempObject.exists())
			{
				DeleteObj( &(gTempObject) );
			}
		}
	}

	// Check the soldier to see how his stats have changed as a result of altering his gear.
	ApplyEquipmentBonuses(pSoldier);

	if (fItemInPool && fSplit)
	{
		CHAR16 pStr[500];
		// Item was split apart. Since it was in the sector inventory, it's common sense that all results 
		// are in the sector inventory as well, so no need to report anything extra.
		swprintf(pStr, gzTransformationMessage[MSG_TRANSFORMATION_SPLIT_PARTS], Item[usOrigItem].szItemName, pSoldier->GetName());
		ScreenMsg( FONT_ORANGE, MSG_INTERFACE, pStr );
	}
	else if (fSplit || fDropped)
	{
		CHAR16 pStr[500];
		if (fSplit && !fDropped)
		{
			// Item was split apart, but all subitems remained in the inventory.
			swprintf(pStr, gzTransformationMessage[MSG_TRANSFORMATION_SPLIT_PARTS_INV], Item[usOrigItem].szItemName, pSoldier->GetName());
			ScreenMsg( FONT_ORANGE, MSG_INTERFACE, pStr );
		}
		else if (fDropped && !fSplit)
		{
			// Either the item itself or another item has been dropped to the sector inventory due to lack of
			// space.
			swprintf(pStr, gzTransformationMessage[MSG_TRANSFORMATION_DROPPED_GROUND], pSoldier->GetName());
			DoScreenIndependantMessageBox( pStr, MSG_BOX_FLAG_OK, NULL );
		}
		else if (fDropped && fSplit)
		{
			// Item was split apart. Either the item itself or another item has been dropped to the sector 
			// inventory due to lack of space.
			swprintf(pStr, gzTransformationMessage[MSG_TRANSFORMATION_SPLIT_GROUND], Item[usOrigItem].szItemName, pSoldier->GetName());
			DoScreenIndependantMessageBox( pStr, MSG_BOX_FLAG_OK, NULL );
		}
	}

	// Flugente: we have to recheck our flashlights
	pSoldier->HandleFlashLights();

	// sevenfm: handle sight change
	HandleSight(pSoldier, SIGHT_LOOK);

	// Signal a successful transformation.
	return TRUE;
}

//Madd: Common Attachment Framework - check if a given point is acceptable
bool IsAttachmentPointAvailable( OBJECTTYPE * pObject, UINT8 subObject, UINT32 attachmentID )
{
	if (pObject)
	{
		if (Item[pObject->usItem].ulAvailableAttachmentPoint > 0 && (Item[attachmentID].attachment  || Item[attachmentID].usItemClass & IC_GRENADE || Item[attachmentID].usItemClass & IC_BOMB)&& Item[attachmentID].ulAttachmentPoint & GetAvailableAttachmentPoint(pObject, subObject))
			return true;
	}

	return false;
}

//Madd: Common Attachment Framework - if we already know the point 
bool IsAttachmentPointAvailable( UINT64 point, UINT32 attachmentID, BOOLEAN onlyCheckAttachments )
{
	if (point > 0 && (!onlyCheckAttachments || (Item[attachmentID].attachment || Item[attachmentID].usItemClass & IC_GRENADE || Item[attachmentID].usItemClass & IC_BOMB)) && Item[attachmentID].ulAttachmentPoint & point)
		return true;
	else
		return false;
}

//Madd: Common Attachment Framework, doesn't look at attachments
bool IsAttachmentPointAvailable( UINT32 itemID, UINT32 attachmentID )
{
	if (Item[itemID].ulAvailableAttachmentPoint > 0 && (Item[attachmentID].attachment || Item[attachmentID].usItemClass & IC_GRENADE || Item[attachmentID].usItemClass & IC_BOMB) && Item[attachmentID].ulAttachmentPoint & Item[itemID].ulAvailableAttachmentPoint) 
		return true;
	else
		return false;
}

//Madd: Common Attachment Framework, get point value from object + attachments
UINT64 GetAvailableAttachmentPoint (OBJECTTYPE * pObject, UINT8 subObject)
{
	UINT64 point = 0;
	if (pObject) 
	{
		point = Item[pObject->usItem].ulAvailableAttachmentPoint;
		for (attachmentList::iterator iter = (*pObject)[subObject]->attachments.begin(); iter != (*pObject)[subObject]->attachments.end(); ++iter) 
		{
			if(iter->exists() && Item[iter->usItem].ulAvailableAttachmentPoint > 0 && Item[iter->usItem].attachment )
				point |= Item[iter->usItem].ulAvailableAttachmentPoint;
		}
	}

	return point;
}

// Flugente: check if and how a bomb has been set up
void CheckBombSpecifics( OBJECTTYPE * pObj, INT8* detonatortype, INT8* setting, INT8* defusefrequency )
{
	if ( pObj && pObj->exists() )
	{
		*detonatortype = (*pObj)[0]->data.misc.bDetonatorType;

		if ( *detonatortype == BOMB_TIMED )
			*setting = (*pObj)[0]->data.misc.bDelay;
		else
			*setting = (*pObj)[0]->data.misc.bFrequency;

		*defusefrequency = (*pObj)[0]->data.bDefuseFrequency;
	}
}

// Flugente: check for specific flags
BOOLEAN HasItemFlag( UINT16 usItem, UINT32 aFlag )
{
	return( (Item[usItem].usItemFlag & aFlag) != 0 );
}

// Flugente: get first item number that has this flag. Use with caution, as we search in all items
BOOLEAN GetFirstItemWithFlag( UINT16* pusItem, UINT32 aFlag )
{
	for (UINT16 i = 1; i < gMAXITEMS_READ; ++i)
	{
		if ( HasItemFlag(i, aFlag) )
		{
			(*pusItem) = i;
			return( TRUE );
		}
	}

	return( FALSE );
}

// Flugente: check if object is currently fed from an external source (belts in inventory, other mercs)
BOOLEAN ObjectIsBeingFedExternal(SOLDIERTYPE* pSoldier, OBJECTTYPE * pObject)
{	
	return( GetExternalFeedingObject(pSoldier, pObject) != NULL );
}

// is this object currently used to feed an externally fed object? This can be in our or someone else's inventory
BOOLEAN ObjectIsExternalFeeder(SOLDIERTYPE* pSoldier, OBJECTTYPE * pObject)
{
	if ( !pSoldier || !pObject)
		return( FALSE );
		
	UINT8  usSoldierFeedingTarget1 = 0;
	UINT16 usGunSlot1 = 0;
	UINT16 usAmmoSlot1 = 0;
	UINT8  usSoldierFeedingTarget2 = 0;
	UINT16 usGunSlot2 = 0;
	UINT16 usAmmoSlot2 = 0;
	if ( pSoldier->IsFeedingExternal(&usSoldierFeedingTarget1, &usGunSlot1, &usAmmoSlot1, &usSoldierFeedingTarget2, &usGunSlot2, &usAmmoSlot2) )
	{
		SOLDIERTYPE* pTargetSoldier = MercPtrs[usSoldierFeedingTarget1];

		if ( pTargetSoldier && &(pSoldier->inv[usAmmoSlot1]) == pObject )
			return( TRUE );

		pTargetSoldier = MercPtrs[usSoldierFeedingTarget2];

		if ( pTargetSoldier && &(pSoldier->inv[usAmmoSlot2]) == pObject )
			return( TRUE );
	}
		
	return( FALSE );
}

OBJECTTYPE* GetExternalFeedingObject(SOLDIERTYPE* pSoldier, OBJECTTYPE * pObject)
{
	OBJECTTYPE* pObjExtMag = NULL;

	if ( !pObject || !(pObject->exists()) || !pSoldier || !pSoldier->bActive || !pSoldier->bInSector || pSoldier->stats.bLife < OKLIFE )
		// how did we even get here?
		return ( pObjExtMag );

	UINT16 usItem = pObject->usItem;

	// if item is a gun that can be belt fed and still has shots left
	if ( Item[ usItem ].usItemClass == IC_GUN && ( HasItemFlag( usItem, BELT_FED ) || HasAttachmentOfClass(pObject, AC_FEEDER) ) && (*pObject)[0]->data.gun.ubGunShotsLeft > 0 )
	{
		// remember the caliber, magsize (TODO: really?) and type of ammo. They all have to fit
		UINT8 ubCalibre = Weapon[usItem].ubCalibre;
		UINT16 ubMagSize = Weapon[usItem].ubMagSize;
		UINT8 ubAmmoType = (*pObject)[0]->data.gun.ubGunAmmoType;

		// now we now that this gun CAN be belt fed in the current situation. We now have to check if it IS
		// we will first check for other mercs who might feed us. Afterwards we look into our own inventory

		// loop over other members of our team in this sector. This includes ourself, as our gun can be fed from a belt in our inventory
		SOLDIERTYPE* pTeamSoldier = NULL;
		INT32 cnt = gTacticalStatus.Team[ pSoldier->bTeam ].bFirstID;
		INT32 lastid = gTacticalStatus.Team[ pSoldier->bTeam ].bLastID;
		for ( pTeamSoldier = MercPtrs[ cnt ]; cnt <= lastid; ++cnt, ++pTeamSoldier)
		{
			// check if teamsoldier exists in this sector
			if ( !pTeamSoldier || !pTeamSoldier->bActive || !pTeamSoldier->bInSector || pTeamSoldier->stats.bLife < OKLIFE || pTeamSoldier->sSectorX != pSoldier->sSectorX || pTeamSoldier->sSectorY != pSoldier->sSectorY || pTeamSoldier->bSectorZ != pSoldier->bSectorZ )
				continue;

			// check if both soldiers are on the same level
			if ( pSoldier->pathing.bLevel != pTeamSoldier->pathing.bLevel )
				continue;

			// we check if that guy is feeding someone, and that someone is really us
			UINT8  usTeamSoldierFeedingTarget1 = 0;
			UINT16 usGunSlot1 = 0;
			UINT16 usAmmoSlot1 = 0;
			UINT8  usTeamSoldierFeedingTarget2 = 0;
			UINT16 usGunSlot2 = 0;
			UINT16 usAmmoSlot2 = 0;
			if ( pTeamSoldier->IsFeedingExternal(&usTeamSoldierFeedingTarget1, &usGunSlot1, &usAmmoSlot1, &usTeamSoldierFeedingTarget2, &usGunSlot2, &usAmmoSlot2)  )
			{
				if ( usTeamSoldierFeedingTarget1 == pSoldier->ubID && pSoldier->inv[usGunSlot1] == (*pObject) )
				{
					if ( pTeamSoldier->inv[usAmmoSlot1].exists() && Item [ pTeamSoldier->inv[usAmmoSlot1].usItem ].usItemClass != IC_AMMO || pTeamSoldier->inv[usAmmoSlot1][0]->data.ubShotsLeft > 0 )
					{
						pObjExtMag = &(pTeamSoldier->inv[usAmmoSlot1]);
						return( pObjExtMag );
					}
				}

				if ( usTeamSoldierFeedingTarget2 == pSoldier->ubID && pSoldier->inv[usGunSlot2] == (*pObject) )
				{
					if ( pTeamSoldier->inv[usAmmoSlot2].exists() && Item [ pTeamSoldier->inv[usAmmoSlot2].usItem ].usItemClass != IC_AMMO || pTeamSoldier->inv[usAmmoSlot2][0]->data.ubShotsLeft > 0 )
					{
						pObjExtMag = &(pTeamSoldier->inv[usAmmoSlot2]);
						return( pObjExtMag );
					}
				}
			}
		}
	}

	return( pObjExtMag );
}

BOOLEAN DeductBulletViaExternalFeeding(SOLDIERTYPE* pSoldier, OBJECTTYPE * pObject)
{
	if ( !pObject || !(pObject->exists()) || !pSoldier || !pSoldier->bActive || !pSoldier->bInSector )
		// how did we even get here?
		return false;

	OBJECTTYPE* pObjExtMag = GetExternalFeedingObject(pSoldier, pObject);

	if ( !pObjExtMag || pObjExtMag->ubNumberOfObjects == 0)
		return false;

	UINT8 lastobjinstack = pObjExtMag->ubNumberOfObjects - 1;

	if ( (*pObjExtMag)[lastobjinstack]->data.ubShotsLeft != 0 )
	{
		(*pObjExtMag)[lastobjinstack]->data.ubShotsLeft--;

		if ( (*pObjExtMag)[lastobjinstack]->data.ubShotsLeft == 0 )
		{
			pObjExtMag->ubNumberOfObjects--;

			if ( !pObjExtMag->exists() )
			{
				// Delete object
				DeleteObj( pObjExtMag );

				// dirty interface panel
				DirtyMercPanelInterface(  pSoldier, DIRTYLEVEL2 );
			}
		}

		return( TRUE );
	}

	return( FALSE );
}

INT8 GetNumberAltFireAimLevels( SOLDIERTYPE * pSoldier, INT32 iGridNo )
{
	if ( !gGameExternalOptions.ubAllowAlternativeWeaponHolding || (gAnimControl[ pSoldier->usAnimState ].ubEndHeight != ANIM_STAND) )
	{
		return -1;
	}

	UINT16 usInHand = pSoldier->inv[ HANDPOS ].usItem;

	// If we are in water and having a pistol, don't allow alternative fire at all
	if ( !Item[usInHand].twohanded && pSoldier->MercInWater() )
	{
		return -1;
	}

	UINT8 ubStandardAimLvls = AllowedAimingLevels( pSoldier, iGridNo );
	
	if ( gGameExternalOptions.ubAllowAlternativeWeaponHolding == 1 )
	{
		// only with no aim clicks at all
		return 0;
	}
	else if ( gGameExternalOptions.ubAllowAlternativeWeaponHolding == 3 )
	{
		// with this mode, we always switch aaiming type manually
		return ubStandardAimLvls;
	}
	else if ( Weapon[usInHand].HeavyGun && Item[usInHand].twohanded)
	{
		// if this gun is flagged as too heavy to shoulder, return the same as standard aim levels
		return ubStandardAimLvls;
	}

	// from here, we assume we are using "hybrid" aiming mechanism, so calculate how many alternative aim levels we have  
	INT8 bAltAimLevels = 0;
	switch ( ubStandardAimLvls )
	{
	case 8:
	case 7:
		bAltAimLevels = 3;
		break;
	case 6:
	case 5:
		bAltAimLevels = 2;
		break;
	case 4:
	case 3:
		bAltAimLevels = 1;
		break;
	default:
		bAltAimLevels = 0;
		break;
	}
	// make LMGs more likely to be fired from hip
	if ( Weapon[usInHand].ubWeaponType == GUN_LMG && ubStandardAimLvls > 6 )
		bAltAimLevels += 1;
	if ( Weapon[usInHand].ubWeaponType == GUN_LMG && ubStandardAimLvls > 2 )
		bAltAimLevels += 1;
	// shotguns may also be more a little more suitable for it
	if ( Weapon[usInHand].ubWeaponType == GUN_SHOTGUN && ubStandardAimLvls > 4 )
		bAltAimLevels += 1;

	return bAltAimLevels ;
}

// get dirt increase for object with attachments, fConsiderAmmo: with ammo
FLOAT GetItemDirtIncreaseFactor( OBJECTTYPE * pObj, BOOLEAN fConsiderAmmo )
{
	FLOAT dirtincreasefactor = Item[pObj->usItem].dirtIncreaseFactor;
	
	if ( pObj->exists() == true ) 
	{
		attachmentList::iterator iterend = (*pObj)[0]->attachments.end();
		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != iterend; ++iter) 
		{
			// it attachment is not a weapon, add its factor
			if (iter->exists() && !(Item[iter->usItem].usItemClass & IC_WEAPON) )
			{
				dirtincreasefactor += Item[iter->usItem].dirtIncreaseFactor;
			}
		}
	}

	// ammo modifies how much dirt a single shot makes, but only while shooting, not when a gun gets dirty due to environmental effects
	if ( fConsiderAmmo )		
		dirtincreasefactor *= (1.0f + AmmoTypes[(*pObj)[0]->data.gun.ubGunAmmoType].dirtModificator);

	// multiply again for global modifer
	dirtincreasefactor *= gGameExternalOptions.iDirtGlobalModifier;
		
	// dirt factor has to be >= 0 (items don't clean themselves)
	dirtincreasefactor = max(0.0f, dirtincreasefactor);

	return dirtincreasefactor;
}

// Flugente: retrieve a specific clothes item, if such a thing exists
BOOLEAN	GetFirstClothesItemWithSpecificData( UINT16* pusItem, PaletteRepID aPalVest, PaletteRepID aPalPants )
{
	bool vestok  = FALSE;
	bool pantsok = FALSE;

	UINT8 filler = 0;
	if ( !GetPaletteRepIndexFromID(aPalVest, &filler) )
		vestok = TRUE;

	if ( !GetPaletteRepIndexFromID(aPalPants, &filler) )
		pantsok = TRUE;

	// getting the best item isn't straightforward. As combo clothes can be found first, we will search for each item whose clothestype matches, and retreive the lowest item that has
	// the lowest clothestype
	UINT32 bestclothestype = 999999;
	UINT16 bestitem = 0;

	for (UINT16 i = 1; i < gMAXITEMS_READ; ++i)
	{
		if ( Item[i].clothestype > 0 )
		{
			bool tmpvestok  = vestok;
			bool tmppantsok = pantsok;

			UINT8 vestpalid = 0;
			GetPaletteRepIndexFromID(aPalVest, &vestpalid);

			// if we are looking for vests, do so
			if ( !tmpvestok  )
			{				
				if ( COMPARE_PALETTEREP_ID(aPalVest, Clothes[Item[i].clothestype].vest) )
				{
					tmpvestok = TRUE;
				}
			}

			// if we are looking for pants, do so
			if ( !tmppantsok )
			{
				if ( COMPARE_PALETTEREP_ID(aPalPants, Clothes[Item[i].clothestype].pants) )
				{
					tmppantsok = TRUE;
				}
			}

			if ( tmpvestok && tmppantsok )
			{	
				if ( Item[i].clothestype < bestclothestype )
				{
					bestclothestype = Item[i].clothestype;
					bestitem = i;
				}
			}
		}
	}

	if ( bestitem > 0 )
	{
		*pusItem = bestitem;
		return( TRUE );
	}

	return( FALSE );
}

#define RANDOM_ITEM_MAX_NUMBER 2000
#define RANDOM_TABOO_MAX		 50

UINT16 randomitemarray[RANDOM_ITEM_MAX_NUMBER];
UINT16 randomitemtabooarray[RANDOM_TABOO_MAX];			// We remember which random items we added, to prevent loops
UINT16 randomitemclasstabooarray[RANDOM_TABOO_MAX];		// We also remember the random item classes

UINT16 itemcnt = 0;
UINT16 rdtaboocnt = 0;
UINT16 rdclasstaboocnt = 0;

INT8  rditemmaxcoolness = 0;

BOOL AddToRandomListFromRandomItemClass( UINT16 usRandomItemClass );
BOOL AddToRandomListFromRandomItem( UINT16 usRandomItem );
BOOL AddToRandomListFromItem( UINT16 usItem );
BOOL RandomItemClassIsTaboo( UINT16 usRandomItemClass );
BOOL RandomItemIsTaboo( UINT16 usRandomItem );

BOOLEAN GetItemFromRandomItem( UINT16 usRandomItem, UINT16* pusNewItem )
{
	*pusNewItem = 0;

	// no random item solving in the map editor
#ifdef JA2EDITOR
	return FALSE;
#endif

	// is this a random item?
	if ( Item[usRandomItem].randomitem > 0 )
	{
		// we build a list of all the items in the random item class this item references to
		// We also have to check for other random item classes
		// as it is also possible to reference to other random items, we also have to check for them

		// clear the random item arrays and reset the counters
		for ( int i = 0; i < RANDOM_ITEM_MAX_NUMBER; ++i)
			randomitemarray[i] = 0;

		for ( int i = 0; i < RANDOM_TABOO_MAX; ++i)
		{
			randomitemtabooarray[i] = 0;
			randomitemclasstabooarray[i] = 0;
		}

		itemcnt = 0;
		rdtaboocnt = 0;
		rdclasstaboocnt = 0;

		// determine maximum allowed coolness
		rditemmaxcoolness = HighestPlayerProgressPercentage() / 10 + 1 + Item[usRandomItem].randomitemcoolnessmodificator;	// the random item itself can modify coolness
		
		// build the list of items to choose from. We will search down the random item class and can even branch into multiple other random item classes.
		// We only stop if maximum number of items or random item classes is reached
		AddToRandomListFromRandomItem(usRandomItem);

		if ( itemcnt )
		{
			// select a random item from our list
			UINT16 random = Random(itemcnt);

			*pusNewItem = randomitemarray[random];

			return TRUE;
		}
	}

	return FALSE;
}

BOOL AddToRandomListFromRandomItemClass( UINT16 usRandomItemClass )
{
	if ( usRandomItemClass && !RandomItemClassIsTaboo(usRandomItemClass) )
	{
		// end if we reached the maximum number of taboos
		if ( rdclasstaboocnt >= RANDOM_TABOO_MAX )
			return FALSE;

		// add items
		for ( int i = 0; i < RANDOM_XML_LENGTH; ++i)
		{
			// if this returns false, we're either at the maximum number of items or taboos - both reasons to end
			if ( !AddToRandomListFromItem(gRandomItemClass[usRandomItemClass].item[i]) )
				return FALSE;
		}

		// add random item classes
		for ( int i = 0; i < RANDOM_XML_LENGTH; ++i)
		{
			// if this returns false, we're either at the maximum number of items or taboos - both reasons to end
			if ( !AddToRandomListFromRandomItemClass(gRandomItemClass[usRandomItemClass].randomitem[i]) )
				return FALSE;
		}
	}

	return TRUE;
}

// add from a random item to the list
BOOL AddToRandomListFromRandomItem( UINT16 usRandomItem )
{
	if ( Item[usRandomItem].randomitem > 0 && !RandomItemIsTaboo( usRandomItem ) )
	{
		// end if we reached the maximum number of taboos
		if ( rdtaboocnt >= RANDOM_TABOO_MAX )
			return FALSE;

		return AddToRandomListFromRandomItemClass(Item[usRandomItem].randomitem);
	}

	return TRUE;
}

// add an item to the list
BOOL AddToRandomListFromItem( UINT16 usItem )
{
	if ( usItem )
	{
		// is it another random item?
		if ( Item[usItem].randomitem > 0 )
		{
			// continue adding items from this new random item
			return AddToRandomListFromRandomItem(usItem);
		}
		else
		{
			// only allow those items that are viable at the current progress
			if ( Item[usItem].ubCoolness <= rditemmaxcoolness )
			{
				// sevenfm: only spawn legal items
				//if ( !gGameOptions.fFoodSystem && Item[usItem].foodtype > 0 && Item[usItem].drugtype == 0 && Item[usItem].canteen == 0 )
				if(ItemIsLegal(usItem))
					randomitemarray[itemcnt++] = usItem;
			}

			// if maximum is reached, return false, thereby signaling an end
			return ( itemcnt < RANDOM_ITEM_MAX_NUMBER );
		}			
	}

	return TRUE;
}

// check whether this class is already on the taboo list (forbidden to add from there again, because this can lead to loops). If not, add this to the taboo list
BOOL RandomItemClassIsTaboo( UINT16 usRandomItemClass )
{
	for ( int i = 0; i < rdclasstaboocnt; ++i)
	{
		if ( randomitemclasstabooarray[i] == usRandomItemClass )
			return TRUE;
	}

	// add to taboo list
	randomitemclasstabooarray[rdclasstaboocnt++] = usRandomItemClass;

	return FALSE;
}

BOOL RandomItemIsTaboo( UINT16 usRandomItem )
{
	for ( int i = 0; i < rdtaboocnt; ++i)
	{
		if ( randomitemtabooarray[i] == usRandomItem )
			return TRUE;
	}

	// add to taboo list
	randomitemtabooarray[rdtaboocnt++] = usRandomItem;

	return FALSE;
}

// Flugente: can item be applied to other people?
BOOLEAN ItemCanBeAppliedToOthers( UINT16 usItem )
{
	if ( Item[ usItem ].drugtype )
		return TRUE;

	if ( Item[ usItem ].gasmask )
		return TRUE;
		
	if ( Item[ usItem ].canteen )
		return TRUE;

	if ( Item[ usItem ].camouflagekit )
		return TRUE;

	if ( Item[ usItem ].clothestype )
		return TRUE;

	if ( Item[ usItem ].usItemClass == IC_BOMB )
		return TRUE;
		
	return FALSE;
}

//zwwooooo - IoV: change RangeBonus to ratable (Orange by kenkenkenken in IoV921)
INT32 GetPercentRangeBonus( OBJECTTYPE * pObj )
{
	INT32 bonus = 10000;
	if (pObj->exists() == true) 
	{
		bonus += ( BonusReduce( Item[pObj->usItem].percentrangebonus, (*pObj)[0]->data.objectStatus ) ) * 100;

		if ( (*pObj)[0]->data.gun.ubGunShotsLeft > 0 )
			bonus = ( bonus * ( 100 +  Item[(*pObj)[0]->data.gun.usGunAmmoItem].percentrangebonus ) ) / 100;

		for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter) 
		{
			if (iter->exists() && (!Item[iter->usItem].duckbill || (Item[iter->usItem].duckbill && (*pObj)[0]->data.gun.ubGunAmmoType == AMMO_BUCKSHOT)))
				bonus = ( bonus * ( 100 +  BonusReduce( Item[iter->usItem].percentrangebonus, (*iter)[0]->data.objectStatus ) ) ) / 100;
		}
	}
	return( bonus );
}

// silversurfer: Finds items with SleepModifier bonus in inventory and returns the highest value
UINT8 GetInventorySleepModifier( SOLDIERTYPE *pSoldier )
{
	UINT8 modifier = 0;

	UINT8 invsize = pSoldier->inv.size();
	for ( UINT8 i = 0; i < invsize; ++i )
	{
		if( pSoldier->inv[ i ].exists() == true && Item[ pSoldier->inv[i].usItem ].ubSleepModifier > 0 )
		{
			modifier = __max(modifier, BonusReduce( Item[ pSoldier->inv[i].usItem ].ubSleepModifier, pSoldier->inv[i][0]->data.objectStatus ));
		}
	}

	return( modifier );
}

void AttachDefaultAttachments(OBJECTTYPE *pObj, BOOLEAN fAllDefaultAttachments)//dnl ch75 261013
{
	if(pObj->usItem && gGameOptions.ubAttachmentSystem)
	{
		RemoveProhibitedAttachments(NULL, pObj, pObj->usItem);
		//Madd: ok, so this drives me nuts -- why bother with default attachments if the map isn't going to load them for you?  
		//this should fix that...
		for(UINT8 cnt=0; cnt<MAX_DEFAULT_ATTACHMENTS; cnt++)
		{
			if(Item[pObj->usItem].defaultattachments[cnt] == NONE || !(gGameOptions.ubAttachmentSystem && Item[Item[pObj->usItem].defaultattachments[cnt]].inseparable || fAllDefaultAttachments))
				break;
			OBJECTTYPE defaultAttachment;
			CreateItem(Item[pObj->usItem].defaultattachments[cnt], (*pObj)[0]->data.gun.bGunStatus, &defaultAttachment);
			pObj->AttachObject(NULL, &defaultAttachment, FALSE);
		}
	}
}

// check if item is grenade and it can use delayed explosion
BOOLEAN CanDelayGrenadeExplosion( UINT16 usItem )
{
	// check that item is a grenade
	if( Item[ usItem ].usItemClass != IC_GRENADE )
	{
		return FALSE;
	}

	// check that grenade type is not gas/smoke/fire or flare
	if( Explosive[Item[ usItem ].ubClassIndex].ubType != EXPLOSV_NORMAL &&
		Explosive[ Item[ usItem ].ubClassIndex ].ubType != EXPLOSV_STUN &&
		Explosive[ Item[ usItem ].ubClassIndex ].ubType != EXPLOSV_FLASHBANG )
	{
		return FALSE;
	}
	
	// check if grenade should always explode on impact
	//if( Explosive[Item[ usItem ].ubClassIndex].fExplodeOnImpact )
	//{
		//return FALSE;
	//}

	return TRUE;
}

// sevenfm: find first hand grenade of type
BOOLEAN GetFirstGrenadeOfType( UINT16 &usItem, UINT8 ubGrenadeType )
{
	for (UINT16 i = 1; i < gMAXITEMS_READ; ++i)
	{
		if( (Item[ i ].usItemClass & IC_GRENADE) &&
			Item[ i ].ubCursor == TOSSCURS &&
			GetLauncherFromLaunchable( i ) == NOTHING &&
			Explosive[ Item[ i ].ubClassIndex ].ubType == ubGrenadeType )
		{
			usItem = i;
			return( TRUE );
		}
	}

	return( FALSE );
}

INT8 FindCanteen(SOLDIERTYPE * pSoldier)
{
	INT8 invsize = (INT8)pSoldier->inv.size();

	for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() && Item[pSoldier->inv[bLoop].usItem].canteen)
		{
			OBJECTTYPE * pObj = &(pSoldier->inv[bLoop]);

			if (pObj && TotalPoints(pObj) > 1)
			{
				return(bLoop);
			}
		}
	}

	return(NO_SLOT);
}

INT8 FindDrug(SOLDIERTYPE * pSoldier, UINT32 usDrugType)
{
	INT8 invsize = (INT8)pSoldier->inv.size();

	for (INT8 bLoop = 0; bLoop < invsize; ++bLoop)
	{
		if (pSoldier->inv[bLoop].exists() && Item[pSoldier->inv[bLoop].usItem].drugtype & usDrugType)
		{
			OBJECTTYPE * pObj = &(pSoldier->inv[bLoop]);

			// canteens should have at least 1 status point to be used
			if (!Item[pSoldier->inv[bLoop].usItem].canteen || pObj && TotalPoints(pObj) > 1)
			{
				return(bLoop);
			}
		}
	}

	return(NO_SLOT);
}

/*FLOAT GetBestScopeMagFactorForGun(UINT16 ausItemGun)
{
	FLOAT bestscopemagfactor = 1.0f;

	for (UINT16 usItem = 1; usItem < gMAXITEMS_READ; ++usItem)
	{
		if (ValidAttachment(usItem, ausItemGun))
		{
			if (bestscopemagfactor < Item[usItem].scopemagfactor)
				bestscopemagfactor = Item[usItem].scopemagfactor;
		}
	}

	return bestscopemagfactor;
}*/

/*bool HasScopeMagFactorForGun(UINT16 ausItemGun, FLOAT aFactor)
{
	for (UINT16 usItem = 1; usItem < gMAXITEMS_READ; ++usItem)
	{
		if (ValidAttachment(usItem, ausItemGun))
		{
			if (std::fabs(Item[usItem].scopemagfactor - aFactor) < 0.1)
				return true;
		}
	}

	return false;
}*/

BOOLEAN FindAttachmentRange(UINT16 usAttachment, UINT32* pStartIndex, UINT32* pEndIndex)
{
	BOOLEAN result = FALSE;

	INT32 leftMargin = 0;
	INT32 rightMargin = (INT32)gMAXATTACHMENTS_READ - 1;
	INT32 middle = 0;
	// use binary search to locate the group of elements for given attachment item Id (usAttachment)
	while (leftMargin <= rightMargin)
	{
		middle = leftMargin + (rightMargin - leftMargin) / 2;

		if (Attachment[middle].attachmentIndex == usAttachment)
		{
			result = TRUE;
			break;
		}
		else if (Attachment[middle].attachmentIndex < usAttachment)
			leftMargin = middle + 1;
		else
			rightMargin = middle - 1;
	}

	if (result)
	{
		// now middle is an index somewhere within the group, seek for beginning and ending of the group
		if (pStartIndex)
		{
			*pStartIndex = (UINT32)middle;
			for (INT32 i = middle - 1; i >= leftMargin; i--)
			if (Attachment[i].attachmentIndex == usAttachment)
				*pStartIndex = (UINT32)i;
			else
				break;
		}
		if (pEndIndex)
		{
			*pEndIndex = (UINT32)middle;
			for (INT32 i = middle + 1; i <= rightMargin; i++)
			if (Attachment[i].attachmentIndex == usAttachment)
				*pEndIndex = (UINT32)i;
			else
				break;
		}
	}

	return result;
}

BOOLEAN ItemIsCanteen(UINT16 usItem)
{
	return Item[usItem].canteen;
}
BOOLEAN ItemIsTwoHanded(UINT16 usItem)
{
	return Item[usItem].twohanded;
}
