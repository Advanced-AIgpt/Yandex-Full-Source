#include "hardcoded_response.h"

#include "hardcoded_response_fast_data.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/push.h>

#include <alice/library/logger/logger.h>
#include <alice/library/proto/proto.h>
#include <alice/library/scled_animations/scled_animations_builder.h>
#include <alice/library/scled_animations/scled_animations_directive_hw.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/generic/variant.h>
#include <util/string/cast.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

inline constexpr TStringBuf EXP_MM_ENABLE_GRANET_IN_HARDCODED_RESPONSE = "mm_enable_granet_in_hardcoded_responses";
inline constexpr TStringBuf FRAME_NAME = "alice.hardcoded_response";
const TString SLOT_NAME = "hardcoded_response_name";

void AddEasterEgg(TResponseBodyBuilder& bodyBuilder) {

    TScledAnimationBuilder scled;

    scled.AddDraw("U   U ", /* brighness= */ 255, /* durationMs= */ 1000);
    scled.AddDraw(" U:U  ", /* brighness= */ 255, /* durationMs= */ 500);
    scled.AddDraw("U   U ", /* brighness= */ 255, /* durationMs= */ 1000);
    NScledAnimation::AddDrawScled(bodyBuilder, scled);
    bodyBuilder.AddTtsPlayPlaceholderDirective();
    return;
}

TMaybe<TString> GetHardcodedResponseName(const TScenarioRunRequestWrapper& request, TRTLogger& logger) {
    const auto frame = request.Input().FindSemanticFrame(FRAME_NAME);
    if (!frame) {
        LOG_ERR(logger) << "Frame " << FRAME_NAME << " not found";
        return Nothing();
    }
    if (!frame->HasTypedSemanticFrame()) {
        LOG_ERR(logger) << "No typed semantic frame";
        return Nothing();
    }
    if (!frame->GetTypedSemanticFrame().HasHardcodedResponseSemanticFrame()) {
        LOG_ERR(logger) << "No hardcoded semantic frame";
        return Nothing();
    }
    return frame->GetTypedSemanticFrame().GetHardcodedResponseSemanticFrame().GetHardcodedResponseName().GetHardcodedResponseNameValue();
}

bool ShouldListen(const TScenarioRunRequestWrapper& request, TRTLogger& logger,
                  const TGranetHardcodedResponse& hardcodedResponse) {
    return request.Input().IsVoiceInput() &&
           hardcodedResponse.IsApplicable(request, logger) &&
           !hardcodedResponse.HasLink();
}

} // namespace

namespace NImpl {
TLayout::TButton CreateButton(const TString& title, const TString& actionId) {
    TLayout::TButton button;

    button.SetTitle(title);
    button.SetActionId(actionId);
    return button;
}

TDirective CreateDirective(const TString& url) {
    TDirective directive;

    auto* openUriDirective = directive.MutableOpenUriDirective();
    openUriDirective->SetUri(url);
    openUriDirective->SetName("hardcoded_response_open_uri");
    return directive;
}

void DoImplDeprecated(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request) {
    auto& logger = ctx.Ctx.Logger();
    auto nlg = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder{&nlg};
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    TNlgData nlgData{logger, request};

    const auto fastData = ctx.Ctx.GlobalContext().FastData().GetFastData<THardcodedResponseFastData>();
    const auto* ptr = fastData->FindPtr(request.Input().Utterance(),
                                        request.Proto().GetBaseRequest().GetClientInfo().GetAppId(),
                                        request.Proto().GetBaseRequest().GetOptions().GetPromoType());
    if (ptr == nullptr) {
        LOG_INFO(logger) << "HardcodedResponses scenario is irrelevant";
        builder.SetIrrelevant();
        bodyBuilder.AddRenderedTextWithButtonsAndVoice("hardcoded_response", "error", /* buttons = */ {}, nlgData);

        ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
        return;
    }

    LOG_INFO(logger) << "Matched utterance to " << ptr->Name() << " hardcoded response";
    bodyBuilder.CreateAnalyticsInfoBuilder().SetIntentName(ptr->Name());

    THardcodedResponseFastDataProto::TResponse textVoice;
    if (ptr->HasChildResponse() && request.Proto().GetBaseRequest().GetUserClassification().GetAge() == TUserClassification::Child) {
        textVoice = ptr->GetChildResponse(ctx.Rng);
    } else {
        textVoice = ptr->GetResponse(ctx.Rng);
    }
    nlgData.Context["text"] = textVoice.GetText();
    if (!request.Input().IsTextInput()) {
        if (textVoice.GetVoice().empty()) {
            nlgData.Context["voice"] = textVoice.GetText();
        } else {
            nlgData.Context["voice"] = textVoice.GetVoice();
        }
        bodyBuilder.SetShouldListen(true);
    }

    TVector<TLayout::TButton> buttons;
    auto* link = ptr->GetLinkPtr(ctx.Rng);
    if (link != nullptr) {
        TString actionId; // It'll be set in AddAction method
        auto directive = CreateDirective(link->GetUrl());
        bodyBuilder.AddAction(std::move(directive), actionId);

        buttons.push_back(CreateButton(link->GetTitle(), actionId));
    }

    // Add animation for SCLED display
    // easter egg - see https://st.yandex-team.ru/HOLLYWOOD-559
    if (ptr->Name() == "toasts" && request.Proto().GetBaseRequest().GetInterfaces().GetHasScledDisplay()) {
        AddEasterEgg(bodyBuilder);
    }

    bodyBuilder.AddRenderedTextWithButtonsAndVoice("hardcoded_response", "render_result", buttons, nlgData);
    ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
}

void RenderTextVoice(const TScenarioRunRequestWrapper& request, TNlgData& nlgData,
                     const THardcodedResponseFastDataProto::TResponse& textVoice) {
    nlgData.Context["text"] = textVoice.GetText();
    if (!request.Input().IsTextInput()) {
        if (textVoice.GetVoice().empty()) {
            nlgData.Context["voice"] = textVoice.GetText();
        } else {
            nlgData.Context["voice"] = textVoice.GetVoice();
        }
    }
}

void DoImpl(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request) {
    auto& logger = ctx.Ctx.Logger();
    auto nlg = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder{&nlg};
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    TNlgData nlgData{logger, request};

    const auto fastData = ctx.Ctx.GlobalContext().FastData().GetFastData<THardcodedResponseFastData>();
    const auto responseName = GetHardcodedResponseName(request, logger);

    const auto makeIrrelevant = [&]() {
        LOG_INFO(logger) << "HardcodedResponses scenario is irrelevant";
        builder.SetIrrelevant();
        bodyBuilder.AddRenderedTextWithButtonsAndVoice("hardcoded_response", "error", /* buttons = */ {}, nlgData);
        ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
    };

    if (!responseName.Defined()) {
        makeIrrelevant();
        return;
    }
    LOG_INFO(logger) << "responseName: " << (*responseName);
    const auto* hardcodedResponse = fastData->FindPtr(*responseName, request, logger);
    if (hardcodedResponse == nullptr) {
        makeIrrelevant();
        return;
    }

    Y_ENSURE(hardcodedResponse->IsApplicable(request, logger) || hardcodedResponse->HasFallback());

    bodyBuilder.CreateAnalyticsInfoBuilder().SetIntentName(hardcodedResponse->GetIntent());

    if (const auto& psn = hardcodedResponse->GetProductScenarioName(); psn.Defined()) {
        bodyBuilder.GetAnalyticsInfoBuilder().SetProductScenarioName(*psn);
    } else {
        bodyBuilder.GetAnalyticsInfoBuilder().SetProductScenarioName("hardcoded_response");
    }

    if (hardcodedResponse->IsApplicable(request, logger)) {
        LOG_INFO(logger) << "Use main response";
        RenderTextVoice(request, nlgData, hardcodedResponse->GetResponse(ctx.Rng));

        if (hardcodedResponse->HasLink()) {
            const auto& link = hardcodedResponse->GetLink();
            const auto directive = CreateDirective(link.GetUrl());
            bodyBuilder.AddRenderedSuggest(TResponseBodyBuilder::TSuggest{
                .Directives = {directive}, .AutoDirective = directive, .SuggestButton = link.GetTitle()});
        }
    } else {
        LOG_INFO(logger) << "Use fallback response";
        RenderTextVoice(request, nlgData, hardcodedResponse->GetFallbackResponse(ctx.Rng));
        if (hardcodedResponse->HasFallbackPush()) {
            const auto& directive = hardcodedResponse->GetFallbackPush();
            TPushDirectiveBuilder{directive.GetTitle(), directive.GetText(), directive.GetUrl(), /* tag= */ "open_site_or_app"}
                .SetThrottlePolicy("unlimited_policy")
                .SetTtlSeconds(900)
                .BuildTo(bodyBuilder);
        }
    }
    bodyBuilder.SetShouldListen(ShouldListen(request, logger, *hardcodedResponse));

    // Add animation for SCLED display
    // easter egg - see https://st.yandex-team.ru/HOLLYWOOD-559
    if (hardcodedResponse->Name() == "toasts" &&
        request.Proto().GetBaseRequest().GetInterfaces().GetHasScledDisplay()) {
        AddEasterEgg(bodyBuilder);
    }

    bodyBuilder.AddRenderedTextWithButtonsAndVoice("hardcoded_response", "render_result", /* buttons */ {}, nlgData);
    ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
}

} // namespace NImpl

void THardcodedResponseRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    if (request.HasExpFlag(EXP_MM_ENABLE_GRANET_IN_HARDCODED_RESPONSE)) {
        NImpl::DoImpl(ctx, request);
    } else {
        NImpl::DoImplDeprecated(ctx, request);
    }
}

REGISTER_SCENARIO("hardcoded_response",
                  AddHandle<THardcodedResponseRunHandle>()
                  .AddFastData<THardcodedResponseFastDataProto, THardcodedResponseFastData>("hardcoded_response/hardcoded_response.pb")
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NHardcodedResponse::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
