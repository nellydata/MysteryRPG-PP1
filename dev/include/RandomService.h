#pragma once
#include <algorithm>
#include <cstdint>
#include <random>

// Every uncertain decision owns a draw. Certain outcomes consume none.
// Explicit integer rejection sampling makes seeded replay independent of the
// standard library's uniform_distribution implementation.
class RandomStream {
public:
    explicit RandomStream(unsigned seed = 0) : engine_(seed) {}
    void Seed(unsigned seed) { engine_.seed(seed); draws_ = 0; }
    std::uint32_t Next() { ++draws_; return engine_(); }
    bool RollPercent(int chance) {
        if (chance <= 0) return false;
        if (chance >= 100) return true;
        constexpr std::uint32_t limit = 4294967200u;
        std::uint32_t value;
        do { value = Next(); } while (value >= limit);
        return value % 100u < static_cast<unsigned>(chance);
    }
    std::uint64_t DrawCount() const { return draws_; }
private:
    std::mt19937 engine_;
    std::uint64_t draws_ = 0;
};
