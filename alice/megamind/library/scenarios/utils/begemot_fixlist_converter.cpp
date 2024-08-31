#include "begemot_fixlist_converter.h"

namespace NAlice::NMegamind {

void ConvertBegemotFixlist(const NBg::NProto::TAliceFixlistResult& aliceFixlist,
                           NScenarios::TBegemotFixlistResult& begemotFixlist)
{
    for (const auto& [key, value] : aliceFixlist.GetMatches()) {
        auto& match = *begemotFixlist.AddMatches();
        match.SetKey(key);
        *match.MutableValue()->MutableIntents() = value.GetIntents();
    }
}

} // namespace NAlice::NMegamind
