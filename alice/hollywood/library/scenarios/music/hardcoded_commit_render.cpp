#include "commit_render_handle.h"
#include "intents.h"

#include <alice/hollywood/library/datasync_adapter/datasync_adapter.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>

#include <alice/library/analytics/common/product_scenarios.h>


using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NMusic {

void TCommitMusicHardcodedRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};
    TCommitResponseBuilder builder;

    const auto datasyncResponse = RetireDataSyncResponseItemsSafe(ctx);
    if (datasyncResponse.Defined()) {
        builder.SetSuccess();
    } else {
        builder.SetError("hardcoded_music_commit_error", "commit failed");
    }

    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood
