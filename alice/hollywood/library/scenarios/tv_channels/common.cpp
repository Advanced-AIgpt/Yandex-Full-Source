#include "common.h"

#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/logger/logger.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <library/cpp/iterator/mapped.h>

#include <util/string/cast.h>

#include <memory>
#include <utility>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NTvChannels {

std::unique_ptr<TScenarioRunResponse> RenderIrrelevant(TRTLogger& logger, const TScenarioHandleContext& ctx,
                                                       const TScenarioRunRequestWrapper& request,
                                                       TMaybe<TFrame> frame) {
    LOG_DEBUG(logger) << "Returning irrelevant result";
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder(frame.Get());
    TNlgData nlgData{logger, request};

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_TEMPLATE_NAME, "live_tv_unavailable", /* buttons = */ {},
                                                   nlgData);

    builder.SetIrrelevant();

    return std::move(builder).BuildResponse();
}
} // namespace NAlice::NHollywood::NTvChannels
