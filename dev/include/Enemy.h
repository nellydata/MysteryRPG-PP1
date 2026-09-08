#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "GameplayTypes.h"

class Enemy {
public:
    Enemy(const EnemyDefinition& definition, int level);
    virtual ~Enemy();

    virtual void takeDamage(int amount);
    void heal(int amount);
    bool isAlive() const;
    const std::string& getName() const;
    int getHP() const;
    int getMaxHP() const;
    int getAtk() const;
    std::int64_t getXPReward() const;
    bool ClaimXPReward();
    bool ClaimInvestigationReward();
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
    void ClearStatuses();

protected:
    std::string name_;
    int hp_;
    int maxHp_;
    int atk_;
    std::int64_t xpReward_;
    int defense_;
    int speed_;
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
    bool investigationRewardClaimed_;

private:
    static std::vector<std::pair<StatusType, int>>& EmptyDamageEvents();
    static int& EmptyHealing();
    int SumStatusBonus(StatusType type) const;
};
