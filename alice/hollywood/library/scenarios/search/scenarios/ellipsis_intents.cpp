#include "app_navigation.h"
#include "ellipsis_intents.h"
#include <alice/hollywood/library/frame/callback.h>
#include <alice/hollywood/library/scenarios/search/utils/utils.h>

namespace NAlice::NHollywood::NSearch {

namespace {

const TString FACTOID_SRC_FRAME = "alice.search_factoid_src";
const TString FACTOID_CALL_FRAME = "alice.search_factoid_call";
const TString SERP_FRAME = "alice.search_serp";
const TString SHOW_ON_MAP = "alice.search_show_on_map";

const THashSet<TString> ACCEPTED_ELLIPSIS_FRAMES{
    FACTOID_SRC_FRAME,
    FACTOID_CALL_FRAME,
    SERP_FRAME,
    OPEN_APP_FRAME,
    "alice.push_notification",
    SHOW_ON_MAP,
};

constexpr std::array<TStringBuf, 4> SERP_CONFIRM_PHRASES = {
    "открой",
    "открывай",
    "давай",
    "поищи",
};

void AddFrameAction(const TString& nluFrameName, TSemanticFrame&& frame, TResponseBodyBuilder& bodyBuilder) {
    TFrameNluHint nluHint;
    nluHint.SetFrameName(nluFrameName);

    NScenarios::TFrameAction action;
    *action.MutableNluHint() = std::move(nluHint);
    *action.MutableCallback() = ToCallback(frame);
    bodyBuilder.AddAction(nluFrameName, std::move(action));
}

NScenarios::TDirective CreateOpenUriDirective(const TString& name, const TString& uri) {
    NScenarios::TDirective directive;
    NScenarios::TOpenUriDirective* openUriDirective = directive.MutableOpenUriDirective();
    openUriDirective->SetUri(uri);
    openUriDirective->SetName(name);
    return directive;
}

bool HandleVoiceButton(TSearchContext& ctx, const TString& url, const TStringBuf slot, const TStringBuf field, const TString& intent, const TStringBuf nlgTemplate)
{
    LOG_INFO(ctx.GetLogger()) << "Handling voice button: " << intent;
    if (url.Empty()) {
       LOG_INFO(ctx.GetLogger()) << "No state for voice button";
       return false;
    }

    auto& slots = ctx.GetNlgData().Form["slots"];
    if (slots.GetArray().size() > 0) {
        slots[0]["value"][slot][field] = url;
        ctx.SetIntent(intent);
    } else {
        LOG_ERROR(ctx.GetLogger()) << "Failed search scenario init!";
    }

    ctx.GetBodyBuilder().AddRenderedTextWithButtonsAndVoice(nlgTemplate, "render_result", /* buttons = */ {}, ctx.GetNlgData());
    const auto& uri = ctx.RenderPhrase("render_uri", nlgTemplate).Text;
    if (uri) {
        LOG_INFO(ctx.GetLogger()) << "Adding open_uri directive";
        ctx.GetBodyBuilder().AddDirective(CreateOpenUriDirective("open_uri", uri));
    }
    return true;
}

void ClearState(TSearchContext& ctx) {
    ctx.GetState().SetFactoidUrl("");
    ctx.GetState().SetSearchUrl("");
    ctx.GetState().SetPhoneUri("");
    ctx.GetState().SetAppBlock("");
    ctx.GetState().SetMapUrl("");
}

} // namespace

void AddFactoidSrcSuggest(TSearchContext& ctx, const NJson::TJsonValue& data) {
    if (!data.Has("url")) {
       return;
    }
    ctx.SetIsUsingState(true);
    ctx.GetState().SetFactoidUrl(data["url"].GetString());
    ctx.AddSuggest(TStringBuf("search__factoid_src"));

    TSemanticFrame frame;
    frame.SetName(FACTOID_SRC_FRAME);
    AddFrameAction(FACTOID_SRC_FRAME, std::move(frame), ctx.GetBodyBuilder());
}

void AddSerpConfirmationButton(TSearchContext& ctx) {
    ctx.SetIsUsingState(true);
    ctx.GetState().SetSearchUrl(ctx.GenerateSearchUri());

    TSemanticFrame frame;
    frame.SetName(SERP_FRAME);
    AddFrameActionWithHint("serp_confirmation", std::move(frame), ctx.GetBodyBuilder(), SERP_CONFIRM_PHRASES);
}

void AddSerpVoiceButton(TSearchContext& ctx) {
    ctx.SetIsUsingState(true);
    ctx.GetState().SetSearchUrl(ctx.GenerateSearchUri());

    TSemanticFrame frame;
    frame.SetName(SERP_FRAME);
    AddFrameAction(SERP_FRAME, std::move(frame), ctx.GetBodyBuilder());
}

void AddShowOnMapButton(TSearchContext& ctx, const TString& mapUrl) {
    ctx.SetIsUsingState(true);
    ctx.GetState().SetMapUrl(mapUrl);

    TSemanticFrame frame;
    frame.SetName(SHOW_ON_MAP);
    AddFrameAction(SHOW_ON_MAP, std::move(frame), ctx.GetBodyBuilder());
}

void AddFactoidCallButton(TSearchContext& ctx, const TString& phoneUri) {
    ctx.SetIsUsingState(true);
    ctx.GetState().SetPhoneUri(phoneUri);

    TSemanticFrame frame;
    frame.SetName(FACTOID_CALL_FRAME);
    AddFrameAction(FACTOID_CALL_FRAME, std::move(frame), ctx.GetBodyBuilder());
}

void AddPushButton(TSearchContext& ctx, const TString& nluHintFrame) {
    TSemanticFrame frame;
    frame.SetName("alice.push_notification");
    AddFrameAction(nluHintFrame, std::move(frame), ctx.GetBodyBuilder());
}

bool CheckEllipsisIntents(TSearchContext& ctx) {
    LOG_INFO(ctx.GetLogger()) << "Checking ellipsis frames";
    const auto& input = ctx.GetRequest().Input();
    const TMaybe<TFrame> callbackFrame = GetCallbackFrame(input.GetCallback());
    if (const TMaybe<TFrame> frame = TryGetFrame(FACTOID_SRC_FRAME, callbackFrame, input); frame.Defined()) {
        return HandleVoiceButton(ctx, ctx.GetState().GetFactoidUrl(), TStringBuf("factoid"), TStringBuf("url"),
                                "factoid_src", TStringBuf("search__factoid_src"));
    }

    if (const TMaybe<TFrame> frame = TryGetFrame(SERP_FRAME, callbackFrame, input); frame.Defined()) {
        return HandleVoiceButton(ctx, ctx.GetState().GetSearchUrl(), TStringBuf("serp"), TStringBuf("url"),
                                "serp", TStringBuf("search__serp"));
    }

    if (const TMaybe<TFrame> frame = TryGetFrame(FACTOID_CALL_FRAME, callbackFrame, input); frame.Defined()) {
        return HandleVoiceButton(ctx, ctx.GetState().GetPhoneUri(), TStringBuf("factoid"), TStringBuf("phone_uri"),
                                "factoid_call", TStringBuf("search__factoid_call"));
    }

    if (const TMaybe<TFrame> frame = TryGetFrame(OPEN_APP_FRAME, callbackFrame, input); frame.Defined()) {
        return TSearchAppNavScenario::HandleAppConfirmationButton(ctx);
    }

    if (const TMaybe<TFrame> frame = TryGetFrame(SHOW_ON_MAP, callbackFrame, input); frame.Defined()) {
        return HandleVoiceButton(ctx, ctx.GetState().GetMapUrl(), TStringBuf("map_search_url"), TStringBuf("url"),
                                "show_on_map", TStringBuf("search__show_on_map"));
    }

    ClearState(ctx);
    return false;
}

bool HasEllipsisFrames(const TScenarioRunRequestWrapper& request) {
    const TMaybe<TFrame> callbackFrame = GetCallbackFrame(request.Input().GetCallback());
    return callbackFrame.Defined() && ACCEPTED_ELLIPSIS_FRAMES.contains(callbackFrame->Name());
}

} // namespace NAlice::NHollywood::NSearch
