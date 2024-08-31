package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor;


import java.util.List;
import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.CommitNeededResponse;
import ru.yandex.alice.kronstadt.core.CommitResult;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
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
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsSkillStateMaintainer;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;

public class BaseKidsNewsRunProcessor extends BaseSemanticFrameWithoutCurrentSkillRunRequestProcessor
        implements CommittingRunProcessor<DialogovoState, ReadNewsApplyArguments> {
    private static final Logger logger = LogManager.getLogger();

    private final NewsSkillProvider skillProvider;
    private final NewsContentsResolver newsContentsResolver;
    private final NewsFeedResolver newsFeedResolver;
    private final NewsSkillStateMaintainer newsSkillStateMaintainer;
    private final NewsCommitService newsCommitService;
    private final NewsFlashBriefingScenarioResponseFactory scenarioResponseFactory;
    private final AppMetricaEventSender appMetricaEventSender;

    @SuppressWarnings("ParameterNumber")
    protected BaseKidsNewsRunProcessor(
            NewsSkillProvider skillProvider,
            NewsContentsResolver newsContentsResolver,
            NewsFeedResolver newsFeedResolver,
            NewsSkillStateMaintainer newsSkillStateMaintainer,
            NewsCommitService newsCommitService,
            NewsFlashBriefingScenarioResponseFactory scenarioResponseFactory,
            String semanticFrameType,
            RunRequestProcessorType runRequestProcessorType,
            AppMetricaEventSender appMetricaEventSender) {
        super(semanticFrameType, runRequestProcessorType);
        this.skillProvider = skillProvider;
        this.newsContentsResolver = newsContentsResolver;
        this.newsFeedResolver = newsFeedResolver;
        this.newsSkillStateMaintainer = newsSkillStateMaintainer;
        this.newsCommitService = newsCommitService;
        this.scenarioResponseFactory = scenarioResponseFactory;
        this.appMetricaEventSender = appMetricaEventSender;
    }

    @Override
    protected BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request,
                                      SemanticFrame semanticFrame) {
        logger.info("Process kids news activations request with semanticFrameType={}", getSemanticFrame());

        String kidsNewsProviderSkillSlug = NewsConstants.KIDS_NEWS_PROVIDER_SKILL_SLUG;
        Optional<NewsSkillInfo> skillO = skillProvider.getSkillBySlug(kidsNewsProviderSkillSlug);
        if (skillO.isEmpty()) {
            logger.info("Unable to detect kids news skill by slug=[{}]", kidsNewsProviderSkillSlug);
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

        Optional<NewsFeed> newsFeedO = newsFeedResolver.findDefaultFeed(skillInfo);
        if (newsFeedO.isEmpty()) {
            logger.warn("Unable to detect news feed by skill [{}]", skillInfo.getId());
        }

        if (!HasFreshContentProviderPredicate.INSTANCE.test(request.getServerTime(), skillInfo)
                || newsFeedO.isEmpty()) {
            writeAnalytics(context, activationSourceType, stateWithoutAppmetrica);

            if (newsFeedO.isEmpty()) {
                logger.info("Unable to activate news skill name=[{}] cause feed not found:" +
                        " return irrelevant", skillInfo.getName());
            } else {
                logger.info("Unable to activate news skill name=[{}] cause stalled content:" +
                        " return irrelevant", skillInfo.getName());
            }
            logger.info("Unable to recommend another news skill, give up - return irrelevant");
            return createIrrelevantResult(request);
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

        // do not suggest any other provider content on kids request
        Optional<NewsSkillInfo> nextProviderPostroll = Optional.empty();
        List<NewsSkillInfo> providersSuggest = List.of();

        if (newsContentO.isEmpty()) {
            writeAnalytics(context, activationSourceType, stateWithoutAppmetrica);
            return new RunOnlyResponse<>(
                    scenarioResponseFactory.onNoNewFreshContentPerFeed(
                            stateWithoutAppmetrica, context, request.getRandom(), newsFeed, skillInfo,
                            nextProviderPostroll,
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

            context.getAnalytics().addAction(AnalyticsInfoActionsFactory.createActivateAction(skillInfo));
            context.getAnalytics().addObject(new NewsProviderAnalyticsInfoObject(
                    skillInfo.getId(),
                    skillInfo.getName(),
                    newsFeed.getId()));
            NewsContent newsContent = newsContentO.get();
            return new CommitNeededResponse<>(
                    scenarioResponseFactory.onReadContent(
                            dialogovoState, context, nextProviderPostroll,
                            new NewsArticle(
                                    skillInfo, newsFeed, newsContent, true,
                                    newsContentsResolver.findNextOneFrom(topNewsByFeed, newsContent.getId())
                                            .isPresent()), request, providersSuggest, false, false),
                    new ReadNewsApplyArguments(skillInfo.getId(), appmetricaArgs)
            );
        }
    }

    private void writeAnalytics(Context context, ActivationSourceType activationSourceType,
                                DialogovoState dialogovoState) {
        context.getAnalytics()
                .setIntent(Intents.EXTERNAL_SKILL_ACTIVATE_KIDS_NEWS)
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
