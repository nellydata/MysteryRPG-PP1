#pragma once
#include "CombatMath.h"
#include "CombatResolution.h"
#include "RandomService.h"
#include <atomic>
#include <limits>

namespace StatusRules {
inline std::uint64_t NextIdentity() {
    static std::atomic<std::uint64_t> next{0};
    return ++next;
}
inline bool Negative(StatusType type) {
    switch (type) {
    case StatusType::Stun: case StatusType::Bleed: case StatusType::Burn:
    case StatusType::Poison: case StatusType::Slow: case StatusType::VerdigrisMark:
    case StatusType::AttackDown: case StatusType::DefenseDown: case StatusType::AccuracyDown:
    case StatusType::HealResistance: return true;
    default: return false;
    }
}
inline int Sum(const std::vector<StatusEffect>& statuses, StatusType type) {
    std::int64_t sum = 0;
    for (const auto& status : statuses) if (status.type == type) sum += status.potency;
    return CombatMath::Saturate(sum);
}
inline int Barriers(const std::vector<StatusEffect>& statuses) { return Sum(statuses, StatusType::Barrier); }
inline bool SameSource(const StatusEffect& a, const StatusEffect& b) {
    if (a.sourceId || b.sourceId) return a.sourceId != 0 && a.sourceId == b.sourceId;
    return !a.source.empty() && a.source == b.source;
}
inline bool Apply(std::vector<StatusEffect>& statuses, std::vector<StatusApplicationResolution>& applications,
                  StatusEffect status, bool alive, int accuracy, int resistance, int durationReduction, RandomStream& random) {
    applications.push_back({status, false});
    if (!alive || static_cast<int>(status.type) < 0 || status.type > StatusType::AccuracyFloor ||
        status.potency < 0 || status.turns < 0 || (status.turns == 0 && status.type != StatusType::Barrier)) return false;
    if (status.type == StatusType::VerdigrisMark) { status.turns = 3; status.potency = 20; }
    if (Negative(status.type)) {
        if (!random.RollPercent(CombatMath::AccuracyChance(100, accuracy, resistance))) {
            applications.back().resisted = true;
            return false;
        }
        status.turns = (std::max)(1, CombatMath::Scale(status.turns, 100 -
            (std::clamp)(durationReduction, 0, CombatMath::Balance::MaximumDebuffReduction)));
    }
    if (status.type == StatusType::Barrier) {
        status.potency = (std::min)(status.potency, (std::numeric_limits<int>::max)() - Barriers(statuses));
        if (status.potency <= 0) return false;
    }
    status.applicationId = NextIdentity();
    const auto policy = GetStatusStackPolicy(status.type);
    if (policy != StackPolicy::IndependentStacks) {
        for (auto& existing : statuses) {
            if (existing.type != status.type || (policy == StackPolicy::UniquePerSource && !SameSource(existing, status))) continue;
            if (policy == StackPolicy::ReplaceIfStronger && existing.potency > status.potency) return false;
            if (policy == StackPolicy::RefreshDuration || policy == StackPolicy::UniquePerSource ||
                existing.potency == status.potency) status.turns = (std::max)(existing.turns, status.turns);
            existing = status;
            applications.back() = {status, true};
            return true;
        }
    }
    statuses.push_back(status);
    applications.back() = {status, true};
    return true;
}
inline int Absorb(std::vector<StatusEffect>& statuses, int damage) {
    int remaining = (std::max)(0, damage);
    while (remaining > 0) {
        StatusEffect* first = nullptr;
        for (auto& status : statuses) {
            if (status.type != StatusType::Barrier || status.potency <= 0) continue;
            if (!first || (status.turns > 0 && (first->turns == 0 || status.turns < first->turns))) first = &status;
        }
        if (!first) break;
        const int absorbed = (std::min)(remaining, first->potency);
        first->potency -= absorbed;
        remaining -= absorbed;
    }
    statuses.erase(std::remove_if(statuses.begin(), statuses.end(), [](const StatusEffect& status) {
        return status.type == StatusType::Barrier && status.potency == 0;
    }), statuses.end());
    return remaining;
}
inline DamageResolution Damage(int& hp, std::vector<StatusEffect>& statuses, int raw, int defense, int reduction) {
    DamageResolution result;
    result.requestedDamage = (std::max)(0, raw);
    result.remainingHP = hp;
    if (raw <= 0 || hp <= 0) return result;
    int remaining = CombatMath::Scale(CombatMath::Mitigate(raw, defense), 100 - (std::clamp)(reduction, 0, 100));
    result.mitigatedDamage = raw - remaining;
    const int barriers = Barriers(statuses);
    remaining = Absorb(statuses, remaining);
    result.barrierAbsorbed = barriers - Barriers(statuses);
    result.barrierBroken = barriers > 0 && Barriers(statuses) == 0;
    result.hpDamage = (std::min)(hp, remaining);
    result.overkillDamage = remaining - result.hpDamage;
    hp -= result.hpDamage;
    result.remainingHP = hp;
    return result;
}
inline HealResolution Heal(int& hp, int maxHP, int amount, int resistance, int potency, int received) {
    HealResolution result;
    result.requestedHealing = (std::max)(0, amount);
    result.remainingHP = hp;
    if (amount <= 0 || hp <= 0) return result;
    amount = CombatMath::Scale(amount, 100 - (std::clamp)(resistance, 0, 100));
    amount = CombatMath::Scale(amount, 100 + (std::clamp)(potency, 0, CombatMath::Balance::MaximumHealingPotency));
    amount = CombatMath::Scale(amount, 100 + (std::clamp)(received, 0, CombatMath::Balance::MaximumHealingPotency));
    result.effectiveHealing = amount;
    result.hpRestored = (std::min)((std::max)(0, maxHP - hp), amount);
    result.overheal = amount - result.hpRestored;
    hp += result.hpRestored;
    result.remainingHP = hp;
    return result;
}
inline int DotDamage(const StatusEffect& status, int maxHP, const std::vector<StatusEffect>& statuses) {
    int base = status.type == StatusType::Bleed ? CombatMath::Scale(status.sourceAttack, 30) :
        status.type == StatusType::Burn ? CombatMath::Scale(status.sourceAttack, 60) :
        status.type == StatusType::Poison ? CombatMath::Scale(maxHP, 5) : 0;
    base = CombatMath::Scale(base, CombatMath::Saturate(100LL + status.dotPotency));
    for (const auto& mark : statuses) if (mark.type == StatusType::VerdigrisMark && SameSource(mark, status)) {
        base = CombatMath::Scale(base, 120);
        break;
    }
    return base;
}
inline void FinishTurn(std::vector<StatusEffect>& statuses, std::vector<std::uint64_t>& snapshot) {
    for (auto& status : statuses) if (status.turns > 0 &&
        std::find(snapshot.begin(), snapshot.end(), status.applicationId) != snapshot.end()) --status.turns;
    statuses.erase(std::remove_if(statuses.begin(), statuses.end(), [&](const StatusEffect& status) {
        return (status.turns == 0 && status.type != StatusType::Barrier) ||
            (status.turns == 0 && std::find(snapshot.begin(), snapshot.end(), status.applicationId) != snapshot.end() &&
                status.type == StatusType::Barrier);
    }), statuses.end());
    snapshot.clear();
}
inline int Cleanse(std::vector<StatusEffect>& statuses) {
    const auto before = statuses.size();
    statuses.erase(std::remove_if(statuses.begin(), statuses.end(), [](const StatusEffect& status) {
        return Negative(status.type);
    }), statuses.end());
    return static_cast<int>(before - statuses.size());
}
}
