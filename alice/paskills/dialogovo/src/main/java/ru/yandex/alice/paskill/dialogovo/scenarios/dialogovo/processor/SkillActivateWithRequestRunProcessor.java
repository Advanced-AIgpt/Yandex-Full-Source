package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.Objects;
import java.util.Optional;
import java.util.function.Predicate;

import javax.annotation.Nullable;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.stereotype.Component;
import org.springframework.util.StringUtils;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.DetectedSkillResult;
import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ApplyingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.SingleSemanticFrameRunProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillFilters;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillLayoutsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ActivateSkillAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;
import ru.yandex.alice.paskill.dialogovo.service.SkillDetector;

import static java.util.function.Predicate.not;
import static ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames.ALICE_EXTERNAL_SKILL_ACTIVATE_WITH_REQUEST;

@Component
public class SkillActivateWithRequestRunProcessor
        implements ApplyingRunProcessor<DialogovoState, RequestSkillApplyArguments>,
        SingleSemanticFrameRunProcessor<DialogovoState> {
    private static final Logger logger = LogManager.getLogger();

    private final SkillDetector skillDetector;
    private final SkillProvider skillProvider;
    private final ProcessRunSkillRequestService processRunSkillRequestService;
    private final SkillLayoutsFactory skillLayoutsFactory;
    private final RequestContext requestContext;
    private final MegaMindRequestSkillApplier skillApplier;
    private final Predicate<MegaMindRequest<DialogovoState>> allowActivateAnotherSkill;

    protected SkillActivateWithRequestRunProcessor(
            @Qualifier("skillDetector") SkillDetector skillDetector,
            SkillProvider skillProvider,
            ProcessRunSkillRequestService processRunSkillRequestService,
            SkillLayoutsFactory skillLayoutsFactory,
            RequestContext requestContext,
            MegaMindRequestSkillApplier skillApplier) {
        this.skillDetector = skillDetector;
        this.skillProvider = skillProvider;
        this.processRunSkillRequestService = processRunSkillRequestService;
        this.skillLayoutsFactory = skillLayoutsFactory;
        this.requestContext = requestContext;
        this.skillApplier = skillApplier;
        this.allowActivateAnotherSkill = IS_SMART_SPEAKER_OR_TV.and(
                r -> r.hasExperiment(Experiments.ACTIVATE_FROM_ANOTHER_SKILL)
                        || r.getStateO()
                        .flatMap(DialogovoState::getCurrentSkillIdO)
                        .flatMap(skillProvider::getSkill)
                        .filter(skillInfo -> skillInfo.hasFeatureFlag(SkillFeatureFlag.ALLOW_ACTIVATE_ANOTHER_SKILL))
                        .isPresent()
        );
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return hasFrame()
                .and(
                        not(IS_IN_SKILL).or(allowActivateAnotherSkill)
                )
                .test(request);
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.EXTERNAL_SKILL_ACTIVATE_WITH_REQUEST;
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        SemanticFrame semanticFrame = request.getSemanticFrameO(getSemanticFrame()).get();
        logger.info("Process activations request with semanticFrameType={}",
                ALICE_EXTERNAL_SKILL_ACTIVATE_WITH_REQUEST);

        // as already checked in #canProcessAdditionally
        String utterance =
                Objects.requireNonNull(semanticFrame.getSlotValue(SemanticSlotType.ACTIVATION_PHRASE.getValue()));
        Optional<DetectedSkillResult> skillRO = detectSkillByInput(utterance, requestContext.getCurrentUserId(),
                request);

        if (skillRO.isEmpty()) {
            logger.info("Unable to detect skill by semanticFrame [{}]", semanticFrame);
            return DefaultIrrelevantResponse.create(
                    Intents.EXTERNAL_SKILL_IRRELEVANT,
                    request.getRandom(),
                    request.isVoiceSession()
            );
        }
        if (!isSkillHasRequest(skillRO.get())) {
            logger.info("Unable to detect skill's request by semanticFrame [{}]", semanticFrame);
            return DefaultIrrelevantResponse.create(
                    Intents.EXTERNAL_SKILL_IRRELEVANT,
                    request.getRandom(),
                    request.isVoiceSession()
            );
        }

        SkillInfo skillInfo = skillRO.get().getSkillInfo();
        if (IS_IN_SKILL.test(request) &&
                request.getStateO().get().getCurrentSkillIdO().get().equals(skillInfo.getId())) {
            logger.info("Activation from the same skill [{}]", semanticFrame);
            return DefaultIrrelevantResponse.create(Intents.EXTERNAL_SKILL_IRRELEVANT, request.getRandom(),
                    request.isVoiceSession());
        }

        if (
                SkillFilters.CONTAINS_RESTRICTED_EXPLICIT_CONTENT.and(
                        SkillFilters.SKILL_CAN_PROCESS_FILTRATION_LEVEL_VERBOSE.negate()
                ).test(request, skillInfo)
        ) {
            context.getAnalytics().addObject(new SkillAnalyticsInfoObject(skillInfo));
            return new RunOnlyResponse<>(new ScenarioResponseBody<>(
                    skillLayoutsFactory.createExplicitContentDenyLayout(request.getFiltrationLevel(),
                            request.getRandom()),
                    context.getAnalytics().toAnalyticsInfo(Intents.EXTERNAL_SKILL_EXPLICIT_CONTENT_DENY_ACTIVATION),
                    false
            ));
        } else {
            logger.info("Process with request skill activation request with possible explicit content " +
                            "with skill id [{}] request filtration level {}",
                    skillInfo.getId(), request.getFiltrationLevel()
            );
        }

        logger.info("Detected skill using pg: {}", skillInfo.getId());
        context.getAnalytics()
                .setIntent(Intents.EXTERNAL_SKILL_ACTIVATE)
                .addAction(ActivateSkillAction.create(skillInfo));

        ActivationSourceType source =
                (request.isVoiceSession() || request.getClientInfo().isYaSmartDevice()) ?
                        ActivationSourceType.DIRECT :
                        ActivationSourceType.UNDETECTED;
        return processRunSkillRequestService.activateSkill(
                skillInfo,
                new SkillActivationArguments(
                        context,
                        request,
                        Optional.of(source),
                        skillRO.flatMap(DetectedSkillResult::getRequestO).orElse(null),
                        skillRO.flatMap(DetectedSkillResult::getRequestO).orElse(null),
                        Optional.empty(),
                        null
                )
        );
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

    private boolean isSkillHasRequest(DetectedSkillResult skill) {
        return skill.getRequestO().isEmpty() || !StringUtils.isEmpty(skill.getRequestO().get());
    }

    private Optional<DetectedSkillResult> detectSkillByInput(String utterance, @Nullable String userId,
                                                             MegaMindRequest<DialogovoState> request) {
        return skillDetector.detectSkills(utterance, true).stream()
                .flatMap(detectedSkill -> skillProvider.getSkill(detectedSkill.getSkillId())
                        .filter(skill -> skill.isAccessibleBy(userId, request) &&
                                !skill.hasFeatureFlag(SkillFeatureFlag.DISABLE_ACTIVATE_WITH_REQUEST))
                        .map(foundSkill -> DetectedSkillResult.builder()
                                .skillInfo(foundSkill)
                                // pass request in skill immediately currently only on skill activated with alice
                                // .external_skill_activate_with_request
                                .requestO(Optional.of(detectedSkill.getSuffix()))
                                .originalUtteranceO(Optional.of(utterance))
                                .build())
                        .stream()
                )
                .findFirst();
    }

    @Override
    public String getSemanticFrame() {
        return ALICE_EXTERNAL_SKILL_ACTIVATE_WITH_REQUEST;
    }
}
