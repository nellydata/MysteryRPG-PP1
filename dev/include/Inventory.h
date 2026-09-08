#pragma once

#include <vector>
#include <memory>
#include <string>
#include <set>
#include "Clue.h"
#include "GameplayTypes.h"

class Item;

struct ItemEntry {
	std::unique_ptr<Item> item;
	int quantity = 1;
	bool locked = false;
	unsigned long long acquisitionOrder = 0;

	Item* operator->() { return item.get(); }
	const Item* operator->() const { return item.get(); }
};

enum class InventorySort {
	AlphabeticalAscending,
	AlphabeticalDescending,
	RarityAscending,
	RarityDescending,
	RecentlyObtained,
	LeastRecentlyObtained,
	HighestStat
};

class Inventory {
public:
	std::vector<Clue> clues;

	void AddClue(const Clue& clue);
	void AddItem(std::unique_ptr<Item> item);
	bool RemoveItem(const std::string& name);
	bool RemoveItem(unsigned long long instanceId);
	Item* FindItem(const std::string& name);
	const std::vector<ItemEntry>& GetItems() const;
	const std::vector<Clue>& GetClues() const;
	bool HasClue(const std::string& description) const;
	int GetQuantity(const std::string& name) const;
	bool IsLocked(const std::string& name) const;
	void SetLocked(const std::string& name, bool locked);
	bool DiscardItem(const std::string& name, int amount = 1);
	bool DiscardItem(unsigned long long instanceId);
	void ProtectItem(unsigned long long instanceId);
	void UnprotectItem(unsigned long long instanceId);
	std::vector<Item*> GetSortedItems(InventorySort sort, ItemStat focusStat = ItemStat::ATK) const;
	std::size_t GetItemCount() const;

private:
	std::vector<ItemEntry> items_;
	std::set<unsigned long long> protectedItems_;
	unsigned long long nextAcquisitionOrder_ = 0;
};
