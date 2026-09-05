#include "Inventory.h"
#include "Item.h"
#include <algorithm>

void Inventory::AddClue(const Clue& clue) {
	clues.push_back(clue);
}

void Inventory::AddItem(std::unique_ptr<Item> item) {
	if (!item) return;
	for (std::size_t index = 0; index < items_.size(); ++index) {
		if (items_[index] && items_[index]->getName() == item->getName()) {
			++quantities_[index];
			acquisitionOrder_[index] = ++nextAcquisitionOrder_;
			return;
		}
	}
	items_.push_back(std::move(item));
	quantities_.push_back(1);
	locked_.push_back(false);
	acquisitionOrder_.push_back(++nextAcquisitionOrder_);
}

bool Inventory::RemoveItem(const std::string& name) {
	for (std::size_t index = 0; index < items_.size(); ++index) {
		if (items_[index] && items_[index]->getName() == name) {
			if (quantities_[index] > 1) {
				--quantities_[index];
			} else if (!locked_[index]) {
				items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
				quantities_.erase(quantities_.begin() + static_cast<std::ptrdiff_t>(index));
				locked_.erase(locked_.begin() + static_cast<std::ptrdiff_t>(index));
				acquisitionOrder_.erase(acquisitionOrder_.begin() + static_cast<std::ptrdiff_t>(index));
			} else return false;
			return true;
		}
	}
	return false;
}

int Inventory::GetQuantity(const std::string& name) const {
	for (std::size_t index = 0; index < items_.size(); ++index)
		if (items_[index] && items_[index]->getName() == name) return quantities_[index];
	return 0;
}

bool Inventory::IsLocked(const std::string& name) const {
	for (std::size_t index = 0; index < items_.size(); ++index)
		if (items_[index] && items_[index]->getName() == name) return locked_[index];
	return false;
}

void Inventory::SetLocked(const std::string& name, bool locked) {
	for (std::size_t index = 0; index < items_.size(); ++index) {
		if (items_[index] && items_[index]->getName() == name) {
			locked_[index] = locked;
			return;
		}
	}
}

bool Inventory::DiscardItem(const std::string& name, int amount) {
	if (amount <= 0 || IsLocked(name)) return false;
	for (std::size_t index = 0; index < items_.size(); ++index) {
		if (items_[index] && items_[index]->getName() == name) {
			if (amount >= quantities_[index]) {
				items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
				quantities_.erase(quantities_.begin() + static_cast<std::ptrdiff_t>(index));
				locked_.erase(locked_.begin() + static_cast<std::ptrdiff_t>(index));
				acquisitionOrder_.erase(acquisitionOrder_.begin() + static_cast<std::ptrdiff_t>(index));
			} else quantities_[index] -= amount;
			return true;
		}
	}
	return false;
}

namespace {
int FocusValue(const Stats& stats, ItemStat stat) {
	switch (stat) {
	case ItemStat::HP: return stats.hp;
	case ItemStat::DEF: return stats.def;
	case ItemStat::SPD: return stats.spd;
	case ItemStat::ATK: return stats.atk;
	case ItemStat::Accuracy: return stats.acc;
	case ItemStat::Resistance: return stats.res;
	case ItemStat::CritRate: return stats.critRate;
	case ItemStat::CritDamage: return stats.critDamage;
	case ItemStat::Evasion: return stats.evasion;
	case ItemStat::HealingPotency: return stats.healingPotency;
	case ItemStat::BarrierPotency: return stats.barrierPotency;
	case ItemStat::DotPotency: return stats.dotPotency;
	case ItemStat::Lifesteal: return stats.lifesteal;
	case ItemStat::DebuffDurationReduction: return stats.debuffDurationReduction;
	}
	return 0;
}
}

std::vector<Item*> Inventory::GetSortedItems(InventorySort sort, ItemStat focusStat) const {
	std::vector<Item*> result;
	for (const auto& item : items_) if (item) result.push_back(item.get());
	std::sort(result.begin(), result.end(), [&](const Item* left, const Item* right) {
		const auto findIndex = [&](const Item* item) {
			for (std::size_t index = 0; index < items_.size(); ++index)
				if (items_[index].get() == item) return index;
			return items_.size();
		};
		const std::size_t leftIndex = findIndex(left);
		const std::size_t rightIndex = findIndex(right);
		if (sort == InventorySort::RecentlyObtained) return acquisitionOrder_[leftIndex] > acquisitionOrder_[rightIndex];
		if (sort == InventorySort::LeastRecentlyObtained) return acquisitionOrder_[leftIndex] < acquisitionOrder_[rightIndex];
		if (sort == InventorySort::RarityAscending) return static_cast<int>(left->getRarity()) < static_cast<int>(right->getRarity());
		if (sort == InventorySort::RarityDescending) return static_cast<int>(left->getRarity()) > static_cast<int>(right->getRarity());
		if (sort == InventorySort::HighestStat) return FocusValue(left->getStats(), focusStat) > FocusValue(right->getStats(), focusStat);
		if (sort == InventorySort::AlphabeticalDescending) return left->getName() > right->getName();
		return left->getName() < right->getName();
	});
	return result;
}

std::size_t Inventory::GetItemCount() const {
	std::size_t count = 0;
	for (int quantity : quantities_) count += static_cast<std::size_t>(quantity);
	return count;
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
