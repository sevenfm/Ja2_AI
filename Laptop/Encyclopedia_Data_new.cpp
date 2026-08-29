#ifdef PRECOMPILEDHEADERS
	#include "Laptop All.h"
#else
	#include "Types.h"
	#include "WCheck.h"
	#include <string.h>
	#include "GameSettings.h"
	#include "Encyclopedia_Data_new.h"
	#include "Encyclopedia_new.h"
	#include "Item Types.h"			//item
	#include "Interface Items.h"	//item: load item graphic
	#include "Weapons.h"			//item
	#include "Queen Command.h"		//location: Sectorinfo
	#include "strategic.h"			//location: townIds
	#include "PostalService.h"		//location: airport
	#include "Campaign Types.h"		//location: facility types, descriptions
	#include "Soldier Profile.h"	//char: names, flags and IDs
	#include "Vehicles.h"			//char: vehicle image
	#include "LocalizedStrings.h"
	#include "Encrypted File.h"
	#include "mercs.h"
	#include "Overhead.h"			//char: menpointer for ubWhatKindofMercAmI
	#include "aim.h"				//char: aim availability
	#include "QuestText.h"			//quest: name
	#include "laptop.h"				//ui positions
	#include "Utilities.h"
	#include "Utils/Cursors.h"
	#include "sysutil.h"			//extra Buffer for scaling image
	#include "vsurface.h"			//fill extra buffer with black color
	#include "Text.h"
	#include "WordWrap.h"
    #include "Quests.h"
#endif

///< Visibility of items. If an item should be visible the value is not 0. 
// Gets loaded from savegame, updated by \ref EncyclopediaSetItemAsVisible() and reset by \ref EncyclopediaInitItemsVisibility().
UINT8 gbEncyclopediaData_ItemVisible[MAXITEMS];
