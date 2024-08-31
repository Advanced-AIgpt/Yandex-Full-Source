#include "render_handle.h"

#include "alice/hollywood/library/framework/core/request.h"
#include "alice/protos/data/scenario/centaur/teasers/teaser_settings.pb.h"
#include "bass.h"
#include "frame.h"
#include "memento_helper.h"
#include "news_block.h"
#include "google/protobuf/stubs/port.h"

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/framework/framework_migration.h>

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/bass_adapter/bass_renderer.h>

#include <alice/library/json/json.h>
#include <alice/library/proto/protobuf.h>
#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/frame_request_params.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/data/scenario/centaur/main_screen.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/data/scenario/news/news.pb.h>

#include <library/cpp/json/writer/json_value.h>

#include <util/generic/fwd.h>
#include <util/generic/hash_set.h>
#include <util/random/random.h>
#include <util/string/builder.h>
#include <util/string/cast.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace NImpl {

constexpr TStringBuf POSTROLL_NEWS_CHANGE_SOURCE_MODE = "news_change_source_postroll_mode";
constexpr TStringBuf POSTROLL_NEWS_RADIO_CHANGE_SOURCE_MODE = "news_radio_news_change_source_postroll_mode";
constexpr TStringBuf TOPIC_SLOT_MEMENTO_RUBRIC_TYPE = "memento.news_topic";

constexpr TStringBuf CONTEXT_HAS_INTRO_AND_ENDING = "voice_has_intro_and_ending";

constexpr TStringBuf CENTAUR_COLLECT_CARDS_SEMANTIC_FRAME = "alice.centaur.collect_cards";
constexpr TStringBuf CENTAUR_COLLECT_TEASERS_PREVIEW_SEMANTIC_FRAME = "alice.centaur.collect_teasers_preview";
constexpr TStringBuf CENTAUR_COLLECT_MAIN_SCREEN_NEWS_SEMANTIC_FRAME = "alice.centaur.collect_main_screen.widgets.news";

constexpr TStringBuf TEASER_CARD_ID_PREFIX = "news.teaser.div.card";
const TString MAIN_SCREEN_CARD_ID = "news.main_screen.div.card";
const TString SCENARIO_WIDGET_MECHANICS_EXP_FLAG_NAME = "scenario_widget_mechanics";
const TString TEASER_SETTINGS_EXP_FLAG_NAME = "teaser_settings";
const TString TEASER_TYPE = "News";
const TString TEASER_NAME = "Новости";

constexpr i32 MAX_EXCLUDE_IDS = 150;

void StripExcludeIds(::google::protobuf::RepeatedPtrField<TString>& excludeIds) {
    if (excludeIds.size() <= MAX_EXCLUDE_IDS) {
        return;
    }
    ::google::protobuf::RepeatedPtrField<TString> stripped{excludeIds.end() - MAX_EXCLUDE_IDS, excludeIds.end()};
    excludeIds = std::move(stripped);
}

void UpdateExcludeIdsState(TNewsState& state, const NJson::TJsonValue& bassResponse) {
    NJson::TJsonValue newsSlotValue = GetSlotValue(bassResponse, "news");
    NJson::TJsonValue& excludeIds = newsSlotValue["exclude_ids"];
    if (excludeIds.IsNull()){
        return;
    }
    for (const auto& id : excludeIds.GetArray()) {
        *state.AddExcludeIds() = id.GetString();
    }

    StripExcludeIds(*state.MutableExcludeIds());
}

void UpdateExcludeIdsStateForCallbackMode(const TNewsState& oldState, TNewsState& state, const TNewsBlock& newsBlock) {
    auto& newId = newsBlock.GetNewsId();
    bool isNew = newId.Defined();
    for (const auto& id : oldState.GetExcludeIds()) {
        if (newId && id == *newId) {
            isNew = false;
        }
        *state.AddExcludeIds() = id;
    }
    if (isNew) {
        *state.AddExcludeIds() = *newId;
    }

    StripExcludeIds(*state.MutableExcludeIds());
}

void SetMoreNewsVoiceButton(std::unique_ptr<NScenarios::TScenarioRunResponse>& scenarioResponse, const NJson::TJsonValue& bassResponse) {
    auto form = bassResponse["form"];

    TFrameNluHint agreeNluHint;
    agreeNluHint.SetFrameName("personal_assistant.scenarios.get_news__more");

    TSemanticFrame newsFrame;
    newsFrame.SetName(form["name"].GetString());

    for (auto slot : form["slots"].GetArray()) {
        if (EqualToOneOf(slot["name"], "news", "news_memento", "smi")) {
            continue;
        }
        TSemanticFrame_TSlot& querySlot = *newsFrame.AddSlots();
        querySlot.SetName(slot["name"].GetString());
        querySlot.SetType(slot["type"].GetString());
        querySlot.SetValue(slot["value"].GetString());
    }

    NScenarios::TFrameAction action;
    *action.MutableNluHint() = std::move(agreeNluHint);
    *action.MutableCallback() = ToCallback(newsFrame);

    auto& actions = *scenarioResponse->MutableResponseBody()->MutableFrameActions();
    actions["more"] = std::move(action);
}


void SetNewsBlockVoiceButton(
    std::unique_ptr<NScenarios::TScenarioRunResponse>& scenarioResponse,
    const NJson::TJsonValue& bassResponse,
    TNewsBlockVoiceButton& voiceButton)
{
    auto form = bassResponse["form"];

    TFrameNluHint agreeNluHint;
    agreeNluHint.SetFrameName(TString(voiceButton.GetFrameName()));

    TSemanticFrame newsFrame;
    newsFrame.SetName(form["name"].GetString());

    for (auto slot : form["slots"].GetArray()) {
        if (EqualToOneOf(slot["name"], "news", "news_memento", "smi")) {
            continue;
        }
        TSemanticFrame_TSlot& querySlot = *newsFrame.AddSlots();
        querySlot.SetName(slot["name"].GetString());
        querySlot.SetType(slot["type"].GetString());
        querySlot.SetValue(slot["value"].GetString());
    }
    voiceButton.UpdateFrame(newsFrame);

    NScenarios::TFrameAction action;
    *action.MutableNluHint() = std::move(agreeNluHint);
    *action.MutableCallback() = ToCallback(newsFrame);
    voiceButton.UpdateCallback(*action.MutableCallback());

    auto& actions = *scenarioResponse->MutableResponseBody()->MutableFrameActions();
    actions[TString(voiceButton.GetButtonName())] = std::move(action);
}

void SetSourceChangePostrollVoiceButton(std::unique_ptr<NScenarios::TScenarioRunResponse>& scenarioResponse) {
    TFrameNluHint agreeNluHint;
    agreeNluHint.SetFrameName("personal_assistant.scenarios.get_news__postroll_answer");

    NScenarios::TFrameAction action;
    *action.MutableNluHint() = std::move(agreeNluHint);

    auto& actions = *scenarioResponse->MutableResponseBody()->MutableFrameActions();
    actions["changeSource"] = std::move(action);
}

bool IsMementable(TMementoHelper& mementoHelper, const NJson::TJsonValue& topicSlot) {
    return !topicSlot.IsNull() &&
        mementoHelper.IsMementableTopic(topicSlot["value"].GetString()) &&
        topicSlot["type"].GetString() != TOPIC_SLOT_MEMENTO_RUBRIC_TYPE;
}

void UpdateFixlistForChangeNewsPostroll(
    TMementoHelper& mementoHelper,
    const NJson::TJsonValue& bassResponse,
    NJson::TJsonValue& fixlist)
{
    NJson::TJsonValue postNewsMode = GetSlotValue(bassResponse, "news")["post_news_mode"];
    NJson::TJsonValue topicSlot = GetSlot(bassResponse, "topic");

    if (postNewsMode.GetString() == POSTROLL_NEWS_CHANGE_SOURCE_MODE
        && IsMementable(mementoHelper, topicSlot))
    {
        fixlist["topic"] = mementoHelper.GetSourceName(topicSlot["value"].GetString());
    }
}

void UpdateFixlistForRadioNewsPostroll(
    NJson::TJsonValue& fixlist,
    const TNewsPostroll* postroll)
{
    if (postroll) {
        fixlist["radio_news_postroll"] = postroll->Postroll;
    }
    else {
        fixlist["radio_news_postroll"] = "";
    }
}


void SetChangeNewsPostroll(
    TMementoHelper& mementoHelper,
    std::unique_ptr<NScenarios::TScenarioRunResponse>& response,
    const NJson::TJsonValue& bassResponse,
    TNewsState& state)
{
    NJson::TJsonValue postNewsMode = GetSlotValue(bassResponse, "news")["post_news_mode"];
    if (postNewsMode.GetString() == POSTROLL_NEWS_CHANGE_SOURCE_MODE) {
        NJson::TJsonValue topicSlot = GetSlot(bassResponse, "topic");
        if (IsMementable(mementoHelper, topicSlot)) {
            state.SetTopic(topicSlot["value"].GetString());
        }
        SetSourceChangePostrollVoiceButton(response);
    }
}

void SetNewsPostrollButton(
    std::unique_ptr<NScenarios::TScenarioRunResponse>& scenarioResponse,
    const TNewsPostroll* postroll)
{
    if (postroll && postroll->HasFrameAction) {
        auto& actions = *scenarioResponse->MutableResponseBody()->MutableFrameActions();
        actions["radio_news"] = std::move(postroll->FrameAction);
    }
}


void SetNewsCallback(TRunResponseBuilder& builder, const TNextNewsBlockLink& newsBlock, bool resetSession, const TMaybe<TFrame>& frame) {
    auto& bodyBuilder = *builder.GetResponseBodyBuilder();
    if (resetSession) {
        bodyBuilder.AddNewSessionStackAction();
    }
    auto resetAddBuilder = bodyBuilder.ResetAddBuilder();
    auto& callback = resetAddBuilder.AddCallback("personal_assistant.scenarios.get_news__next_block");
    if (frame) {
        auto frameProto = frame->ToProto();
        newsBlock.UpdateFrame(frameProto);
        callback = ToCallback(frameProto);
    }
    newsBlock.UpdateCallback(callback);
}

bool HasErrorBlocks(const NJson::TJsonValue& bassResponse) {
    auto& blocks = bassResponse["blocks"];
    for (auto& node : blocks.GetArray()) {
        if (node["type"] == "error") {
            return true;
        }
    }
    return false;
}

int GetNewsCount(const NJson::TJsonValue& bassResponse) {
    const auto& slots = bassResponse["form"]["slots"];
    for (const auto& slot : slots.GetArray()) {
        if (slot["name"] == "news") {
            return slot["value"]["news"].GetArray().size();
        }
    }
    return 0;
}

TMaybe<NJson::TJsonValue> GetNewsData(const NJson::TJsonValue& bassResponse) {
    const auto& slots = bassResponse["form"]["slots"];
    for (const auto& slot : slots.GetArray()) {
        if (slot["name"] == "news") {
            return slot["value"]["news"];
        }
    }
    return Nothing();
}

const TMaybe<TString> GetTopic(const NJson::TJsonValue& bassResponse) {
    const auto& slots = bassResponse["form"]["slots"];
    for (const auto& slot : slots.GetArray()) {
        if (slot["name"] == "topic") {
            if (const auto& topicValue = slot["value"]; topicValue.IsString()) {
                return topicValue.GetString();
            }
        }
    }
    return Nothing();
}

void AddShowView(
    const TScenarioRunRequestWrapper& runRequest,
    const NJson::TJsonValue& bassResponse,
    NHollywood::TContext& ctx,
    TRunResponseBuilder& builder,
    int currentNewsId = 0)
{
    if (!runRequest.Interfaces().GetSupportsShowView()) {
        return;
    }

    const auto newsData = GetNewsData(bassResponse);
    if (!newsData.Defined()) {
        LOG_WARN(ctx.Logger()) << "News Data not found in form.slots; skipping adding ShowView";
        return;
    }

    LOG_INFO(ctx.Logger()) << "Adding ShowView Directive";
    NRenderer::TDivRenderData renderData;
    renderData.SetCardId("news.scenario.div.card");
    auto& newsScenarioData = *renderData.MutableScenarioData()->MutableNewsGalleryData();
    for (const auto& newsItemJsonVal : newsData->GetArray()) {
        auto& newsItem = *newsScenarioData.AddNewsItems();
        newsItem.SetText(newsItemJsonVal["text"].GetString());
        newsItem.SetUrl(newsItemJsonVal["url"].GetString());
        newsItem.SetAgency(newsItemJsonVal["agency"].GetString());
        newsItem.SetPubDate(newsItemJsonVal["date"].GetUInteger());
        if (const auto* logo = newsItemJsonVal.GetValueByPath("logo")) {
            newsItem.SetLogo(logo->GetString());
        }
        if (const auto* imageJsonVal = newsItemJsonVal.GetValueByPath("image")) {
            auto& image = *newsItem.MutableImage();
            image.SetSrc((*imageJsonVal)["src"].GetString());
            image.SetWidth((*imageJsonVal)["orig_width"].GetInteger());
            image.SetHeight((*imageJsonVal)["orig_height"].GetInteger());
        }

        if (const auto* extendedNews = newsItemJsonVal.GetValueByPath("extended_news")) {
            for (const auto& extendedNewsJsonVal : extendedNews->GetArray()) {
                auto& exNewsItem = *newsItem.AddExtendedNews();
                exNewsItem.SetText(extendedNewsJsonVal["text"].GetString());
                exNewsItem.SetUrl(extendedNewsJsonVal["url"].GetString());
                exNewsItem.SetAgency(extendedNewsJsonVal["agency"].GetString());
            }
        }

    }
    newsScenarioData.SetCurrentNewsItem(currentNewsId);
    newsScenarioData.SetTz(runRequest.ClientInfo().Timezone);
    if (auto newsTopic = GetTopic(bassResponse)) {
        newsScenarioData.SetTopic(*newsTopic);
    }

    auto& bodyBuilder = *builder.GetResponseBodyBuilder();
    bodyBuilder.AddShowViewDirective(std::move(renderData),  NScenarios::TShowViewDirective_EInactivityTimeout_Short);
    bodyBuilder.AddClientActionDirective("tts_play_placeholder", {});
}

void AddTeasers(
    const TScenarioRunRequestWrapper& runRequest,
    const NJson::TJsonValue& bassResponse,
    NHollywood::TContext& ctx,
    TRunResponseBuilder& builder,
    const TSemanticFrame& semanticFrame)
{
    if (!runRequest.Interfaces().GetSupportsShowView()) {
        return;
    }

    const auto frame = TFrame::FromProto(semanticFrame);

    const auto newsData = GetNewsData(bassResponse);
    if (!newsData.Defined()) {
        LOG_WARN(ctx.Logger()) << "News Data not found in form.slots; skipping adding AddCard direcrives";
        return;
    }
    const auto& newsArray = newsData->GetArray();
    const auto topic = GetTopic(bassResponse);

    auto& bodyBuilder = *builder.GetResponseBodyBuilder();

    LOG_INFO(ctx.Logger()) << "Adding AddCard directives";
    for (auto i = 0u; i < TEASER_NEWS_COUNT && i < newsArray.size(); ++i) {
        const auto& newsItemJsonVal = newsArray.at(i);
        NRenderer::TDivRenderData renderData;

        renderData.SetCardId(TStringBuilder{} << TEASER_CARD_ID_PREFIX << "." << ToString(i));
        auto& newsScenarioData = *renderData.MutableScenarioData()->MutableNewsTeaserData();
        newsScenarioData.SetTz(runRequest.ClientInfo().Timezone);
        if (topic) {
            newsScenarioData.SetTopic(*topic);
        }
        auto& newsItem = *newsScenarioData.MutableNewsItem();
        newsItem.SetText(newsItemJsonVal["text"].GetString());
        newsItem.SetUrl(newsItemJsonVal["url"].GetString());
        newsItem.SetAgency(newsItemJsonVal["agency"].GetString());
        newsItem.SetPubDate(newsItemJsonVal["date"].GetUInteger());
        newsItem.SetLogo(newsItemJsonVal["logo"].GetString());
        if (const auto* imageJsonVal = newsItemJsonVal.GetValueByPath("image")) {
            auto& image = *newsItem.MutableImage();
            image.SetSrc((*imageJsonVal)["src"].GetString());
            image.SetWidth((*imageJsonVal)["orig_width"].GetInteger());
            image.SetHeight((*imageJsonVal)["orig_height"].GetInteger());
        }

        TString actionId = TStringBuilder{} << TEASER_CARD_ID_PREFIX << ".nlu_hint." << ToString(i);
        TActionSpace actionSpace;
        TActionSpace_TAction action;
        auto& analytics = *action.MutableSemanticFrame()->MutableAnalytics();
        analytics.SetPurpose("get_news_by_id");
        analytics.SetOrigin(::NAlice::TAnalyticsTrackingModule_EOrigin::TAnalyticsTrackingModule_EOrigin_Scenario);
        action.MutableSemanticFrame()->MutableTypedSemanticFrame()->MutableNewsSemanticFrame()->MutableNewsIdx()->SetNumValue(i);
        (*actionSpace.MutableActions())[actionId] = action;
        auto& nluHint = *actionSpace.AddNluHints();
        nluHint.SetActionId(actionId);
        nluHint.SetSemanticFrameName(GET_DETAILED_NEWS.Data());

        bodyBuilder.AddActionSpace(actionId, actionSpace);
        if(runRequest.HasExpFlag(TEASER_SETTINGS_EXP_FLAG_NAME)) {
            bodyBuilder.AddCardDirectiveWithTeaserTypeAndId(std::move(renderData), TEASER_TYPE, actionId);
        } else {
            bodyBuilder.AddCardDirective(std::move(renderData), actionId);
        }
    }
}

void AddTeasersPreview(
    const TScenarioRunRequestWrapper& runRequest,
    const NJson::TJsonValue& bassResponse,
    NHollywood::TContext& ctx,
    TRunResponseBuilder& builder,
    const TSemanticFrame& semanticFrame)
{
    const auto frame = TFrame::FromProto(semanticFrame);

    const auto newsData = GetNewsData(bassResponse);
    if (!newsData.Defined()) {
        LOG_WARN(ctx.Logger()) << "News Data not found in form.slots; skipping adding AddCard direcrives";
        return;
    }
    const auto& newsArray = newsData->GetArray();
    const auto topic = GetTopic(bassResponse);

    auto& bodyBuilder = *builder.GetResponseBodyBuilder();

    NData::TScenarioData scenarioData;
    const auto& teaserPreviewData = scenarioData.MutableTeasersPreviewData();

    LOG_INFO(ctx.Logger()) << "Adding teasers previews";
    for (auto i = 0u; i < TEASER_NEWS_PREVIEW_COUNT && i < newsArray.size(); ++i) {
        const auto& newsItemJsonVal = newsArray.at(i);

        auto& previewData = *teaserPreviewData->AddTeaserPreviews();
        previewData.SetTeaserName(TEASER_NAME);

        auto& teaserConfigData = *previewData.MutableTeaserConfigData();
        teaserConfigData.SetTeaserType(TEASER_TYPE);

        auto& newsScenarioData = *previewData.MutableTeaserPreviewScenarioData()->MutableNewsTeaserData();
        newsScenarioData.SetTz(runRequest.ClientInfo().Timezone);
        if (topic) {
            newsScenarioData.SetTopic(*topic);
        }
        auto& newsItem = *newsScenarioData.MutableNewsItem();
        newsItem.SetText(newsItemJsonVal["text"].GetString());
        newsItem.SetUrl(newsItemJsonVal["url"].GetString());
        newsItem.SetAgency(newsItemJsonVal["agency"].GetString());
        newsItem.SetPubDate(newsItemJsonVal["date"].GetUInteger());
        newsItem.SetLogo(newsItemJsonVal["logo"].GetString());
        if (const auto* imageJsonVal = newsItemJsonVal.GetValueByPath("image")) {
            auto& image = *newsItem.MutableImage();
            image.SetSrc((*imageJsonVal)["src"].GetString());
            image.SetWidth((*imageJsonVal)["orig_width"].GetInteger());
            image.SetHeight((*imageJsonVal)["orig_height"].GetInteger());
        }
    }
    bodyBuilder.AddScenarioData(std::move(scenarioData));
}

void AddMainScreenData(
    const TScenarioRunRequestWrapper& runRequest,
    const NJson::TJsonValue& bassResponse,
    NHollywood::TContext& ctx,
    TRunResponseBuilder& builder)
{

    const auto newsData = GetNewsData(bassResponse);
    if (!newsData.Defined()) {
        LOG_WARN(ctx.Logger()) << "News Data not found in form.slots; skipping adding NewsMainScreenData";
        return;
    }

    LOG_INFO(ctx.Logger()) << "Adding NewsMainScreenData";
    NRenderer::TDivRenderData renderData;
    renderData.SetCardId(MAIN_SCREEN_CARD_ID);
    auto& newsMainScreenData = *renderData.MutableScenarioData()->MutableNewsMainScreenData();
    for (const auto& newsItemJsonVal : newsData->GetArray()) {
        auto& newsItem = *newsMainScreenData.AddNewsItems();
        newsItem.SetText(newsItemJsonVal["text"].GetString());
        newsItem.SetUrl(newsItemJsonVal["url"].GetString());
        newsItem.SetAgency(newsItemJsonVal["agency"].GetString());
        newsItem.SetPubDate(newsItemJsonVal["date"].GetUInteger());
        if (const auto* logo = newsItemJsonVal.GetValueByPath("logo")) {
            newsItem.SetLogo(logo->GetString());
        }
        if (const auto* imageJsonVal = newsItemJsonVal.GetValueByPath("image")) {
            auto& image = *newsItem.MutableImage();
            image.SetSrc((*imageJsonVal)["src"].GetString());
            image.SetWidth((*imageJsonVal)["orig_width"].GetInteger());
            image.SetHeight((*imageJsonVal)["orig_height"].GetInteger());
        }
    }
    newsMainScreenData.SetTz(runRequest.ClientInfo().Timezone);
    if (auto newsTopic = GetTopic(bassResponse)) {
        newsMainScreenData.SetTopic(*newsTopic);
    }

    auto& bodyBuilder = *builder.GetResponseBodyBuilder();
    bodyBuilder.AddScenarioData(renderData.GetScenarioData());
}

void AddMainScreenWidget(
    const NJson::TJsonValue& bassResponse,
    NHollywood::TContext& ctx,
    TRunResponseBuilder& builder)
{
    NData::TScenarioData scenarioData;
    auto& widgetData = *scenarioData.MutableCentaurScenarioWidgetData();
    widgetData.SetWidgetType("news");
    auto& newsCardData = *widgetData.AddWidgetCards()->MutableNewsCardData();

    const auto newsData = GetNewsData(bassResponse);
    if (!newsData.Defined()) {
        LOG_WARN(ctx.Logger()) << "News Data not found in form.slots; skipping adding NewsWidgetData";
        return;
    }

    const auto& newsItems = newsData->GetArray();
    if (!newsItems.empty()) {
        const auto rand = RandomNumber<unsigned short>(newsItems.size());
        const auto& newsItemJsonVal = newsItems[rand];

        newsCardData.SetTitle(newsItemJsonVal["agency"].GetString());
        newsCardData.SetContent(newsItemJsonVal["text"].GetString());
        if (const auto* imageJsonVal = newsItemJsonVal.GetValueByPath("image")) {
            newsCardData.SetImageUrl((*imageJsonVal)["src"].GetString());
        }
        if (auto newsTopic = GetTopic(bassResponse)) {
            newsCardData.SetTopic(*newsTopic);
        }
    }

    builder.GetResponseBodyBuilder()->AddScenarioData(scenarioData);
}

void TrySetMementoIsOnboarded(TRunResponseBuilder& builder, const TScenarioRunRequestWrapper& runRequest, bool isOnboarded) {
    if (runRequest.BaseRequestProto().GetMemento().GetUserConfigs().GetNewConfig().GetIsOnboarded() != isOnboarded) {
        NScenarios::TMementoData mementoData = runRequest.BaseRequestProto().GetMemento();
        mementoData.MutableUserConfigs()->MutableNewConfig()->SetIsOnboarded(isOnboarded);
        AddMementoChangeUserObjectsDirective(builder.GetOrCreateResponseBodyBuilder(), mementoData);
    }
}

[[nodiscard]] std::unique_ptr<TScenarioRunResponse> NewsRenderInCallbackMode(
    const TScenarioRunRequestWrapper& runRequest,
    const NHollywoodFw::TRunRequest& runRequestNew,
    const NJson::TJsonValue& bassResponse,
    NHollywood::TContext& ctx,
    TRunResponseBuilder& builder,
    const TNewsBlock& newsBlock,
    const TNewsFastData& fastData,
    NAlice::IRng& rng)
{
    TBassResponseRenderer bassRenderer(runRequest, runRequest.Input(), builder, runRequestNew.Debug().Logger(),
                                       false /*suggestAutoAction*/);
    bassRenderer.SetContextValue(CONTEXT_HAS_INTRO_AND_ENDING, GetHasIntroAndEnding(runRequest));

    NJson::TJsonValue fixlist(NJson::EJsonValueType::JSON_MAP);

    TMementoHelper mementoHelper(fastData);

    //------------------ STAGE BEFORE RENDER ----------------------

    newsBlock.UpdateFixList(fixlist);

    const TNewsPostroll* postroll = nullptr;
    if (fastData.GetPostrollsCount(runRequest)
        && GetSlotValue(bassResponse, "news")["post_news_mode"] == POSTROLL_NEWS_RADIO_CHANGE_SOURCE_MODE)
    {
        postroll = fastData.GetRandomPostroll(runRequest, rng);
    }

    if (newsBlock.IsEnding()) {
        UpdateFixlistForChangeNewsPostroll(mementoHelper, bassResponse, fixlist);
        UpdateFixlistForRadioNewsPostroll(fixlist, postroll);
    }

    //------------------ RENDER ----------------------

    bassRenderer.SetContextValue("fixlist", fixlist);
    bassRenderer.Render("get_news", "render_result", bassResponse, Default<TString>(),  Default<TString>(),
                        true /*processSuggestsOnError*/);

    TNewsState oldState;
    TNewsState state;
    state.SetBassResponse(JsonToString(bassResponse));
    ReadScenarioState<TNewsState>(runRequest.BaseRequestProto(), oldState);

    UpdateExcludeIdsStateForCallbackMode(oldState, state, newsBlock);

    auto frame = TryGetNewsFrame(runRequest);
    const auto maybeNewsIdx = TryGetNewsArrayPosition(frame);
    if (const auto collectCardsSemanticFrame = runRequest.Input().FindSemanticFrame(CENTAUR_COLLECT_CARDS_SEMANTIC_FRAME)) {
        AddTeasers(runRequest, bassResponse, ctx, builder, *collectCardsSemanticFrame);
    } else if (const auto collectCardsSemanticFrame = runRequest.Input().FindSemanticFrame(CENTAUR_COLLECT_TEASERS_PREVIEW_SEMANTIC_FRAME)) {
        AddTeasersPreview(runRequest, bassResponse, ctx, builder, *collectCardsSemanticFrame);
    }
    else if (bassResponse.Has("form") && bassResponse["form"].Has("name") && bassResponse["form"]["name"].GetString() == CENTAUR_COLLECT_MAIN_SCREEN_NEWS_SEMANTIC_FRAME
          || runRequest.Input().FindSemanticFrame(COLLECT_WIDGET_GALLERY_FRAME)) {
        if (runRequest.HasExpFlag(SCENARIO_WIDGET_MECHANICS_EXP_FLAG_NAME)) {
            AddMainScreenWidget(bassResponse, ctx, builder);
        } else {
            AddMainScreenData(runRequest, bassResponse, ctx, builder);
        }

    } else {
        AddShowView(runRequest, bassResponse, ctx, builder, maybeNewsIdx.GetOrElse(0));
    }

    auto nextBlockLink = newsBlock.GetNextBlock();
    if (nextBlockLink.Defined() && !IsSingleNewsRequest(frame)) {
        SetNewsCallback(builder, *nextBlockLink, newsBlock.IsFirstBlock(), frame);
    }

    //------------------ PREPARE REPONSE ----------------------

    auto response = std::move(builder).BuildResponse();

    //------------------ RESPONSE UPDATE ----------------------

    if (!GetDisableVoiceButtons(runRequest)) {
        TVector<TNewsBlockVoiceButton> voiceButtons;
        newsBlock.SetVoiceButtons(voiceButtons);

        for (auto& button : voiceButtons) {
            SetNewsBlockVoiceButton(response, bassResponse, button);
        }

        if (newsBlock.IsEnding()) {
            if (newsBlock.IsShouldListenBlock()) {
                response->MutableResponseBody()->MutableLayout()->SetShouldListen(true);
            }
            SetChangeNewsPostroll(mementoHelper, response, bassResponse, state);
            SetNewsPostrollButton(response, postroll);
        }
    }

    response->MutableResponseBody()->MutableState()->PackFrom(state);
    return response;
}

[[nodiscard]] std::unique_ptr<TScenarioRunResponse> NewsRenderDoImpl(
    const TScenarioRunRequestWrapper& runRequest,
    const NHollywoodFw::TRunRequest& runRequestNew,
    const NJson::TJsonValue& newBassResponse,
    NHollywood::TContext& ctx,
    TRunResponseBuilder& builder,
    NAlice::IRng& rng)
{
    TBassResponseRenderer bassRenderer(runRequest, runRequest.Input(), builder, ctx.Logger(),
                                       false /*suggestAutoAction*/);
    bassRenderer.SetContextValue(CONTEXT_HAS_INTRO_AND_ENDING, GetHasIntroAndEnding(runRequest));
    const auto& fastData = *runRequestNew.System().GetFastData().GetFastData<TNewsFastData>();
    TNewsState state;
    ReadScenarioState<TNewsState>(runRequest.Proto().GetBaseRequest(), state);
    NJson::TJsonValue fixlist(NJson::EJsonValueType::JSON_MAP);

    TFrameNews frameNews(runRequestNew, GET_NEWS_FRAME);
    // TMaybe<TFrame> frame = TryGetNewsFrame(runRequest);
    NJson::TJsonValue stateBassResponse = GetBassResponse(state);
    bool merged = false;
    if (frameNews.Defined()) {
        if (frameNews.ArrayPosition.Defined()) {
            AppendBassResponse(stateBassResponse, newBassResponse);
            LOG_INFO(ctx.Logger()) << "Appended bass response, news position " << frameNews.ArrayPosition.Value << "/" << GetNewsCount(stateBassResponse);
            merged = true;
        }
    }

    const auto& bassResponse = merged ? stateBassResponse : newBassResponse;

    if (runRequest.HasExpFlag(EXP_HW_NEWS_RESET_IS_ONBOARDED)) {
        TrySetMementoIsOnboarded(builder, runRequest, false);
    } else if (GetSlotValue(bassResponse, "news")["onboarding_mode"].IsDefined() && GetHasIntroAndEnding(runRequest)) {
        TrySetMementoIsOnboarded(builder, runRequest, true);
    }

    const TNewsBlock newsBlock(runRequestNew, bassResponse, true);

    if (newsBlock.IsNewsBlock() && !HasErrorBlocks(bassResponse)) {
        return NewsRenderInCallbackMode(
            runRequest,
            runRequestNew,
            bassResponse,
            ctx,
            builder,
            newsBlock,
            fastData,
            rng
        );
    }

    TMementoHelper mementoHelper(fastData);

    const TNewsPostroll* postroll = nullptr;
    if (fastData.GetPostrollsCount(runRequest)
        && GetSlotValue(bassResponse, "news")["post_news_mode"] == POSTROLL_NEWS_RADIO_CHANGE_SOURCE_MODE)
    {
        postroll = fastData.GetRandomPostroll(runRequest, rng);
    }

    UpdateFixlistForChangeNewsPostroll(mementoHelper, bassResponse, fixlist);
    UpdateFixlistForRadioNewsPostroll(fixlist, postroll);

    bassRenderer.SetContextValue("fixlist", fixlist);
    bassRenderer.Render("get_news", "render_result", bassResponse, Default<TString>(),  Default<TString>(),
                        true /*processSuggestsOnError*/);

    AddShowView(runRequest, bassResponse, ctx, builder);

    auto response = std::move(builder).BuildResponse();

    UpdateExcludeIdsState(state, bassResponse);

    if (!GetDisableVoiceButtons(runRequest)) {
        SetMoreNewsVoiceButton(response, bassResponse);
        SetChangeNewsPostroll(mementoHelper, response, bassResponse, state);
        SetNewsPostrollButton(response, postroll);
        response->MutableResponseBody()->MutableLayout()->SetShouldListen(true);
    }

    response->MutableResponseBody()->MutableState()->PackFrom(state);
    return response;
}

} // namespace NImpl

void TBassNewsRenderHandle::Do(TScenarioHandleContext& ctx) const {
    Y_ENSURE(ctx.NewContext, "This functioncan be called throgh new flow only");
    const NHollywoodFw::TRunRequest& runRequest = *(ctx.NewContext->RunRequest);

    const auto bassResponseBody = RetireBassRequest(ctx);
    const auto runRequestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request(runRequestProto, ctx.ServiceCtx);

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto response = NImpl::NewsRenderDoImpl(request, runRequest, bassResponseBody, ctx.Ctx, builder, ctx.Rng);
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
    if (auto responseBodyBuilder = builder.GetResponseBodyBuilder()) {
        for (auto const& [cardId, cardData] : responseBodyBuilder->GetRenderData()) {
            LOG_INFO(ctx.Ctx.Logger()) << "Adding render_data to context";
            ctx.ServiceCtx.AddProtobufItem(cardData, RENDER_DATA_ITEM);
        }
    }
}

} // namespace NAlice::NHollywood
