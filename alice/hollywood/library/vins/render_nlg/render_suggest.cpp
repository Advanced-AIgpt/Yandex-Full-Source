#include "render_suggest.h"
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/library/json/json.h>
#include <util/generic/guid.h>

namespace NAlice::NHollywoodFw {

namespace {

const TString& GetSuggestType(const NJson::TJsonValue& bassBlock) {
    return bassBlock["suggest_type"].GetStringSafe();
}

const TString& GetSearchSuggestQuery(const NJson::TJsonValue& bassBlock) {
    for (const auto& slot : bassBlock["form_update"]["slots"].GetArray()) {
        if (slot["name"].GetString() == "query") {
            return slot["value"].GetStringSafe();
        }
    }
    return Default<TString>();
}

void RenderSearchSuggest(const NJson::TJsonValue& bassBlock, TRenderBassBlockContext ctx) {
    if (ctx.Request.Input().GetUserLanguage() != ELanguage::LANG_RUS) {
        // we do not have proper original-language query to build search suggest, so we just drop it
        LOG_DEBUG(ctx.Request.Debug().Logger()) << "Search suggest is skipped for non-russian request";
        return;
    }

    const auto query = GetSearchSuggestQuery(bassBlock);
    LOG_DEBUG(ctx.Request.Debug().Logger()) << "Rendering search suggest for query '" << query << "'";

    auto nlgContext = ctx.NlgContext;
    nlgContext["query"] = query;

    const auto caption = ctx.Request.Nlg().RenderPhrase(
        "suggests", "search_suggest_caption", nlgContext).Text;

    auto& searchSuggest = *ctx.ResponseBody.MutableLayout()->AddSuggestButtons()->MutableSearchButton();
    searchSuggest.SetTitle(caption);
    searchSuggest.SetQuery(query);
}

NScenarios::TLayout::TSuggest::TActionButton& AddSuggestActionButton(const NJson::TJsonValue& bassBlock, TRenderBassBlockContext ctx) {
    const TString captionPhraseId = TStringBuilder() << "render_suggest_caption__" << GetSuggestType(bassBlock);
    const auto caption = ctx.Request.Nlg().RenderPhrase(
        ctx.NlgTemplate, captionPhraseId, ctx.NlgContext).Text;

    auto& actionButton = *ctx.ResponseBody.MutableLayout()->AddSuggestButtons()->MutableActionButton();
    actionButton.SetTitle(caption);
    if (const auto& themeImageUrl = bassBlock["data"]["theme"]["image_url"].GetString()) {
        actionButton.MutableTheme()->SetImageUrl(themeImageUrl);
    }
    return actionButton;
}

void TryAddUserUtteranceDirective(const NJson::TJsonValue& bassBlock, TRenderBassBlockContext ctx, NScenarios::TFrameAction& action) {
    const TString phraseId = TStringBuilder() << "render_suggest_user_utterance__" << GetSuggestType(bassBlock);
    if (ctx.Request.Nlg().HasPhrase(ctx.NlgTemplate, phraseId)) {
        *action.MutableDirectives()->AddList()->MutableTypeTextSilentDirective()->MutableText() =
            ctx.Request.Nlg().RenderPhrase(ctx.NlgTemplate, phraseId, ctx.NlgContext).Text;
    }
}

void TryAddOpenUriDirective(const NJson::TJsonValue& bassBlock, TRenderBassBlockContext ctx, NScenarios::TFrameAction& action) {
    const TString phraseId = TStringBuilder() << "render_suggest_uri__" << GetSuggestType(bassBlock);
    if (ctx.Request.Nlg().HasPhrase(ctx.NlgTemplate, phraseId)) {
        *action.MutableDirectives()->AddList()->MutableOpenUriDirective()->MutableUri() =
            ctx.Request.Nlg().RenderPhrase(ctx.NlgTemplate, phraseId, ctx.NlgContext).Text;
    }
}

void TryAddUtteranceDirective(const NJson::TJsonValue& bassBlock, TRenderBassBlockContext ctx, NScenarios::TFrameAction& action) {
    const TString phraseId = TStringBuilder() << "render_suggest_utterance__" << GetSuggestType(bassBlock);
    if (ctx.Request.Nlg().HasPhrase(ctx.NlgTemplate, phraseId)) {
        *action.MutableDirectives()->AddList()->MutableTypeTextDirective()->MutableText() =
            ctx.Request.Nlg().RenderPhrase(ctx.NlgTemplate, phraseId, ctx.NlgContext).Text;
    }
}

void TryAddFormUpdate(const NJson::TJsonValue& bassBlock, TRenderBassBlockContext ctx, NScenarios::TFrameAction& action) {
    Y_UNUSED(ctx);
    static const TString FormUpdateKey = "form_update";
    static const TString ResubmitKey = "resubmit";

    const auto& bassBlockFormUpdate = bassBlock[FormUpdateKey];
    if (!bassBlockFormUpdate.IsMap()) {
        return;
    }

    auto& callbackDirective = *action.MutableDirectives()->AddList()->MutableCallbackDirective();
    callbackDirective.SetName("update_form");

    auto formUpdate = bassBlockFormUpdate;
    formUpdate.EraseValue(ResubmitKey);

    auto resubmit = bassBlockFormUpdate.GetMapSafe().Value(ResubmitKey, false);

    const auto payload = NJson::TJsonMap({
        {FormUpdateKey, std::move(formUpdate)},
        {ResubmitKey, std::move(resubmit)},
    });

    const auto status = ::NAlice::JsonToProto(payload, *callbackDirective.MutablePayload());
    Y_ENSURE(status.ok(), "Failed to convert provided Json to Proto: " << status.ToString());
}

} // namespace

void RenderSuggest(const NJson::TJsonValue& bassBlock, TRenderBassBlockContext ctx) {
    static constexpr TStringBuf SearchSuggestType = "search_internet_fallback";
    if (GetSuggestType(bassBlock) == SearchSuggestType) {
        RenderSearchSuggest(bassBlock, ctx);
        return;
    }


    LOG_DEBUG(ctx.Request.Debug().Logger()) << "Rendering suggest " << GetSuggestType(bassBlock);

    const auto actionId = CreateGuidAsString();

    auto& actionButton = AddSuggestActionButton(bassBlock, ctx);
    actionButton.SetActionId(actionId);

    auto& action = (*ctx.ResponseBody.MutableFrameActions())[actionId];
    TryAddUserUtteranceDirective(bassBlock, ctx, action);
    TryAddOpenUriDirective(bassBlock, ctx, action);
    TryAddUtteranceDirective(bassBlock, ctx, action);
    // suggest's commands are not added yet
    TryAddFormUpdate(bassBlock, ctx, action);
}

} // namespace NAlice::NHollywood
