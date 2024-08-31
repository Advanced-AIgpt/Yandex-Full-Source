package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor;


import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SessionAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.analytics.AnalyticsInfoActionsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsSubscriptionDeclineDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsSkillStateMaintainer;

@Component
public class NewsSubscriptionDeclineCallbackProcessor implements RunRequestProcessor<DialogovoState> {
    private static final Logger logger = LogManager.getLogger();

    private final NewsSkillProvider skillProvider;
    private final NewsSkillStateMaintainer newsSkillStateMaintainer;
    private final Phrases phrases;

    public NewsSubscriptionDeclineCallbackProcessor(NewsSkillProvider skillProvider,
                                                    NewsSkillStateMaintainer newsSkillStateMaintainer,
                                                    Phrases phrases) {
        this.skillProvider = skillProvider;
        this.newsSkillStateMaintainer = newsSkillStateMaintainer;
        this.phrases = phrases;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return request.getInput().isCallback(NewsSubscriptionDeclineDirective.class);
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        NewsSubscriptionDeclineDirective payload =
                request.getInput().getDirective(NewsSubscriptionDeclineDirective.class);

        logger.info("Received subscription decline for skill by id [{}]", payload.getSkillId());

        Optional<NewsSkillInfo> skillO = skillProvider.getSkill(payload.getSkillId());
        if (skillO.isEmpty()) {
            logger.info("Unable to detect news skill by id [{}]", payload.getSkillId());
            return DefaultIrrelevantResponse.create(Intents.EXTERNAL_SKILL_NEWS_IRRELEVANT, request.getRandom(),
                    request.isVoiceSession());
        }

        NewsSkillInfo skillInfo = skillO.get();

        DialogovoState.NewsState currNewsState = newsSkillStateMaintainer.getNewsState(request);

        ActivationSourceType activationSourceType =
                newsSkillStateMaintainer.getActivationSourceTypeFromSession(request.getStateO());

        DialogovoState dialogovoState = newsSkillStateMaintainer.generateDialogovoState(
                request.getStateO(),
                currNewsState,
                activationSourceType,
                request.getServerTime());

        context.getAnalytics()
                .setIntent(Intents.EXTERNAL_SKILL_NEWS_SUBSCRIPTION_DECLINE)
                .addAction(AnalyticsInfoActionsFactory.createContinueAction(skillInfo))
                .addObject(new SessionAnalyticsInfoObject(
                        dialogovoState.getSessionO()
                                .map(Session::getSessionId)
                                .orElse(""),
                        activationSourceType));

        String text = phrases.getRandom("news.content.subscription.promo.refuse.reaction",
                request.getRandom());

        return new RunOnlyResponse<>(
                new ScenarioResponseBody<>(
                        Layout
                                .builder()
                                .outputSpeech(text)
                                .textCard(text)
                                .shouldListen(false)
                                .build(),
                        Optional.of(dialogovoState),
                        context.getAnalytics().toAnalyticsInfo(),
                        false)
        );
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.EXTERNAL_SKILL_NEWS_SUBSCRIPTION_DECLINE;
    }
}
