#pragma once

#include "Inventory.h"
#include "Player.h"
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

namespace InventoryCorrectnessValidation {

inline ItemDefinition Definition(const std::string& name, ItemCategory category = ItemCategory::Consumable) {
    ItemDefinition result;
    result.name = name;
    result.category = category;
    if (category == ItemCategory::Equippable) {
        result.slot = EquipSlot::Hand;
        result.mainStat = "atk";
        result.mainStatPercent = 10;
    }
    return result;
}

inline unsigned long long Add(Inventory& inventory, const std::string& name, int quantity = 1,
                             ItemCategory category = ItemCategory::Consumable) {
    auto item = std::make_unique<Item>(Definition(name, category));
    const auto id = item->getInstanceId();
    inventory.AddItem(std::move(item), quantity);
    return id;
}

inline std::map<unsigned long long, int> Quantities(const Inventory& inventory) {
    std::map<unsigned long long, int> result;
    for (const auto& entry : inventory.GetItems()) {
        assert(entry.item && entry.quantity > 0);
        assert(result.emplace(entry.item->getInstanceId(), entry.quantity).second);
    }
    return result;
}

inline void StackRemoval() {
    Inventory inventory;
    const auto first = Add(inventory, "stack", 3);
    const auto second = Add(inventory, "stack", 4);
    assert(first != second && inventory.GetItems().size() == 2);
    assert(inventory.GetQuantity("stack") == 7 && inventory.GetItemCount() == 7);
    assert(inventory.Contains(first) && inventory.Contains(second));
    assert(inventory.CanRemoveItem(first, 3) && !inventory.CanRemoveItem(first, 4));
    assert(!inventory.CanRemoveItem(first, 0) && !inventory.CanRemoveItem(first, -1));
    assert(inventory.RemoveItem(first));
    assert(inventory.Contains(first) && Quantities(inventory).at(first) == 2);
    assert(inventory.GetQuantity("stack") == 6 && inventory.GetItemCount() == 6);
    assert(inventory.DiscardItem(first));
    assert(!inventory.Contains(first) && inventory.GetQuantity("stack") == 4);
    assert(inventory.RemoveItem("stack"));
    assert(inventory.Contains(second) && Quantities(inventory).at(second) == 3);
    assert(inventory.DiscardItem("stack", 3));
    assert(inventory.GetItems().empty() && inventory.GetItemCount() == 0);
    assert(!inventory.Contains(second) && !inventory.RemoveItem(second));
    assert(!inventory.DiscardItem("missing", 1) && !inventory.FindUsableItem("missing"));
}

inline void AtomicBulkRemoval() {
    Inventory inventory;
    const auto locked = Add(inventory, "shared", 2);
    const auto usable = Add(inventory, "shared", 3);
    const auto protectedId = Add(inventory, "shared", 4);
    const auto usableSecond = Add(inventory, "shared", 2);
    inventory.SetLocked(locked, true);
    inventory.ProtectItem(protectedId);
    assert(inventory.IsLocked("shared") && inventory.IsLocked(locked));
    assert(!inventory.IsLocked(usable) && !inventory.IsLocked(protectedId));
    assert(inventory.FindUsableItem("shared")->getInstanceId() == usable);
    assert(!inventory.CanRemoveItem(locked) && !inventory.CanRemoveItem(protectedId));
    const auto before = Quantities(inventory);
    assert(!inventory.DiscardItem("shared", 6));
    assert(Quantities(inventory) == before && inventory.GetQuantity("shared") == 11);
    assert(!inventory.DiscardItem("shared", 0) && !inventory.DiscardItem("shared", -1));
    assert(Quantities(inventory) == before);
    assert(inventory.DiscardItem("shared", 4));
    assert(inventory.GetQuantity("shared") == 7);
    const auto after = Quantities(inventory);
    assert(after.at(locked) == 2 && after.at(protectedId) == 4);
    int removableRemaining = 0;
    for (const auto& entry : inventory.GetItems())
        if (entry.item->getInstanceId() == usable || entry.item->getInstanceId() == usableSecond)
            removableRemaining += entry.quantity;
    assert(removableRemaining == 1);
    assert(inventory.RemoveItem("shared"));
    assert(inventory.GetQuantity("shared") == 6 && !inventory.FindUsableItem("shared"));
    assert(!inventory.RemoveItem("shared") && !inventory.RemoveItem(locked));
    assert(!inventory.DiscardItem(protectedId));
    inventory.SetLocked(locked, false);
    assert(!inventory.IsLocked("shared"));
    assert(inventory.FindUsableItem("shared")->getInstanceId() == locked);
    assert(inventory.RemoveItem("shared") && Quantities(inventory).at(locked) == 1);
    inventory.UnprotectItem(protectedId);
    assert(inventory.DiscardItem("shared", 5));
    assert(inventory.GetItems().empty());
}

inline void ExactAndNameWideLocks() {
    Inventory inventory;
    const auto first = Add(inventory, "twins", 2);
    const auto second = Add(inventory, "twins", 3);
    inventory.SetLocked(second, true);
    assert(inventory.IsLocked("twins") && !inventory.IsLocked(first) && inventory.IsLocked(second));
    assert(inventory.FindUsableItem("twins")->getInstanceId() == first);
    assert(!inventory.RemoveItem(second) && Quantities(inventory).at(second) == 3);
    inventory.SetLocked("twins", true);
    assert(inventory.IsLocked(first) && inventory.IsLocked(second));
    const auto before = Quantities(inventory);
    assert(!inventory.RemoveItem("twins") && !inventory.DiscardItem(first));
    assert(Quantities(inventory) == before);
    inventory.SetLocked("twins", false);
    assert(!inventory.IsLocked(first) && !inventory.IsLocked(second) && !inventory.IsLocked("twins"));
    assert(inventory.RemoveItem(second) && Quantities(inventory).at(second) == 2);
    assert(Quantities(inventory).at(first) == 2);
}

inline void IntrinsicStoryProtection() {
    Inventory inventory;
    for (const auto category : {ItemCategory::Clue, ItemCategory::Crux}) {
        const std::string name = category == ItemCategory::Clue ? "story clue" : "story crux";
        const auto id = Add(inventory, name, 1, category);
        inventory.ProtectItem(id);
        inventory.UnprotectItem(id);
        inventory.SetLocked(id, false);
        assert(inventory.Contains(id) && !inventory.CanRemoveItem(id));
        assert(!inventory.RemoveItem(id) && !inventory.DiscardItem(id));
        assert(!inventory.RemoveItem(name) && !inventory.DiscardItem(name, 1));
        assert(inventory.GetQuantity(name) == 1 && !inventory.FindUsableItem(name));
    }
}

inline void RejectEquipmentStacks() {
    Inventory inventory;
    Add(inventory, "existing", 2);
    const auto before = Quantities(inventory);
    bool rejected = false;
    try { inventory.AddItem(std::make_unique<Item>(Definition("invalid equipment stack", ItemCategory::Equippable)), 2); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected && Quantities(inventory) == before);
    const auto id = Add(inventory, "valid equipment", 1, ItemCategory::Equippable);
    assert(inventory.Contains(id) && Quantities(inventory).at(id) == 1);
}

inline void EquipmentOwnershipAndPointerStability() {
    Player player;
    const auto definition = Definition("owned hand item", ItemCategory::Equippable);
    Item external(definition);
    assert(!player.Equip(external) && !player.ToggleEquip(external));
    assert(!player.GetEquipped(EquipSlot::Hand));
    auto owned = std::make_unique<Item>(definition);
    Item* ownedPointer = owned.get();
    const auto ownedId = ownedPointer->getInstanceId();
    player.AddItem(std::move(owned));
    assert(player.Equip(*ownedPointer));
    assert(player.GetEquipped(EquipSlot::Hand) == ownedPointer);
    assert(!player.GetInventory().CanRemoveItem(ownedId));
    assert(!player.GetInventory().RemoveItem(ownedId) && !player.GetInventory().DiscardItem(ownedId));
    assert(!player.GetInventory().DiscardItem(definition.name, 1));
    for (int index = 0; index < 160; ++index)
        Add(player.GetInventory(), "reallocation item " + std::to_string(index));
    assert(player.GetInventory().Contains(ownedId));
    assert(player.GetEquipped(EquipSlot::Hand) == ownedPointer && ownedPointer->getName() == definition.name);
    for (const auto sort : {InventorySort::RecentlyObtained, InventorySort::LeastRecentlyObtained,
                           InventorySort::AlphabeticalAscending, InventorySort::AlphabeticalDescending,
                           InventorySort::RarityAscending, InventorySort::RarityDescending, InventorySort::HighestStat}) {
        const auto view = player.GetInventory().GetSortedItems(sort);
        int occurrences = 0;
        for (const Item* item : view) occurrences += item == ownedPointer;
        assert(occurrences == 1 && player.GetEquipped(EquipSlot::Hand) == ownedPointer);
    }
    auto replacement = std::make_unique<Item>(Definition("replacement hand item", ItemCategory::Equippable));
    Item* replacementPointer = replacement.get();
    const auto replacementId = replacementPointer->getInstanceId();
    player.AddItem(std::move(replacement));
    assert(player.Equip(*replacementPointer));
    assert(player.GetInventory().CanRemoveItem(ownedId));
    assert(player.GetInventory().DiscardItem(ownedId));
    assert(!player.GetInventory().Contains(ownedId));
    assert(player.GetEquipped(EquipSlot::Hand) == replacementPointer);
    assert(replacementPointer->getInstanceId() == replacementId);
    assert(player.ToggleEquip(*replacementPointer));
    assert(!player.GetEquipped(EquipSlot::Hand));
    assert(player.GetInventory().DiscardItem(replacementId));
    assert(!player.GetInventory().Contains(replacementId));
}
}

inline void TestInventoryCorrectness() {
    using namespace InventoryCorrectnessValidation;
    StackRemoval();
    AtomicBulkRemoval();
    ExactAndNameWideLocks();
    IntrinsicStoryProtection();
    RejectEquipmentStacks();
    EquipmentOwnershipAndPointerStability();
    std::cout << "Inventory correctness regressions passed.\n";
}
