#include "Inventory.h"
#include "Item.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {
bool IsStoryItem(const Item& item) {
    return item.getCategory() == ItemCategory::Clue || item.getCategory() == ItemCategory::Crux;
}
}

void Inventory::AddClue(const Clue& clue) {
    if (!HasClue(clue.description)) clues.push_back(clue);
}

void Inventory::AddItem(std::unique_ptr<Item> item, int quantity) {
    if (!item) return;
    if (quantity <= 0 || (quantity != 1 && item->getCategory() != ItemCategory::Consumable))
        throw std::invalid_argument("Only consumables may have a stacked quantity; quantity must be positive.");
    if (Contains(item->getInstanceId())) throw std::invalid_argument("Item instance is already in this inventory.");
    // Preserve separate rolled/owned instances. A consumable entry can represent
    // a stack, but insertion never merges a new instance into an existing one.
    items_.push_back({std::move(item), quantity, false, ++nextAcquisitionOrder_});
}

bool Inventory::Contains(unsigned long long instanceId) const {
    for (const auto& entry : items_)
        if (entry.item && entry->getInstanceId() == instanceId) return true;
    return false;
}

bool Inventory::CanRemoveItem(unsigned long long instanceId, int amount) const {
    if (amount <= 0 || protectedItems_.count(instanceId)) return false;
    for (const auto& entry : items_)
        if (entry.item && entry->getInstanceId() == instanceId)
            return !entry.locked && !IsStoryItem(*entry.item) && entry.quantity >= amount;
    return false;
}

bool Inventory::RemoveItem(unsigned long long instanceId) {
    if (!CanRemoveItem(instanceId)) return false;
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if (it->item->getInstanceId() != instanceId) continue;
        if (--it->quantity == 0) items_.erase(it);
        return true;
    }
    return false;
}

bool Inventory::RemoveItem(const std::string& name) { return DiscardItem(name, 1); }

bool Inventory::DiscardItem(unsigned long long instanceId) {
    if (!CanRemoveItem(instanceId)) return false;
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if (it->item->getInstanceId() != instanceId) continue;
        items_.erase(it);
        return true;
    }
    return false;
}

bool Inventory::DiscardItem(const std::string& name, int amount) {
    if (amount <= 0) return false;
    std::int64_t removable = 0;
    for (const auto& entry : items_)
        if (entry.item && entry->getName() == name && CanRemoveItem(entry->getInstanceId()))
            removable += entry.quantity;
    if (removable < amount) return false;
    // Validate the full request first. Locked, equipped and story instances are
    // skipped; an insufficient eligible quantity leaves every entry untouched.
    for (auto it = items_.begin(); it != items_.end() && amount > 0;) {
        if (it->item->getName() != name || !CanRemoveItem(it->item->getInstanceId())) { ++it; continue; }
        const int removed = (std::min)(amount, it->quantity);
        it->quantity -= removed;
        amount -= removed;
        if (it->quantity == 0) it = items_.erase(it);
        else ++it;
    }
    return true;
}

int Inventory::GetQuantity(const std::string& name) const {
    std::int64_t quantity = 0;
    for (const auto& entry : items_)
        if (entry.item && entry->getName() == name) quantity += entry.quantity;
    return static_cast<int>((std::min)(quantity, static_cast<std::int64_t>((std::numeric_limits<int>::max)())));
}

int Inventory::GetQuantity(unsigned long long instanceId) const {
    for (const auto& entry : items_)
        if (entry.item && entry->getInstanceId() == instanceId) return entry.quantity;
    return 0;
}

bool Inventory::IsLocked(unsigned long long instanceId) const {
    for (const auto& entry : items_)
        if (entry.item && entry->getInstanceId() == instanceId) return entry.locked;
    return false;
}

bool Inventory::IsLocked(const std::string& name) const {
    for (const auto& entry : items_)
        if (entry.item && entry->getName() == name && entry.locked) return true;
    return false;
}

bool Inventory::SetLocked(unsigned long long instanceId, bool locked) {
    for (auto& entry : items_)
        if (entry.item && entry->getInstanceId() == instanceId) { entry.locked = locked; return true; }
    return false;
}

void Inventory::SetLocked(const std::string& name, bool locked) {
    for (auto& entry : items_)
        if (entry.item && entry->getName() == name) entry.locked = locked;
}

void Inventory::ProtectItem(unsigned long long instanceId) { if (Contains(instanceId)) protectedItems_.insert(instanceId); }
void Inventory::UnprotectItem(unsigned long long instanceId) { protectedItems_.erase(instanceId); }

std::vector<Item*> Inventory::GetSortedItems(InventorySort sort, ItemStat focusStat, const Stats& referenceStats) const {
    std::vector<const ItemEntry*> entries;
    for (const auto& entry : items_) if (entry.item) entries.push_back(&entry);
    std::stable_sort(entries.begin(), entries.end(), [&](const ItemEntry* leftEntry, const ItemEntry* rightEntry) {
        const Item* left = leftEntry->item.get();
        const Item* right = rightEntry->item.get();
        if (sort == InventorySort::RecentlyObtained) return leftEntry->acquisitionOrder > rightEntry->acquisitionOrder;
        if (sort == InventorySort::LeastRecentlyObtained) return leftEntry->acquisitionOrder < rightEntry->acquisitionOrder;
        if (sort == InventorySort::RarityAscending) return left->getRarity() < right->getRarity();
        if (sort == InventorySort::RarityDescending) return left->getRarity() > right->getRarity();
        if (sort == InventorySort::HighestStat)
            return left->GetStatContribution(focusStat, referenceStats) > right->GetStatContribution(focusStat, referenceStats);
        if (sort == InventorySort::AlphabeticalDescending) return left->getName() > right->getName();
        return left->getName() < right->getName();
    });
    std::vector<Item*> result;
    result.reserve(entries.size());
    for (const auto* entry : entries) result.push_back(entry->item.get());
    return result;
}

std::size_t Inventory::GetItemCount() const {
    std::size_t count = 0;
    for (const auto& entry : items_) count += static_cast<std::size_t>(entry.quantity);
    return count;
}

Item* Inventory::FindItem(const std::string& name) {
    for (auto& entry : items_) if (entry.item && entry->getName() == name) return entry.item.get();
    return nullptr;
}

Item* Inventory::FindItem(unsigned long long instanceId) {
    for (auto& entry : items_) if (entry.item && entry->getInstanceId() == instanceId) return entry.item.get();
    return nullptr;
}

Item* Inventory::FindUsableItem(const std::string& name) {
    for (auto& entry : items_)
        if (entry.item && entry->getName() == name && CanRemoveItem(entry->getInstanceId())) return entry.item.get();
    return nullptr;
}

const std::vector<ItemEntry>& Inventory::GetItems() const { return items_; }
const std::vector<Clue>& Inventory::GetClues() const { return clues; }
bool Inventory::HasClue(const std::string& description) const {
    for (const auto& clue : clues) if (clue.description == description) return true;
    return false;
}
