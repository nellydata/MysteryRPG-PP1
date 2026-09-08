#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "GameplayTypes.h"
#include "CombatResolution.h"

class Enemy {
public:
    Enemy(const EnemyDefinition& definition, int level, bool firstClearBoss = false,
          bool farmBoss = false, std::int64_t xpOverride = -1);
    virtual ~Enemy();

    virtual void takeDamage(int amount);
    void heal(int amount);
    bool isAlive() const;
    const std::string& getName() const;
    int getHP() const;
    int getMaxHP() const;
    int getAtk() const;
    std::int64_t getXPReward() const;
    bool CanReceiveDefeatRewards() const;
    bool HasClaimedXPReward() const;
    bool HasClaimedLootReward() const;
    void ForfeitRewards();
    bool IsFirstClearBoss() const;
    bool IsFarmBoss() const;
    const DamageResolution& GetLastDamageResult() const;
    const std::vector<StatusDamageResolution>& GetStatusDamageResults() const;
    const std::vector<StatusApplicationResolution>& GetStatusApplications() const;
    int getDefense() const;
    int getSpeed() const;
    int getLevel() const;
    int getResistance() const;
    int getCritRate() const;
    int getCritDamage() const;
    int getAccuracy() const;
    int getEvasion() const;
    EnemyFaction getFaction() const;
    EnemyTier getTier() const;
    const std::vector<EnemySkillDefinition>& GetSkills() const;
    int GetSkillCooldown(std::size_t index) const;
    bool IsSkillReady(std::size_t index) const;
    void StartSkillCooldown(std::size_t index);
    void TickSkillCooldowns();
    int GetPhase() const;
    bool UpdatePhase();
    bool HasStatus(StatusType type) const;
    const std::vector<StatusEffect>& GetStatuses() const;
    bool ApplyStatus(const StatusEffect& status, int accuracy = 100);
    void ProcessStatuses(std::vector<std::pair<StatusType, int>>& damageEvents = EmptyDamageEvents(),
                         int& healing = EmptyHealing());
    int CleanseNegativeStatuses();
    void ClearStatuses();

protected:
    std::string name_;
    int hp_;
    int maxHp_;
    int atk_;
    std::int64_t xpReward_;
    int defense_;
    int speed_;
    int phaseBaseAtk_;
    int phaseBaseDefense_;
    int phaseBaseSpeed_;
    int level_;
    int resistance_;
    int critRate_;
    int critDamage_;
    int accuracy_;
    int evasion_;
    std::vector<StatusEffect> statuses_;
    EnemyFaction faction_;
    EnemyTier tier_;
    std::vector<EnemySkillDefinition> skills_;
    std::vector<int> skillCooldowns_;
    int phase_;
    bool xpRewardClaimed_;
    bool lootRewardClaimed_;
    bool rewardsForfeited_ = false;
    bool firstClearBoss_;
    bool farmBoss_;
    DamageResolution lastDamageResult_;
    std::vector<StatusDamageResolution> statusDamageResults_;
    std::vector<StatusApplicationResolution> statusApplications_;

private:
    friend class CombatManager;
    friend class EncounterManager;
    bool ClaimXPReward();
    bool ClaimLootReward();
    static std::vector<std::pair<StatusType, int>>& EmptyDamageEvents();
    static int& EmptyHealing();
    int SumStatusBonus(StatusType type) const;
};
