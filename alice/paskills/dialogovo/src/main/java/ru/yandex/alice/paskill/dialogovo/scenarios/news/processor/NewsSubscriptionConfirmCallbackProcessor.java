package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor;


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
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.CommittingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SessionAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.analytics.AnalyticsInfoActionsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.directives.NewsSubscriptionConfirmDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.service.NewsSkillStateMaintainer;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;
import ru.yandex.alice.paskill.dialogovo.service.memento.MementoService;
import ru.yandex.alice.paskill.dialogovo.service.memento.NewsProviderSubscription;

@Component
public class NewsSubscriptionConfirmCallbackProcessor
        implements CommittingRunProcessor<DialogovoState, ReadNewsApplyArguments> {
    private static final Logger logger = LogManager.getLogger();

    private final MementoService mementoService;
    private final NewsSkillProvider skillProvider;
    private final RequestContext requestContext;
    private final NewsSkillStateMaintainer newsSkillStateMaintainer;
    private final Phrases phrases;
    private final AppMetricaEventSender appMetricaEventSender;

    public NewsSubscriptionConfirmCallbackProcessor(MementoService mementoService,
                                                    NewsSkillProvider skillProvider,
                                                    RequestContext requestContext,
                                                    NewsSkillStateMaintainer newsSkillStateMaintainer,
                                                    Phrases phrases, AppMetricaEventSender appMetricaEventSender) {
        this.mementoService = mementoService;
        this.skillProvider = skillProvider;
        this.requestContext = requestContext;
        this.newsSkillStateMaintainer = newsSkillStateMaintainer;
        this.phrases = phrases;
        this.appMetricaEventSender = appMetricaEventSender;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return request.getInput().isCallback(NewsSubscriptionConfirmDirective.class);
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        NewsSubscriptionConfirmDirective payload =
                request.getInput().getDirective(NewsSubscriptionConfirmDirective.class);

        logger.info("Received subscription confirm for skill by id [{}]", payload.getSkillId());

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

        DialogovoState stateWithoutAppmetrica = newsSkillStateMaintainer.generateDialogovoState(
                request.getStateO(),
                currNewsState,
                activationSourceType,
                request.getServerTime(),
                request.getStateO().flatMap(DialogovoState::getSessionO)
                        .map(Session::getAppMetricaEventCounter).orElse(1L)
        );

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

        context.getAnalytics()
                .setIntent(Intents.EXTERNAL_SKILL_NEWS_SUBSCRIPTION_ACCEPT)
                .addAction(AnalyticsInfoActionsFactory.createContinueAction(skillInfo))
                .addObject(new SessionAnalyticsInfoObject(
                        dialogovoState.getSessionO()
                                .map(Session::getSessionId)
                                .orElse(""),
                        activationSourceType));

        String text = phrases.getRandom("news.content.subscription.promo.accept.reaction",
                request.getRandom());

        return new CommitNeededResponse<>(
                new ScenarioResponseBody<>(
                        Layout
                                .builder()
                                .outputSpeech(text)
                                .textCard(text)
                                .shouldListen(false)
                                .build(),
                        Optional.of(dialogovoState),
                        context.getAnalytics().toAnalyticsInfo(),
                        false),
                new ReadNewsApplyArguments(skillInfo.getId(), appmetricaArgs)
        );
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.EXTERNAL_SKILL_NEWS_SUBSCRIPTION_CONFIRM;
    }

    @Override
    public CommitResult commit(MegaMindRequest<DialogovoState> request, Context context,
                               ReadNewsApplyArguments applyArguments) {
        String skillId = applyArguments.getSkillId();
        logger.info("Adding news user subscription with skillId [{}]", skillId);

        if (requestContext.getCurrentUserTicket() == null) {
            logger.info("User not logged - skipping subscription with skillId [{}]", skillId);
            return CommitResult.Success;
        }

        Optional<NewsSkillInfo> skillO = skillProvider.getSkill(skillId);

        if (skillO.isEmpty()) {
            logger.warn("News skill [{}] not found", skillId);
            return CommitResult.Success;
        }

        NewsSkillInfo skill = skillO.get();

        try {
            mementoService.updateNewsProviderSubscription(
                    requestContext.getCurrentUserTicket(),
                    new NewsProviderSubscription(skill.getSlug(), Optional.empty()));
        } catch (Exception ex) {
            logger.error("News subscription failed", ex);
            return CommitResult.Error;
        }

        return CommitResult.Success;
    }

    @Override
    public Class<ReadNewsApplyArguments> getApplyArgsType() {
        return ReadNewsApplyArguments.class;
    }
}
