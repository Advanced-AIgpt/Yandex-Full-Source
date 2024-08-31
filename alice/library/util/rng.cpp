#include "rng.h"

#include <util/random/random.h>

namespace NAlice {

// IRng ------------------------------------------------------------------------

IRng::~IRng() = default;

// TRng ------------------------------------------------------------------------

TRng::TRng()
    : TRng(RandomNumber<ui64>()) {
}

TRng::TRng(ui64 seed)
    : Engine(seed) {
}

double TRng::RandomDouble() {
    return Engine.GenRandReal4(); // draws from the [0, 1) interval
}

ui64 TRng::RandomInteger() {
    return Engine.GenRand();
}

// TFakeRng --------------------------------------------------------------------

TFakeRng::TFakeRng(const TFakeRng::TDoubleGenerator& doubleGen, const TFakeRng::TIntegerGenerator& intGen)
    : DoubleGen(doubleGen)
    , IntGen(intGen) {
}

TFakeRng::TFakeRng(TFakeRng::TDoubleTag, const TFakeRng::TDoubleGenerator& doubleGen)
    : DoubleGen(doubleGen) {
}

TFakeRng::TFakeRng(TFakeRng::TIntegerTag, const TFakeRng::TIntegerGenerator& intGen)
    : IntGen(intGen) {
}

double TFakeRng::RandomDouble() {
    if (DoubleGen.Defined()) {
        return (*DoubleGen)();
    }

    return 0;
}

ui64 TFakeRng::RandomInteger() {
    if (IntGen.Defined()) {
        return (*IntGen)();
    }

    return 0;
}

} // namespace NAlice
