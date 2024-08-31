#include "factors.h"

namespace NAlice::NSearch {

TFactorsMap ParseFactors(TStringBuf relev) {
    TFactorsMap factors;
    while (!relev.Empty()) {
        TStringBuf value;

        relev.NextTok(';', value);
        if (!value.Empty()) {
            TStringBuf name;
            value.NextTok('=', name);
            factors.emplace(name, value);
        }

    }
    return factors;
}

} // namespace NAlice::NSearch
