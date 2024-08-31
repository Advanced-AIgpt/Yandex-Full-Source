#include "status.h"

template<>
void Out<NAlice::NJokerLight::TError>(IOutputStream& out, const NAlice::NJokerLight::TError& error) {
    out << error.AsString();
}
