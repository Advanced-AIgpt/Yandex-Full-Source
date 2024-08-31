package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor;


import java.util.List;
import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.CommitNeededResponse;
import ru.yandex.alice.kronstadt.core.CommitResult;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
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
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsContent;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.ScenarioResponseResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsCommitService;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsContentsResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsFeedResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsSkillStateMaintainer;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;

@Component
public class ActivateFactOfTheDayRunProcessor extends BaseSemanticFrameWithoutCurrentSkillRunRequestProcessor
        implements CommittingRunProcessor<DialogovoState, ReadNewsApplyArguments> {
    private static final Logger logger = LogManager.getLogger();

    private final NewsSkillProvider skillProvider;
    private final NewsContentsResolver newsContentsResolver;
    private final NewsFeedResolver newsFeedResolver;
    private final NewsSkillStateMaintainer newsSkillStateMaintainer;
    private final NewsCommitService newsCommitService;
    private final ScenarioResponseResolver scenarioResponseResolver;
    private final AppMetricaEventSender appMetricaEventSender;
    private final String yandexQFactSkillId;

    @SuppressWarnings("ParameterNumber")
    protected ActivateFactOfTheDayRunProcessor(
            NewsSkillProvider skillProvider,
            NewsContentsResolver newsContentsResolver,
            NewsFeedResolver newsFeedResolver,
            NewsSkillStateMaintainer newsSkillStateMaintainer,
            NewsCommitService newsCommitService,
            ScenarioResponseResolver scenarioResponseResolver,
            AppMetricaEventSender appMetricaEventSender,
            @Value("${yandexQFactSkillId}") String yandexQFactSkillId
    ) {
        super(SemanticFrames.ALICE_EXTERNAL_FLASH_BRIEFING_FACTS, RunRequestProcessorType.EXTERNAL_SKILL_NEWS_FACTS);
        this.skillProvider = skillProvider;
        this.newsContentsResolver = newsContentsResolver;
        this.newsFeedResolver = newsFeedResolver;
        this.newsSkillStateMaintainer = newsSkillStateMaintainer;
        this.newsCommitService = newsCommitService;
        this.scenarioResponseResolver = scenarioResponseResolver;
        this.appMetricaEventSender = appMetricaEventSender;
        this.yandexQFactSkillId = yandexQFactSkillId;
    }

    @Override
    protected boolean canProcessAdditionally(MegaMindRequest<DialogovoState> request, SemanticFrame frame) {
        return hasExperiment(Experiments.FACT_OF_THE_DAY)
                .and(IS_SMART_SPEAKER_OR_TV)
                .test(request);
    }

    @Override
    protected BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request,
                                      SemanticFrame semanticFrame) {
        logger.info("Process news activations request with semanticFrameType={}",
                SemanticFrames.ALICE_EXTERNAL_FLASH_BRIEFING_FACTS);

        Optional<NewsSkillInfo> skillO = skillProvider.getSkill(yandexQFactSkillId);
        if (skillO.isEmpty()) {
            logger.info("Unable to detect fact skill");
            return createIrrelevantResult(request);
        }
        NewsSkillInfo skillInfo = skillO.get();

        Optional<NewsFeed> newsFeedO = newsFeedResolver.findDefaultFeed(skillInfo);
        if (newsFeedO.isEmpty()) {
            logger.warn("Unable to detect fact skill feed by skill [{}]", skillInfo.getId());
            return createIrrelevantResult(request);
        }

        NewsFeed newsFeed = newsFeedO.get();
        logger.info("Prepared to tell facts from skill [{}] default feed [{}] name [{}] topic [{}]",
                skillInfo.getId(), newsFeed.getId(), newsFeed.getName(), newsFeed.getTopic());

        List<NewsContent> topNewsByFeed = newsFeed.getTopContents();
        DialogovoState.NewsState currNewsState = newsSkillStateMaintainer.getNewsState(request);

        Optional<String> lasNewsReadBySkillFeedO = currNewsState.getLastNewsReadBySkillFeed(skillInfo.getId(),
                newsFeed.getId());
        logger.info("Last facts read by skill feed [{}]", lasNewsReadBySkillFeedO);
        Optional<NewsContent> newsContentO = newsContentsResolver.findNextOne(
                topNewsByFeed,
                lasNewsReadBySkillFeedO,
                newsFeed.getDepth());
        logger.info("Resolved facts content id: [{}]", newsContentO.map(NewsContent::getId));
        logger.debug("Resolved facts content source [{}]", newsContentO);

        newsSkillStateMaintainer.updateNewsStateWithContent(skillInfo, newsFeed, currNewsState, newsContentO);

        ActivationSourceType activationSourceType =
                newsSkillStateMaintainer.getActivationSourceTypeOnActivate(semanticFrame, request);

        DialogovoState stateWithoutAppmetrica = newsSkillStateMaintainer.generateDialogovoState(
                request.getStateO(),
                currNewsState,
                activationSourceType,
                request.getServerTime());

        if (newsContentO.isEmpty()) {
            writeAnalytics(context, skillInfo, activationSourceType, stateWithoutAppmetrica);
            return new RunOnlyResponse<>(
                    scenarioResponseResolver.get(skillInfo.getFlashBriefingType())
                            .onEmptyContentPerFeed(stateWithoutAppmetrica, context, request.getRandom(), newsFeed,
                                    skillInfo,
                                    request.getClientInfo(), List.of())
            );
        }

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
        writeAnalytics(context, skillInfo, activationSourceType, dialogovoState);

        NewsContent newsContent = newsContentO.get();
        return new CommitNeededResponse<>(
                scenarioResponseResolver.get(skillInfo.getFlashBriefingType())
                        .onReadContent(dialogovoState, context, Optional.empty(), new NewsArticle(
                                skillInfo, newsFeed, newsContent, true,
                                newsContentsResolver.findNextOneFrom(topNewsByFeed, newsContent.getId())
                                        .isPresent()), request, List.of()),
                new ReadNewsApplyArguments(skillInfo.getId(), appmetricaArgs)
        );
    }

    private void writeAnalytics(Context context, NewsSkillInfo skillInfo, ActivationSourceType activationSourceType,
                                DialogovoState dialogovoState) {
        context.getAnalytics()
                .setIntent(Intents.EXTERNAL_SKILL_ACTIVATE_NEWS_FROM_PROVIDER)
                .addAction(AnalyticsInfoActionsFactory.createActivateAction(skillInfo))
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
        return newsCommitService.commit(request, applyArguments.getSkillId(), "facts_read",
                applyArguments);
    }
}
