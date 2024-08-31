#pragma once

#include <library/cpp/dbg_output/dump.h>
#include <util/generic/string.h>

namespace NAlice::NHollywood::NFood {

    struct TAuthInput {
        TString Phone;
        TString YandexUid;
        TString TaxiUid;
    };

} // namespace NAlice::NHollywood::NFood

DEFINE_DUMPER(NAlice::NHollywood::NFood::TAuthInput, Phone, YandexUid, TaxiUid);
