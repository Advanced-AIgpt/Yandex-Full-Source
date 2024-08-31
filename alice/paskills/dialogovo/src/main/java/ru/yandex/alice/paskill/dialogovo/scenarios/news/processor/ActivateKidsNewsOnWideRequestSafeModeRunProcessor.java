package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor;


import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.NewsFlashBriefingScenarioResponseFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsCommitService;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsContentsResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsFeedResolver;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsSkillStateMaintainer;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;

import static java.util.function.Predicate.not;

@Component
public class ActivateKidsNewsOnWideRequestSafeModeRunProcessor extends BaseKidsNewsRunProcessor {

    /**
     * Активация по "расскажи новости" в детском режиме
     */
    @SuppressWarnings("ParameterNumber")
    protected ActivateKidsNewsOnWideRequestSafeModeRunProcessor(
            NewsSkillProvider skillProvider,
            NewsContentsResolver newsContentsResolver,
            NewsFeedResolver newsFeedResolver,
            NewsSkillStateMaintainer newsSkillStateMaintainer,
            NewsCommitService newsCommitService,
            NewsFlashBriefingScenarioResponseFactory scenarioResponseFactory,
            AppMetricaEventSender appMetricaEventSender) {
        super(skillProvider, newsContentsResolver, newsFeedResolver, newsSkillStateMaintainer, newsCommitService,
                scenarioResponseFactory,
                SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_GET_NEWS,
                RunRequestProcessorType.EXTERNAL_SKILL_KIDS_NEWS_ON_WIDE_REQUEST_SAFE_MODE_ACTIVATE,
                appMetricaEventSender);
    }

    @Override
    protected boolean canProcessAdditionally(MegaMindRequest<DialogovoState> request, SemanticFrame frame) {
        // детский режим
        return request.isChildMode() &&
                RadionewsPredicates.COUNTRY.test(request) &&
                // нет слотов(место, время и т.д.) в грамматике get_news
                not(hasAnyValueSlot(frame)
                        .and(RadionewsPredicates.SURFACE))
                        .test(request);
    }
}
