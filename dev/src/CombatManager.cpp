#include "CombatManager.h"
#include "Enemy.h"
#include "Player.h"
#include "ProgressionManager.h"
#include <cmath>
#include "ShimmerSkill.h"
#include "CombatMath.h"
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
        playerGauge += (std::max)(1, playerSpeed);
        enemyGauge += (std::max)(1, enemySpeed);
    }
}
}

void CombatManager::AdvanceTurn(Player& player, Enemy& enemy) {
    if (groupEnemyTurn_ || !player.isAlive() || !enemy.isAlive()) return;
    AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
    playerTurn_ = playerGauge_ >= 1000;
    if (playerTurn_) StartPlayerTurn(player);
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
    if (!player.isAlive()) return;
    consumableUsedThisTurn_ = false;
    for (auto& cooldown : shimmerCooldowns_) if (cooldown.second > 0) --cooldown.second;
    if (defaultSkill_ != nullptr) defaultSkill_->TickCooldown();
    if (player.HasActiveSet("Verdigris Matron")) {
        const int before = player.getHP();
        player.heal(player.getMaxHP() * 25 / 100);
        const int amount = player.getHP() - before;
        if (amount > 0) AddEvent(CombatEvent::Kind::Heal, player.getName() + " restores " + std::to_string(amount) + " HP from Verdigris Matron.", amount);
    }
    auto& turn = AddEvent(CombatEvent::Kind::Turn, player.getName() + "'s action begins (gauge " + std::to_string(playerGauge_) + ").");
    turn.actor = player.getName();
    const bool stunned = player.HasStatus(StatusType::Stun);
    std::vector<std::pair<StatusType, int>> playerDamageEvents;
    int playerHealing = 0;
    player.ProcessStatuses(playerDamageEvents, playerHealing);
    for (const auto& tick : player.GetStatusDamageResults())
        RecordDamage(StatusDisplayName(tick.type), player.getName(), StatusDisplayName(tick.type), tick.damage, CombatEvent::Kind::DotTick);
    if (playerHealing > 0) AddEvent(CombatEvent::Kind::Heal, player.getName() + " heals " + std::to_string(playerHealing) + " HP from Heal-over-time.", playerHealing);
    if (!player.isAlive()) {
        ForfeitEncounterRewards();
        AddEvent(CombatEvent::Kind::Death, player.getName() + " has fallen.");
    }
    if (stunned && player.isAlive()) {
        AddEvent(CombatEvent::Kind::Info, player.getName() + " loses the turn to Stun.");
        playerGauge_ -= 1000;
        playerTurn_ = false;
        return;
    }
}

CombatManager::CombatManager()
    : blocked_(false), weaknessExposed_(false),
      turnNumber_(0), playerTurn_(true), activeEnemy_(nullptr),
      playerGauge_(0), enemyGauge_(0),
      consumableUsedThisTurn_(false),
      defaultSkill_(ShimmerSkill::Create("Pulsus Luminis")) {}

CombatManager::~CombatManager() = default;

const ShimmerSkill* CombatManager::GetDefaultSkill() const { return defaultSkill_.get(); }

const std::vector<CombatManager::CombatEvent>& CombatManager::GetEvents() const { return events_; }
void CombatManager::ClearEvents() { if (activeEnemy_ == nullptr) events_.clear(); }
void CombatManager::ResetEncounter() {
    activeEnemy_ = nullptr;
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
        " requested damage; barrier absorbs " + std::to_string(result.barrierAbsorbed) + "; " +
        std::to_string(result.hpDamage) + " actual HP damage. HP: " + std::to_string(result.remainingHP) + ".", result.hpDamage);
    event.actor = actor;
    event.target = target;
    event.skill = skill;
    event.requestedDamage = result.requestedDamage;
    event.barrierAbsorbed = result.barrierAbsorbed;
    event.hpDamage = result.hpDamage;
    event.remainingHP = result.remainingHP;
    if (result.barrierBroken) AddEvent(CombatEvent::Kind::BarrierBreak, target + "'s barrier breaks.");
    if (result.healing > 0) AddEvent(CombatEvent::Kind::Heal, target + " recovers " + std::to_string(result.healing) + " HP from its defensive effect.", result.healing);
    if (result.barrierGained > 0) AddEvent(CombatEvent::Kind::Shield, target + " gains " + std::to_string(result.barrierGained) + " barrier from its defensive effect.", result.barrierGained);
}

void CombatManager::PrepareTurn(Player& player, Enemy& enemy) {
    if (!enemy.isAlive()) return;
    if (!battleStartEffectsApplied_) {
        const int before = player.GetBarrier();
        player.StartBattleEffects();
        battleStartEffectsApplied_ = true;
        const int gained = player.GetBarrier() - before;
        if (gained > 0) AddEvent(CombatEvent::Kind::Shield, player.getName() + " gains " + std::to_string(gained) + " barrier at battle start.", gained);
    }
    if (playerGauge_ == 0 && enemyGauge_ == 0 && !playerTurn_) {
        AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
        playerTurn_ = playerGauge_ >= 1000;
        if (playerTurn_) StartPlayerTurn(player);
    }
    if (playerTurn_ && player.isAlive()) AddEvent(CombatEvent::Kind::Info, "Waiting for " + player.getName() + " to choose an action.");
}

void CombatManager::ResumeAfterRespawn(Player& player, Enemy& enemy) {
    if (!enemy.isAlive()) return;
    playerGauge_ = 1000;
    enemyGauge_ = 0;
    playerTurn_ = true;
    consumableUsedThisTurn_ = false;
    StartPlayerTurn(player);
    AddEvent(CombatEvent::Kind::Info, player.getName() + " returns from the fountain and may act.");
}

void CombatManager::BeginEncounter(Enemy& enemy) {
    if (activeEnemy_ == &enemy) return;
    activeEnemy_ = &enemy;
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
    BeginEncounter(enemy);
    auto& participants = AddEvent(CombatEvent::Kind::EncounterStart,
        player.getName() + " (level " + std::to_string(playerLevel) + ", HP " + std::to_string(player.getHP()) + ") faces " + enemy.getName() + ".");
    participants.actor = player.getName();
    participants.actorLevel = playerLevel;
    participants.target = enemy.getName();
    participants.targetLevel = enemy.getLevel();
    participants.enemyTier = enemy.getTier();
}

void CombatManager::ContinueEncounter(Enemy& enemy) {
    activeEnemy_ = &enemy;
    playerGauge_ = 0;
    enemyGauge_ = 0;
    playerTurn_ = false;
    consumableUsedThisTurn_ = false;
    AddEvent(CombatEvent::Kind::Info, enemy.getName() + " enters the battle.");
}

bool CombatManager::resolve(CombatAction action, Player& player, Enemy& enemy,
                            ShimmerSkill* skill, ProgressionManager* progression) {
    if (activeEnemy_ == nullptr) BeginEncounter(enemy);
    blocked_ = false;
    if (turnNumber_ == 0 && playerGauge_ == 0 && enemyGauge_ == 0) {
        AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
        playerTurn_ = playerGauge_ >= 1000;
    }

    if (!player.isAlive()) {
        enemy.ForfeitRewards();
        ForfeitEncounterRewards();
        return false;
    }
    if (encounterCompleted_ || !enemy.isAlive()) return false;
    if (!playerTurn_) return false;
    ++turnNumber_;

    switch (action) {
    case CombatAction::Attack: {
        AddEvent(CombatEvent::Kind::Attack, player.getName() + " uses Attack on " + enemy.getName() + ".");
        const unsigned hitRoll = static_cast<unsigned>(turnNumber_ * 37 + player.getAttack() * 11 + enemy.getHP()) % 100;
        int accuracyFloor = 0;
        for (const auto& status : player.GetStatuses()) {
            if (status.type == StatusType::AccuracyFloor) accuracyFloor += status.potency;
        }
        const int hitChance = (std::max)(CombatMath::AccuracyChance(100, player.getAccuracy(), enemy.getEvasion()), accuracyFloor);
        if (hitRoll >= static_cast<unsigned>(hitChance)) {
            AddEvent(CombatEvent::Kind::Miss, player.getName() + " attacks " + enemy.getName() + ", but the strike is evaded.");
            playerGauge_ -= 1000;
            playerTurn_ = false;
            AdvanceTurn(player, enemy);
            return true;
        }
        const unsigned critRoll = static_cast<unsigned>(turnNumber_ * 53 + player.getAttack() * 7 + enemy.getHP() * 3) % 100;
        const bool critical = player.HasStatus(StatusType::GuaranteedCrit) ||
            critRoll < static_cast<unsigned>((std::min)(100, player.getCritRate()));
        const int rawDamage = critical
            ? player.getAttack() * player.getCritDamage() / 100
            : player.getAttack();
        const int damage = CombatMath::Mitigate(rawDamage, enemy.getDefense()) + (weaknessExposed_ ? 2 : 0);
        AddEvent(critical ? CombatEvent::Kind::Critical : CombatEvent::Kind::Hit,
            player.getName() + (critical ? " critically hits " : " hits ") + enemy.getName() + ".");
        enemy.takeDamage(damage);
        const int actualDamage = enemy.GetLastDamageResult().hpDamage;
        RecordDamage(player.getName(), enemy.getName(), "Attack", enemy.GetLastDamageResult());
        if (player.getLifesteal() > 0) {
            const int recovery = actualDamage * (std::min)(CombatMath::Balance::MaximumLifesteal, player.getLifesteal()) / 100;
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
        playerGauge_ -= 1000;
        playerTurn_ = false;
        AdvanceTurn(player, enemy);
        return true;
    }
    case CombatAction::Shimmer:
    {
        if (skill == nullptr) {
            AddEvent(CombatEvent::Kind::Info, "Select an equipped Shimmer skill first.");
            return false;
        }
        const std::string effect = skill->getEffect();
        const int remainingCooldown = shimmerCooldowns_[skill->getName()];
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
                player.heal(player.getMaxHP() * 5 / 100);
        }
        const auto recordStatuses = [&](const std::vector<StatusApplicationResolution>& applications, std::size_t first, const std::string& target) {
            for (std::size_t index = first; index < applications.size(); ++index) {
                const auto& application = applications[index];
                auto& statusEvent = AddEvent(application.applied ? CombatEvent::Kind::Status : CombatEvent::Kind::Resistance,
                    target + (application.applied ? " receives " : " resists ") + StatusDisplayName(application.status.type) + " from " + skill->getName() + ".");
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
        playerGauge_ -= 1000;
        playerTurn_ = false;
        AdvanceTurn(player, enemy);
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
        enemy.ForfeitRewards();
        AddEvent(CombatEvent::Kind::Info, player.getName() + " retreats from the encounter.");
        CompleteEncounter("fled");
        return true;
    }

    return false;
}

bool CombatManager::UseConsumable(const std::string& itemName, Player& player, Enemy& enemy, ProgressionManager* progression) {
    if (!player.isAlive()) {
        enemy.ForfeitRewards();
        ForfeitEncounterRewards();
        return false;
    }
    if (encounterCompleted_ || !player.isAlive() || !enemy.isAlive() || !playerTurn_ || consumableUsedThisTurn_) {
        AddEvent(CombatEvent::Kind::Info, "Only one consumable can be used per turn.");
        return false;
    }
    const int healthBefore = player.getHP();
    const int enemyHealthBefore = enemy.getHP();
    const auto enemyStatusesBefore = enemy.GetStatuses().size();
    if (!player.UseConsumable(itemName, &enemy)) {
        AddEvent(CombatEvent::Kind::Info, "That consumable cannot be used in battle.");
        return false;
    }
    consumableUsedThisTurn_ = true;
    AddEvent(CombatEvent::Kind::Info, player.getName() + " uses " + itemName + ".");
    const int healing = player.getHP() - healthBefore;
    const int damage = enemyHealthBefore - enemy.getHP();
    if (healing > 0) AddEvent(CombatEvent::Kind::Heal, player.getName() + " heals " + std::to_string(healing) + " HP.", healing);
    if (damage > 0) RecordDamage(player.getName(), enemy.getName(), itemName, enemy.GetLastDamageResult());
    if (enemy.GetStatuses().size() > enemyStatusesBefore)
        AddEvent(CombatEvent::Kind::Status, enemy.getName() + " receives " + StatusDisplayName(enemy.GetStatuses().back().type) + " from " + itemName + ".");
    if (!enemy.isAlive()) AddEvent(CombatEvent::Kind::Death, enemy.getName() + " has been defeated.");
    ResolveDefeatReward(player, enemy, progression);
    playerGauge_ -= 1000;
    playerTurn_ = false;
    AdvanceTurn(player, enemy);
    return true;
}

void CombatManager::enemyTurn(Player& player, Enemy& enemy, ProgressionManager* progression) {
    if (!player.isAlive()) {
        enemy.ForfeitRewards();
        ForfeitEncounterRewards();
        return;
    }
    if (encounterCompleted_) return;
    if (!enemy.isAlive()) {
        ResolveDefeatReward(player, enemy, progression);
        return;
    }
    if (enemyGauge_ < 1000) {
        AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
        playerTurn_ = playerGauge_ >= 1000;
        if (playerTurn_ && !groupEnemyTurn_) return;
    }
    enemy.TickSkillCooldowns();
    auto& turn = AddEvent(CombatEvent::Kind::Turn, enemy.getName() + "'s action begins (gauge " + std::to_string(enemyGauge_) + ").");
    turn.actor = enemy.getName();
    const bool stunned = enemy.HasStatus(StatusType::Stun);
    std::vector<std::pair<StatusType, int>> damageEvents;
    int healing = 0;
    enemy.ProcessStatuses(damageEvents, healing);
    for (const auto& tick : enemy.GetStatusDamageResults())
        RecordDamage(StatusDisplayName(tick.type), enemy.getName(), StatusDisplayName(tick.type), tick.damage, CombatEvent::Kind::DotTick);
    if (healing > 0) AddEvent(CombatEvent::Kind::Heal, enemy.getName() + " heals " + std::to_string(healing) + " HP from Heal-over-time.", healing);
    if (!enemy.isAlive()) {
        AddEvent(CombatEvent::Kind::Death, enemy.getName() + " has been defeated by a status effect.");
        ResolveDefeatReward(player, enemy, progression);
        enemyGauge_ = 0;
        return;
    }
    if (stunned) {
        AddEvent(CombatEvent::Kind::Status, enemy.getName() + " loses its turn to Stun.");
        enemyGauge_ -= 1000;
        playerTurn_ = false;
        AdvanceTurn(player, enemy);
        return;
    }
    const auto finishAction = [&](bool extraTurn = false) {
        if (!player.isAlive()) {
            enemy.ForfeitRewards();
            ForfeitEncounterRewards();
            AddEvent(CombatEvent::Kind::Death, player.getName() + " has been defeated.");
        }
        enemyGauge_ -= 1000;
        if (extraTurn) enemyGauge_ = 1000;
        playerTurn_ = false;
        AdvanceTurn(player, enemy);
    };
    const auto damagePlayer = [&](int requested, const std::string& skillName, bool guaranteedCritical = false) {
        const int hitRoll = (turnNumber_ * 41 + enemy.getAtk() * 7 + player.getHP()) % 100;
        const int hitChance = CombatMath::AccuracyChance(100, enemy.getAccuracy(), player.getEvasion());
        if (hitRoll >= hitChance) {
            auto& miss = AddEvent(CombatEvent::Kind::Miss, enemy.getName() + " misses " + player.getName() + ".");
            miss.actor = enemy.getName(); miss.target = player.getName(); miss.skill = skillName;
            return false;
        }
        const unsigned critRoll = static_cast<unsigned>((turnNumber_ * 53 + enemy.getAtk() * 7 + player.getHP() * 3) % 100);
        const bool critical = guaranteedCritical || enemy.HasStatus(StatusType::GuaranteedCrit) ||
            critRoll < static_cast<unsigned>((std::min)(100, enemy.getCritRate() + (enemy.HasStatus(StatusType::CritUp) ? 20 : 0)));
        const int rawDamage = critical ? requested * (enemy.getCritDamage() + (enemy.HasStatus(StatusType::CritDamageUp) ? 50 : 0)) / 100 : requested;
        auto& hit = AddEvent(critical ? CombatEvent::Kind::Critical : CombatEvent::Kind::Hit,
            enemy.getName() + (critical ? " critically hits " : " hits ") + player.getName() + ".");
        hit.actor = enemy.getName(); hit.target = player.getName(); hit.skill = skillName;
        // Player::takeDamage applies reductions/barriers, but does not apply DEF.
        player.takeDamage(CombatMath::Mitigate(rawDamage, player.getDefense()));
        RecordDamage(enemy.getName(), player.getName(), skillName, player.GetLastDamageResult());
        return true;
    };
    const auto applyDebuff = [&](StatusType type, const EnemySkillDefinition& skill) {
        const int duration = skill.duration > 0 ? skill.duration : 2;
        const bool applied = player.ApplyStatus({type, duration, skill.statusPotency, enemy.getAtk(), 0, enemy.getDefense(), skill.name}, enemy.getAccuracy());
        const auto& result = player.GetStatusApplications().back();
        auto& event = AddEvent(applied ? CombatEvent::Kind::Debuff : CombatEvent::Kind::Resistance,
            player.getName() + (applied ? " receives " : " resists ") + StatusDisplayName(type) +
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
        const bool landed = skill.multiplier <= 0 || damagePlayer((std::max)(1, enemy.getAtk() * skill.multiplier / 100), skill.name, skill.effect == "guaranteed_crit");
        if (ally != nullptr && skill.effect.find("heal") != std::string::npos) {
            const int before = ally->getHP();
            ally->heal((std::max)(1, ally->getMaxHP() * 15 / 100));
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
            enemy.heal((std::max)(1, enemy.getMaxHP() * 15 / 100));
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
            else if (skill.effect == "heal_resistance") buffs.push_back(StatusType::HealResistance);
            else if (skill.effect == "accuracy_floor") buffs.push_back(StatusType::AccuracyFloor);
            else if (skill.effect == "dot_reduction") buffs.push_back(StatusType::DotReduction);
            else if (skill.effect == "accuracy_crit_up") buffs = {StatusType::AccuracyUp, StatusType::CritUp};
            for (StatusType type : buffs) {
                const bool applied = enemy.ApplyStatus({type, skill.duration > 0 ? skill.duration : 2, skill.statusPotency});
                AddEvent(applied ? CombatEvent::Kind::Buff : CombatEvent::Kind::Resistance, enemy.getName() + (applied ? " gains " : " resists ") + StatusDisplayName(type) + ".");
            }
        }
        const bool extraTurn = skill.effect == "extra_turn";
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
    currentEnemyParty_.clear();
    for (Enemy* enemy : enemies) if (enemy != nullptr && enemy->isAlive()) currentEnemyParty_.push_back(enemy);
    std::stable_sort(currentEnemyParty_.begin(), currentEnemyParty_.end(), [](const Enemy* left, const Enemy* right) {
        if (left->getSpeed() != right->getSpeed()) return left->getSpeed() > right->getSpeed();
        return left->getName() < right->getName();
    });
    nextPartyEnemy_ = 0;
    groupEnemyTurn_ = !currentEnemyParty_.empty();
    playerTurn_ = !groupEnemyTurn_;
    enemyGauge_ = groupEnemyTurn_ ? 1000 : 0;
}

bool CombatManager::StepEnemyPartyTurn(Player& player, const std::vector<Enemy*>& enemies, ProgressionManager* progression) {
    if (encounterCompleted_) return false;
    if (!groupEnemyTurn_) PrepareGroupTurn(player, enemies);
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
    playerTurn_ = player.isAlive();
    if (playerTurn_) StartPlayerTurn(player);
    return false;
}

void CombatManager::enemyTurn(Player& player, Enemy& enemy, const std::vector<Enemy*>& enemies, ProgressionManager* progression) {
    PrepareGroupTurn(player, enemies);
    while (StepEnemyPartyTurn(player, enemies, progression)) {}
}

bool CombatManager::IsPlayerTurn() const { return playerTurn_; }
bool CombatManager::HasUsedConsumableThisTurn() const { return consumableUsedThisTurn_; }
