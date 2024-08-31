package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor;

import java.util.List;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.scenarios.BaseSemanticFrameWithoutCurrentSkillRunRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SessionAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.NewsFlashBriefingScenarioResponseFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsProviderSuggestService;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsSkillStateMaintainer;

@Component
public class RadionewsOnboardingRunProcessor extends BaseSemanticFrameWithoutCurrentSkillRunRequestProcessor {
    private static final Logger logger = LogManager.getLogger();
    private static final int SUGGEST_PROVIDERS_NUM = 3;

    private final NewsSkillStateMaintainer newsSkillStateMaintainer;
    private final NewsProviderSuggestService newsProviderSuggestService;
    private final NewsFlashBriefingScenarioResponseFactory scenarioResponseFactory;

    public RadionewsOnboardingRunProcessor(NewsSkillStateMaintainer newsSkillStateMaintainer,
                                           NewsProviderSuggestService newsProviderSuggestService,
                                           NewsFlashBriefingScenarioResponseFactory scenarioResponseFactory) {
        super(SemanticFrames.ALICE_EXTERNAL_SKILL_RADIONEWS_ONBOARDING,
                RunRequestProcessorType.EXTERNAL_SKILL_RADIONEWS_ONBOARDING);
        this.newsSkillStateMaintainer = newsSkillStateMaintainer;
        this.newsProviderSuggestService = newsProviderSuggestService;
        this.scenarioResponseFactory = scenarioResponseFactory;
    }

    @Override
    protected boolean canProcessAdditionally(MegaMindRequest<DialogovoState> request, SemanticFrame frame) {
        return RadionewsPredicates.COUNTRY
                .and(RadionewsPredicates.SURFACE)
                .test(request);
    }

    @Override
    protected BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request,
                                      SemanticFrame semanticFrame) {
        logger.info("Process radionews onboarding request with semanticFrameType={}",
                SemanticFrames.ALICE_EXTERNAL_SKILL_RADIONEWS_ONBOARDING);

        DialogovoState.NewsState currNewsState = newsSkillStateMaintainer.getNewsState(request);
        logger.debug("Current news state: [{}]", currNewsState);

        ActivationSourceType activationSourceType =
                newsSkillStateMaintainer.getActivationSourceTypeOnActivate(semanticFrame, request);

        DialogovoState dialogovoState = newsSkillStateMaintainer.generateDialogovoState(
                request.getStateO(),
                currNewsState,
                activationSourceType,
                request.getServerTime());

        List<NewsSkillInfo> suggestedProviders = newsProviderSuggestService.suggest(currNewsState, request.getRandom(),
                request.getServerTime(),
                SUGGEST_PROVIDERS_NUM);

        if (suggestedProviders.isEmpty()) {
            logger.info("Cant offer any news - give up with irrelevant");
            return DefaultIrrelevantResponse.create(Intents.EXTERNAL_SKILL_NEWS_IRRELEVANT, request.getRandom(),
                    request.isVoiceSession());
        }

        context.getAnalytics()
                .setIntent(Intents.EXTERNAL_SKILL_RADIONEWS_ONBOARDING)
                .addObject(new SessionAnalyticsInfoObject(
                        dialogovoState.getSessionO()
                                .map(Session::getSessionId)
                                .orElse(""),
                        activationSourceType));

        return new RunOnlyResponse<>(scenarioResponseFactory.onRadionewsOnboardingSuggest(
                dialogovoState, context, request.getRandom(), suggestedProviders, request.getClientInfo()));

    }
}
