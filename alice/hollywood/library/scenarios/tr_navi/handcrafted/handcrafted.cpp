#include "handcrafted.h"

#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/tr_navi/handcrafted/nlg/register.h>

namespace NAlice::NHollywood::NTrNavi {

namespace {

constexpr TStringBuf ANALYTICS_HANDCRAFTED_SCENARIO_NAME = "alice.vinsless.handcrafted";
constexpr TStringBuf TEMPLATE_HANDCRAFTED = "handcrafted";
constexpr TStringBuf MICROINTENTS_FRAME = "alice.microintents";
constexpr TStringBuf MICROINTENT_NAME_SLOT = "name";

bool TryParseMicrointent(const TPtrWrapper<NAlice::TSemanticFrame>& frameProto, TString& microintent) {
    if (!frameProto) {
        return false;
    }
    const auto frame = TFrame::FromProto(*frameProto);
    const auto microintentNameSlot = frame.FindSlot(MICROINTENT_NAME_SLOT);
    if (!microintentNameSlot) {
        return false;
    }
    microintent = microintentNameSlot->Value.AsString();
    return true;
}

} // namespace

void THandcraftedTrRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder responseBuilder(&nlgWrapper);
    auto& bodyBuilder = responseBuilder.CreateResponseBodyBuilder();

    TString microintent;
    const auto frame = request.Input().FindSemanticFrame(MICROINTENTS_FRAME);
    if (!TryParseMicrointent(frame, microintent)) {
        LOG_WARNING(ctx.Ctx.Logger()) << "Failed to get " << MICROINTENTS_FRAME << " semantic frame";
        responseBuilder.SetIrrelevant();
        responseBuilder.CreateResponseBodyBuilder();
    } else {
        TNlgData nlgData{ctx.Ctx.Logger(), request};
        nlgData.Context["microintent"] = microintent;
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_HANDCRAFTED, "render_result", /* buttons = */ {}, nlgData);
        const auto intentName = TString::Join(ANALYTICS_HANDCRAFTED_SCENARIO_NAME, ".", microintent);
        bodyBuilder.CreateAnalyticsInfoBuilder().SetIntentName(intentName);
    }

    const auto response = std::move(responseBuilder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

REGISTER_SCENARIO("handcrafted_tr",
                  AddHandle<THandcraftedTrRunHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTrNavi::NHandcrafted::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NTrNavi
