#include "CombatManager.h"
#include "Enemy.h"
#include "Player.h"
#include "ProgressionManager.h"
#include <cmath>
#include "ShimmerSkill.h"
#include "CombatMath.h"
#include "ConsumableEffects.h"
#include <algorithm>
#include <string>
#include <memory>

namespace {
const char* TierDisplayName(EnemyTier tier) {
    switch (tier) {
    case EnemyTier::Basic: return "Basic";
    case EnemyTier::Rare: return "Rare";
    case EnemyTier::Epic: return "Epic";
    case EnemyTier::Elite: return "Elite";
    case EnemyTier::Boss: return "Boss";
    }
    return "Unknown";
}

const char* StatusDisplayName(StatusType type) {
    switch (type) {
    case StatusType::Bleed: return "Bleed";
    case StatusType::Burn: return "Burn";
    case StatusType::Poison: return "Poison";
    case StatusType::Stun: return "Stun";
    case StatusType::HealOverTime: return "Healing-over-time";
    case StatusType::Slow: return "Slow";
    case StatusType::VerdigrisMark: return "Verdigris";
    case StatusType::DamageReduction: return "Damage Reduction";
    case StatusType::Barrier: return "Barrier";
    case StatusType::AttackUp: return "Attack Up";
    case StatusType::AttackDown: return "Attack Down";
    case StatusType::DefenseUp: return "Defense Up";
    case StatusType::DefenseDown: return "Defense Down";
    case StatusType::SpeedUp: return "Speed Up";
    case StatusType::AccuracyDown: return "Accuracy Down";
    case StatusType::HealingPotencyUp: return "Healing Potency Up";
    case StatusType::AccuracyUp: return "Accuracy Up";
    case StatusType::CritUp: return "Critical Rate Up";
    case StatusType::CritDamageUp: return "Critical Damage Up";
    case StatusType::GuaranteedCrit: return "Guaranteed Critical";
    case StatusType::DotUp: return "DOT Up";
    case StatusType::DotReduction: return "DOT Reduction";
    case StatusType::HealResistance: return "Healing Resistance";
    case StatusType::AccuracyFloor: return "Accuracy Floor";
    }
    return "Status";
}

void AccumulateGaugesUntilReady(int& playerGauge, int& enemyGauge,
                                int playerSpeed, int enemySpeed) {
    for (int step = 0; step < 10000 && playerGauge < 1000 && enemyGauge < 1000; ++step) {
        playerGauge = CombatMath::Saturate(static_cast<std::int64_t>(playerGauge) + (std::max)(1, playerSpeed));
        enemyGauge = CombatMath::Saturate(static_cast<std::int64_t>(enemyGauge) + (std::max)(1, enemySpeed));
    }
}
}

void CombatManager::AdvanceTurn(Player& player, Enemy& enemy) {
    if (groupEnemyTurn_ || !player.isAlive()) return;
    if (!currentEnemyParty_.empty()) {
        PrepareGroupTurn(player, currentEnemyParty_);
        return;
    }
    if (!enemy.isAlive()) return;
    AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
    playerTurn_ = playerGauge_ >= 1000;
    if (playerTurn_ && !playerActionStarted_) StartPlayerTurn(player);
}

void CombatManager::FinishPlayerAction(Player& player, Enemy& enemy) {
    player.EndStatusTurn();
    playerActionStarted_ = false;
    ++turnNumber_;
    playerGauge_ = (std::max)(0, playerGauge_ - 1000);
    playerTurn_ = false;
    AdvanceTurn(player, enemy);
}

std::int64_t CombatManager::DeliverDefeatXP(Player& player, Enemy& enemy, ProgressionManager& progression) {
    if (!player.isAlive()) {
        enemy.ForfeitRewards();
        return 0;
    }
    if (!enemy.CanReceiveDefeatRewards() || enemy.HasClaimedXPReward()) return 0;
    const auto before = progression.GetXP();
    progression.GrantXP(enemy.getXPReward());
    const auto accepted = progression.GetXP() - before;
    // Delivery occurs before the claim. Mark before level-up item generation so
    // an allocation failure there cannot cause the already-delivered XP to repeat.
    enemy.ClaimXPReward();
    progression.CheckLevelUp(player);
    return accepted;
}

void CombatManager::ResolveDefeatReward(Player& player, Enemy& enemy, ProgressionManager* progression) {
    if (!player.isAlive()) {
        enemy.ForfeitRewards();
        return;
    }
    if (progression == nullptr) return;
    const auto accepted = DeliverDefeatXP(player, enemy, *progression);
    if (accepted > 0) AddEvent(CombatEvent::Kind::Reward, "XP gained: +" + std::to_string(accepted) + ".");
}

void CombatManager::ForfeitEncounterRewards() {
    if (activeEnemy_ != nullptr) activeEnemy_->ForfeitRewards();
    for (Enemy* enemy : currentEnemyParty_) if (enemy != nullptr) enemy->ForfeitRewards();
}

void CombatManager::StartPlayerTurn(Player& player) {
    if (!player.isAlive() || playerActionStarted_) return;
    playerActionStarted_ = true;
    consumableUsedThisTurn_ = false;
    for (auto& cooldown : shimmerCooldowns_) if (cooldown.second > 0) --cooldown.second;
    if (defaultSkill_ != nullptr) defaultSkill_->TickCooldown();
    if (player.HasActiveSet("Verdigris Matron")) {
        const int before = player.getHP();
        player.heal(CombatMath::Scale(player.getMaxHP(), 25));
        const int amount = player.getHP() - before;
        if (amount > 0) AddEvent(CombatEvent::Kind::Heal, player.getName() + " restores " + std::to_string(amount) + " HP from Verdigris Matron.", amount);
    }
    auto& turn = AddEvent(CombatEvent::Kind::Turn, player.getName() + "'s action begins (gauge " + std::to_string(playerGauge_) + ").");
    turn.actor = player.getName();
    const bool stunned = player.HasStatus(StatusType::Stun);
    std::vector<std::pair<StatusType, int>> playerDamageEvents;
    int playerHealing = 0;
    player.BeginStatusTurn(playerDamageEvents, playerHealing);
    for (const auto& tick : player.GetStatusDamageResults())
        RecordDamage(tick.source.empty() ? StatusDisplayName(tick.type) : tick.source, player.getName(), StatusDisplayName(tick.type), tick.damage, CombatEvent::Kind::DotTick);
    if (playerHealing > 0) AddEvent(CombatEvent::Kind::Heal, player.getName() + " heals " + std::to_string(playerHealing) + " HP from Heal-over-time.", playerHealing);
    if (!player.isAlive()) {
        ForfeitEncounterRewards();
        AddEvent(CombatEvent::Kind::Death, player.getName() + " has fallen.");
    }
    if (stunned && player.isAlive()) {
        AddEvent(CombatEvent::Kind::Info, player.getName() + " loses the turn to Stun.");
        playerGauge_ -= 1000;
        playerTurn_ = false;
        player.EndStatusTurn();
        playerActionStarted_ = false;
        ++turnNumber_;
        return;
    }
}

CombatManager::CombatManager(unsigned seed)
    : blocked_(false), weaknessExposed_(false),
      turnNumber_(0), playerTurn_(true), activeEnemy_(nullptr),
      playerGauge_(0), enemyGauge_(0), random_(seed),
      consumableUsedThisTurn_(false),
      defaultSkill_(ShimmerSkill::Create("Pulsus Luminis")) {}

CombatManager::~CombatManager() = default;
void CombatManager::SetSeed(unsigned seed) {
    if (activeEnemy_ != nullptr) return;
    random_.Seed(seed);
}

const ShimmerSkill* CombatManager::GetDefaultSkill() const { return defaultSkill_.get(); }

const std::vector<CombatManager::CombatEvent>& CombatManager::GetEvents() const { return events_; }
void CombatManager::ClearEvents() { if (activeEnemy_ == nullptr) events_.clear(); }
void CombatManager::ResetEncounter() {
    activeEnemy_ = nullptr;
    activePlayer_ = nullptr;
    playerActionStarted_ = false;
    nextEnemyBonus_ = false;
    blocked_ = false;
    weaknessExposed_ = false;
    turnNumber_ = 0;
    playerTurn_ = true;
    playerGauge_ = 0;
    enemyGauge_ = 0;
    consumableUsedThisTurn_ = false;
    battleStartEffectsApplied_ = false;
    groupEnemyTurn_ = false;
    currentEnemyParty_.clear();
    seededEnemies_.clear();
    shimmerCooldowns_.clear();
    nextPartyEnemy_ = 0;
}
void CombatManager::RecordInfo(const std::string& message) { AddEvent(CombatEvent::Kind::Info, message); }
void CombatManager::RecordReward(const std::string& message) { AddEvent(CombatEvent::Kind::Reward, message); }
void CombatManager::RecordEnvironmentalDefeat(const std::string& source, const Player& player) {
    if (player.isAlive()) return;
    // A lethal exploration hazard starts a new history after the prior fight.
    // Use the damage result captured by Player at the actual HP mutation.
    ResetEncounter();
    events_.clear();
    encounterCompleted_ = false;
    AddEvent(CombatEvent::Kind::EncounterStart, "Exploration hazard: " + source + ".");
    RecordDamage(source, player.getName(), source, player.GetLastDamageResult(), CombatEvent::Kind::Environment);
    AddEvent(CombatEvent::Kind::Death, player.getName() + " has fallen.");
    CompleteEncounter("defeat");
}
void CombatManager::CompleteEncounter(const std::string& outcome) {
    if (encounterCompleted_) return;
    if (outcome == "fled" || outcome == "defeat") ForfeitEncounterRewards();
    encounterCompleted_ = true;
    AddEvent(CombatEvent::Kind::Completion, "Encounter complete: " + outcome + ".");
}
int CombatManager::GetSkillCooldown(const std::string& skillName) const {
    const auto found = shimmerCooldowns_.find(skillName);
    return found == shimmerCooldowns_.end() ? 0 : found->second;
}
CombatManager::CombatEvent& CombatManager::AddEvent(CombatEvent::Kind kind, const std::string& message, int value) {
    CombatEvent event{kind, message, value};
    event.sequenceId = nextEventSequence_++;
    event.playerGauge = playerGauge_;
    event.enemyGauge = enemyGauge_;
    events_.push_back(std::move(event));
    return events_.back();
}

void CombatManager::RecordDamage(const std::string& actor, const std::string& target,
                                const std::string& skill, const DamageResolution& result,
                                CombatEvent::Kind kind) {
    auto& event = AddEvent(kind, actor + " -> " + target + ": " + std::to_string(result.requestedDamage) +
        " requested damage; mitigated " + std::to_string(result.mitigatedDamage) +
        "; barrier absorbs " + std::to_string(result.barrierAbsorbed) + "; " +
        std::to_string(result.hpDamage) + " actual HP damage. HP: " + std::to_string(result.remainingHP) + ".", result.hpDamage);
    event.actor = actor;
    event.target = target;
    event.skill = skill;
    event.requestedDamage = result.requestedDamage;
    event.mitigatedDamage = result.mitigatedDamage;
    event.overkillDamage = result.overkillDamage;
    event.barrierAbsorbed = result.barrierAbsorbed;
    event.hpDamage = result.hpDamage;
    event.remainingHP = result.remainingHP;
    if (result.barrierBroken) AddEvent(CombatEvent::Kind::BarrierBreak, target + "'s barrier breaks.");
    if (result.healing > 0) AddEvent(CombatEvent::Kind::Heal, target + " recovers " + std::to_string(result.healing) + " HP from its defensive effect.", result.healing);
    if (result.barrierGained > 0) AddEvent(CombatEvent::Kind::Shield, target + " gains " + std::to_string(result.barrierGained) + " barrier from its defensive effect.", result.barrierGained);
}

void CombatManager::PrepareTurn(Player& player, Enemy& enemy) {
    if (activeEnemy_ == nullptr) BeginEncounter(enemy);
    if (activeEnemy_ != &enemy || encounterCompleted_ || !player.isAlive() || !enemy.isAlive()) return;
    if (activePlayer_ != nullptr && activePlayer_ != &player) return;
    if (activePlayer_ == nullptr) { activePlayer_ = &player; player.SetStatusSeed(random_.Next()); }
    if (!battleStartEffectsApplied_) {
        const int before = player.GetBarrier();
        player.StartBattleEffects();
        battleStartEffectsApplied_ = true;
        const int gained = player.GetBarrier() - before;
        if (gained > 0) AddEvent(CombatEvent::Kind::Shield, player.getName() + " gains " + std::to_string(gained) + " barrier at battle start.", gained);
    }
    if (playerGauge_ == 0 && enemyGauge_ == 0 && !playerTurn_) {
        AdvanceTurn(player, enemy);
    }
    if (playerTurn_ && player.isAlive()) AddEvent(CombatEvent::Kind::Info, "Waiting for " + player.getName() + " to choose an action.");
}

void CombatManager::ResumeAfterRespawn(Player& player, Enemy& enemy) {
    if (!player.isAlive() || !enemy.isAlive() || encounterCompleted_ || activePlayer_ != &player || activeEnemy_ != &enemy) return;
    playerGauge_ = 1000;
    enemyGauge_ = 0;
    playerTurn_ = true;
    playerActionStarted_ = false;
    consumableUsedThisTurn_ = false;
    StartPlayerTurn(player);
    AddEvent(CombatEvent::Kind::Info, player.getName() + " returns from the fountain and may act.");
}

void CombatManager::BeginEncounter(Enemy& enemy) {
    if (activeEnemy_ == &enemy) return;
    activeEnemy_ = &enemy;
    nextEnemyBonus_ = false;
    activePlayer_ = nullptr;
    playerActionStarted_ = false;
    enemy.SetStatusSeed(random_.Next());
    seededEnemies_.clear();
    seededEnemies_.insert(&enemy);
    blocked_ = false;
    turnNumber_ = 0;
    playerGauge_ = 0;
    enemyGauge_ = 0;
    playerTurn_ = false;
    weaknessExposed_ = false;
    consumableUsedThisTurn_ = false;
    shimmerCooldowns_.clear();
    events_.clear();
    battleStartEffectsApplied_ = false;
    encounterCompleted_ = false;
    groupEnemyTurn_ = false;
    nextPartyEnemy_ = 0;
    currentEnemyParty_.clear();
    auto& start = AddEvent(CombatEvent::Kind::EncounterStart, "Encounter begins: " + enemy.getName() + " (level " + std::to_string(enemy.getLevel()) + ", " + TierDisplayName(enemy.getTier()) + ").");
    start.target = enemy.getName();
    start.targetLevel = enemy.getLevel();
    start.enemyTier = enemy.getTier();
}

void CombatManager::BeginEncounter(Player& player, Enemy& enemy, int playerLevel) {
    if (activeEnemy_ == &enemy && activePlayer_ == &player) return;
    BeginEncounter(enemy);
    activePlayer_ = &player;
    player.SetStatusSeed(random_.Next());
    auto& participants = AddEvent(CombatEvent::Kind::EncounterStart,
        player.getName() + " (level " + std::to_string(playerLevel) + ", HP " + std::to_string(player.getHP()) + ") faces " + enemy.getName() + ".");
    participants.actor = player.getName();
    participants.actorLevel = playerLevel;
    participants.target = enemy.getName();
    participants.targetLevel = enemy.getLevel();
    participants.enemyTier = enemy.getTier();
}

void CombatManager::ContinueEncounter(Enemy& enemy) {
    if (activeEnemy_ == &enemy || !enemy.isAlive() || encounterCompleted_) return;
    activeEnemy_ = &enemy;
    if (seededEnemies_.insert(&enemy).second) enemy.SetStatusSeed(random_.Next());
    playerActionStarted_ = false;
    playerGauge_ = 0;
    enemyGauge_ = 0;
    playerTurn_ = false;
    consumableUsedThisTurn_ = false;
    AddEvent(CombatEvent::Kind::Info, enemy.getName() + " enters the battle.");
}

bool CombatManager::resolve(CombatAction action, Player& player, Enemy& enemy,
                            ShimmerSkill* skill, ProgressionManager* progression) {
    if (activeEnemy_ == nullptr) BeginEncounter(player, enemy, progression ? progression->GetLevel() : 1);
    if (activePlayer_ != nullptr && activePlayer_ != &player) return false;
    if (activeEnemy_ != &enemy && std::find(currentEnemyParty_.begin(), currentEnemyParty_.end(), &enemy) == currentEnemyParty_.end()) return false;
    blocked_ = false;
    PrepareTurn(player, enemy);

    if (!player.isAlive()) {
        enemy.ForfeitRewards();
        ForfeitEncounterRewards();
        return false;
    }
    if (encounterCompleted_ || !enemy.isAlive()) return false;
    if (!playerTurn_) return false;

    switch (action) {
    case CombatAction::Attack: {
        AddEvent(CombatEvent::Kind::Attack, player.getName() + " uses Attack on " + enemy.getName() + ".");
        int accuracyFloor = 0;
        for (const auto& status : player.GetStatuses()) {
            if (status.type == StatusType::AccuracyFloor) accuracyFloor = (std::max)(accuracyFloor, status.potency);
        }
        const int hitChance = (std::max)(CombatMath::AccuracyChance(100, player.getAccuracy(), enemy.getEvasion()), accuracyFloor);
        if (!random_.RollPercent(hitChance)) {
            AddEvent(CombatEvent::Kind::Miss, player.getName() + " attacks " + enemy.getName() + ", but the strike is evaded.");
            FinishPlayerAction(player, enemy);
            return true;
        }
        const bool critical = player.HasStatus(StatusType::GuaranteedCrit) ||
            random_.RollPercent(player.getCritRate());
        const int rawDamage = critical
            ? CombatMath::Scale(player.getAttack(), player.getCritDamage())
            : player.getAttack();
        AddEvent(critical ? CombatEvent::Kind::Critical : CombatEvent::Kind::Hit,
            player.getName() + (critical ? " critically hits " : " hits ") + enemy.getName() + ".");
        enemy.ApplyDirectDamage(rawDamage);
        const int actualDamage = enemy.GetLastDamageResult().hpDamage;
        RecordDamage(player.getName(), enemy.getName(), "Attack", enemy.GetLastDamageResult());
        if (player.getLifesteal() > 0) {
            const int recovery = CombatMath::Scale(actualDamage, player.getLifesteal());
            if (recovery > 0) {
                const int beforeHeal = player.getHP();
                player.heal(recovery);
                const int actualHeal = player.getHP() - beforeHeal;
                AddEvent(CombatEvent::Kind::Heal, player.getName() + " recovers " + std::to_string(actualHeal) + " HP from lifesteal.", actualHeal);
            }
        }
        if (enemy.UpdatePhase()) {
            AddEvent(CombatEvent::Kind::Info, "Boss enters Phase " + std::to_string(enemy.GetPhase()) + ".");
        }

        if (!enemy.isAlive()) {
            AddEvent(CombatEvent::Kind::Death, enemy.getName() + " has been defeated.");
            ResolveDefeatReward(player, enemy, progression);
        }
        FinishPlayerAction(player, enemy);
        return true;
    }
    case CombatAction::Shimmer:
    {
        if (skill == nullptr) {
            AddEvent(CombatEvent::Kind::Info, "Select an equipped Shimmer skill first.");
            return false;
        }
        const std::string effect = skill->getEffect();
        const int remainingCooldown = GetSkillCooldown(skill->getName());
        if (remainingCooldown > 0) {
            AddEvent(CombatEvent::Kind::Info, skill->getName() + " is recharging for " + std::to_string(remainingCooldown) + (remainingCooldown == 1 ? " turn." : " turns."));
            return false;
        }
        const int playerHealthBefore = player.getHP();
        const int barrierBefore = player.GetBarrier();
        const auto playerApplications = player.GetStatusApplications().size();
        const auto enemyApplications = enemy.GetStatusApplications().size();
        const auto statusesBefore = player.GetStatuses().size();
        const bool offensive = skill->getTargetType() != ShimmerSkill::TargetType::Self;
        auto& actionEvent = AddEvent(CombatEvent::Kind::Shimmer, player.getName() + " uses Shimmer skill " + skill->getName() + " on " + (offensive ? enemy.getName() : player.getName()) + ".");
        actionEvent.actor = player.getName(); actionEvent.target = offensive ? enemy.getName() : player.getName(); actionEvent.skill = skill->getName();
        skill->apply(player, enemy);
        if (offensive && effect.find("damage") == 0) {
            auto& hit = AddEvent(CombatEvent::Kind::Hit, skill->getName() + " hits " + enemy.getName() + ".");
            hit.actor = player.getName(); hit.target = enemy.getName(); hit.skill = skill->getName();
            RecordDamage(player.getName(), enemy.getName(), skill->getName(), enemy.GetLastDamageResult());
            if (player.HasActiveSet("Shimmer Ascendant") && enemy.GetLastDamageResult().hpDamage > 0)
                player.heal(CombatMath::Scale(player.getMaxHP(), 5));
        }
        const auto recordStatuses = [&](const std::vector<StatusApplicationResolution>& applications, std::size_t first, const std::string& target) {
            for (std::size_t index = first; index < applications.size(); ++index) {
                const auto& application = applications[index];
                auto& statusEvent = AddEvent(application.applied ? CombatEvent::Kind::Status :
                    application.resisted ? CombatEvent::Kind::Resistance : CombatEvent::Kind::Info,
                    target + (application.applied ? " receives " : application.resisted ? " resists " : " does not receive ") +
                    StatusDisplayName(application.status.type) + " from " + skill->getName() + ".");
                statusEvent.actor = player.getName(); statusEvent.target = target; statusEvent.skill = skill->getName();
            }
        };
        recordStatuses(player.GetStatusApplications(), playerApplications, player.getName());
        recordStatuses(enemy.GetStatusApplications(), enemyApplications, enemy.getName());
        const int healed = player.getHP() - playerHealthBefore;
        if (healed > 0) AddEvent(CombatEvent::Kind::Heal, player.getName() + " restores " + std::to_string(healed) + " HP with " + skill->getName() + ".", healed);
        const int shield = player.GetBarrier() - barrierBefore;
        if (shield > 0) AddEvent(CombatEvent::Kind::Shield, player.getName() + " gains " + std::to_string(shield) + " barrier with " + skill->getName() + ".", shield);
        if (effect == "cleanse_resist" && statusesBefore + (player.GetStatusApplications().size() - playerApplications) > player.GetStatuses().size())
            AddEvent(CombatEvent::Kind::Cleanse, player.getName() + " cleanses negative statuses with " + skill->getName() + ".");
        if (enemy.isAlive() && enemy.UpdatePhase()) AddEvent(CombatEvent::Kind::PhaseTransition, "Boss enters Phase " + std::to_string(enemy.GetPhase()) + ".");
        shimmerCooldowns_[skill->getName()] = skill->getCooldown();
        if (!enemy.isAlive()) {
            AddEvent(CombatEvent::Kind::Death, enemy.getName() + " has been defeated.");
            ResolveDefeatReward(player, enemy, progression);
        }
        FinishPlayerAction(player, enemy);
        return true;
    }
    case CombatAction::Item: {
        if (consumableUsedThisTurn_) {
            AddEvent(CombatEvent::Kind::Info, "Only one consumable can be used per turn.");
            return false;
        }
        AddEvent(CombatEvent::Kind::Info, "Select a consumable to use.");
        return false;
    }
    case CombatAction::Flee:
        player.EndStatusTurn();
        playerActionStarted_ = false;
        enemy.ForfeitRewards();
        AddEvent(CombatEvent::Kind::Info, player.getName() + " retreats from the encounter.");
        CompleteEncounter("fled");
        return true;
    }

    return false;
}

bool CombatManager::UseConsumable(const std::string& itemName, Player& player, Enemy& enemy, ProgressionManager* progression) {
    const Item* item = player.GetInventory().FindUsableItem(itemName);
    if (item == nullptr) {
        AddEvent(CombatEvent::Kind::Info, "No unlocked consumable with that name is available.");
        return false;
    }
    return UseConsumable(item->getInstanceId(), player, enemy, progression);
}

bool CombatManager::CanUseConsumableNow(const Player& player, const Enemy* target) const {
    if (activePlayer_ != &player || encounterCompleted_ || !player.isAlive() || !playerTurn_ || consumableUsedThisTurn_ ||
        activeEnemy_ == nullptr || !activeEnemy_->isAlive()) return false;
    if (target == nullptr) return true;
    if (!target->isAlive()) return false;
    return target == activeEnemy_ || std::find(currentEnemyParty_.begin(), currentEnemyParty_.end(), target) != currentEnemyParty_.end();
}

bool CombatManager::HasActiveShimmerCooldowns() const {
    return std::any_of(shimmerCooldowns_.begin(), shimmerCooldowns_.end(), [](const auto& cooldown) { return cooldown.second > 0; });
}

int CombatManager::ReduceShimmerCooldowns(int turns) {
    if (turns <= 0) return 0;
    int reduced = 0;
    for (auto& cooldown : shimmerCooldowns_) {
        const int reduction = (std::min)(turns, cooldown.second);
        cooldown.second -= reduction;
        reduced += reduction;
    }
    return reduced;
}

bool CombatManager::UseConsumable(unsigned long long instanceId, Player& player, Enemy& enemy, ProgressionManager* progression) {
    if (!player.isAlive()) {
        enemy.ForfeitRewards();
        ForfeitEncounterRewards();
        return false;
    }
    if (!CanUseConsumableNow(player, &enemy)) {
        AddEvent(CombatEvent::Kind::Info, "A consumable requires your active turn and a living encounter target.");
        return false;
    }
    const Item* item = player.GetInventory().FindItem(instanceId);
    if (item == nullptr) return false;
    const std::string itemName = item->getName();
    const auto* effect = FindConsumableEffect(item->getEffect());
    const int healthBefore = player.getHP();
    const int barrierBefore = player.GetBarrier();
    const auto playerApplicationsBefore = player.GetStatusApplications().size();
    const auto enemyApplicationsBefore = enemy.GetStatusApplications().size();
    const auto countNegative = [](const Player& actor) {
        return std::count_if(actor.GetStatuses().begin(), actor.GetStatuses().end(), [](const StatusEffect& status) { return IsNegativeConsumableStatus(status.type); });
    };
    const auto negativeBefore = countNegative(player);
    int cooldownBefore = 0;
    for (const auto& cooldown : shimmerCooldowns_) cooldownBefore += cooldown.second;
    if (!player.UseConsumable(instanceId, &enemy, this)) {
        AddEvent(CombatEvent::Kind::Info, "That consumable is locked, unavailable, or has no effect in the current context.");
        return false;
    }
    consumableUsedThisTurn_ = true;
    const bool offensive = effect != nullptr && (effect->kind == ConsumableEffectKind::FlatDamage || effect->kind == ConsumableEffectKind::ApplyBurn);
    auto& action = AddEvent(CombatEvent::Kind::Skill, player.getName() + " uses " + itemName + " on " + (offensive ? enemy.getName() : player.getName()) + ".");
    action.actor = player.getName(); action.target = offensive ? enemy.getName() : player.getName(); action.skill = itemName;
    const int healing = player.getHP() - healthBefore;
    if (healing > 0) {
        auto& event = AddEvent(CombatEvent::Kind::Heal, player.getName() + " heals " + std::to_string(healing) + " HP.", healing);
        event.actor = player.getName(); event.target = player.getName(); event.skill = itemName; event.remainingHP = player.getHP();
    }
    if (player.GetBarrier() > barrierBefore)
        AddEvent(CombatEvent::Kind::Shield, player.getName() + " gains " + std::to_string(player.GetBarrier() - barrierBefore) + " barrier.", player.GetBarrier() - barrierBefore);
    if (effect != nullptr && effect->kind == ConsumableEffectKind::FlatDamage)
        RecordDamage(player.getName(), enemy.getName(), itemName, enemy.GetLastDamageResult());
    const auto recordApplications = [&](const std::vector<StatusApplicationResolution>& applications, std::size_t first, const std::string& targetName) {
        for (std::size_t index = first; index < applications.size(); ++index) {
            const auto& result = applications[index];
            auto& event = AddEvent(result.applied ? CombatEvent::Kind::Status :
                result.resisted ? CombatEvent::Kind::Resistance : CombatEvent::Kind::Info,
                targetName + (result.applied ? " receives " : result.resisted ? " resists " : " does not receive ") + StatusDisplayName(result.status.type) +
                (result.applied ? " (" + std::to_string(result.status.turns) + " turns, potency " + std::to_string(result.status.potency) + ")" : "") + " from " + itemName + ".");
            event.actor = player.getName(); event.target = targetName; event.skill = itemName;
        }
    };
    recordApplications(player.GetStatusApplications(), playerApplicationsBefore, player.getName());
    recordApplications(enemy.GetStatusApplications(), enemyApplicationsBefore, enemy.getName());
    const auto cleansed = negativeBefore - countNegative(player);
    if (cleansed > 0) AddEvent(CombatEvent::Kind::Cleanse, player.getName() + " cleanses " + std::to_string(cleansed) + " negative statuses.", static_cast<int>(cleansed));
    int cooldownAfter = 0;
    for (const auto& cooldown : shimmerCooldowns_) cooldownAfter += cooldown.second;
    if (cooldownAfter < cooldownBefore) AddEvent(CombatEvent::Kind::Info, itemName + " removes " + std::to_string(cooldownBefore - cooldownAfter) + " total Shimmer cooldown turns.", cooldownBefore - cooldownAfter);
    if (!enemy.isAlive()) AddEvent(CombatEvent::Kind::Death, enemy.getName() + " has been defeated.");
    ResolveDefeatReward(player, enemy, progression);
    FinishPlayerAction(player, enemy);
    return true;
}

void CombatManager::enemyTurn(Player& player, Enemy& enemy, ProgressionManager* progression) {
    if (activeEnemy_ == nullptr) { BeginEncounter(player, enemy, progression ? progression->GetLevel() : 1); PrepareTurn(player, enemy); }
    if (activePlayer_ != &player ||
        (activeEnemy_ != &enemy && std::find(currentEnemyParty_.begin(), currentEnemyParty_.end(), &enemy) == currentEnemyParty_.end())) return;
    if (groupEnemyTurn_ && (nextPartyEnemy_ >= currentEnemyParty_.size() ||
        currentEnemyParty_[nextPartyEnemy_] != &enemy)) return;
    if (!player.isAlive()) {
        enemy.ForfeitRewards();
        ForfeitEncounterRewards();
        return;
    }
    if (encounterCompleted_) return;
    if (playerTurn_ && !groupEnemyTurn_) return;
    if (!enemy.isAlive()) {
        ResolveDefeatReward(player, enemy, progression);
        return;
    }
    if (enemyGauge_ < 1000) {
        AdvanceTurn(player, enemy);
        if (playerTurn_ || enemyGauge_ < 1000 || !player.isAlive()) return;
    }
    const bool bonusAction = nextEnemyBonus_;
    nextEnemyBonus_ = false;
    if (!bonusAction) enemy.TickSkillCooldowns();
    auto& turn = AddEvent(CombatEvent::Kind::Turn, enemy.getName() + "'s action begins (gauge " + std::to_string(enemyGauge_) + ").");
    turn.actor = enemy.getName();
    const bool stunned = enemy.HasStatus(StatusType::Stun);
    std::vector<std::pair<StatusType, int>> damageEvents;
    int healing = 0;
    if (!bonusAction) {
        enemy.BeginStatusTurn(damageEvents, healing);
        for (const auto& tick : enemy.GetStatusDamageResults())
            RecordDamage(tick.source.empty() ? StatusDisplayName(tick.type) : tick.source, enemy.getName(), StatusDisplayName(tick.type), tick.damage, CombatEvent::Kind::DotTick);
    }
    if (healing > 0) AddEvent(CombatEvent::Kind::Heal, enemy.getName() + " heals " + std::to_string(healing) + " HP from Heal-over-time.", healing);
    if (!enemy.isAlive()) {
        AddEvent(CombatEvent::Kind::Death, enemy.getName() + " has been defeated by a status effect.");
        ResolveDefeatReward(player, enemy, progression);
        enemyGauge_ = 0;
        enemy.EndStatusTurn();
        return;
    }
    if (stunned) {
        AddEvent(CombatEvent::Kind::Status, enemy.getName() + " loses its turn to Stun.");
        enemyGauge_ -= 1000;
        if (!bonusAction) enemy.EndStatusTurn();
        playerTurn_ = false;
        AdvanceTurn(player, enemy);
        return;
    }
    const auto finishAction = [&](bool extraTurn = false) {
        if (!bonusAction) enemy.EndStatusTurn();
        if (!player.isAlive()) {
            enemy.ForfeitRewards();
            ForfeitEncounterRewards();
            AddEvent(CombatEvent::Kind::Death, player.getName() + " has been defeated.");
        }
        enemyGauge_ -= 1000;
        if (extraTurn && !bonusAction) { enemyGauge_ = 1000; nextEnemyBonus_ = true; }
        playerTurn_ = false;
        if (!nextEnemyBonus_) AdvanceTurn(player, enemy);
    };
    const auto damagePlayer = [&](int requested, const std::string& skillName, bool guaranteedCritical = false) {
        int accuracyFloor = 0;
        for (const auto& status : enemy.GetStatuses()) if (status.type == StatusType::AccuracyFloor)
            accuracyFloor = (std::max)(accuracyFloor, status.potency);
        const int hitChance = (std::max)(accuracyFloor, CombatMath::AccuracyChance(100, enemy.getAccuracy(), player.getEvasion()));
        if (!random_.RollPercent(hitChance)) {
            auto& miss = AddEvent(CombatEvent::Kind::Miss, enemy.getName() + " misses " + player.getName() + ".");
            miss.actor = enemy.getName(); miss.target = player.getName(); miss.skill = skillName;
            return false;
        }
        const bool critical = guaranteedCritical || enemy.HasStatus(StatusType::GuaranteedCrit) ||
            random_.RollPercent(enemy.getCritRate());
        const int rawDamage = critical ? CombatMath::Scale(requested, enemy.getCritDamage()) : requested;
        auto& hit = AddEvent(critical ? CombatEvent::Kind::Critical : CombatEvent::Kind::Hit,
            enemy.getName() + (critical ? " critically hits " : " hits ") + player.getName() + ".");
        hit.actor = enemy.getName(); hit.target = player.getName(); hit.skill = skillName;
        player.ApplyDirectDamage(rawDamage);
        RecordDamage(enemy.getName(), player.getName(), skillName, player.GetLastDamageResult());
        return true;
    };
    const auto applyDebuff = [&](StatusType type, const EnemySkillDefinition& skill) {
        const int duration = skill.duration > 0 ? skill.duration : 2;
        const bool applied = player.ApplyStatus({type, duration, skill.statusPotency, enemy.getAtk(),
            enemy.getDotPotency(), enemy.getDefense(), enemy.getName(), enemy.GetCombatantId()}, enemy.getAccuracy());
        const auto& result = player.GetStatusApplications().back();
        auto& event = AddEvent(applied ? CombatEvent::Kind::Debuff : result.resisted ? CombatEvent::Kind::Resistance : CombatEvent::Kind::Info,
            player.getName() + (applied ? " receives " : result.resisted ? " resists " : " does not receive ") + StatusDisplayName(type) +
            (applied ? " (" + std::to_string(result.status.turns) + " turns, potency " + std::to_string(result.status.potency) + ")" : "") + " from " + skill.name + ".");
        event.actor = enemy.getName(); event.target = player.getName(); event.skill = skill.name;
    };
    const auto& skills = enemy.GetSkills();
    for (std::size_t index = 0; index < skills.size(); ++index) {
        const auto& skill = skills[index];
        if (!enemy.IsSkillReady(index)) continue;
        if (skill.effect == "heal_self" && enemy.getHP() >= enemy.getMaxHP() / 2) continue;
        if (skill.effect == "cleanse_self" && enemy.GetStatuses().empty()) continue;
        Enemy* ally = nullptr;
        if (skill.effect.find("ally") != std::string::npos) {
            for (Enemy* candidate : currentEnemyParty_)
                if (candidate != nullptr && candidate != &enemy && candidate->isAlive()) { ally = candidate; break; }
            if (ally == nullptr && skill.multiplier <= 0) continue;
        }
        const bool offensive = skill.multiplier > 0 || skill.effect == "bleed" || skill.effect == "burn" || skill.effect == "poison" ||
            skill.effect == "tri_dot" || skill.effect == "burn_bleed" || skill.effect == "def_down" || skill.effect == "accuracy_down";
        const std::string target = ally != nullptr ? ally->getName() : offensive ? player.getName() : enemy.getName();
        auto& action = AddEvent(CombatEvent::Kind::Skill, enemy.getName() + " uses " + skill.name + " on " + target + ".");
        action.actor = enemy.getName(); action.target = target; action.skill = skill.name;
        const bool landed = skill.multiplier <= 0 || damagePlayer((std::max)(1, CombatMath::Scale(enemy.getAtk(), skill.multiplier)), skill.name, skill.effect == "guaranteed_crit");
        if (ally != nullptr && skill.effect.find("heal") != std::string::npos) {
            const int before = ally->getHP();
            ally->heal((std::max)(1, CombatMath::Scale(ally->getMaxHP(), 15)));
            AddEvent(CombatEvent::Kind::Heal, ally->getName() + " heals " + std::to_string(ally->getHP() - before) + " HP.", ally->getHP() - before);
        }
        if (landed && player.isAlive()) {
            if (skill.effect == "bleed" || skill.effect == "burn" || skill.effect == "poison")
                applyDebuff(skill.effect == "bleed" ? StatusType::Bleed : skill.effect == "burn" ? StatusType::Burn : StatusType::Poison, skill);
            else if (skill.effect == "tri_dot" || skill.effect == "burn_bleed") {
                for (StatusType type : {StatusType::Burn, StatusType::Bleed, StatusType::Poison})
                    if (!(skill.effect == "burn_bleed" && type == StatusType::Poison)) applyDebuff(type, skill);
            } else if (skill.effect == "def_down" || skill.effect == "accuracy_down")
                applyDebuff(skill.effect == "def_down" ? StatusType::DefenseDown : StatusType::AccuracyDown, skill);
        }
        if (skill.effect == "heal_self" || skill.effect == "heal_cleanse") {
            const int before = enemy.getHP();
            enemy.heal((std::max)(1, CombatMath::Scale(enemy.getMaxHP(), 15)));
            AddEvent(CombatEvent::Kind::Heal, enemy.getName() + " heals " + std::to_string(enemy.getHP() - before) + " HP.", enemy.getHP() - before);
        }
        if (skill.effect == "cleanse_self" || skill.effect == "heal_cleanse") {
            const int count = enemy.CleanseNegativeStatuses();
            AddEvent(CombatEvent::Kind::Cleanse, enemy.getName() + " cleanses " + std::to_string(count) + " negative statuses.", count);
        } else if (skill.effect == "barrier" || skill.effect == "barrier_accuracy") {
            const int amount = enemy.getMaxHP() / 5;
            const bool applied = enemy.ApplyStatus({StatusType::Barrier, skill.duration > 0 ? skill.duration : 2, amount});
            if (applied) AddEvent(CombatEvent::Kind::Shield, enemy.getName() + " gains " + std::to_string(amount) + " barrier.", amount);
            if (skill.effect == "barrier_accuracy" && enemy.ApplyStatus({StatusType::AccuracyUp, skill.duration > 0 ? skill.duration : 2, skill.statusPotency}))
                AddEvent(CombatEvent::Kind::Buff, enemy.getName() + " gains Accuracy Up.");
        } else {
            std::vector<StatusType> buffs;
            if (skill.effect == "attack_up") buffs.push_back(StatusType::AttackUp);
            else if (skill.effect == "accuracy_up") buffs.push_back(StatusType::AccuracyUp);
            else if (skill.effect == "crit_up") buffs.push_back(StatusType::CritUp);
            else if (skill.effect == "crit_damage_up") buffs.push_back(StatusType::CritDamageUp);
            else if (skill.effect == "guaranteed_crit") buffs.push_back(StatusType::GuaranteedCrit);
            else if (skill.effect == "dot_up") buffs.push_back(StatusType::DotUp);
            else if (skill.effect == "heal_resistance" && player.isAlive()) applyDebuff(StatusType::HealResistance, skill);
            else if (skill.effect == "accuracy_floor") buffs.push_back(StatusType::AccuracyFloor);
            else if (skill.effect == "dot_reduction") buffs.push_back(StatusType::DotReduction);
            else if (skill.effect == "accuracy_crit_up") buffs = {StatusType::AccuracyUp, StatusType::CritUp};
            for (StatusType type : buffs) {
                const bool applied = enemy.ApplyStatus({type, skill.duration > 0 ? skill.duration : 2, skill.statusPotency});
                AddEvent(applied ? CombatEvent::Kind::Buff : CombatEvent::Kind::Info, enemy.getName() +
                    (applied ? " gains " : " keeps its existing ") + StatusDisplayName(type) + ".");
            }
        }
        const bool extraTurn = skill.effect == "extra_turn" && !bonusAction;
        if (extraTurn) AddEvent(CombatEvent::Kind::ExtraTurn, enemy.getName() + " gains an extra turn.");
        enemy.StartSkillCooldown(index);
        finishAction(extraTurn);
        return;
    }
    auto& attack = AddEvent(CombatEvent::Kind::Attack, enemy.getName() + " uses Attack on " + player.getName() + ".");
    attack.actor = enemy.getName(); attack.target = player.getName(); attack.skill = "Attack";
    damagePlayer(enemy.getAtk(), "Attack");
    finishAction();
}

void CombatManager::PrepareGroupTurn(Player& player, const std::vector<Enemy*>& enemies) {
    // Compatibility party battles use one speed-ordered enemy round followed
    // by one player action. Preparation cannot restart a round or steal input.
    if (!player.isAlive() || (encounterCompleted_ && activeEnemy_ != nullptr) || groupEnemyTurn_ || playerActionStarted_) return;
    if (activePlayer_ != nullptr && activePlayer_ != &player) return;
    const auto candidates = enemies; // AdvanceTurn may pass our own party.
    if (activeEnemy_ == nullptr) {
        for (Enemy* enemy : candidates) if (enemy && enemy->isAlive()) {
            BeginEncounter(player, *enemy, 1); break;
        }
    }
    if (activeEnemy_ == nullptr) return;
    if (activePlayer_ == nullptr) { activePlayer_ = &player; player.SetStatusSeed(random_.Next()); }
    currentEnemyParty_.clear();
    for (Enemy* enemy : candidates) if (enemy != nullptr && enemy->isAlive() &&
        std::find(currentEnemyParty_.begin(), currentEnemyParty_.end(), enemy) == currentEnemyParty_.end()) {
        currentEnemyParty_.push_back(enemy);
        if (seededEnemies_.insert(enemy).second) enemy->SetStatusSeed(random_.Next());
    }
    std::stable_sort(currentEnemyParty_.begin(), currentEnemyParty_.end(), [](const Enemy* left, const Enemy* right) {
        if (left->getSpeed() != right->getSpeed()) return left->getSpeed() > right->getSpeed();
        return left->getName() < right->getName();
    });
    nextPartyEnemy_ = 0;
    groupEnemyTurn_ = !currentEnemyParty_.empty();
    playerTurn_ = false;
    enemyGauge_ = groupEnemyTurn_ ? 1000 : 0;
    if (groupEnemyTurn_) PrepareTurn(player, *activeEnemy_);
}

bool CombatManager::StepEnemyPartyTurn(Player& player, const std::vector<Enemy*>& enemies, ProgressionManager* progression) {
    if (encounterCompleted_ || (activePlayer_ != nullptr && activePlayer_ != &player) || playerActionStarted_) return false;
    if (!groupEnemyTurn_) PrepareGroupTurn(player, enemies);
    if (!groupEnemyTurn_) return false;
    if (!player.isAlive()) ForfeitEncounterRewards();
    while (nextPartyEnemy_ < currentEnemyParty_.size() && !currentEnemyParty_[nextPartyEnemy_]->isAlive()) ++nextPartyEnemy_;
    if (player.isAlive() && nextPartyEnemy_ < currentEnemyParty_.size()) {
        Enemy* candidate = currentEnemyParty_[nextPartyEnemy_];
        enemyGauge_ = 1000;
        enemyTurn(player, *candidate, progression);
        if (enemyGauge_ < 1000 || !candidate->isAlive()) ++nextPartyEnemy_;
    }
    if (player.isAlive() && nextPartyEnemy_ < currentEnemyParty_.size()) return true;
    groupEnemyTurn_ = false;
    enemyGauge_ = 0;
    playerGauge_ = 1000;
    playerTurn_ = player.isAlive() && std::any_of(currentEnemyParty_.begin(), currentEnemyParty_.end(),
        [](const Enemy* enemy) { return enemy->isAlive(); });
    if (playerTurn_) StartPlayerTurn(player);
    return false;
}

void CombatManager::enemyTurn(Player& player, Enemy& enemy, const std::vector<Enemy*>& enemies, ProgressionManager* progression) {
    PrepareGroupTurn(player, enemies);
    while (StepEnemyPartyTurn(player, enemies, progression)) {}
}

bool CombatManager::IsPlayerTurn() const { return playerTurn_; }
bool CombatManager::HasUsedConsumableThisTurn() const { return consumableUsedThisTurn_; }
