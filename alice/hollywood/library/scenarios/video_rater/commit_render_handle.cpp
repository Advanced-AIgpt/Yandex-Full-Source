#include "commit_render_handle.h"

#include <alice/hollywood/library/datasync_adapter/datasync_adapter.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/json/json.h>
#include <alice/library/proto/proto.h>

#include <library/cpp/resource/resource.h>

#include <util/generic/variant.h>
#include <util/random/shuffle.h>
#include <util/string/cast.h>

using namespace NAlice::NScenarios;
using namespace NAlice::NHollywood;

namespace NAlice::NHollywood::NVideoRater {

void TVideoRaterCommitRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    TCommitResponseBuilder builder;

    builder.SetSuccess();

    auto response = std::move(builder).BuildResponse();

    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);

    LOG_INFO(ctx.Ctx.Logger()) << "Video Rater: successfully added commit response";
}

} // namespace NAlice::NHollywood::NVideoRater
