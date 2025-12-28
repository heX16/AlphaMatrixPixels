#pragma once

#include <stdint.h>

namespace amp {

// Special random generator
class csRandGen {
public:

    // X(n+1) = (2053 * X(n)) + 13849)
    static constexpr auto RAND16_2053 = ((uint16_t)(2053));
    static constexpr auto RAND16_13849 = ((uint16_t)(13849));
    static constexpr auto RAND16_SEED = 1337;

    /// random number seed
    uint16_t rand_seed;

    csRandGen(uint16_t set_rand_seed = RAND16_SEED) {
        rand_seed = set_rand_seed;
    };

    /// Generate an 8-bit random number
    uint8_t rand()
    {
        // hard test
        //return 0;
        // test
        //return this->rand_seed++;

        rand_seed = (this->rand_seed * RAND16_2053) + RAND16_13849;
        // return first uint8_t (for more speed)
        return (uint8_t)(this->rand_seed & 0xFF);

        // return the sum of the high and low uint8_ts, for better
        //  mixing and non-sequential correlation
        //return (uint8_t)(((uint8_t)(this->rand_seed & 0xFF)) +
        //                 ((uint8_t)(this->rand_seed >> 8)));
    }

    /// Generate an 8-bit random number between 0 and lim
    /// @param lim the upper bound (not inclusive) for the result
    uint8_t rand(uint8_t lim)
    {
        uint8_t r = this->rand();
        r = (r*lim) >> 8;
        return r;
    }

    /// Generate an 8-bit random number in the given range
    /// @param min the lower bound for the random number
    /// @param max upper bound (inclusive) for the random number
    uint8_t randRange(uint8_t min, uint8_t max)
    {
        uint8_t delta = max - min + 1;
        uint8_t r = this->rand(delta) + min;
        return r;
    }

    /// Add entropy into the random number generator
    void addEntropy(uint16_t entropy)
    {
        this->rand_seed += entropy;
    }

};

} // namespace amp

