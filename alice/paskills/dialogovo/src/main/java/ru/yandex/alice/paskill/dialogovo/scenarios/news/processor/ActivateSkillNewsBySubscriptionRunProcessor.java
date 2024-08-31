package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor;


import java.util.List;
import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.CommitNeededResponse;
import ru.yandex.alice.kronstadt.core.CommitResult;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.CommittingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.scenarios.BaseSemanticFrameWithoutCurrentSkillRunRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SessionAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.analytics.AnalyticsInfoActionsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.analytics.NewsProviderAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.FlashBriefingType;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsContent;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.NewsFlashBriefingScenarioResponseFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.HasFreshContentProviderPredicate;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsCommitService;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsContentsResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsFeedResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsProviderSuggestService;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsSkillStateMaintainer;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;
import ru.yandex.alice.paskill.dialogovo.service.memento.MementoService;
import ru.yandex.alice.paskill.dialogovo.service.memento.NewsProviderSubscription;

import static java.util.function.Predicate.not;

@Component
public class ActivateSkillNewsBySubscriptionRunProcessor extends BaseSemanticFrameWithoutCurrentSkillRunRequestProcessor
        implements CommittingRunProcessor<DialogovoState, ReadNewsApplyArguments> {
    private static final Logger logger = LogManager.getLogger();
    private static final int MAX_SUGGEST_PROVIDERS = 4;

    private final NewsSkillProvider skillProvider;
    private final NewsContentsResolver newsContentsResolver;
    private final NewsFeedResolver newsFeedResolver;
    private final NewsSkillStateMaintainer newsSkillStateMaintainer;
    private final NewsCommitService newsCommitService;
    private final NewsFlashBriefingScenarioResponseFactory scenarioResponseFactory;
    private final NewsProviderSuggestService newsProviderSuggestService;
    private final MementoService mementoService;
    private final RequestContext requestContext;
    private final AppMetricaEventSender appMetricaEventSender;

    @SuppressWarnings("ParameterNumber")
    protected ActivateSkillNewsBySubscriptionRunProcessor(
            NewsSkillProvider skillProvider,
            NewsContentsResolver newsContentsResolver,
            NewsFeedResolver newsFeedResolver,
            NewsSkillStateMaintainer newsSkillStateMaintainer,
            NewsCommitService newsCommitService,
            NewsFlashBriefingScenarioResponseFactory scenarioResponseFactory,
            NewsProviderSuggestService newsProviderSuggestService,
            MementoService mementoService,
            RequestContext requestContext,
            AppMetricaEventSender appMetricaEventSender) {
        super(SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_GET_NEWS,
                RunRequestProcessorType.EXTERNAL_SKILL_NEWS_ACTIVATE_BY_SUBSCRIPTION);
        this.skillProvider = skillProvider;
        this.newsContentsResolver = newsContentsResolver;
        this.newsFeedResolver = newsFeedResolver;
        this.newsSkillStateMaintainer = newsSkillStateMaintainer;
        this.newsCommitService = newsCommitService;
        this.scenarioResponseFactory = scenarioResponseFactory;
        this.newsProviderSuggestService = newsProviderSuggestService;
        this.mementoService = mementoService;
        this.requestContext = requestContext;
        this.appMetricaEventSender = appMetricaEventSender;
    }

    @Override
    protected boolean canProcessAdditionally(MegaMindRequest<DialogovoState> request, SemanticFrame frame) {

        // activate by subscription only on wide news intent - without slots
        return not(hasAnyValueSlot(frame))
                // can check subscription only for authorized user
                .and(empty -> Optional.ofNullable(requestContext.getCurrentUserTicket()).isPresent())
                .and(IS_SMART_SPEAKER_OR_TV
                        .or(empty -> request.getClientInfo().isSearchApp())
                        .or(empty -> request.getClientInfo().isNavigatorOrMaps()))
                .test(request);
    }

    @Override
    protected BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request,
                                      SemanticFrame semanticFrame) {
        logger.info("Process news activations request with semanticFrameType={}",
                SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_GET_NEWS);

        // non-nulliness provided by canProcessAdditionally
        String currentUserTicket = requestContext.getCurrentUserTicket();
        logger.info("Check subscription for user [{}]", requestContext.getCurrentUserId());
        Optional<NewsProviderSubscription> userNewsProviderSubscriptionO =
                mementoService.getUserNewsProviderSubscription(currentUserTicket);

        if (userNewsProviderSubscriptionO.isEmpty()) {
            // can return irrelevant here - cause new semantic-frame PERSONAL_ASSISTANT_SCENARIOS_GET_NEWS relevant
            // for us only on subscription
            logger.info("Subscription not found for user [{}]. Return with irrelevant",
                    requestContext.getCurrentUserId());
            return createIrrelevantResult(request);
        }

        NewsProviderSubscription newsProviderSubscription = userNewsProviderSubscriptionO.get();
        logger.info("Found subscription [{}] for user [{}]. Checking provider origin",
                newsProviderSubscription, requestContext.getCurrentUserId());

        String newsProviderSlug = newsProviderSubscription.getNewsProviderSlug();
        Optional<NewsSkillInfo> skillO = detectSkillBySlug(newsProviderSlug);
        if (skillO.isEmpty()) {
            logger.info("Unable to detect news skill by slug=[{}]. Return with irrelevant", newsProviderSlug);
            return createIrrelevantResult(request);
        }

        NewsSkillInfo skillInfo = skillO.get();

        DialogovoState.NewsState currNewsState = newsSkillStateMaintainer.getNewsState(request);
        logger.debug("Current news state: [{}]", currNewsState);

        ActivationSourceType activationSourceType = ActivationSourceType.NEWS_PROVIDER_SUBSCRIPTION;

        DialogovoState stateWithoutAppmetrica = newsSkillStateMaintainer.generateDialogovoState(
                request.getStateO(),
                currNewsState,
                activationSourceType,
                request.getServerTime());

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

            return new RunOnlyResponse<>(scenarioResponseFactory.onActivatingFailedRecommendNext(stateWithoutAppmetrica,
                    context, request.getRandom(), anotherRecommendedO.get(), request.getClientInfo()));
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
                            nextProviderPostrollO,
                            request.getClientInfo(), providersSuggest)
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

            NewsContent newsContent = newsContentO.get();
            context.getAnalytics().addAction(AnalyticsInfoActionsFactory.createActivateAction(skillInfo));
            context.getAnalytics().addObject(new NewsProviderAnalyticsInfoObject(
                    skillInfo.getId(),
                    skillInfo.getName(),
                    newsFeed.getId()));
            return new CommitNeededResponse<>(
                    scenarioResponseFactory.onReadContent(
                            dialogovoState, context, nextProviderPostrollO,
                            new NewsArticle(
                                    skillInfo, newsFeed, newsContent, true,
                                    newsContentsResolver.findNextOneFrom(topNewsByFeed, newsContent.getId())
                                            .isPresent(), true),
                            request, providersSuggest),
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
                .setIntent(Intents.EXTERNAL_SKILL_ACTIVATE_NEWS_BY_SUBSCRIPTION)
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

    private Optional<NewsSkillInfo> detectSkillBySlug(String slug) {
        return skillProvider.getSkillBySlug(slug)
                .filter(skill -> skill.getFlashBriefingType() == FlashBriefingType.RADIONEWS);
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
}
