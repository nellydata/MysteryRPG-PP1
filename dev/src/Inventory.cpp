#include "Inventory.h"
#include "Item.h"
#include <algorithm>

void Inventory::AddClue(const Clue& clue) {
	clues.push_back(clue);
}

bool Inventory::RemoveItem(unsigned long long instanceId) {
	if (protectedItems_.count(instanceId) != 0) return false;
	for (std::size_t index = 0; index < items_.size(); ++index) {
		if (!items_[index].item || items_[index].item->getInstanceId() != instanceId) continue;
		if (items_[index].locked) return false;
		items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
		return true;
	}
	return false;
}

void Inventory::AddItem(std::unique_ptr<Item> item) {
	if (!item) return;
	items_.push_back({ std::move(item), 1, false, ++nextAcquisitionOrder_ });
}

bool Inventory::DiscardItem(unsigned long long instanceId) {
	if (protectedItems_.count(instanceId) != 0) return false;
	for (std::size_t index = 0; index < items_.size(); ++index) {
		if (!items_[index].item || items_[index].item->getInstanceId() != instanceId || items_[index].locked) continue;
		items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
		return true;
	}
	return false;
}

bool Inventory::RemoveItem(const std::string& name) {
	for (std::size_t index = 0; index < items_.size(); ++index) {
		if (items_[index].item && items_[index].item->getName() == name) {
			if (protectedItems_.count(items_[index].item->getInstanceId()) != 0) return false;
			if (items_[index].quantity > 1) {
				--items_[index].quantity;
			} else if (!items_[index].locked) {
				items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
			} else return false;
			return true;
		}
	}
	return false;
}

int Inventory::GetQuantity(const std::string& name) const {
	int quantity = 0;
	for (std::size_t index = 0; index < items_.size(); ++index)
		if (items_[index].item && items_[index].item->getName() == name) quantity += items_[index].quantity;
	return quantity;
}

bool Inventory::IsLocked(const std::string& name) const {
	for (std::size_t index = 0; index < items_.size(); ++index)
		if (items_[index].item && items_[index].item->getName() == name) return items_[index].locked;
	return false;
}

void Inventory::SetLocked(const std::string& name, bool locked) {
	for (std::size_t index = 0; index < items_.size(); ++index) {
		if (items_[index].item && items_[index].item->getName() == name) {
			items_[index].locked = locked;
			return;
		}
	}
}

bool Inventory::DiscardItem(const std::string& name, int amount) {
	if (amount <= 0 || IsLocked(name)) return false;
	for (std::size_t index = 0; index < items_.size(); ++index) {
		if (items_[index].item && items_[index].item->getName() == name) {
			if (protectedItems_.count(items_[index].item->getInstanceId()) != 0) return false;
			if (amount >= items_[index].quantity) {
				items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
			} else items_[index].quantity -= amount;
			return true;
		}
	}
	return false;
}

void Inventory::ProtectItem(unsigned long long instanceId) { protectedItems_.insert(instanceId); }
void Inventory::UnprotectItem(unsigned long long instanceId) { protectedItems_.erase(instanceId); }

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
	for (const auto& entry : items_) if (entry.item) result.push_back(entry.item.get());
	std::sort(result.begin(), result.end(), [&](const Item* left, const Item* right) {
		const auto findIndex = [&](const Item* item) {
			for (std::size_t index = 0; index < items_.size(); ++index)
				if (items_[index].item.get() == item) return index;
			return items_.size();
		};
		const std::size_t leftIndex = findIndex(left);
		const std::size_t rightIndex = findIndex(right);
		if (sort == InventorySort::RecentlyObtained) return items_[leftIndex].acquisitionOrder > items_[rightIndex].acquisitionOrder;
		if (sort == InventorySort::LeastRecentlyObtained) return items_[leftIndex].acquisitionOrder < items_[rightIndex].acquisitionOrder;
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
	for (const auto& entry : items_) count += static_cast<std::size_t>(entry.quantity);
	return count;
}

Item* Inventory::FindItem(const std::string& name) {
	for (auto& entry : items_) {
		if (entry.item && entry->getName() == name) return entry.item.get();
	}
	return nullptr;
}

const std::vector<ItemEntry>& Inventory::GetItems() const { return items_; }
const std::vector<Clue>& Inventory::GetClues() const { return clues; }

bool Inventory::HasClue(const std::string& description) const {
	for (const auto& clue : clues) {
		if (clue.description == description) return true;
	}
	return false;
}
