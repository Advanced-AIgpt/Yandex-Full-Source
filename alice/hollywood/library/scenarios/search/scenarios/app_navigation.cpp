#include "app_navigation.h"

#include <alice/hollywood/library/response/push.h>
#include <alice/hollywood/library/scenarios/search/utils/utils.h>

#include <alice/library/app_navigation/navigation.h>
#include <alice/library/client/interfaces_util.h>
#include <alice/library/logger/rtlog_adapter.h>
#include <alice/library/json/json.h>
#include <alice/library/url_builder/url_builder.h>

#include <util/generic/algorithm.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NSearch {

namespace {

const TString CLOUD_UI_SCREEN_ID = "cloud_ui";
const TString OPEN_APPS_FIXLIST_CLOSE_DIALOG_DIRECTIVE_NAME = "open_apps_fixlist_close_dialog";
const TString APPS_FIXLIST_FRAME = "alice.apps_fixlist";
const TString FIXLIST_DATA_SLOT = "app_data";
const TString DEFAULT_SUPPORTED_FEATURE = "CanOpenLink";
const THashSet<TString> FEATURES_WITH_ALLOWED_PUSHES = {
    DEFAULT_SUPPORTED_FEATURE,
    "CanOpenLinkYellowskin",
    "CanOpenLinkIntent",
};
const TString FALLBACK_NAV = "fallback_nav";
constexpr TStringBuf NEED_SUPPORTED_FEATURE = "need_feature";
constexpr TStringBuf IRRELEVANT_ON_UNSUPPORTED_FEATURE = "irrelevant_on_unsupported_feature";
constexpr TStringBuf INTENT_FIELD_NAME = "intent";

constexpr std::array<TStringBuf, 4> OPEN_APP_CONFIRM_PHRASES = {
    "открой",
    "открывай",
    "запускай",
    "давай",
};

const TStringBuf GetSupportedFeature(const NSc::TValue& fixlistAnswer) {
    if (fixlistAnswer.Has(NEED_SUPPORTED_FEATURE)) {
        return fixlistAnswer[NEED_SUPPORTED_FEATURE].GetString();
    }
    return DEFAULT_SUPPORTED_FEATURE;
}

bool HasFallbackBlock(const NSc::TValue& fixlistAnswer) {
    return fixlistAnswer.Has(FALLBACK_NAV);
}

NSc::TValue ParseFixlistAnswer(const TSemanticFrame& frame, TRTLogger& logger) {
    NSc::TValue fixlistAnswer;
    LOG_INFO(logger) << "Found fixlist frame: " << frame.GetName();
    for (const auto& slot : frame.GetSlots()) {
        if (slot.GetName() != FIXLIST_DATA_SLOT) {
            continue;
        }
        fixlistAnswer = NSc::TValue::FromJson(slot.GetValue());
        LOG_INFO(logger) << "Frame fixlist data: " << fixlistAnswer;
    }
    return fixlistAnswer;
}

} // namespace

bool TSearchAppNavScenario::FixlistAnswers(const TSearchResult& /* response */) {
    NSc::TValue fixlistAnswer;

    if (const auto* fixlist = Ctx.GetResources().GetNavigationFixList()) {
        fixlistAnswer = fixlist->Find(Ctx.GetQuery(), TRTLogAdapter(Ctx.GetLogger()));
    }

    return ProcessFixlistAnswer(fixlistAnswer, /* confirmed */false);
}

bool TSearchAppNavScenario::FrameFixlistAnswers() {
    return ProcessFixlistAnswer(GetFrameFixlistAnswer(), /* confirmed */false);
}

NSc::TValue TSearchAppNavScenario::GetFrameFixlistAnswer() {
    NSc::TValue fixlistAnswer;
    if (const auto frame = Ctx.GetRequest().Input().FindSemanticFrame(APPS_FIXLIST_FRAME)) {
        fixlistAnswer = ParseFixlistAnswer(*frame, Ctx.GetLogger());
    }
    return fixlistAnswer;
}

bool TSearchAppNavScenario::ProcessFixlistAnswer(const NSc::TValue& fixlistAnswer, bool confirmed) {
    if (fixlistAnswer.IsNull()) {
        LOG_INFO(Ctx.GetLogger()) << "Fixlist answer not found";
        return false;
    }
    LOG_INFO(Ctx.GetLogger()) << "Fixlist answer found";

    auto supportedFeature = GetSupportedFeature(fixlistAnswer);
    const auto& interfaces = Ctx.GetRequest().BaseRequestProto().GetInterfaces();

    const NSc::TValue block = [&] {
        if (!CheckFeatureSupport(interfaces, supportedFeature, Ctx.GetLogger()) && HasFallbackBlock(fixlistAnswer)) {
            supportedFeature = DEFAULT_SUPPORTED_FEATURE;
            return std::move(
                NAlice::CreateNavBlock(fixlistAnswer, Ctx.GetRequest(), true, FALLBACK_NAV));
        }
        return std::move(NAlice::CreateNavBlock(fixlistAnswer, Ctx.GetRequest(), true));
    }();

    if (block.IsNull()) {
        LOG_WARNING(Ctx.GetLogger()) << "Fixlist answer has incorrect format or experiments do not match";
        return false;
    }

    const auto& clientInfo = Ctx.GetRequest().ClientInfo();

    Ctx.SetResultSlot(TStringBuf("nav"), block.ToJsonValue(), /* setIntent= */ !block.Has(INTENT_FIELD_NAME));
    Ctx.GetAnalyticsInfoBuilder().SetProductScenarioName("nav_url");
    if (block.Has(INTENT_FIELD_NAME)) {
        Ctx.SetIntent(TString{block[INTENT_FIELD_NAME].GetString()});
    }

    const bool sendPush = FEATURES_WITH_ALLOWED_PUSHES.contains(supportedFeature);
    if (supportedFeature == DEFAULT_SUPPORTED_FEATURE) {
        Ctx.AddAttention("simple_open_link");
    }
    if (!CheckFeatureSupport(interfaces, supportedFeature, Ctx.GetLogger()) || clientInfo.IsElariWatch() || clientInfo.IsYaAuto()) {
        if (fixlistAnswer.Has(IRRELEVANT_ON_UNSUPPORTED_FEATURE) &&
            fixlistAnswer[IRRELEVANT_ON_UNSUPPORTED_FEATURE].GetBool(false)) {
            LOG_INFO(Ctx.GetLogger()) << "Fixlist answer is irrelevant by unsupported feature";
            return false;
        }
        Ctx.AddAttention("unsupported_feature");
        if (sendPush) {
            Ctx.AddAttention("send_push");
            TPushDirectiveBuilder{TString{block["text"].GetString()},
                                  TString::Join("Открыть ", block["text_name"].GetString()),
                                  TString{block["url"].GetString()},
                                  "open_site_or_app"}
                .SetThrottlePolicy("unlimited_policy")
                .SetTtlSeconds(900)
                .BuildTo(Ctx.GetBodyBuilder());
        }
    } else {
        if (Ctx.GetRequest().Interfaces().GetHasNavigator() && !confirmed) {
            AddOpenAppConfirmButton(fixlistAnswer.ToJsonSafe());
            Ctx.AddSuggest(TStringBuf("search__nav"), /* autoaction */false);
            Ctx.AddAttention("ask_confirmation");
            return true;
        }
        Ctx.AddSuggest(TStringBuf("search__nav"), /* autoaction */true);
        Ctx.SetShouldListen(false);
        if (block["turboapp"].GetBool() && Ctx.GetRequest().BaseRequestProto().GetInterfaces().GetCanOpenLinkTurboApp()) {
            Ctx.GetAnalyticsInfoBuilder().AddAction(TString("open_turboapp_") + block["url"].GetString(),
                                                    "open_turboapp",
                                                    TString("Открывается турбоапп ") + block["text"].GetString());
            LOG_INFO(Ctx.GetLogger()) << "Opening turboapp";
        }
        if (block["close_dialog"].GetBool() && interfaces.GetSupportsCloudUi()) {
            NAlice::NScenarios::TDirective directive;
            auto& closeDialogDirective = *directive.MutableCloseDialogDirective();
            closeDialogDirective.SetName(OPEN_APPS_FIXLIST_CLOSE_DIALOG_DIRECTIVE_NAME);
            closeDialogDirective.SetDialogId(Ctx.GetRequest().DialogId());
            closeDialogDirective.SetScreenId(CLOUD_UI_SCREEN_ID);
            Ctx.GetBodyBuilder().AddDirective(std::move(directive));
        }

        if (interfaces.GetSupportsCloudUiFilling()) {
            if (const auto& text = block["text"].GetString()) {
                NAlice::NScenarios::TDirective directive;
                auto& fillCloudUidDirective = *directive.MutableFillCloudUiDirective();
                fillCloudUidDirective.SetText(text.data(), text.size());
                Ctx.GetBodyBuilder().AddDirective(std::move(directive));
            }
        }
    }

    if (TStringBuf serpSuggestText = fixlistAnswer.TrySelect("suggests/serp/text").GetString()) {
        AddSerp(/* add_suggest */ true, /* auto_action */ false, /* query */ serpSuggestText);
    }
    return true;
}

void TSearchAppNavScenario::AddOpenAppConfirmButton(const TString& appData) {
    Ctx.SetIsUsingState(true);
    Ctx.GetState().SetAppBlock(appData);
    LOG_INFO(Ctx.GetLogger()) << "Add confirmation button";

    TSemanticFrame frame;
    frame.SetName(OPEN_APP_FRAME);
    AddFrameActionWithHint(OPEN_APP_FRAME, std::move(frame), Ctx.GetBodyBuilder(), OPEN_APP_CONFIRM_PHRASES);
}

bool TSearchAppNavScenario::HandleAppConfirmationButton(TSearchContext& ctx) {
    LOG_INFO(ctx.GetLogger()) << "Handling app confirmation button";
    const auto fixlistAnswer = NSc::TValue::FromJson(ctx.GetState().GetAppBlock());

    if (fixlistAnswer.IsNull()) {
       LOG_INFO(ctx.GetLogger()) << "No state for voice button";
       return false;
    }

    if (TSearchAppNavScenario(ctx).ProcessFixlistAnswer(fixlistAnswer, /* confirmed */true)) {
        ctx.AddResult();
        return true;
    }
    return false;
}

} // namespace NAlice::NHollywood::NSearch
