package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor;


import java.util.List;
import java.util.Optional;
import java.util.Set;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.Data;
import lombok.NoArgsConstructor;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.CommitNeededResponse;
import ru.yandex.alice.kronstadt.core.CommitResult;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.CommittingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ProcessorType;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticSlotEntityType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SessionAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.analytics.AnalyticsInfoActionsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.analytics.NewsProviderAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsContent;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.NewsConstants;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.NewsFlashBriefingScenarioResponseFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.HasFreshContentProviderPredicate;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsCommitService;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsContentsResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsFeedResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsProviderSuggestService;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsSkillStateMaintainer;
import ru.yandex.alice.paskill.dialogovo.service.SkillDetector;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;
import ru.yandex.alice.paskill.dialogovo.service.memento.MementoService;
import ru.yandex.alice.paskill.dialogovo.service.memento.NewsProviderSubscription;

import static java.util.function.Predicate.not;

@Component
public class ActivateSkillNewsFromProviderRunProcessor implements RunRequestProcessor<DialogovoState>,
        CommittingRunProcessor<DialogovoState, ReadNewsApplyArguments> {
    private static final Logger logger = LogManager.getLogger();

    private static final int MAX_SUGGEST_PROVIDERS = 4;

    private final SkillDetector skillDetector;
    private final NewsSkillProvider skillProvider;
    private final NewsContentsResolver newsContentsResolver;
    private final NewsFeedResolver newsFeedResolver;
    private final NewsSkillStateMaintainer newsSkillStateMaintainer;
    private final NewsCommitService newsCommitService;
    private final NewsFlashBriefingScenarioResponseFactory scenarioResponseFactory;
    private final NewsProviderSuggestService newsProviderSuggestService;
    private final MementoService mementoService;
    private final RequestContext requestContext;
    private final ObjectMapper mapper;
    private final AppMetricaEventSender appMetricaEventSender;

    @SuppressWarnings("ParameterNumber")
    protected ActivateSkillNewsFromProviderRunProcessor(
            @Qualifier("newsSkillDetector") SkillDetector skillDetector,
            NewsSkillProvider skillProvider,
            NewsContentsResolver newsContentsResolver,
            NewsFeedResolver newsFeedResolver,
            NewsSkillStateMaintainer newsSkillStateMaintainer,
            NewsCommitService newsCommitService,
            NewsFlashBriefingScenarioResponseFactory scenarioResponseFactory,
            NewsProviderSuggestService newsProviderSuggestService,
            MementoService mementoService,
            RequestContext requestContext,
            ObjectMapper mapper,
            AppMetricaEventSender appMetricaEventSender) {
        this.skillDetector = skillDetector;
        this.skillProvider = skillProvider;
        this.newsContentsResolver = newsContentsResolver;
        this.newsFeedResolver = newsFeedResolver;
        this.newsSkillStateMaintainer = newsSkillStateMaintainer;
        this.newsCommitService = newsCommitService;
        this.scenarioResponseFactory = scenarioResponseFactory;
        this.newsProviderSuggestService = newsProviderSuggestService;
        this.mementoService = mementoService;
        this.requestContext = requestContext;
        this.mapper = mapper;
        this.appMetricaEventSender = appMetricaEventSender;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return not(IS_IN_SKILL)
                .and(RadionewsPredicates.COUNTRY)
                .and(RadionewsPredicates.SURFACE)
                .and(
                        //  general get_news with slot or flash_briefing's activate
                        hasFrame(SemanticFrames.ALICE_EXTERNAL_FLASH_BRIEFING_ACTIVATE).or(
                                hasFrame(SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_GET_NEWS)
                                        .and(hasSlotEntityType(
                                                SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_GET_NEWS,
                                                SemanticSlotType.PROVIDER.getValue(),
                                                SemanticSlotEntityType.NEWS_PROVIDER))
                        ))
                .test(request);
    }

    @Override
    public ProcessorType getType() {
        return RunRequestProcessorType.EXTERNAL_SKILL_NEWS_ACTIVATE;
    }

    @Override
    @SuppressWarnings("MethodLength")
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        // Presence of one of sf declared in canProcess
        var semanticFrame = request.getAnySemanticFrameO(
                Set.of(
                        SemanticFrames.ALICE_EXTERNAL_FLASH_BRIEFING_ACTIVATE,
                        SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_GET_NEWS)
        ).get();
        logger.info("Process news activations request with semanticFrame=[{}]",
                semanticFrame);

        Optional<NewsSkillInfo> skillO = Optional.empty();

        if (semanticFrame.is(SemanticFrames.ALICE_EXTERNAL_FLASH_BRIEFING_ACTIVATE)) {
            Optional<String> newsSourceSlugSlotValueO = semanticFrame.getTypedEntityValueO(
                    SemanticSlotType.NEWS_SOURCE_SLUG.getValue(), SemanticSlotEntityType.CUSTOM_NEWS_SOURCE);

            if (newsSourceSlugSlotValueO.isPresent()) {
                skillO = skillProvider.getSkillBySlug(newsSourceSlugSlotValueO.get());
            }

            if (skillO.isEmpty()) {
                logger.info("Radionews activation by slug not succeeded, try activate by text slot");
                skillO = detectSkillByInput(semanticFrame);
            }

        } else {
            // get_news
            Optional<String> newsSourceSlugSlotValueO = semanticFrame
                    .getTypedEntityValueO(SemanticSlotType.PROVIDER.getValue(), SemanticSlotEntityType.NEWS_PROVIDER)
                    .flatMap(this::getNewsProviderSlotValueO)
                    .map(newsProvider -> newsProvider.newsSource);

            if (newsSourceSlugSlotValueO.isPresent()) {
                skillO = skillProvider.getSkillBySlug(newsSourceSlugSlotValueO.get());
            }
        }

        if (skillO.isEmpty()) {
            logger.info("Unable to detect news skill by semanticFrame=[{}]", semanticFrame);
            return createIrrelevantResult(request);
        }

        NewsSkillInfo skillInfo = skillO.get();

        DialogovoState.NewsState currNewsState = newsSkillStateMaintainer.getNewsState(request);
        logger.debug("Current news state: [{}]", currNewsState);

        ActivationSourceType activationSourceType =
                newsSkillStateMaintainer.getActivationSourceTypeOnActivate(semanticFrame, request);

        DialogovoState stateWithoutAppmetrica = newsSkillStateMaintainer.generateDialogovoState(
                request.getStateO(),
                currNewsState,
                activationSourceType,
                request.getServerTime());

        if (request.isChildMode() && !skillInfo.getSlug().equals(NewsConstants.KIDS_NEWS_PROVIDER_SKILL_SLUG)) {
            writeAnalytics(context, activationSourceType, stateWithoutAppmetrica);
            logger.info("Start of adults news in safe mode detected. Return stub");
            return new RunOnlyResponse<>(scenarioResponseFactory.safeModeStub(stateWithoutAppmetrica, context,
                    request));
        }

        Optional<NewsFeed> newsFeedO = newsFeedResolver.findDefaultFeed(skillInfo);
        if (newsFeedO.isEmpty()) {
            logger.warn("Unable to detect news feed by skill [{}]", skillInfo.getId());
        }

        if (!HasFreshContentProviderPredicate.INSTANCE.test(request.getServerTime(), skillInfo)
                || newsFeedO.isEmpty()) {
            writeAnalytics(context, activationSourceType, stateWithoutAppmetrica);
            if (newsFeedO.isEmpty()) {
                logger.info("Unable to activate news skill name=[{}] cause feed not found:" +
                        " try to recommend another", skillInfo.getName());
            } else {
                logger.info("Unable to activate news skill name=[{}] cause stalled content:" +
                        " try to recommend another", skillInfo.getName());
            }
            Optional<NewsSkillInfo> anotherRecommendedO = newsProviderSuggestService.postrollAfter(skillInfo,
                    currNewsState, request.getRandom(), request.getServerTime(), false);

            if (anotherRecommendedO.isEmpty()) {
                logger.info("Unable to recommend another news skill, give up - return irrelevant");
                return createIrrelevantResult(request);
            }

            return new RunOnlyResponse<>(scenarioResponseFactory.onActivatingFailedRecommendNext(
                    stateWithoutAppmetrica, context, request.getRandom(), anotherRecommendedO.get(),
                    request.getClientInfo()));
        }

        NewsFeed newsFeed = newsFeedO.get();
        logger.info("Prepared to tell news from skill [{}] default feed [{}] name [{}] topic [{}]",
                skillInfo.getId(), newsFeed.getId(), newsFeed.getName(), newsFeed.getTopic());
        List<NewsContent> topNewsByFeed = newsFeed.getTopContents();

        Optional<String> lastNewsReadBySkillFeedO = currNewsState.getLastNewsReadBySkillFeed(skillInfo.getId(),
                newsFeed.getId());
        logger.info("Last news read by skill feed [{}]", lastNewsReadBySkillFeedO);

        Optional<NewsContent> newsContentO = newsContentsResolver.findNextOne(
                topNewsByFeed,
                lastNewsReadBySkillFeedO,
                newsFeed.getDepth());
        logger.info("Resolved news content id: [{}]", newsContentO.map(NewsContent::getId));
        logger.debug("Resolved news content source [{}]", newsContentO);

        // for outplatform tests
        if (newsContentO.isEmpty() && request.hasExperiment(Experiments.ALLOW_RADIONEWS_RESTART)) {
            logger.info("Forcing stale radionews by flag {}", Experiments.ALLOW_RADIONEWS_RESTART);
            newsContentO = newsContentsResolver.findNextOne(
                    topNewsByFeed,
                    Optional.empty(),
                    newsFeed.getDepth());
        }

        newsSkillStateMaintainer.updateNewsStateWithContent(skillInfo, newsFeed, currNewsState, newsContentO);
        newsSkillStateMaintainer.resetLastPostrolledTimeAfterTtl(currNewsState, request.getServerTime());
        logger.debug("News state in activation after reset and content update = [{}]", currNewsState);

        Optional<NewsSkillInfo> nextProviderPostrollO = resolveAfterNewsPostroll(
                request, skillInfo, currNewsState);

        List<NewsSkillInfo> providersSuggest = newsProviderSuggestService.suggest(currNewsState, request.getRandom(),
                request.getServerTime(), MAX_SUGGEST_PROVIDERS);

        if (newsContentO.isEmpty()) {
            writeAnalytics(context, activationSourceType, stateWithoutAppmetrica);
            return new RunOnlyResponse<>(
                    scenarioResponseFactory.onNoNewFreshContentPerFeed(
                            stateWithoutAppmetrica, context, request.getRandom(), newsFeed, skillInfo,
                            nextProviderPostrollO, request.getClientInfo(), providersSuggest)
            );
        } else {
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
            writeAnalytics(context, activationSourceType, dialogovoState);

            boolean allowSubscriptionSuggest = requestContext.getCurrentUserTicket() != null;
            Optional<NewsProviderSubscription> userNewsProviderSubscriptionO =
                    allowSubscriptionSuggest
                            ? mementoService.getUserNewsProviderSubscription(requestContext.getCurrentUserTicket())
                            : Optional.empty();
            boolean hasSubscription = allowSubscriptionSuggest
                    && userNewsProviderSubscriptionO.isPresent()
                    && !userNewsProviderSubscriptionO.get().isDefaultValue();

            context.getAnalytics().addAction(AnalyticsInfoActionsFactory.createActivateAction(skillInfo));
            context.getAnalytics().addObject(new NewsProviderAnalyticsInfoObject(
                    skillInfo.getId(),
                    skillInfo.getName(),
                    newsFeed.getId()));

            NewsContent newsContent = newsContentO.get();
            var body = scenarioResponseFactory.onReadContent(
                    dialogovoState, context, nextProviderPostrollO,
                    new NewsArticle(
                            skillInfo, newsFeed, newsContent, true,
                            newsContentsResolver.findNextOneFrom(topNewsByFeed, newsContent.getId())
                                    .isPresent()), request, providersSuggest, hasSubscription,
                    allowSubscriptionSuggest);
            return new CommitNeededResponse<>(
                    body,
                    new ReadNewsApplyArguments(skillInfo.getId(), appmetricaArgs)
            );
        }
    }

    private Optional<NewsSkillInfo> resolveAfterNewsPostroll(MegaMindRequest<DialogovoState> request,
                                                             NewsSkillInfo skillInfo,
                                                             DialogovoState.NewsState currNewsState) {
        Optional<NewsSkillInfo> nextProviderPostrollO = newsProviderSuggestService
                .postrollAfter(skillInfo, currNewsState, request.getRandom(), request.getServerTime());
        newsSkillStateMaintainer.updateNewsStateWithNextPostrollingProvider(currNewsState, nextProviderPostrollO,
                request.getServerTime());
        logger.debug("News state in activation after reset, content update, postroll = [{}]", currNewsState);
        return nextProviderPostrollO;
    }

    private void writeAnalytics(Context context, ActivationSourceType activationSourceType,
                                DialogovoState dialogovoState) {
        context.getAnalytics()
                .setIntent(Intents.EXTERNAL_SKILL_ACTIVATE_NEWS_FROM_PROVIDER)
                .addObject(new SessionAnalyticsInfoObject(
                        dialogovoState.getSessionO()
                                .map(Session::getSessionId)
                                .orElse(""),
                        activationSourceType));
    }

    private BaseRunResponse createIrrelevantResult(MegaMindRequest<DialogovoState> request) {
        return DefaultIrrelevantResponse.create(Intents.EXTERNAL_SKILL_NEWS_IRRELEVANT, request.getRandom(),
                request.isVoiceSession());
    }

    private Optional<NewsSkillInfo> detectSkillByInput(SemanticFrame frame) {
        return frame.getSlotValueO(SemanticSlotType.NEWS_PROVIDER.getValue())
                .flatMap(slotValue -> skillDetector.tryDetectSkill(slotValue, false)
                        .flatMap(detectedSkill -> skillProvider.getSkill(detectedSkill.getSkillId())));
    }

    @Override
    public Class<ReadNewsApplyArguments> getApplyArgsType() {
        return ReadNewsApplyArguments.class;
    }

    @Override
    public CommitResult commit(MegaMindRequest<DialogovoState> request, Context context,
                               ReadNewsApplyArguments applyArguments) {
        return newsCommitService.commit(request, applyArguments.getSkillId(), "news_read",
                applyArguments);
    }

    private Optional<NewsProvider> getNewsProviderSlotValueO(String newsProviderSlotValue) {
        try {
            NewsProvider newsProviderValue = mapper.readValue(newsProviderSlotValue,
                    ActivateSkillNewsFromProviderRunProcessor.NewsProvider.class);
            return Optional.of(newsProviderValue);
        } catch (Exception ex) {
            logger.error("Error while parsing news_provider type : [" + newsProviderSlotValue + "]", ex);
            return Optional.empty();
        }
    }

    @Data
    @NoArgsConstructor
    private static class NewsProvider {
        @JsonProperty("news_source")
        private String newsSource;
        @JsonProperty("rubric")
        private String rubric;
    }
}
