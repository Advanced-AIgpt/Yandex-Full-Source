#pragma once

#include <util/generic/maybe.h>
#include <util/generic/yexception.h>
#include <util/random/mersenne.h>

#include <functional>

namespace NAlice {

struct IRng {
    virtual ~IRng();

    virtual double RandomDouble() = 0;
    virtual ui64 RandomInteger() = 0;

    // Draws from the uniform real distribution on [a, b).
    double RandomDouble(double a, double b) {
        Y_ENSURE(a < b);
        return a + (b - a) * RandomDouble();
    }

    // Draws from the uniform integer distribution on [a, b).
    i64 RandomInteger(i64 a, i64 b) {
        return a + RandomInteger(b - a);
    }

    i64 RandomInteger(i64 limit) {
        Y_ENSURE(limit > 0);
        ui64 rand = RandomInteger();
        return static_cast<i64>(rand % static_cast<ui64>(limit));
    }

    // Compatibility patch with some functions from util/random (e.g. ShuffleRange)
    // Certain template functions from util/random require RNG object with "Uniform" method
    i64 Uniform(i64 limit) {
        return RandomInteger(limit);
    }
};

class TRng : public IRng {
public:
    TRng();
    explicit TRng(ui64 seed);

    using IRng::RandomDouble;
    using IRng::RandomInteger;

    double RandomDouble() override;
    ui64 RandomInteger() override;

private:
    TMersenne<ui64> Engine;
};

class TFakeRng : public IRng {
public:
    using TDoubleGenerator = std::function<double()>;
    using TIntegerGenerator = std::function<ui64()>;

    struct TDoubleTag {};
    struct TIntegerTag {};

    TFakeRng() = default;
    TFakeRng(const TDoubleGenerator& doubleGen, const TIntegerGenerator& intGen);
    explicit TFakeRng(TDoubleTag, const TDoubleGenerator& doubleGen);
    explicit TFakeRng(TIntegerTag, const TIntegerGenerator& intGen);

    using IRng::RandomDouble;
    using IRng::RandomInteger;

    double RandomDouble() override;
    ui64 RandomInteger() override;

private:
    TMaybe<TDoubleGenerator> DoubleGen;
    TMaybe<TIntegerGenerator> IntGen;
};

} // namespace NAlice
