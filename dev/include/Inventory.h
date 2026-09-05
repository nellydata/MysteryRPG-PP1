#pragma once

#include <vector>
#include <memory>
#include <string>
#include "Clue.h"
#include "GameplayTypes.h"

class Item;

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
	Item* FindItem(const std::string& name);
	const std::vector<std::unique_ptr<Item>>& GetItems() const;
	const std::vector<Clue>& GetClues() const;
	bool HasClue(const std::string& description) const;
	int GetQuantity(const std::string& name) const;
	bool IsLocked(const std::string& name) const;
	void SetLocked(const std::string& name, bool locked);
	bool DiscardItem(const std::string& name, int amount = 1);
	std::vector<Item*> GetSortedItems(InventorySort sort, ItemStat focusStat = ItemStat::ATK) const;
	std::size_t GetItemCount() const;

private:
	std::vector<std::unique_ptr<Item>> items_;
	std::vector<int> quantities_;
	std::vector<bool> locked_;
	std::vector<unsigned long long> acquisitionOrder_;
	unsigned long long nextAcquisitionOrder_ = 0;
};
