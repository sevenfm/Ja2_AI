#ifndef __CRAFTINFO_H_
#define __CRAFTINFO_H_

#include "types.h"

#include <vector>

// MVP: craft recipes viewer.
// Collects all craft recipes (Merge, AttachmentComboMerge, Transformations) related to an item
// and prints them to the message log. Can be reused later by any GUI.

typedef struct
{
	// recipe text: ingredient1 + ingredient2 + ingredient3 = result
	// unused ingredients are 0
	UINT16 usIngredient1;
	UINT16 usIngredient2;
	UINT16 usIngredient3;
	UINT16 usResult;
} CraftRecipe;

class CraftInfo
{
public:
	CraftInfo( UINT16 usItemIndex );

	// number of recipes found for this item
	UINT32 NumRecipes( void ) const { return m_recipes.size(); }

	// access to the collected recipes
	const std::vector<CraftRecipe>& GetRecipes( void ) const { return m_recipes; }

	// print all collected recipes to the message log
	void Print( void ) const;

protected:
	UINT16 m_usItemIndex;
	std::vector<CraftRecipe> m_recipes;

	void CollectMergeRecipes( void );
	void CollectComboMergeRecipes( void );
	void CollectTransformationRecipes( void );
};

// convenience function: collect recipes for the item and print them to the log
void ShowItemCraftInfo( UINT16 usItemIndex );

#endif
