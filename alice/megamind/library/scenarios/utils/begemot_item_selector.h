#pragma once

#include <alice/megamind/protos/scenarios/begemot.pb.h>
#include <search/begemot/rules/alice/item_selector/proto/alice_item_selector.pb.h>

namespace NAlice::NMegamind {

void ConvertBegemotItemSelector(const NBg::NProto::TAliceItemSelectorResult& aliceItemSelector,
                                NScenarios::TBegemotItemSelectorResult& begemotItemSelector);

} // namespace NAlice::NMegamind
