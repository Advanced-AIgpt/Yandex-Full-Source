#pragma once

#include <alice/megamind/protos/scenarios/external_markup.pb.h>
#include <search/begemot/rules/external_markup/proto/format.pb.h>

namespace NAlice::NMegamind {

void ConvertExternalMarkup(const NBg::NProto::TExternalMarkupProto& begemotMarkup,
                           NScenarios::TBegemotExternalMarkup& externalMarkup);

} // namespace NAlice::NMegamind
