#include "ProgressionManager.h"
#include "ContentCatalog.h"
#include "Player.h"
#include "ShimmerSkill.h"
#include <algorithm>
#include <random>
#include <cmath>
#include <limits>

std::int64_t XPRequirementForLevel(int level) {
    if (level <= 1) return 100;
    std::int64_t requirement = 100;
    for (int current = 1; current < (std::min)(100, level); ++current)
        requirement = static_cast<std::int64_t>(std::llround(static_cast<double>(requirement) * 1.10));
    return requirement;
}

int InvestigationAreaDepth(const std::string& areaName) {
    if (areaName == "Archive Room") return 2;
    if (areaName == "Chapel") return 3;
    if (areaName == "Storage Room") return 4;
    return 1;
}

std::int64_t CalculateInvestigationXP(int playerLevel, int areaDepth) {
    const int level = (std::max)(1, (std::min)(100, playerLevel));
    const int depth = (std::max)(1, (std::min)(5, areaDepth));
    const std::int64_t requirement = XPRequirementForLevel(level);
    return (std::max)(std::int64_t{1}, requirement * (5 + depth) / 100);
}

std::int64_t CalculateEnemyXP(int enemyLevel, int enemyTier) {
    const int level = (std::max)(1, (std::min)(100, enemyLevel));
    const double baseShare = 0.10;
    static const double tierMultipliers[] = {1.0, 1.4, 1.9, 2.7, 4.0};
    const int tier = (std::max)(0, (std::min)(4, enemyTier));
    return (std::max)(std::int64_t{1}, static_cast<std::int64_t>(std::llround(
        static_cast<double>(XPRequirementForLevel(level)) * baseShare * tierMultipliers[tier])));
}

ProgressionManager::ProgressionManager()
    : level_(1), xp_(0), xpToNextLevel_(100) {}

void ProgressionManager::GrantXP(std::int64_t amount) {
    if (amount > 0 && level_ < 100)
        xp_ += (std::min)(amount, (std::numeric_limits<std::int64_t>::max)() - xp_);
}

void ProgressionManager::GrantClueXP() {
    GrantXP(CalculateInvestigationXP(level_, 1));
}

void ProgressionManager::GrantInvestigationXP(const std::string& areaName) {
    GrantXP(CalculateInvestigationXP(level_, InvestigationAreaDepth(areaName)));
}

bool ProgressionManager::UnequipShimmerSkill(const std::string& skillName, int cost) {
    const auto skill = ShimmerSkill::Create(skillName);
    return skill && cost == skill->getCost() && UnequipShimmerSkill(skillName);
}

bool ProgressionManager::UnequipShimmerSkill(const std::string& skillName) {
    return equippedShimmerSkills_.erase(skillName) != 0;
}

bool ProgressionManager::CheckLevelUp(Player& player) {
    if (!player.isAlive()) return false;
    bool leveledUp = false;

    while (level_ < 100 && xpToNextLevel_ > 0 && xp_ >= xpToNextLevel_) {
        xp_ -= xpToNextLevel_;
        ++level_;
        player.SetLevelStats(level_);
        player.RestoreHealth(player.getMaxHP());
        xpToNextLevel_ = XPRequirementForLevel(level_);
        QueueLevelUpChoice();
        AddLevelUpEquipmentRewards(player);
        levelUpMessages_.push_back("Level up! " + player.getName() + " is now level " + std::to_string(level_) + ". Five equipment rewards were added.");
        leveledUp = true;
    }

    return leveledUp;
}

bool ProgressionManager::HasPendingShimmerChoice() const {
    return !pendingLevelUpRewards_.empty();
}

const std::vector<std::string>& ProgressionManager::GetPendingShimmerChoices() const {
    static const std::vector<std::string> empty;
    return pendingLevelUpRewards_.empty() ? empty : pendingLevelUpRewards_.front().shimmerChoices;
}

bool ProgressionManager::SelectPendingShimmerSkill(const std::string& skillName) {
    if (!HasPendingShimmerChoice() ||
        std::find(GetPendingShimmerChoices().begin(), GetPendingShimmerChoices().end(), skillName) == GetPendingShimmerChoices().end() ||
        HasShimmerSkill(skillName)) return false;
    const std::string selected = skillName;
    if (!UnlockShimmerSkill(selected)) return false;
    pendingLevelUpRewards_.pop_front();
    levelUpMessages_.push_back("Shimmer skill unlocked: " + selected + ".");
    return true;
}

bool ProgressionManager::ResolvePendingShimmerChoice() {
    if (!HasPendingShimmerChoice() || !GetPendingShimmerChoices().empty()) return false;
    pendingLevelUpRewards_.pop_front();
    levelUpMessages_.push_back("All available Shimmer skills are unlocked.");
    return true;
}

std::vector<std::string> ProgressionManager::ConsumeLevelUpMessages() {
    std::vector<std::string> messages;
    messages.swap(levelUpMessages_);
    return messages;
}

void ProgressionManager::QueueLevelUpChoice() {
    const std::vector<std::string> names = BuildLatinShimmerSkillNames();
    std::vector<std::string> candidates;
    for (const auto& name : names) {
        if (!HasShimmerSkill(name) && ShimmerSkill::Create(name) != nullptr) candidates.push_back(name);
    }
    static std::mt19937 generator(0x4D494341u);
    std::shuffle(candidates.begin(), candidates.end(), generator);
    const std::size_t count = (std::min)(static_cast<std::size_t>(3), candidates.size());
    LevelUpReward reward;
    reward.levelReached = level_;
    reward.shimmerChoices.assign(candidates.begin(), candidates.begin() + static_cast<std::ptrdiff_t>(count));
    pendingLevelUpRewards_.push_back(std::move(reward));
}

void ProgressionManager::AddLevelUpEquipmentRewards(Player& player) {
    const auto catalog = BuildEquippableCatalog();
    static std::mt19937 generator(0x4D494342u);
    const auto rarityForLevel = [this]() {
        if (level_ >= 40) return Rarity::Epic;
        if (level_ >= 20) return Rarity::Rare;
        if (level_ >= 8) return Rarity::Uncommon;
        return Rarity::Common;
    };
    const Rarity rewardRarity = rarityForLevel();
    for (EquipSlot slot : {EquipSlot::Head, EquipSlot::Torso, EquipSlot::Hand, EquipSlot::Boots, EquipSlot::Accessory}) {
        std::vector<const ItemDefinition*> slotCandidates;
        for (const auto& definition : catalog) {
            if (definition.slot == slot && definition.rarity == rewardRarity) slotCandidates.push_back(&definition);
        }
        if (slotCandidates.empty()) continue;
        std::uniform_int_distribution<std::size_t> pick(0, slotCandidates.size() - 1);
        const ItemDefinition rolled = RollLootInstance(*slotCandidates[pick(generator)], generator());
        player.AddItem(std::make_unique<Item>(rolled));
    }
}

std::string ProgressionManager::RollShimmerSkill() {
    static std::mt19937 generator(0x4D494341u);
    static const std::vector<std::pair<int, int>> costs = {{2, 45}, {3, 30}, {4, 17}, {5, 8}};
    std::uniform_int_distribution<int> roll(1, 100);
    const int targetCost = [&]() {
        int value = roll(generator);
        for (const auto& entry : costs) {
            if (value <= entry.second) return entry.first;
            value -= entry.second;
        }
        return 2;
    }();
    std::vector<std::string> candidates;
    const std::vector<std::string> names = BuildLatinShimmerSkillNames();
    for (const auto& name : names) {
        auto skill = ShimmerSkill::Create(name);
        if (skill != nullptr && skill->getEquipCost() == targetCost && !HasShimmerSkill(name)) candidates.push_back(name);
    }
    if (candidates.empty()) {
        for (const auto& name : names) if (!HasShimmerSkill(name)) return name;
        return {};
    }
    return candidates[static_cast<std::size_t>(roll(generator) % candidates.size())];
}

int ProgressionManager::GetLevel() const { return level_; }
std::int64_t ProgressionManager::GetXP() const { return xp_; }
std::int64_t ProgressionManager::GetXPToNextLevel() const { return xpToNextLevel_; }
void ProgressionManager::SetMaxLevel(Player& player) {
    level_ = 100;
    xp_ = 0;
    // Keep the threshold valid so future XP rewards cannot create an endless
    // level-up loop while the player remains at the completion-mode level.
    xpToNextLevel_ = 100;
    player.SetLevelStats(level_);
    player.heal(player.getMaxHP());
}

bool ProgressionManager::UnlockCrux(const std::string& itemName) {
    return !itemName.empty() && cruxItems_.insert(itemName).second;
}

bool ProgressionManager::HasCrux(const std::string& itemName) const {
    return cruxItems_.find(itemName) != cruxItems_.end();
}

bool ProgressionManager::UnlockShimmerSkill(const std::string& skillName) {
    const std::string selected = skillName;
    if (!ShimmerSkill::Create(selected) || !shimmerSkills_.insert(selected).second) return false;
    for (auto& reward : pendingLevelUpRewards_) {
        auto& choices = reward.shimmerChoices;
        choices.erase(std::remove(choices.begin(), choices.end(), selected), choices.end());
        if (choices.empty()) {
            for (const auto& candidate : BuildLatinShimmerSkillNames()) {
                if (!HasShimmerSkill(candidate) && ShimmerSkill::Create(candidate)) choices.push_back(candidate);
                if (choices.size() == 3) break;
            }
        }
    }
    return true;
}

bool ProgressionManager::HasShimmerSkill(const std::string& skillName) const {
    return shimmerSkills_.find(skillName) != shimmerSkills_.end();
}

std::vector<std::string> ProgressionManager::GetUnlockedShimmerSkills() const {
    return std::vector<std::string>(shimmerSkills_.begin(), shimmerSkills_.end());
}

std::vector<std::string> ProgressionManager::GetEquippedShimmerSkills() const {
    return std::vector<std::string>(equippedShimmerSkills_.begin(), equippedShimmerSkills_.end());
}

int ProgressionManager::GetShimmerPoints() const {
    int used = 0;
    for (const auto& name : equippedShimmerSkills_)
        if (const auto skill = ShimmerSkill::Create(name)) used += skill->getCost();
    return 12 - used;
}
bool ProgressionManager::EquipShimmerSkill(const std::string& skillName) {
    const auto skill = ShimmerSkill::Create(skillName);
    if (!skill || !HasShimmerSkill(skillName) || skill->getCost() <= 0 ||
        skill->getCost() > GetShimmerPoints()) return false;
    return equippedShimmerSkills_.insert(skillName).second;
}
bool ProgressionManager::EquipShimmerSkill(const std::string& skillName, int cost) {
    const auto skill = ShimmerSkill::Create(skillName);
    return skill && cost == skill->getCost() && EquipShimmerSkill(skillName);
}

bool ProgressionManager::UnlockShimmerFromTrigger(const std::string& trigger) {
    static const std::pair<const char*, const char*> unlocks[] = {
        {"mirror", "Claustrum Lucis"}, {"Matron of the Catacombs", "Curatio Refractionis"},
        {"Catacombs", "Praesidium Lucis"}, {"corpse", "Siphon Lucis"},
        {"statue", "Corruptio Lucis"}, {"Overlord-Prime", "Refractio Ultima"},
        {"light-spear", "Lumen Spiculum"}, {"relic", "Purgatio Lucis"},
        {"vortex", "Vortex Lucis"}, {"Oracle Core", "Lorica Lucis"}
    };
    for (const auto& unlock : unlocks) if (trigger.find(unlock.first) != std::string::npos) return UnlockShimmerSkill(unlock.second);
    return false;
}
