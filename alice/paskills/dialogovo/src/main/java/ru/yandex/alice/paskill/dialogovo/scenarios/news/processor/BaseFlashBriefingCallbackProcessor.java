package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor;


import java.util.List;
import java.util.Optional;
import java.util.Random;

import lombok.Data;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.CommitNeededResponse;
import ru.yandex.alice.kronstadt.core.CommitResult;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.CommittingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SessionAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.analytics.AnalyticsInfoActionsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsCallbackDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsFeedCallbackDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsContent;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsCommitService;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsFeedResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsProviderSuggestService;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsSkillStateMaintainer;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;

abstract class BaseFlashBriefingCallbackProcessor
        implements CommittingRunProcessor<DialogovoState, ReadNewsApplyArguments> {
    private static final Logger logger = LogManager.getLogger();
    private static final int MAX_SUGGEST_PROVIDERS = 4;

    private final NewsCommitService newsCommitService;
    private final NewsSkillProvider skillProvider;
    private final NewsFeedResolver newsFeedResolver;
    private final NewsSkillStateMaintainer newsSkillStateMaintainer;
    private final NewsProviderSuggestService newsProviderSuggestService;
    private final AppMetricaEventSender appMetricaEventSender;

    protected BaseFlashBriefingCallbackProcessor(NewsCommitService newsCommitService,
                                                 NewsSkillProvider skillProvider,
                                                 NewsFeedResolver newsFeedResolver,
                                                 NewsSkillStateMaintainer newsSkillStateMaintainer,
                                                 NewsProviderSuggestService newsProviderSuggestService,
                                                 AppMetricaEventSender appMetricaEventSender) {
        this.newsCommitService = newsCommitService;
        this.skillProvider = skillProvider;
        this.newsFeedResolver = newsFeedResolver;
        this.newsSkillStateMaintainer = newsSkillStateMaintainer;
        this.newsProviderSuggestService = newsProviderSuggestService;
        this.appMetricaEventSender = appMetricaEventSender;
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {

        NewsPayload payload = getNewsPayload(request);

        DialogovoState.NewsState currNewsState = newsSkillStateMaintainer.getNewsState(request);

        ActivationSourceType activationSourceType =
                newsSkillStateMaintainer.getActivationSourceTypeFromSession(request.getStateO());

        Optional<NewsSkillInfo> skillO = skillProvider.getSkill(payload.getSkillId());
        if (skillO.isEmpty()) {
            logger.info("Unable to detect news skill by id [{}]", payload.getSkillId());
            return createIrrelevantResult(request);
        }
        NewsSkillInfo skillInfo = skillO.get();

        DialogovoState stateWithoutAppmetrica = newsSkillStateMaintainer.generateDialogovoState(
                request.getStateO(),
                currNewsState,
                activationSourceType,
                request.getServerTime());

        Optional<NewsFeed> newsFeedO = newsFeedResolver.findFeedById(skillInfo, payload.getFeedId());
        if (newsFeedO.isEmpty()) {
            writeAnalytics(context, skillInfo, activationSourceType, stateWithoutAppmetrica, getIntent());

            logger.info("Unable to detect feed by id [{}] try default feed", payload.getFeedId());
            Optional<BaseRunResponse> onFeedByIdNotFoundResponse = onFeedByIdNotFound(request);
            if (onFeedByIdNotFoundResponse.isPresent()) {
                return onFeedByIdNotFoundResponse.get();
            }
            Optional<NewsFeed> defaultFeedO = newsFeedResolver.findDefaultFeed(skillInfo);
            if (defaultFeedO.isEmpty()) {
                logger.info("Unable to detect news feed by skill [{}]", skillInfo.getId());
                return createIrrelevantResult(request);
            } else {
                newsFeedO = defaultFeedO;
            }
        }

        NewsFeed newsFeed = newsFeedO.get();
        logger.info("Prepared to tell news from skill [{}] default feed [{}] name [{}] topic [{}]",
                skillInfo.getId(), newsFeed.getId(), newsFeed.getName(), newsFeed.getTopic());

        List<NewsContent> topNewsByFeed = newsFeed.getTopContents();

        Optional<String> lasNewsReadBySkillFeedO = currNewsState.getLastNewsReadBySkillFeed(skillInfo.getId(),
                newsFeed.getId());
        logger.info("Last news read by skill feed [{}]", lasNewsReadBySkillFeedO);
        if (lasNewsReadBySkillFeedO.isEmpty()) {
            Optional<BaseRunResponse> onLastNewsReadBySkillFeedEmptyResponseO =
                    onLastNewsReadBySkillFeedEmpty(stateWithoutAppmetrica, context, request.getRandom(), newsFeed,
                            skillInfo);

            if (onLastNewsReadBySkillFeedEmptyResponseO.isPresent()) {
                writeAnalytics(context, skillInfo, activationSourceType, stateWithoutAppmetrica, getIntent());
                return onLastNewsReadBySkillFeedEmptyResponseO.get();
            }
        }

        logger.info("Reading news content from [{}]", payload.getContentIdO());
        Optional<NewsContent> newsContentO = getNewsContent(
                topNewsByFeed, payload.getContentIdO(), newsFeed.getDepth());
        logger.info("Resolved news content id: [{}]", newsContentO.map(NewsContent::getId));
        logger.debug("Resolved news content source [{}]", newsContentO);

        List<NewsSkillInfo> proversSuggests = newsProviderSuggestService.suggest(currNewsState, request.getRandom(),
                request.getServerTime(), MAX_SUGGEST_PROVIDERS);

        if (newsContentO.isEmpty()) {
            writeAnalytics(context, skillInfo, activationSourceType, stateWithoutAppmetrica, getIntent());
            return onNewsContentEmpty(stateWithoutAppmetrica, context, request.getRandom(),
                    newsFeed, skillInfo, request, proversSuggests).get();
        }

        newsSkillStateMaintainer.updateNewsStateWithContent(skillInfo, newsFeed, currNewsState, newsContentO);

        AppmetricaCommitArgs appmetricaArgs = appMetricaEventSender.prepareClientEventsForCommit(
                SourceType.USER,
                skillInfo.getId(),
                SkillInfo.Look.EXTERNAL,
                skillInfo.getEncryptedAppMetricaApiKey(),
                request.getClientInfo(),
                request.getLocationInfoO(),
                request.getServerTime(),
                stateWithoutAppmetrica.getSessionO(),
                MetricaEventConstants.INSTANCE.getNewsReadMetricaEvent(),
                Optional.empty(),
                request.isTest(),
                skillInfo.hasFeatureFlag(SkillFeatureFlag.APPMETRICA_EVENTS_TIMESTAMP_COUNTER)
        );

        DialogovoState dialogovoState = newsSkillStateMaintainer.generateDialogovoState(
                request.getStateO(),
                currNewsState,
                activationSourceType,
                request.getServerTime(),
                request.getStateO().flatMap(DialogovoState::getSessionO)
                        .map(Session::getAppMetricaEventCounter).orElse(1L)
                        + (appmetricaArgs == null ?
                        0L : appmetricaArgs.getReportMessage().getSessions(0).getEventsCount())
        );

        writeAnalytics(context, skillInfo, activationSourceType, dialogovoState, getIntent());

        return new CommitNeededResponse<>(
                onProcessFinished(
                        skillInfo,
                        context,
                        request.getRandom(),
                        newsFeed,
                        newsContentO.get(),
                        dialogovoState,
                        topNewsByFeed,
                        request,
                        proversSuggests),
                new ReadNewsApplyArguments(skillInfo.getId(), appmetricaArgs)
        );
    }

    @SuppressWarnings("ParameterNumber")
    protected abstract ScenarioResponseBody<DialogovoState> onProcessFinished(
            NewsSkillInfo skillInfo,
            Context context,
            Random random,
            NewsFeed newsFeed,
            NewsContent newsContent,
            DialogovoState dialogovoState,
            List<NewsContent> topNewsByFeed,
            MegaMindRequest<DialogovoState> request,
            List<NewsSkillInfo> proversSuggest);

    protected abstract Optional<NewsContent> getNewsContent(List<NewsContent> topNewsByFeed,
                                                            Optional<String> contentIdO, int depth);

    protected abstract NewsPayload getNewsPayload(MegaMindRequest<DialogovoState> request);

    @Data
    public static class NewsPayload {
        private final String skillId;
        private final String feedId;
        private final Optional<String> contentIdO;

        public static NewsPayload from(NewsCallbackDirective payload) {
            return new NewsPayload(payload.getSkillId(), payload.getFeedId(), Optional.of(payload.getContentId()));
        }

        public static NewsPayload from(NewsFeedCallbackDirective payload) {
            return new NewsPayload(payload.getSkillId(), payload.getFeedId(), Optional.empty());
        }
    }

    protected Optional<BaseRunResponse> onFeedByIdNotFound(MegaMindRequest<DialogovoState> request) {
        return Optional.empty();
    }

    protected Optional<BaseRunResponse> onNewsContentEmpty(DialogovoState dialogovoState, Context context,
                                                           Random random, NewsFeed newsFeed, NewsSkillInfo skillInfo,
                                                           MegaMindRequest<DialogovoState> request,
                                                           List<NewsSkillInfo> proversSuggest) {
        return Optional.of(createIrrelevantResult(request));
    }

    protected Optional<BaseRunResponse> onLastNewsReadBySkillFeedEmpty(DialogovoState dialogovoState, Context context,
                                                                       Random random, NewsFeed newsFeed,
                                                                       NewsSkillInfo skillInfo) {
        return Optional.empty();
    }

    protected String getIntent() {
        return Intents.EXTERNAL_SKILL_CONTINUE_READ_NEWS_FROM_PROVIDER;
    }

    @Override
    public CommitResult commit(MegaMindRequest<DialogovoState> request, Context context,
                               ReadNewsApplyArguments applyArguments) {
        return newsCommitService.commit(request, applyArguments.getSkillId(), getMetricaEventName(),
                applyArguments);
    }

    protected String getMetricaEventName() {
        return "news_read";
    }

    @Override
    public Class<ReadNewsApplyArguments> getApplyArgsType() {
        return ReadNewsApplyArguments.class;
    }

    protected BaseRunResponse createIrrelevantResult(MegaMindRequest<DialogovoState> request) {
        return DefaultIrrelevantResponse.create(Intents.EXTERNAL_SKILL_NEWS_IRRELEVANT, request.getRandom(),
                request.isVoiceSession());
    }

    private void writeAnalytics(Context context, NewsSkillInfo skillInfo, ActivationSourceType activationSourceType,
                                DialogovoState dialogovoState, String intent) {
        context.getAnalytics()
                .setIntent(intent)
                .addAction(AnalyticsInfoActionsFactory.createContinueAction(skillInfo))
                .addObject(new SessionAnalyticsInfoObject(
                        dialogovoState.getSessionO()
                                .map(Session::getSessionId)
                                .orElse(""),
                        activationSourceType));
    }
}
