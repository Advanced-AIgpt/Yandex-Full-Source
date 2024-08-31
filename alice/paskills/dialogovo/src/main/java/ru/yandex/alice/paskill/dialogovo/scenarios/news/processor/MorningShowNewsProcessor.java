package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor;


import java.util.List;
import java.util.Optional;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.Data;
import lombok.NoArgsConstructor;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.CommitNeededResponse;
import ru.yandex.alice.kronstadt.core.CommitResult;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
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
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.NewsFlashBriefingScenarioResponseFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsCommitService;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsContentsResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsFeedResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsSkillStateMaintainer;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;

import static java.util.Collections.emptyList;

@Component
public class MorningShowNewsProcessor implements RunRequestProcessor<DialogovoState>,
        CommittingRunProcessor<DialogovoState, ReadNewsApplyArguments> {
    private static final Logger logger = LogManager.getLogger();

    private final NewsSkillProvider skillProvider;
    private final NewsContentsResolver newsContentsResolver;
    private final NewsFeedResolver newsFeedResolver;
    private final NewsSkillStateMaintainer newsSkillStateMaintainer;
    private final NewsCommitService newsCommitService;
    private final NewsFlashBriefingScenarioResponseFactory scenarioResponseFactory;
    private final ObjectMapper mapper;
    private final AppMetricaEventSender appMetricaEventSender;

    @SuppressWarnings("ParameterNumber")
    protected MorningShowNewsProcessor(
            NewsSkillProvider skillProvider,
            NewsContentsResolver newsContentsResolver,
            NewsFeedResolver newsFeedResolver,
            NewsSkillStateMaintainer newsSkillStateMaintainer,
            NewsCommitService newsCommitService,
            NewsFlashBriefingScenarioResponseFactory scenarioResponseFactory,
            ObjectMapper mapper,
            AppMetricaEventSender appMetricaEventSender) {
        this.skillProvider = skillProvider;
        this.newsContentsResolver = newsContentsResolver;
        this.newsFeedResolver = newsFeedResolver;
        this.newsSkillStateMaintainer = newsSkillStateMaintainer;
        this.newsCommitService = newsCommitService;
        this.scenarioResponseFactory = scenarioResponseFactory;
        this.mapper = mapper;
        this.appMetricaEventSender = appMetricaEventSender;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return RadionewsPredicates.COUNTRY
                .and(RadionewsPredicates.SURFACE)
                .and(req -> req.getRequestSource() == MegaMindRequest.RequestSource.GET_NEXT)
                .and(
                        //  general get_news with slot or flash_briefing's activate
                        hasFrame(SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_GET_NEWS)
                                .and(hasSlotEntityType(
                                        SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_GET_NEWS,
                                        SemanticSlotType.PROVIDER.getValue(),
                                        SemanticSlotEntityType.NEWS_PROVIDER))
                )
                .test(request);
    }

    @Override
    public ProcessorType getType() {
        return RunRequestProcessorType.MORNING_SHOW_NEWS;
    }

    @Override
    @SuppressWarnings("MethodLength")
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        // Presence of one of sf declared in canProcess
        var semanticFrame = request.getSemanticFrameO(SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_GET_NEWS).get();
        logger.info("Process news activations request with semanticFrame=[{}]", semanticFrame);

        Optional<NewsSkillInfo> skillO = semanticFrame
                .getTypedEntityValueO(SemanticSlotType.PROVIDER.getValue(), SemanticSlotEntityType.NEWS_PROVIDER)
                .flatMap(this::getNewsProviderSlotValueO)
                .map(newsProvider -> newsProvider.newsSource)
                .flatMap(skillProvider::getSkillBySlug);

        if (skillO.isEmpty()) {
            logger.info("Unable to detect news skill by probider=[{}]", semanticFrame
                    .getTypedEntityValueO(SemanticSlotType.PROVIDER.getValue(), SemanticSlotEntityType.NEWS_PROVIDER));
            return createIrrelevantResult(request);
        }

        NewsSkillInfo skillInfo = skillO.get();

        DialogovoState.NewsState currNewsState = newsSkillStateMaintainer.getNewsState(request);
        logger.debug("Current news state: [{}]", currNewsState);

        ActivationSourceType activationSourceType = ActivationSourceType.SHOW;

        Optional<NewsFeed> newsFeedO = newsFeedResolver.findDefaultFeed(skillInfo);
        if (newsFeedO.isEmpty()) {
            logger.warn("Unable to detect news feed by skill [{}]", skillInfo.getId());
            return createIrrelevantResult(request);
        }


        NewsFeed newsFeed = newsFeedO.get();
        logger.info("Prepared to tell news from skill [{}] default feed [{}] name [{}] topic [{}]",
                skillInfo.getId(), newsFeed.getId(), newsFeed.getName(), newsFeed.getTopic());

        List<NewsContent> topNewsByFeed = newsFeed.getTopContents();


        Optional<NewsContent> newsContentO =
                newsContentsResolver.findNextOne(topNewsByFeed, Optional.empty(), newsFeed.getDepth());

        logger.info("Resolved news content id: [{}]", newsContentO.map(NewsContent::getId));
        logger.debug("Resolved news content source [{}]", newsContentO);

        newsSkillStateMaintainer.updateNewsStateWithContent(skillInfo, newsFeed, currNewsState, newsContentO);
        newsSkillStateMaintainer.resetLastPostrolledTimeAfterTtl(currNewsState, request.getServerTime());
        logger.debug("News state in activation after reset and content update = [{}]", currNewsState);

        DialogovoState stateWithoutAppmetrica = newsSkillStateMaintainer.generateDialogovoState(
                request.getStateO(),
                currNewsState,
                activationSourceType,
                request.getServerTime());

        if (newsContentO.isEmpty()) {
            writeAnalytics(context, activationSourceType, stateWithoutAppmetrica);

            return createIrrelevantResult(request);
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
                    skillInfo.hasFeatureFlag(SkillFeatureFlag.APPMETRICA_EVENTS_TIMESTAMP_COUNTER));

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
            var body = scenarioResponseFactory.onReadContent(
                    dialogovoState, context, Optional.empty(),
                    new NewsArticle(skillInfo, newsFeed, newsContent, true, false),
                    request, emptyList(), false, false, true);
            return new CommitNeededResponse<>(
                    body,
                    new ReadNewsApplyArguments(skillInfo.getId(), appmetricaArgs)
            );
        }
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


    @Override
    public Class<ReadNewsApplyArguments> getApplyArgsType() {
        return ReadNewsApplyArguments.class;
    }

    @Override
    public CommitResult commit(MegaMindRequest<DialogovoState> request, Context context,
                               ReadNewsApplyArguments applyArguments) {
        return newsCommitService.commit(request, applyArguments.getSkillId(), "news_read", applyArguments);
    }

    private Optional<NewsProvider> getNewsProviderSlotValueO(String newsProviderSlotValue) {
        try {
            NewsProvider newsProviderValue = mapper.readValue(newsProviderSlotValue,
                    MorningShowNewsProcessor.NewsProvider.class);
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
