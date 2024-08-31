package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.AliceHandledException;
import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.SingleSemanticFrameRunProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillLayoutsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.DeactivateSkillAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;

import static ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor.getSkillId;
import static ru.yandex.alice.paskill.dialogovo.scenarios.SkillFilters.HAS_MORDOVIA_VIEW;

@Component
public class SkillDeactivateRunProcessor implements SingleSemanticFrameRunProcessor<DialogovoState> {
    private static final Logger logger = LogManager.getLogger();
    public static final String SILENT_RESPONSE_FRAME_SLOT = "silent_response";

    private final SkillLayoutsFactory skillLayoutsFactory;
    private final AppMetricaEventSender appMetricaEventSender;
    private final SkillProvider skillProvider;

    protected SkillDeactivateRunProcessor(
            SkillLayoutsFactory skillLayoutsFactory,
            AppMetricaEventSender appMetricaEventSender,
            SkillProvider skillProvider) {
        this.skillLayoutsFactory = skillLayoutsFactory;
        this.appMetricaEventSender = appMetricaEventSender;
        this.skillProvider = skillProvider;
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.EXTERNAL_SKILL_DEACTIVATE;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        final boolean hasDeactivateSemanticSlot =
                request.getSemanticFrameO(SemanticFrames.EXTERNAL_SKILL_DEACTIVATE).isPresent();
        final boolean hasDeactivateOnSmartSpeakerSlot =
                request.hasExperiment(Experiments.USE_SMART_SPEAKER_DEACTIVATE_GRAMMAR)
                        && request.getClientInfo().isYaSmartDevice()
                        && request.getSemanticFrameO(SemanticFrames.EXTERNAL_SKILL_SMART_SPEAKER_DEACTIVATE)
                        .isPresent();
        final boolean hasForceDeactivateSemanticSlot =
                request.getSemanticFrameO(SemanticFrames.EXTERNAL_SKILL_FORCE_DEACTIVATE).isPresent();
        return IS_IN_SKILL.test(request) && (hasDeactivateSemanticSlot || hasDeactivateOnSmartSpeakerSlot)
                || (HAS_CURRENT_SKILL_SESSION.test(request) && hasForceDeactivateSemanticSlot);
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        logger.info("deactivate skill");

        boolean silentResponse = request.getSemanticFrameO(SemanticFrames.EXTERNAL_SKILL_FORCE_DEACTIVATE)
                .flatMap(frame -> frame.getBoolSlotValueO(SILENT_RESPONSE_FRAME_SLOT))
                .orElse(false);

        var skillInfo = getSkillId(request)
                .flatMap(skillProvider::getSkill)
                .orElseThrow(() -> new AliceHandledException(
                        "Навык не найден", ErrorAnalyticsInfoAction.REQUEST_FAILURE, "Произошла ошибка"));

        context.getAnalytics()
                .addObject(new SkillAnalyticsInfoObject(skillInfo))
                .setIntent(Intents.EXTERNAL_SKILL_DEACTIVATE)
                .addAction(DeactivateSkillAction.INSTANCE);

        var layout = skillLayoutsFactory.createDeactivateSkillLayout(
                skillInfo.getId(),
                "Отлично, будет скучно — обращайтесь.",
                request.getClientInfo(),
                HAS_MORDOVIA_VIEW.test(request, skillInfo),
                request.hasActiveAudioPlayer(),
                silentResponse);

        appMetricaEventSender.sendEndSessionEvents(
                        context.getSource(),
                        skillInfo.getId(),
                        skillInfo.getLook(),
                        skillInfo.getEncryptedAppMetricaApiKey(),
                        request.getClientInfo(),
                        request.getLocationInfoO(),
                        request.getServerTime(),
                        request.getStateO().flatMap(DialogovoState::getSessionO),
                        request.isTest(),
                        skillInfo.hasFeatureFlag(SkillFeatureFlag.APPMETRICA_EVENTS_TIMESTAMP_COUNTER)
        );

        return new RunOnlyResponse<>(new ScenarioResponseBody<>(layout,
                context.getAnalytics().toAnalyticsInfo(), false));
    }

    @Override
    public String getSemanticFrame() {
        return SemanticFrames.EXTERNAL_SKILL_DEACTIVATE;
    }
}
