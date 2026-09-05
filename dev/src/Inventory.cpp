#include "Inventory.h"
#include "Item.h"

void Inventory::AddClue(const Clue& clue) {
	clues.push_back(clue);
}

void Inventory::AddItem(std::unique_ptr<Item> item) {
	if (item) items_.push_back(std::move(item));
}

bool Inventory::RemoveItem(const std::string& name) {
	for (auto it = items_.begin(); it != items_.end(); ++it) {
		if (*it && (*it)->getName() == name) {
			items_.erase(it);
			return true;
		}
	}
	return false;
}

Item* Inventory::FindItem(const std::string& name) {
	for (auto& item : items_) {
		if (item && item->getName() == name) return item.get();
	}
	return nullptr;
}

const std::vector<std::unique_ptr<Item>>& Inventory::GetItems() const { return items_; }
const std::vector<Clue>& Inventory::GetClues() const { return clues; }

bool Inventory::HasClue(const std::string& description) const {
	for (const auto& clue : clues) {
		if (clue.description == description) return true;
	}
	return false;
}
