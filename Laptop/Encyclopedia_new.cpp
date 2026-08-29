/*TODOs:
*remove every unneeded data (temp arrays, save arrays, backup arrays, data arrays, ec.)
*Items:		- needs three state ini option to either be saved into file (any new item in inventory or sector stash will be added) 
*				UINT8 array [MAX_ITEMS]: 0 not yet discovered (invisible), 1 in sector stash (unreachable) or on enemy, 2 seen at bobby ray, 3 seen in dealer inventory, 4 in sector stash (reachable) or solder inventory
*			- or to gather data from all sector inventories (mark all as reachable, as there is no way to figure out in unloaded sectors), bobby rays current inventory, known dealers, soldier inventory
*			- or to reveal all items from game (thats the old encyclopedia behavier)
*			- make sure that this data gets updated only when accessing the laptop encyclopedia to avoid any impact on gameplay performance memory/speed (not possible for option 1!!)
*Profiles:	- gather data from existing structs
*Quests:	- gather data from existing structs
*Locations	- gather data from exisitng structs
*
*only current DATA object is stored (graphic, names, description text, type ec..) and replaced when [next] or [previous] button was pressed. Stored in order to rerender dirty area if needed.
* DONE: filters are handled on button press (gathering data in sequence and skip filtered items)
*if possible do not read/write from files to get some data
* DONE: reuse all graphics and layout from old Enc.
*
* PARTIALLY DONE: on button press: check if next/previous item exists to hide or disable (TBD) buttons and play bad beep sound (applys to clicks on filter buttons as well!)
*
*minimize usage of functions: default laptop www site has following functions:
*	void GameInitXXX()											inits coords, validates data ec. everything that does not change (called when starting JA2/loading game/starting new game).
*	void EnterXXX()												creates video objects, buttons, fasthelp texts, mousregions, ec. DOES NOT CALL RENDER!!
*	void ExitXXX()												clean up everything, memory usage before enter and after exit should be equal
*	void HandleXXX()											helper function gets called AFTER rendering by game loop. Can be used for animations, blinking and stuff (showing position of person on a map by blinking, simulate loading of site, animated zoom ec.)
*	void RenderXXX()											obviously renders the site
*	void ChangingXXXSubPage(UINT8)								prepares a change to a different subpage (called by BtnCallback)
*	void BtnXXXButtonCallback(GUI_BUTTON *btn,INT32 reason)		callback for buttons / regions. If possible reuse these by adding some local swiches for readability of code (f.i.one for filters, one for next/previous).
*/

/// uncomment to use just graphic + mouseregion instead of real buttons. No sounds, no button states, just plain 'hyperlinks'.
#define ENC_USE_BUTTONSYSTEM

#ifdef PRECOMPILEDHEADERS
	#include "Laptop All.h"
#else
	#include "Types.h"
	#include "WCheck.h"
	#include "DEBUG.H"
	#include "GameSettings.h"
	#include "laptop.h"//UI dimensions, mouse regions
	#include "Utilities.h"//file names
	#include "vobject.h"//video objects
	#include "Utils/Cursors.h"
	#include "Text.h"//button text
#ifdef ENC_USE_BUTTONSYSTEM
	#include "Button System.h"
#else
	#include "WordWrap.h"//centered text
#endif
	#include "Encyclopedia_new.h"
	#include "Encyclopedia_Data_new.h"
#endif	

/**
* @brief Saves Item visibility array to file.
*
* @return TRUE if data is written.
*/
BOOLEAN SaveEncyclopediaItemVisibility( HWFILE hFile )
{
	UINT32 bytesWritten;

	CHECKF( FileWrite( hFile, gbEncyclopediaData_ItemVisible, MAXITEMS, &bytesWritten ) );

	return (TRUE);
}

/**
* @brief Loads item visibility array from file.
*
* @return TRUE if data is read.
*/
BOOLEAN LoadEncyclopediaItemVisibility( HWFILE hFile )
{
	UINT32 bytesRead;

	CHECKF( FileRead( hFile, gbEncyclopediaData_ItemVisible, MAXITEMS, &bytesRead) );

	return (TRUE);
}
