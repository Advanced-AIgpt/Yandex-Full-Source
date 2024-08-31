#include "cec_commands.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <util/generic/maybe.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {
    namespace {
        constexpr TStringBuf NLG_TEMPLATE_NAME = "cec_commands";

        constexpr TStringBuf SCREEN_ON_FRAME = "alice.screen_on";
        constexpr TStringBuf SCREEN_OFF_FRAME = "alice.screen_off";
        constexpr std::initializer_list<TStringBuf> FRAMES = {SCREEN_ON_FRAME, SCREEN_OFF_FRAME};

        TDirective CreateScreenOnDirective() {
            TDirective directive;
            auto& screenOn = *directive.MutableScreenOnDirective();
            screenOn.SetName("screen_on");
            return directive;
        }

        TDirective CreateScreenOffDirective() {
            TDirective directive;
            auto& screenOff = *directive.MutableScreenOffDirective();
            screenOff.SetName("screen_off");
            return directive;
        }
    }

    void TCecCommandsHandle::Do(TScenarioHandleContext& ctx) const {
        const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
        const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
        const auto input = request.Input();

        TMaybe<TFrame> curFrame{};

        for (const auto supportedFrame : FRAMES) {
            if (input.FindSemanticFrame(supportedFrame) != nullptr) {
                curFrame = input.CreateRequestFrame(supportedFrame);
                break;
            }
        }

        TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
        TRunResponseBuilder builder(&nlgWrapper);
        auto& bodyBuilder = builder.CreateResponseBodyBuilder(curFrame.Get());

        auto& analyticsInfo = bodyBuilder.CreateAnalyticsInfoBuilder();
        analyticsInfo.SetProductScenarioName("cec_commands");

        const bool isCecAvailable = request.Interfaces().GetHasCEC();
        const bool isTvPlugged = request.Interfaces().GetIsTvPlugged();

        TStringBuf phraseName = "something_strange"; // default fallback

        if (curFrame.Empty()) {
            phraseName = "something_strange";
            builder.SetIrrelevant();
        } else if (!isCecAvailable) {
            phraseName = "no_cec_available";
            builder.SetIrrelevant();
        } else if (!isTvPlugged) {
            phraseName = "tv_not_plugged";
            builder.SetIrrelevant();  // TODO(vl-trifonov) logic leak (want to fallback in other scenarios in this case)
        } else if (curFrame->Name() == SCREEN_ON_FRAME) {
            phraseName = "turn_tv_on";
            bodyBuilder.AddDirective(std::move(CreateScreenOnDirective()));
        } else if (curFrame->Name() == SCREEN_OFF_FRAME) {
            phraseName = "turn_tv_off";
            bodyBuilder.AddDirective(std::move(CreateScreenOffDirective()));
        }

        TNlgData nlgData{ctx.Ctx.Logger(), request};
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(
            NLG_TEMPLATE_NAME, phraseName, /* buttons = */ {}, nlgData);

        auto response = std::move(builder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
    }

    REGISTER_SCENARIO(
        "cec_commands",
        AddHandle<TCecCommandsHandle>()
            .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NCecCommands::NNlg::RegisterAll));
}
