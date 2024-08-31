#pragma once

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/classifiers/pre.h>
#include <alice/megamind/library/proactivity/common/common.h>
#include <alice/megamind/library/util/status.h>

namespace NAlice::NMegamind {

TStatus AppHostProactivitySetup(IAppHostCtx& ahCtx,
                                const IContext& ctx,
                                const TRequest& requestModel,
                                const TProactivityStorage& storage,
                                const TScenarioToRequestFrames& scenarioToFrames);

TStatus AppHostProactivityPostSetup(IAppHostCtx& ahCtx, NDJ::NAS::TProactivityResponse& fullInfo);

} // namespace NAlice::NMegamind
