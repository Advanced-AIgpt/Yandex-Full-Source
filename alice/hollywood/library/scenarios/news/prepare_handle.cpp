//
// NEWS Prepare handler
//
// Обрабатывает запросы к новостям, а также запросы на обслуживание системы новостей:
//
// personal_assistant.scenarios.get_news (основной запрос новостей)
// personal_assistant.scenarios.get_free_news (free news?)
// personal_assistant.scenarios.get_news__more (продолжение новостей, эллипсис)
// personal_assistant.scenarios.get_news__previous (возврат к предыдущим новостям)
// personal_assistant.scenarios.get_news_settings (установка подписки на новости в memento)
// personal_assistant.scenarios.get_news__postroll_answer (ответ на запросы подписки на новости)
//

#include "prepare_handle.h"

#include "alice/hollywood/library/framework/core/request.h"
#include "alice/hollywood/library/framework/core/semantic_frames.h"
#include "bass.h"
#include "frame.h"
#include "memento_helper.h"
#include "news_block.h"
#include "news_fast_data.h"
#include "news_settings_push.h"
#include "render_handle.h"
#include "date_helper.h"
#include "util/generic/yexception.h"

#include <alice/hollywood/library/scenarios/news/proto/news.pb.h>

#include <alice/hollywood/library/scenarios/search/context/context.h>

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/framework/framework_migration.h>
#include <alice/hollywood/library/bass_adapter/bass_renderer.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/web_search_source.pb.h>

#include <alice/library/app_navigation/navigation.h>
#include <alice/library/json/json.h>
#include <alice/library/proto/protobuf.h>

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>
#include <util/string/join.h>

using namespace ru::yandex::alice::memento::proto;

namespace {

constexpr TStringBuf IGNORE_MEMENTO_FLAG = "news_ignore_memento";

constexpr TStringBuf MAX_DAYS_BEFORE_PREFIX = "max_days_before=";

const NSc::TValue SETTINGS_ANSWER = NSc::TValue::FromJson(R"({
    "nav": {
        "url": {
            "_": "https://yandex.ru/quasar/account/news",
            "desktop": "https://yandex.ru/quasar/account/news"
        },
        "text": "Настройка новостей Алисы",
        "voice_name": "страницу настройки новостей",
        "text_name": "страницу настройки новостей"
    }
})");

} // anon namespace

namespace NAlice::NHollywood {

namespace NImpl {

void AddDeclineButton(TResponseBodyBuilder& bodyBuilder) {
    TFrameNluHint declineNluHint;
    declineNluHint.SetFrameName("alice.proactivity.decline");

    TSemanticFrame doNothingFrame;
    doNothingFrame.SetName("alice.do_nothing");

    NScenarios::TFrameAction actionDecline;
    *actionDecline.MutableNluHint() = std::move(declineNluHint);
    *actionDecline.MutableFrame() = std::move(doNothingFrame);
    bodyBuilder.AddAction("decline", std::move(actionDecline));
}

void RenderSuggest(NSearch::TSearchContext& ctx, const NSearch::TSuggest suggest) {
    auto renderedSuggest = TBassResponseRenderer::CreateSuggest(ctx.GetNlg(), TStringBuf("get_news"), suggest.Type,
                                                        /* analyticsTypeAction = */ "", suggest.AutoAction,
                                                        suggest.Data, ctx.GetNlgData(), &ctx.GetRequest().Interfaces());
    if (renderedSuggest.Defined()) {
        ctx.GetBodyBuilder().AddRenderedSuggest(std::move(renderedSuggest.GetRef()));
    }
}

void ProcessSettingsAnswer(NSearch::TSearchContext& ctx) {
    const NSc::TValue block = NAlice::CreateNavBlock(SETTINGS_ANSWER, ctx.GetRequest(), true);
    ctx.SetResultSlot(TStringBuf("nav"), block.ToJsonValue());
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName("news_settings");
    ctx.AddAttention("simple_open_link");
    ctx.AddSuggest(TStringBuf("news_settings"), /* autoaction = */ true);
    ctx.SetShouldListen(false);
}

NScenarios::TScenarioRunResponse MakeTopicDefault(
    TRunResponseBuilder& response,
    TNlgData& nlgData,
    NScenarios::TMementoData& mementoData,
    TString smiMementoId,
    TString successNlg)
{
    nlgData.Context["attentions"]["postroll_push"] = false;
    auto& bodyBuilder = response.CreateResponseBodyBuilder();
    nlgData.Context["source"] = smiMementoId;
    AddMementoChangeUserObjectsDirective(bodyBuilder, mementoData);
    bodyBuilder.AddRenderedTextWithButtonsAndVoice("get_news", successNlg,
                                                    /* buttons = */ {}, nlgData);
    return *std::move(response).BuildResponse();
}

NScenarios::TScenarioRunResponse TryMakeTopicDefault(
    TRunResponseBuilder& response,
    TNlgData& nlgData,
    NScenarios::TMementoData& mementoData,
    TMementoHelper& mementoHelper,
    TString topic,
    TString successNlg,
    TString failNlg)
{
    auto& bodyBuilder = response.CreateResponseBodyBuilder();
    if (!mementoHelper.PrepareChangeDefaultRequest(topic, mementoData)) {
        bodyBuilder.AddRenderedTextWithButtonsAndVoice("get_news", failNlg,
                                                        /* buttons = */ {}, nlgData);
        auto result = std::move(response).BuildResponse();
        SetSourceChangePostrollVoiceButton(result);
        return *result;
    }
    nlgData.Context["attentions"]["postroll_push"] = false;
    AddMementoChangeUserObjectsDirective(bodyBuilder, mementoData);
    nlgData.Context["source"] = mementoHelper.GetSourceName(topic);
    bodyBuilder.AddRenderedTextWithButtonsAndVoice("get_news", successNlg,
                                                    /* buttons = */ {}, nlgData);
    return *std::move(response).BuildResponse();
}

void SetIfIsMementable(TMementoHelper& mementoHelper, NJson::TJsonValue& mementoRecord, const TMaybe<TString> topicSlot, const TMaybe<TString> whereSlot) {
    if (!topicSlot || whereSlot
        || *topicSlot == mementoRecord[MEMENTO_RUBRIC].GetString()
        || *topicSlot == mementoRecord[MEMENTO_SOURCE].GetString())
    {
        mementoRecord["is_mementable_request_topic"] = false;
        return;
    }
    mementoRecord["is_mementable_request_topic"] = mementoHelper.IsMementableTopic(*topicSlot);
}

NScenarios::TScenarioRunResponse GetNoNewsResponse(
    const TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    TRTLogger& logger,
    bool irrelevant)
{
    TRunResponseBuilder response(&nlg);
    if (irrelevant) {
        response.SetIrrelevant();
    }

    TNlgData nlgData{logger, request};

    // Phrase render_error__nonews checks attentions.
    nlgData.Context["attentions"].SetType(NJson::JSON_MAP);
    nlgData.ReqInfo["experiments"].SetType(NJson::JSON_MAP);
    for (const auto& [name, value] : request.ExpFlags()) {
        if (value.Defined()) {
            nlgData.ReqInfo["experiments"][name] = *value;
        }
    }
    auto& bodyBuilder = response.CreateResponseBodyBuilder();
    bodyBuilder.AddRenderedTextWithButtonsAndVoice("get_news", "render_error__nonews",
                                                    /* buttons = */ {}, nlgData);

    return *std::move(response).BuildResponse();
}

NScenarios::TScenarioRunResponse GetIrrelevantResponse(
    const TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    TRTLogger& logger)
{
    return GetNoNewsResponse(request, nlg, logger, true);
}

const TSmi* TryResolveSmiFromTopic(const TString& topic, const TNewsFastData& fastData) {
    if (const TSmi* smi = fastData.GetSmiByGranetId(topic)) {
        return smi;
    }
    return fastData.GetSmiByMementoId(topic);
}

void TryAddResolvedSmiSlot(TMaybe<TFrame>& frame, const TNewsFastData& fastData) {
    TPtrWrapper<TSlot> topicSlot = frame->FindSlot(TOPIC_SLOT);
    if (!topicSlot) {
        return;
    }
    TString topicValue = topicSlot->Value.AsString();
    const TSmi* smi = TryResolveSmiFromTopic(topicValue, fastData);
    if (!smi) {
        return;
    }
    NJson::TJsonValue smiObj;
    // Note: Names are same as in the bass TSmi constructor
    smiObj["aid"] = smi->ApiId;
    smiObj["alias"] = smi->GranetId;
    smiObj["name"] = smi->Name;
    smiObj["url"] = smi->Uri;
    smiObj["logo"] = smi->Logo;
    frame->AddSlot(TSlot{"smi", "smi", TSlot::TValue{JsonToString(smiObj)}});
}

bool HasNewsInWebSearchDocs(const NHollywoodFw::TRunRequest& request) {
    auto* webDocs = request.GetDataSource(EDataSourceType::WEB_SEARCH_DOCS);
    if (!webDocs) {
        return false;
    }
    auto& webSearchDocs = webDocs->GetWebSearchDocs();
    for (auto& doc : webSearchDocs.GetDocs()) {
        auto& snippet = doc.GetSnippets().GetFull();
        if (JsonFromProto(snippet)["type"] == "news") {
            return true;
        }
    }
    return false;
}

std::variant<THttpProxyRequest, NScenarios::TScenarioRunResponse> NewsPrepareDoImpl(
    TContext& ctx,
    TNlgWrapper& nlg,
    TRunResponseBuilder& builder,
    const TScenarioRunRequestWrapper& request,
    const NScenarios::TRequestMeta& meta,
    const NJson::TJsonValue& appHostParams,
    NAlice::IRng& rng,
    const NHollywoodFw::TRunRequest& runRequest)
{
    auto& logger = runRequest.Debug().Logger();
    const NHollywood::TPtrWrapper<NScenarios::TCallbackDirective> callback = runRequest.Input().FindCallback();
    const auto& fastData = *runRequest.System().GetFastData().GetFastData<TNewsFastData>();

    TNewsState state;
    if (!ReadScenarioState<TNewsState>(request.BaseRequestProto(), state)) {
        state.Clear();
    }
    const NJson::TJsonValue bassResponse = GetBassResponse(state);
    const TNewsBlock newsBlock(runRequest, bassResponse, false);

    if (callback)
    {
        if (newsBlock.IsNewsBlock() && !newsBlock.NeedNewRequest())
        {
            return *NewsRenderInCallbackMode(
                request,
                runRequest,
                bassResponse,
                ctx,
                builder,
                newsBlock,
                fastData,
                rng
            );
        }
        if (newsBlock.GetCallbackMode() == CALLBACK_MODE_MORE_INFO)
        {
            TNlgData nlgData{ctx.Logger(), request};
            TResponseBodyBuilder& bodyBuilder = builder.CreateResponseBodyBuilder();

            auto url = newsBlock.GetNewsUrl(bassResponse);
            auto text = newsBlock.GetNewsText(bassResponse);

            AddPushMessageDirective(bodyBuilder, MORE_INFO_NEWS, text, url);
            bodyBuilder.AddRenderedTextWithButtonsAndVoice("get_news", "render_news_more_info",
                                                            /* buttons = */ {}, nlgData);

            AddDeclineButton(bodyBuilder);
            auto response = std::move(builder).BuildResponse();
            auto confirmButton = newsBlock.GetMoreNewsAfterHandoffButton();
            SetNewsBlockVoiceButton(response, bassResponse, confirmButton);
            response->MutableResponseBody()->MutableLayout()->SetShouldListen(true);

            return *response;
        }
    }

    const TMaybe<TFrame> callbackFrame = callback ? GetCallbackFrame(callback.Get()) : Nothing();
    TMaybe<TFrame> frame = TryGetFrame(GET_NEWS_FRAME, callbackFrame, request.Input());
    TMaybe<TFrame> freeNewsFrame = TryGetFrame(GET_FREE_NEWS_FRAME, callbackFrame, request.Input());
    TMaybe<TFrame> collectCardsFrame = TryGetFrame(COLLECT_CARDS_FRAME, callbackFrame, request.Input());
    TMaybe<TFrame> collectTeasersPreviewFrame = TryGetFrame(COLLECT_TEASERS_PREVIEW_FRAME, callbackFrame, request.Input());
    TMaybe<TFrame> collectMainScreenFrame = TryGetFrame(COLLECT_MAIN_SCREEN_FRAME, callbackFrame, request.Input());
    TMaybe<TFrame> collectWidgetGalleryFrame = TryGetFrame(COLLECT_WIDGET_GALLERY_FRAME, callbackFrame, request.Input());

    TFrameNews frameNews(runRequest, GET_NEWS_FRAME);
    TFrameNews frameFreeNews(runRequest, GET_FREE_NEWS_FRAME);
    TFrameNews frameCollectCards(runRequest, COLLECT_CARDS_FRAME);
    TFrameNews frameCollectTeasersPreview(runRequest, COLLECT_TEASERS_PREVIEW_FRAME);
    TFrameNews frameCollectMainScreen(runRequest, COLLECT_MAIN_SCREEN_FRAME);
    TFrameNews frameCollectWidgetGallery(runRequest, COLLECT_WIDGET_GALLERY_FRAME);
    TFrameNews frameEmptyCentaur(runRequest, COLLECT_MAIN_SCREEN_NEWS_FRAME, EMPTY_FRAME_MODE);

    Y_ENSURE(frameNews.Defined() == frame.Defined());
    Y_ENSURE(frameFreeNews.Defined() == freeNewsFrame.Defined());
    Y_ENSURE(frameCollectCards.Defined() == collectCardsFrame.Defined());
    Y_ENSURE(frameCollectTeasersPreview.Defined() == collectTeasersPreviewFrame.Defined());
    Y_ENSURE(frameCollectMainScreen.Defined() == collectMainScreenFrame.Defined());
    Y_ENSURE(frameCollectWidgetGallery.Defined() == collectWidgetGalleryFrame.Defined());

    TFrameSettings settingsFrame(runRequest, GET_NEWS_SETTINGS_FRAME);
    TFrameSettings postrollAnswerFrame(runRequest, GET_NEWS_POSTROLL_ANSWER_FRAME);

    TFrameNews* currentFrame = &frameNews;
    if (!settingsFrame.Defined()) {
        if (!currentFrame->Defined() && frameFreeNews.Defined()) {
            currentFrame = &frameFreeNews;
            frame = freeNewsFrame; // temp - this code is still needed for PrepareBassVinsRequest
        }
        if (!currentFrame->Defined() && frameCollectCards.Defined()) {
            currentFrame = &frameCollectCards;
            frame = collectCardsFrame; // temp - this code is still needed for PrepareBassVinsRequest
        }
        if (!currentFrame->Defined() && frameCollectTeasersPreview.Defined()) {
            currentFrame = &frameCollectTeasersPreview;
            frame = collectTeasersPreviewFrame; // temp - this code is still needed for PrepareBassVinsRequest
        }
        if (!currentFrame->Defined() && frameCollectMainScreen.Defined()) {
            currentFrame = &frameEmptyCentaur;
            frame = TFrame{TString{COLLECT_MAIN_SCREEN_NEWS_FRAME}};; // temp - this code is still needed for PrepareBassVinsRequest
        }
        if (!currentFrame->Defined() && frameCollectWidgetGallery.Defined()) {
            currentFrame = &frameCollectWidgetGallery;
            frame = collectWidgetGalleryFrame; // temp - this code is still needed for PrepareBassVinsRequest
        }
    }

    if (runRequest.Flags().IsExperimentEnabled(EXP_DISABLE_NEWS)) {
        const bool isWizardRequest = currentFrame->Defined() &&
            !currentFrame->WhereSlot.Defined() &&
            currentFrame->TopicSlot.Defined() &&
            currentFrame->TopicSlot.GetType() == "string";
        LOG_INFO(logger) << "News disabled";
        return GetNoNewsResponse(request, nlg, logger, isWizardRequest);
    }

    if (currentFrame->Defined() && currentFrame->GetName() == GET_FREE_NEWS_FRAME && !HasNewsInWebSearchDocs(runRequest)) {
        LOG_INFO(logger) << "No wizard for free grammar";
        return GetIrrelevantResponse(request, nlg, logger);
    }

    if (currentFrame->Defined() && currentFrame->GetName() == GET_NEWS_FRAME && currentFrame->NotNewsSlot.Defined() && !HasNewsInWebSearchDocs(runRequest)) {
        LOG_INFO(logger) << "No wizard for free grammar";
        return GetIrrelevantResponse(request, nlg, logger);
    }

    if (currentFrame->Defined() && currentFrame->GetName() == GET_NEWS_FRAME && newsBlock.IsNewsBlock() && !newsBlock.NeedNewRequest()) {
        return *NewsRenderInCallbackMode(request, runRequest, bassResponse, ctx, builder, newsBlock, fastData, rng);
    }

    TMementoHelper mementoHelper(fastData);
    NScenarios::TMementoData mementoData = request.BaseRequestProto().GetMemento();

    NJson::TJsonValue mementoRecord = mementoHelper.ParseMementoReponse(
        mementoData,
        runRequest.Flags().IsExperimentEnabled(IGNORE_MEMENTO_FLAG));
    LOG_INFO(logger) << "memento data from request: " << JsonFromProto(mementoData);
    LOG_INFO(logger) << "memento default response: " << mementoRecord;

    if (currentFrame->Defined() && !settingsFrame.Defined()) {
        bool isDefaultRequest = !currentFrame->TopicSlot.Defined() && !currentFrame->WhereSlot.Defined();
        int maxDaysBefore = runRequest.Flags().GetSubValue(MAX_DAYS_BEFORE_PREFIX, 7);
        if (!IsActual(currentFrame->DateSlot.Value, TInstant::Now(), maxDaysBefore)) {
            LOG_INFO(logger) << "Requesting news for more than 7 days. It is probably search request";
            return GetIrrelevantResponse(request, nlg, logger);
        }

        // TODO костыль: метода IsMementableTopic нет в бассе. Надо перенести логику установки постролла в рендер
        SetIfIsMementable(mementoHelper, mementoRecord, currentFrame->TopicSlot.Value, currentFrame->WhereSlot.Value);

        if (isDefaultRequest && !mementoRecord.IsNull()) {
            if (mementoRecord[MEMENTO_RESULT] == MEMENTO_RESULT_ANOTHER_SCENARIO && (request.ClientInfo().IsQuasar()
                || request.ClientInfo().IsMiniSpeaker() || request.ClientInfo().IsSearchApp() || request.ClientInfo().IsNavigator()))
            {
                LOG_INFO(logger) << "memento: irrelevant because it is radio request";
                return GetIrrelevantResponse(request, nlg, logger);
            }
            if (mementoRecord[MEMENTO_RESULT] == MEMENTO_RESULT_SUCCESS) {
                TString topicSlotValue = mementoRecord[MEMENTO_SOURCE] == MEMENTO_SOURCE_DEFAULT
                    ? mementoRecord[MEMENTO_RUBRIC].GetString()
                    : mementoRecord[MEMENTO_SOURCE].GetString();
                if (!topicSlotValue.is_null()) {
                    frame->AddSlot(TSlot{TString(TOPIC_SLOT), TString(TOPIC_SLOT_MEMENTO_RUBRIC_TYPE), TSlot::TValue{topicSlotValue}});
                    LOG_INFO(logger) << "memento: append news topic - " << topicSlotValue;
                }
            }
        }

        NJson::TJsonValue excludeIds;
        for (auto idToExclude : state.GetExcludeIds()) {
            excludeIds.AppendValue(idToExclude);
        }
        NJson::TJsonValue news;
        news["exclude_ids"] = excludeIds;
        frame->AddSlot(TSlot{"news", "news", TSlot::TValue{JsonToString(news)}});
        frame->AddSlot(TSlot{"news_memento", "news_memento", TSlot::TValue{JsonToString(mementoRecord)}});
        frame->AddSlot(TSlot{"is_default_request", "is_default_request", TSlot::TValue{ToString(isDefaultRequest)}});
        TryAddResolvedSmiSlot(frame, fastData);

        const TVector<EDataSourceType> webSearchDataSource = {EDataSourceType::BLACK_BOX, EDataSourceType::WEB_SEARCH_DOCS};
        return PrepareBassVinsRequest(logger, request, *frame, /* sourceTextProvider= */ nullptr,
                                      meta, /* imageSearch= */ false, appHostParams,
                                      /* forbidWebSearch= */ true, webSearchDataSource);
    }

    if (postrollAnswerFrame.Defined() || settingsFrame.Defined()) {
        TRunResponseBuilder response(&nlg);
        bool isSettingsMode = settingsFrame.Defined();
        auto setupFrame = isSettingsMode ? &settingsFrame : &postrollAnswerFrame;

        TNlgData nlgData{logger, request};

        if (GetUid(request).empty()) {
            auto& bodyBuilder = response.CreateResponseBodyBuilder();
            bodyBuilder.AddRenderedTextWithButtonsAndVoice("get_news", "render_news_settings_without_auth",
                                                            /* buttons = */ {}, nlgData);
            return *std::move(response).BuildResponse();
        }
        if (setupFrame->RadioSource.Defined()) {
            mementoHelper.PrepareConfig(mementoData, "main", *setupFrame->RadioSource.Value);
            return MakeTopicDefault(response, nlgData, mementoData, *setupFrame->RadioSource.Value, "render_postroll_new_source_set");
        }
        if (setupFrame->Topic.Defined()) {
            return TryMakeTopicDefault(response, nlgData, mementoData, mementoHelper, *setupFrame->Topic.Value,
                "render_postroll_new_source_set", "render_postroll_unknown_source_response");
        }
        if (setupFrame->Answer.Defined() && *setupFrame->Answer.Value == "yes" && !state.GetTopic().is_null()) {
            return TryMakeTopicDefault(response, nlgData, mementoData, mementoHelper, state.GetTopic(),
                "render_postroll_new_source_set", "render_postroll_unknown_source_response");
        }
        if (setupFrame->Answer.Defined() && *setupFrame->Answer.Value == "no") {
            TResponseBodyBuilder& bodyBuilder = response.CreateResponseBodyBuilder();
            if (mementoRecord[MEMENTO_RESULT] == MEMENTO_RESULT_EMPTY) {
                mementoHelper.PrepareChangeDefaultRequest("index", mementoData);
                AddMementoChangeUserObjectsDirective(bodyBuilder, mementoData);
            }
            nlgData.Context["attentions"]["search__push_sent"] = true;
            bodyBuilder.AddRenderedTextWithButtonsAndVoice("get_news", "render_postroll_negative_response",
                                                            /* buttons = */ {}, nlgData);
            return *std::move(response).BuildResponse();
        }

        if (isSettingsMode || setupFrame->Answer.Defined() && *setupFrame->Answer.Value == "yes") {
            if (!request.ClientInfo().IsSmartSpeaker() && !request.ClientInfo().IsTvDevice() && !request.ClientInfo().IsLegatus()) {
                auto context = NSearch::TSearchContext(request, response, ctx, meta, rng);
                ProcessSettingsAnswer(context);
                for (const auto& suggest : context.GetSuggests()) {
                    RenderSuggest(context, suggest);
                }
                context.GetBodyBuilder().AddRenderedTextWithButtonsAndVoice("get_news", "render_search_app_news_settings",
                                                            /* buttons = */ {}, nlgData);
                return *std::move(response).BuildResponse();
            }


            TResponseBodyBuilder& bodyBuilder = response.CreateResponseBodyBuilder();
            if (mementoRecord[MEMENTO_RESULT] == MEMENTO_RESULT_EMPTY) {
                mementoHelper.PrepareChangeDefaultRequest("index", mementoData);
                AddMementoChangeUserObjectsDirective(bodyBuilder, mementoData);
            }

            nlgData.Context["attentions"]["search__push_sent"] = true;

            AddPushMessageDirective(bodyBuilder, PUSH_BODIES[0].Title, PUSH_BODIES[0].Text, PUSH_URI);

            bodyBuilder.AddRenderedTextWithButtonsAndVoice("get_news", "render_postroll_response",
                                                            /* buttons = */ {}, nlgData);
            return *std::move(response).BuildResponse();
        }
    }

    LOG_WARNING(logger) << "Failed to get get_news semantic frame";

    return GetIrrelevantResponse(request, nlg, logger);
}

} // namespace NImpl

void TBassNewsPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    Y_ENSURE(ctx.NewContext, "This functioncan be called throgh new flow only");
    const NHollywoodFw::TRunRequest& runRequest = *(ctx.NewContext->RunRequest);

    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    auto nlg = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlg);

    const auto result = NImpl::NewsPrepareDoImpl(ctx.Ctx, nlg, builder, request, ctx.RequestMeta,
        ctx.AppHostParams, ctx.Rng, runRequest);

    struct {
        TScenarioHandleContext& Ctx;
        TRunResponseBuilder& builder;
        // Common case: request BASS.
        void operator()(const THttpProxyRequest& bassRequest) {
            AddBassRequestItems(Ctx, bassRequest);
        }
        // Irrelevant case: exit.
        void operator()(const NScenarios::TScenarioRunResponse& response) {
            Ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
            if (auto responseBodyBuilder = builder.GetResponseBodyBuilder()) {
                for (auto const& [cardId, cardData] : responseBodyBuilder->GetRenderData()) {
                    LOG_INFO(Ctx.Ctx.Logger()) << "Adding render_data to context";
                    Ctx.ServiceCtx.AddProtobufItem(cardData, RENDER_DATA_ITEM);
                }
            }
        }
    } visitor{ctx, builder};
    std::visit(visitor, result);
}

} // namespace NAlice::NHollywood
