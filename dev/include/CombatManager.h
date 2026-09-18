#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <set>
#include "CombatResolution.h"
#include "RandomService.h"

class Player;
class Enemy;
class ShimmerSkill;
class ProgressionManager;

enum class CombatAction {
    Attack,
    Item,
    Shimmer,
    Flee
};

class CombatManager {
public:
    struct CombatEvent {
        enum class Kind {
            Turn, Attack, Skill, Damage, DotTick, Heal, Shield, Buff, Debuff,
            Cleanse, ExtraTurn, PhaseTransition, Shimmer, Mutation, Death,
            Revive, Environment, Status, Info, EncounterStart, Hit, Miss, Critical,
            Resistance, BarrierBreak, Reward, Completion
        };
        Kind kind;
        std::string message;
        int value = 0;
        std::uint64_t sequenceId = 0;
        std::string actor;
        std::string target;
        std::string skill;
        int requestedDamage = 0;
        int mitigatedDamage = 0;
        int overkillDamage = 0;
        int barrierAbsorbed = 0;
        int hpDamage = 0;
        int remainingHP = -1;
        int actorLevel = 0;
        int targetLevel = 0;
        int playerGauge = 0;
        int enemyGauge = 0;
        EnemyTier enemyTier = EnemyTier::Basic;
    };
    explicit CombatManager(unsigned seed = 0);
    void SetSeed(unsigned seed);
    std::uint64_t GetRandomDrawCount() const { return random_.DrawCount(); }
    ~CombatManager();

    void BeginEncounter(Enemy& enemy);
    void BeginEncounter(Player& player, Enemy& enemy, int playerLevel);
    void ContinueEncounter(Enemy& enemy);
    void PrepareTurn(Player& player, Enemy& enemy);
    void PrepareGroupTurn(Player& player, const std::vector<Enemy*>& enemies);
    void ResumeAfterRespawn(Player& player, Enemy& enemy);
    bool resolve(CombatAction action, Player& player, Enemy& enemy,
                 ShimmerSkill* skill = nullptr,
                 ProgressionManager* progression = nullptr);
    bool UseConsumable(const std::string& itemName, Player& player, Enemy& enemy,
                       ProgressionManager* progression = nullptr);
    bool UseConsumable(unsigned long long instanceId, Player& player, Enemy& enemy,
                       ProgressionManager* progression = nullptr);
    bool CanUseConsumableNow(const Player& player, const Enemy* target = nullptr) const;
    bool HasActiveShimmerCooldowns() const;
    int ReduceShimmerCooldowns(int turns);
    void enemyTurn(Player& player, Enemy& enemy, ProgressionManager* progression = nullptr);
    void enemyTurn(Player& player, Enemy& enemy, const std::vector<Enemy*>& enemies,
                   ProgressionManager* progression = nullptr);
    bool StepEnemyPartyTurn(Player& player, const std::vector<Enemy*>& enemies,
                           ProgressionManager* progression = nullptr);
    const ShimmerSkill* GetDefaultSkill() const;
    bool IsPlayerTurn() const;
    int GetPlayerGauge() const { return playerGauge_; }
    int GetEnemyGauge() const { return enemyGauge_; }
    bool HasUsedConsumableThisTurn() const;
    const std::vector<CombatEvent>& GetEvents() const;
    void RecordInfo(const std::string& message);
    void RecordReward(const std::string& message);
    void RecordEnvironmentalDefeat(const std::string& source, const Player& player);
    void CompleteEncounter(const std::string& outcome);
    int GetSkillCooldown(const std::string& skillName) const;
    void ClearEvents();
    void ResetEncounter();
    // Combat and the shared victory fallback use this same XP delivery path.
    // Returns the XP actually accepted (zero at level 100 or after saturation).
    static std::int64_t DeliverDefeatXP(Player& player, Enemy& enemy, ProgressionManager& progression);

private:
    bool blocked_;
    bool weaknessExposed_;
    int turnNumber_;
    bool playerTurn_;
    int playerGauge_;
    int enemyGauge_;
    Enemy* activeEnemy_;
    Player* activePlayer_ = nullptr;
    RandomStream random_;
    bool playerActionStarted_ = false;
    bool nextEnemyBonus_ = false;
    std::unique_ptr<ShimmerSkill> defaultSkill_;
    std::vector<CombatEvent> events_;
    std::uint64_t nextEventSequence_ = 1;
    std::size_t nextPartyEnemy_ = 0;
    bool encounterCompleted_ = false;
    bool consumableUsedThisTurn_;
    bool battleStartEffectsApplied_ = false;
    bool groupEnemyTurn_ = false;
    std::map<std::string, int> shimmerCooldowns_;
    std::vector<Enemy*> currentEnemyParty_;
    std::set<Enemy*> seededEnemies_;
    int normalActions_ = 0;
    int offensiveShimmerCasts_ = 0;
    int markedShimmerCasts_ = 0;
    bool firstDirectAttackUsed_ = false;
    bool firstCriticalThisTurn_ = false;
    bool dotCooldownUsedThisTurn_ = false;
    bool barrierBreakUsedThisTurn_ = false;
    bool freeCooldownUsedThisTurn_ = false;
    bool enemyHitTriggeredThisAction_ = false;
    bool echoArmed_ = false;
    bool bonusPlayerAction_ = false;
    std::uint64_t artifactEquipmentRevision_ = 0;

    CombatEvent& AddEvent(CombatEvent::Kind kind, const std::string& message, int value = 0);
    void RecordDamage(const std::string& actor, const std::string& target,
                      const std::string& skill, const DamageResolution& result,
                      CombatEvent::Kind kind = CombatEvent::Kind::Damage);
    void StartPlayerTurn(Player& player);
    void AdvanceTurn(Player& player, Enemy& enemy);
    void FinishPlayerAction(Player& player, Enemy& enemy, bool normalAction = false);
    void ResolveDefeatReward(Player& player, Enemy& enemy, ProgressionManager* progression);
    void ForfeitEncounterRewards();
    void ReduceLongestCooldown(Player& player, bool offensiveOnly = false);
    void OnPlayerDotDamage(Player& player, const StatusDamageResolution& tick);
    void OnPlayerDirectHit(Player& player, Enemy& enemy, bool critical);
    void SyncArtifactEquipment(Player& player);

    // Enemy-turn breakdown: each concern lives in its own short helper so the
    // orchestrating enemyTurn() stays small and individually readable.
    struct EnemySkillContext {
        Player& player;
        Enemy& enemy;
        bool bonusAction;
        std::vector<Enemy*>& party;
    };
    void FinishEnemyAction(Player& player, Enemy& enemy, bool bonusAction, bool extraTurn);
    bool AttemptEnemySkillAttack(const EnemySkillDefinition& skill, EnemySkillContext& context);
    Enemy* FindLivingAlly(Enemy& enemy) const;
    void ApplyEnemyDebuff(Player& player, Enemy& enemy, StatusType type, const EnemySkillDefinition& skill);
    void ApplyEnemySkillEffect(EnemySkillContext& context, const EnemySkillDefinition& skill, bool landed);
    bool RunEnemySkillIndex(EnemySkillContext& context, std::size_t index);
};
