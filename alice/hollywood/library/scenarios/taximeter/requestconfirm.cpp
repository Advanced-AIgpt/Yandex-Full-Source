#include "requestconfirm.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/library/proto/proto.h>

#include <util/generic/variant.h>
#include <util/string/cast.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf FRAME = "alice.taximeter.requestconfirm_order_offer";

} // namespace

void TTaximeterRequestConfirmRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    TNlgData nlgData{ctx.Ctx.Logger(), request};

    // check if the text command was parsed by Behemoth
    if (!request.ClientInfo().IsTaximeter() || !request.Input().FindSemanticFrame(FRAME)) {
        builder.SetIrrelevant();
        auto& bodyBuilder = builder.CreateResponseBodyBuilder();
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(
            "requestconfirm", "not_ok", /* buttons = */ {}, nlgData);
        ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
        return;
    }

    // read response from semantic_frame - yes or no, and add voice pronunciation
    const auto frame = request.Input().CreateRequestFrame(FRAME);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);
    bodyBuilder.CreateAnalyticsInfoBuilder().SetIntentName(TString{FRAME});
    if (const auto command = frame.FindSlot(TStringBuf("confirmation"))) {
        const auto value = command->Value.AsString();
        if (value == "yes") {
            NJson::TJsonValue deeplink;
            deeplink["uri"] = "taximeter://income_order?action=accept";
            bodyBuilder.AddClientActionDirective("open_uri", "requestconfirm_accept", deeplink);

            bodyBuilder.AddRenderedTextWithButtonsAndVoice(
                "requestconfirm", "ok_yes", /* buttons = */ {}, nlgData);
        } else if (value == "no") {
            NJson::TJsonValue deeplink;
            deeplink["uri"] = "taximeter://income_order?action=decline";
            bodyBuilder.AddClientActionDirective("open_uri", "requestconfirm_decline", deeplink);

            bodyBuilder.AddRenderedTextWithButtonsAndVoice(
                "requestconfirm", "ok_no", /* buttons = */ {}, nlgData);
        }
    } else {
      bodyBuilder.AddRenderedTextWithButtonsAndVoice(
          "requestconfirm", "not_ok", /* buttons = */ {}, nlgData);
    }

    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

REGISTER_SCENARIO("taximeter", AddHandle<TTaximeterRequestConfirmRunHandle>()
                               .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTaximeter::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
