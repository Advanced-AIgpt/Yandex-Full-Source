package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.Optional;
import java.util.function.Predicate;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ApplyingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillFilters;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillLayoutsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ActivateSkillAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;

import static java.util.function.Predicate.not;

@Component
public class FairytaleActivateRunProcessor implements ApplyingRunProcessor<DialogovoState, RequestSkillApplyArguments> {

    private static final Logger logger = LogManager.getLogger();

    // поверхности, на которых запрос "расскажи сказку" приводит к активации навыка, а не нативного сценария Алисы
    private static final Predicate<MegaMindRequest<DialogovoState>> SURFACE_USES_FAIRYTALES_SKILL = request ->
            request.getClientInfo().isElariWatch() ||
                    request.getClientInfo().isYaAuto() && !request.hasExperiment(Experiments.INTERNAL_MUSIC_PLAYER);

    private final SkillProvider skillProvider;
    private final ProcessRunSkillRequestService processRunSkillRequestService;
    private final String internalFairytaleSkillId;
    private final SkillLayoutsFactory skillLayoutsFactory;
    private final MegaMindRequestSkillApplier skillApplier;

    protected FairytaleActivateRunProcessor(
            SkillProvider skillProvider,
            ProcessRunSkillRequestService processRunSkillRequestService,
            @Value("${internalFairytaleSkillId}") String internalFairytaleSkillId,
            SkillLayoutsFactory skillLayoutsFactory,
            MegaMindRequestSkillApplier skillApplier) {
        this.skillProvider = skillProvider;
        this.processRunSkillRequestService = processRunSkillRequestService;
        this.internalFairytaleSkillId = internalFairytaleSkillId;
        this.skillLayoutsFactory = skillLayoutsFactory;
        this.skillApplier = skillApplier;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return not(IS_IN_SKILL)
                .and(hasAnyOfFrames(
                        SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_MUSIC_FAIRYTALE_OLD,
                        SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_MUSIC_FAIRYTALE_NEW))
                .and(SURFACE_USES_FAIRYTALES_SKILL)
                .test(request);
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        logger.info("Process fairy tale activation request");

        SkillInfo skillInfo = skillProvider.getSkill(internalFairytaleSkillId)
                .orElseThrow(() -> new RuntimeException("Fairytale skill not found"));

        if (
                SkillFilters.CONTAINS_RESTRICTED_EXPLICIT_CONTENT.and(
                        SkillFilters.SKILL_CAN_PROCESS_FILTRATION_LEVEL_VERBOSE.negate()
                ).test(request, skillInfo)
        ) {
            context.getAnalytics().addObject(new SkillAnalyticsInfoObject(skillInfo));
            return new RunOnlyResponse<>(new ScenarioResponseBody<>(
                    skillLayoutsFactory.createExplicitContentDenyLayout(
                            request.getFiltrationLevel(), request.getRandom()
                    ),

                    context.getAnalytics().toAnalyticsInfo(Intents.EXTERNAL_SKILL_EXPLICIT_CONTENT_DENY_ACTIVATION),
                    false
            ));
        }

        logger.info("Process activation request with skill id [{}] request [] original_utterance []",
                internalFairytaleSkillId
        );
        context.getAnalytics().setIntent(Intents.EXTERNAL_SKILL_ACTIVATE)
                .addAction(ActivateSkillAction.create(skillInfo));

        return processRunSkillRequestService.activateSkill(
                internalFairytaleSkillId,
                new SkillActivationArguments(
                        context,
                        request,
                        Optional.of(ActivationSourceType.DIRECT)
                )
        );
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.EXTERNAL_SKILL_FAIRYTALE_ACTIVATE;
    }

    @Override
    public Class<RequestSkillApplyArguments> getApplyArgsType() {
        return RequestSkillApplyArguments.class;
    }

    @Override
    public ScenarioResponseBody<DialogovoState> apply(MegaMindRequest<DialogovoState> request, Context context,
                                                      RequestSkillApplyArguments applyArguments) {
        return skillApplier.processApply(request, context, applyArguments);
    }

}
