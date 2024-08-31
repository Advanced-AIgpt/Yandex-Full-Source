#pragma once

#include <util/generic/string.h>

namespace NAlice::NJokerLight {

struct TStubId {
    TString SessionId;
    TString ReqId;
    TString Hash;
};

} // namespace NAlice::NJokerLight
