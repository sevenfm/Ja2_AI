#include "builddefines.h"

#include <stdio.h>
#include <stdarg.h>
#include <vector>

#include "sgp.h"
#include "CraftInfo.h"
#include "Items.h"
#include "Item Types.h"
#include "overhead.h"
#include "message.h"
#include "text.h"
#include "Font Control.h"

// MVP: craft recipes viewer (text output)

CraftInfo::CraftInfo( UINT16 usItemIndex )
{
	m_usItemIndex = usItemIndex;

	CollectMergeRecipes();
	CollectComboMergeRecipes();
	CollectTransformationRecipes();
}

void CraftInfo::CollectMergeRecipes( void )
{
	for ( UINT16 i = 0; i <= MAXITEMS; i++ )
	{
		UINT16 usIngredient1 = Merge[i][0];
		UINT16 usIngredient2 = Merge[i][1];
		UINT16 usResult1 = Merge[i][2];
		UINT16 usResult2 = Merge[i][3];

		BOOLEAN fIsReactant = ( usIngredient1 == m_usItemIndex );
		BOOLEAN fIsTarget = ( usIngredient1 != m_usItemIndex && usIngredient2 == m_usItemIndex );

		if ( !fIsReactant && !fIsTarget )
		{
			// this item is one of the results: "can be crafted from A + B"
			if ( (usResult1 == m_usItemIndex || usResult2 == m_usItemIndex) &&
				ItemIsLegal( usIngredient1, TRUE ) && ItemIsLegal( usIngredient2, TRUE ) )
			{
				CraftRecipe recipe;
				recipe.usIngredient1 = usIngredient1;
				recipe.usIngredient2 = usIngredient2;
				recipe.usIngredient3 = 0;
				recipe.usResult = m_usItemIndex;
				m_recipes.push_back( recipe );
			}
			continue;
		}

		if ( !ItemIsLegal( usIngredient1, TRUE ) || !ItemIsLegal( usIngredient2, TRUE ) )
		{
			continue;
		}

		BOOLEAN fResult1 = ItemIsLegal( usResult1, TRUE );
		BOOLEAN fResult2 = ItemIsLegal( usResult2, TRUE ) && usResult2 != usResult1;
		if ( !fResult1 && !fResult2 )
		{
			continue;
		}

		if ( fResult1 )
		{
			CraftRecipe recipe;
			recipe.usIngredient1 = usIngredient1;
			recipe.usIngredient2 = usIngredient2;
			recipe.usIngredient3 = 0;
			recipe.usResult = usResult1;
			m_recipes.push_back( recipe );
		}
		if ( fResult2 )
		{
			CraftRecipe recipe;
			recipe.usIngredient1 = usIngredient1;
			recipe.usIngredient2 = usIngredient2;
			recipe.usIngredient3 = 0;
			recipe.usResult = usResult2;
			m_recipes.push_back( recipe );
		}
	}
}

void CraftInfo::CollectComboMergeRecipes( void )
{
	for ( UINT16 i = 0; i <= MAXITEMS; i++ )
	{
		if ( !ItemIsLegal( AttachmentComboMerge[i].usItem, TRUE ) || !ItemIsLegal( AttachmentComboMerge[i].usResult, TRUE ) )
		{
			continue;
		}

		BOOLEAN fRelated = ( AttachmentComboMerge[i].usItem == m_usItemIndex );
		if ( !fRelated )
		{
			for ( UINT16 a = 0; a < MAX_DEFAULT_ATTACHMENTS; a++ )
			{
				if ( AttachmentComboMerge[i].usAttachment[a] == m_usItemIndex )
				{
					fRelated = TRUE;
					break;
				}
			}
		}
		if ( !fRelated )
		{
			continue;
		}

		// recipe line: base item + attachment1 + attachment2 = result
		CraftRecipe recipe;
		recipe.usIngredient1 = AttachmentComboMerge[i].usItem;
		recipe.usIngredient2 = 0;
		recipe.usIngredient3 = 0;
		recipe.usResult = AttachmentComboMerge[i].usResult;

		// use the first two non-empty attachments as additional ingredients
		UINT16 usNumIngredients = 0;
		for ( UINT16 a = 0; a < MAX_DEFAULT_ATTACHMENTS && usNumIngredients < 2; a++ )
		{
			UINT16 usAttachment = AttachmentComboMerge[i].usAttachment[a];
			if ( usAttachment == NOTHING || usAttachment == 0 || !ItemIsLegal( usAttachment, TRUE ) )
			{
				continue;
			}
			if ( usNumIngredients == 0 )
			{
				recipe.usIngredient2 = usAttachment;
			}
			else
			{
				recipe.usIngredient3 = usAttachment;
			}
			usNumIngredients++;
		}

		m_recipes.push_back( recipe );
	}
}

void CraftInfo::CollectTransformationRecipes( void )
{
	for ( INT32 x = 0; x <= MAXITEMS; x++ )
	{
		if ( Transform[x].usItem == (UINT16)-1 )
		{
			break;
		}
		if ( Transform[x].usItem != m_usItemIndex )
		{
			continue;
		}

		for ( INT32 r = 0; r < MAX_NUM_TRANSFORMATION_RESULTS; r++ )
		{
			if ( Transform[x].usResult[r] == 0 || !ItemIsLegal( Transform[x].usResult[r], TRUE ) )
			{
				continue;
			}

			// transformation: the item alone turns into the result
			CraftRecipe recipe;
			recipe.usIngredient1 = m_usItemIndex;
			recipe.usIngredient2 = 0;
			recipe.usIngredient3 = 0;
			recipe.usResult = Transform[x].usResult[r];
			m_recipes.push_back( recipe );
		}
	}
}

void CraftInfo::Print( void ) const
{
	ScreenMsg( FONT_MCOLOR_WHITE, MSG_INTERFACE, gzCraftInfoMessage[CRAFTINFO_HEADER], ItemNames[m_usItemIndex], m_usItemIndex );

	if ( m_recipes.empty() )
	{
		ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, gzCraftInfoMessage[CRAFTINFO_NONE] );
		return;
	}

	for ( std::vector<CraftRecipe>::const_iterator it = m_recipes.begin(); it != m_recipes.end(); ++it )
	{
		if ( it->usIngredient2 != 0 )
		{
			if ( it->usIngredient3 != 0 )
			{
				CHAR16 zIngredients[400];
				swprintf( zIngredients, L"%s + %s + %s", ItemNames[it->usIngredient1], ItemNames[it->usIngredient2], ItemNames[it->usIngredient3] );
				ScreenMsg( FONT_ORANGE, MSG_INTERFACE, gzCraftInfoMessage[CRAFTINFO_CAN_BE_CRAFTED_FROM], zIngredients, ItemNames[it->usResult] );
			}
			else
			{
				ScreenMsg( FONT_ORANGE, MSG_INTERFACE, gzCraftInfoMessage[CRAFTINFO_USED_FOR], ItemNames[it->usIngredient1], ItemNames[it->usIngredient2], ItemNames[it->usResult] );
			}
		}
		else
		{
			ScreenMsg( FONT_ORANGE, MSG_INTERFACE, gzCraftInfoMessage[CRAFTINFO_CREATED_FROM], ItemNames[it->usIngredient1], ItemNames[it->usResult] );
		}
	}
}

void ShowItemCraftInfo( UINT16 usItemIndex )
{
	CraftInfo info( usItemIndex );
	info.Print();
}
