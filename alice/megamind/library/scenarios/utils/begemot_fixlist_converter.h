#pragma once

#include <alice/megamind/protos/scenarios/begemot.pb.h>
#include <search/begemot/rules/alice/fixlist/proto/alice_fixlist.pb.h>

namespace NAlice::NMegamind {

void ConvertBegemotFixlist(const NBg::NProto::TAliceFixlistResult& aliceFixlist,
                           NScenarios::TBegemotFixlistResult& begemotFixlist);

} // namespace NAlice::NMegamind
