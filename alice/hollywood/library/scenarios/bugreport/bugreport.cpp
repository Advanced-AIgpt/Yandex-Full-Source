#include "bugreport.h"

#include <alice/hollywood/library/scenarios/bugreport/nlg/register.h>

#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/request/request.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf MSG_RENDER_RESULT = "render_result";

} // namespace

void TBugreportRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    TNlgData nlgData{ctx.Ctx.Logger(), request};

    bodyBuilder.AddRenderedTextWithButtonsAndVoice("bugreport", MSG_RENDER_RESULT, /* buttons = */ {}, nlgData);

    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

REGISTER_SCENARIO(
    "bugreport",
    AddHandle<TBugreportRunHandle>()
        .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NBugreport::NNlg::RegisterAll)
);

} // namespace NAlice::NHollywood
