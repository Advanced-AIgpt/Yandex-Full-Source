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

@Component
public class ActivateKidsNewsRunProcessor extends BaseKidsNewsRunProcessor {

    /**
     * Активация по "расскажи детские новости"
     * Режим (детский, семейный) - не важен
     */
    @SuppressWarnings("ParameterNumber")
    protected ActivateKidsNewsRunProcessor(
            NewsSkillProvider skillProvider,
            NewsContentsResolver newsContentsResolver,
            NewsFeedResolver newsFeedResolver,
            NewsSkillStateMaintainer newsSkillStateMaintainer,
            NewsCommitService newsCommitService,
            NewsFlashBriefingScenarioResponseFactory scenarioResponseFactory,
            AppMetricaEventSender appMetricaEventSender) {
        super(skillProvider, newsContentsResolver, newsFeedResolver, newsSkillStateMaintainer, newsCommitService,
                scenarioResponseFactory, SemanticFrames.ALICE_EXTERNAL_SKILL_KIDS_NEWS_ACTIVATE,
                RunRequestProcessorType.EXTERNAL_SKILL_KIDS_NEWS_ACTIVATE, appMetricaEventSender);
    }

    @Override
    protected boolean canProcessAdditionally(MegaMindRequest<DialogovoState> request, SemanticFrame frame) {
        return RadionewsPredicates.COUNTRY
                .and(RadionewsPredicates.SURFACE)
                .test(request);
    }
}
