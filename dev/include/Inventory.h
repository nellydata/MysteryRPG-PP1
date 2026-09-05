#pragma once

#include <vector>
#include <memory>
#include <string>
#include "Clue.h"

class Item;

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

private:
	std::vector<std::unique_ptr<Item>> items_;
};
