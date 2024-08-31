#include "general_conversation.h"

#include <alice/bass/forms/automotive/media_control.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/navigator/route_intents.h>
#include <alice/bass/forms/route_helpers.h>
#include <alice/bass/forms/route.h>

#include <alice/bass/libs/config/config.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/string/builder.h>

namespace NBASS {

namespace {

constexpr TStringBuf GeneralConversationFormName = "personal_assistant.general_conversation.general_conversation";
constexpr TStringBuf GeneralConversationDummyFormName = "personal_assistant.general_conversation.general_conversation_dummy";
constexpr TStringBuf GeneralConversationMicrointentsFormName = "personal_assistant.general_conversation.microintents.*";
constexpr TStringBuf HandcraftedFormName = "personal_assistant.handcrafted.*";
constexpr TStringBuf NegativeFeedBackFormName = "personal_assistant.handcrafted.user_reactions_negative_feedback";
constexpr TStringBuf PositiveFeedBackFormName = "personal_assistant.handcrafted.user_reactions_positive_feedback";

} // namespace

TGeneralConversationFormHandler::TGeneralConversationFormHandler() {
}

TResultValue TGeneralConversationFormHandler::Do(TRequestHandler& r) {
    auto& ctx = r.Ctx();
    if (ctx.FormName() == NegativeFeedBackFormName || ctx.FormName() == PositiveFeedBackFormName) {
        ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::FEEDBACK);
    } else {
        ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::GENERAL_CONVERSATION);
    }
    if (!ctx.Meta().PureGC() &&
        (
            ctx.FormName() == TStringBuf("personal_assistant.handcrafted.cancel") ||
            ctx.FormName() == TStringBuf("personal_assistant.handcrafted.fast_cancel")
        )
    ) {
        ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::STOP);
        if (ctx.MetaClientInfo().IsSmartSpeaker()) {
            ctx.AddCommand<TGeneralConversationPlayerPauseDirective>(TStringBuf("player_pause"), NSc::Null());
        }
        if (ctx.ClientFeatures().SupportsNavigator() && NRoute::CheckNavigatorState(ctx, NRoute::WAITING_STATE)) {
            TExternalConfirmationIntent navigatorIntentHandler(ctx, false /* isConfirmed */);
            return navigatorIntentHandler.Do();
        }
        if (ctx.MetaClientInfo().IsYaAuto()) {
            return NAutomotive::HandleMediaControl(ctx, TStringBuf("player_pause"));
        }
    } else if (
        ctx.MetaClientInfo().IsSmartSpeaker() &&
        ctx.FormName() == TStringBuf("personal_assistant.handcrafted.quasar.turn_display_off") &&
        ctx.ClientFeatures().SupportsCecAvailable() &&
        ctx.HasExpFlag("cec_available")
    ) {
        ctx.AddCommand<TScreenOffDirective>(TStringBuf("screen_off"), {});
        ctx.AddAttention(TStringBuf("cec_screen_off"));
    } else if ((ctx.MetaClientInfo().IsSmartSpeaker() || ctx.MetaClientInfo().IsYaAuto())
            && ctx.FormName() == PositiveFeedBackFormName
            && !ctx.Meta().PureGC()
    ) {
        ctx.AddStopListeningBlock();
    }
    ctx.AddSearchSuggest();
    ctx.AddOnboardingSuggest();
    return TResultValue();
}

void TGeneralConversationFormHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TGeneralConversationFormHandler>();
    };

    handlers->emplace(GeneralConversationFormName, handler);
    handlers->emplace(GeneralConversationDummyFormName, handler);
    handlers->emplace(GeneralConversationMicrointentsFormName, handler);
    handlers->emplace(HandcraftedFormName, handler);
}

// static
TResultValue TGeneralConversationFormHandler::SetAsResponse(TContext& ctx) {
    TString oldFormName = TString{TStringBuf(ctx.FormName()).RNextTok('.')};
    auto newCtx = ctx.SetResponseForm(GeneralConversationFormName, false);
    Y_ENSURE(newCtx);
    newCtx->AddAttention(TStringBuilder() << "switch_to_gc_from_" << oldFormName);
    return TResultValue();
}

}
