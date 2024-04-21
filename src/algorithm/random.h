#ifndef _LEVEL_DB_XY_RANDOM_H_
#define _LEVEL_DB_XY_RANDOM_H_

#include <cstdint>

namespace ns_algorithm {


class Random {
private:
    uint32_t seed_;

public:
    explicit Random(uint32_t s) :
        seed_(s & 0x7FFFFFFFU) {
        if (seed_ == 0 || seed_ == INT32_MAX) {
            seed_ = 1;
        }
    }
    uint32_t Next() {
        static constexpr uint32_t M = INT32_MAX;
        static constexpr uint64_t A = 16807;
        uint64_t product = seed_ * A;
        seed_ = static_cast<uint32_t>((product >> 31) + (product & M));
        if (seed_ > M) {
            seed_ -= M;
        }
        return seed_;
    }

    uint32_t Uniform(int32_t n) {
        return Next() % n;
    }
    bool OneIn(int32_t n) {
        return (Next() % n) == 0;
    }
    uint32_t Skewed(int32_t max_log) {
        return Uniform(1 << Uniform(max_log + 1));
    }
};

} // ns_algorithm

#endif