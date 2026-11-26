#include "Feature/ContainerLoot/ContainerLootRandomItems.h"

#include "Item Types.h"
#include "Items.h"
#include "random.h"

#include <vector>

namespace
{
	struct RandomItemCollector
	{
		RandomItemCollector()
			: maxCoolness(0)
		{
			for (INT32 i = 0; i < RANDOM_ITEM_MAX_CATEGORIES; ++i)
				visited[i] = false;
		}

		bool IsValidItem(UINT16 usItem) const
		{
			if (usItem == 0 || usItem >= gMAXITEMS_READ)
				return false;

			if (usItem == SWITCH || usItem == ACTION_ITEM || usItem == OWNERSHIP)
				return false;

			if (Item[usItem].usItemClass == 0)
				return false;

			if (Item[usItem].ubCoolness > maxCoolness)
				return false;

			if (!ItemIsLegal(usItem, TRUE))
				return false;

			if (Item[usItem].defaultundroppable)
				return false;

			if (Item[usItem].usItemClass & IC_KEY)
				return false;

			return true;
		}

		void AddItem(UINT16 usItem)
		{
			if (usItem == 0 || usItem >= gMAXITEMS_READ)
				return;

			// travnik: вложенный random item разворачиваем в его класс
			if (Item[usItem].randomitem > 0)
			{
				AddClass(Item[usItem].randomitem);
				return;
			}

			if (!IsValidItem(usItem))
				return;

			for (size_t i = 0; i < items.size(); ++i)
			{
				if (items[i] == usItem)
					return;
			}

			items.push_back(usItem);
		}

		void AddClass(UINT16 usClass)
		{
			if (usClass == 0 || usClass >= RANDOM_ITEM_MAX_CATEGORIES)
				return;

			if (visited[usClass])
				return;

			visited[usClass] = true;

			const RANDOM_ITEM_CHOICE_TYPE& choice = gRandomItemClass[usClass];

			for (INT32 i = 0; i < RANDOM_XML_LENGTH; ++i)
			{
				if (choice.randomitem[i] > 0)
					AddClass(choice.randomitem[i]);
			}

			for (INT32 i = 0; i < RANDOM_XML_LENGTH; ++i)
			{
				if (choice.item[i] > 0)
					AddItem(choice.item[i]);
			}
		}

		bool visited[RANDOM_ITEM_MAX_CATEGORIES];
		std::vector<UINT16> items;
		UINT8 maxCoolness;
	};
}

BOOLEAN ContainerLootRandomItems::Pick(UINT16 usClass, UINT8 ubMaxCoolness, UINT16* pusItem)
{
	if (!pusItem)
		return FALSE;

	*pusItem = 0;

	RandomItemCollector collector;
	collector.maxCoolness = ubMaxCoolness;
	collector.AddClass(usClass);

	if (collector.items.empty())
		return FALSE;

	*pusItem = collector.items[Random((UINT32)collector.items.size())];
	return TRUE;
}
