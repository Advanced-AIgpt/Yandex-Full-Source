package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.Collection;
import java.util.List;
import java.util.Optional;
import java.util.function.Predicate;

import javax.annotation.Nullable;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ApplyingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.MultipleSemanticFrameRunProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillFilters;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillLayoutsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticSlotEntityTypes;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticSlotTypes;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ActivateSkillAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;
import ru.yandex.alice.paskill.dialogovo.utils.VerbosePredicate;

import static java.util.function.Predicate.not;
import static ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames.EXTERNAL_SKILL_FIXED_ACTIVATE_FAST;
import static ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticFrames.EXTERNAL_SKILL_FIXED_ACTIVATE;
import static ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticSlotTypes.ACTIVATION_SOURCE_TYPE;
import static ru.yandex.alice.paskill.dialogovo.utils.ActivationTypedSemanticFrameUtils.getActivationTypedSemanticFrame;

@Component
public class FixedSkillActivateRunProcessor
        implements ApplyingRunProcessor<DialogovoState, RequestSkillApplyArguments>,
        MultipleSemanticFrameRunProcessor<DialogovoState> {
    private static final Logger logger = LogManager.getLogger();

    private final SkillProvider skillProvider;
    private final ProcessRunSkillRequestService processRunSkillRequestService;
    private final SkillLayoutsFactory skillLayoutsFactory;
    private final RequestContext requestContext;
    private final MegaMindRequestSkillApplier skillApplier;

    private final Predicate<MegaMindRequest<DialogovoState>> allowActivateAnotherSkill;

    protected FixedSkillActivateRunProcessor(
            SkillProvider skillProvider,
            ProcessRunSkillRequestService processRunSkillRequestService,
            SkillLayoutsFactory skillLayoutsFactory, RequestContext requestContext, ProtoUtil protoUtil,
            MegaMindRequestSkillApplier skillApplier) {
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
        return RunRequestProcessorType.EXTERNAL_SKILL_FIXED_ACTIVATE;
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        SemanticFrame semanticFrame = getFirstMatchedSemanticFrame(request).get();
        logger.info("Process activations request with semanticFrame={}", semanticFrame);

        Optional<SkillInfo> skillInfoO = detectSkillByInput(semanticFrame, request);

        if (skillInfoO.isEmpty()) {
            logger.info("Unable to detect skill by semanticFrame [{}]", semanticFrame);
            return DefaultIrrelevantResponse.create(Intents.EXTERNAL_SKILL_IRRELEVANT, request.getRandom(),
                    request.isVoiceSession());
        }
        SkillInfo skillInfo = skillInfoO.get();
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
        }

        context.getAnalytics()
                .setIntent(Intents.EXTERNAL_SKILL_ACTIVATE)
                .addAction(ActivateSkillAction.create(skillInfo));

        @Nullable
        String command = semanticFrame.getSlotValue(SemanticSlotType.ACTIVATION_COMMAND.getValue());
        return processRunSkillRequestService.activateSkill(
                skillInfo,
                new SkillActivationArguments(
                        context,
                        request,
                        Optional.of(getSkillActivateSourceType(Optional.of(semanticFrame), request)),
                        command,
                        null,
                        semanticFrame.getSlotValueO(SemanticSlotType.PAYLOAD.getValue()),
                        getActivationTypedSemanticFrame(semanticFrame)
                )
        );
    }

    private ActivationSourceType getSkillActivateSourceType(Optional<SemanticFrame> semanticFrame,
                                                            MegaMindRequest<DialogovoState> request) {
        return semanticFrame
                .flatMap(sf -> sf.getTypedEntityValueO(
                        ACTIVATION_SOURCE_TYPE,
                        SkillsSemanticSlotEntityTypes.ACTIVATION_SOURCE_TYPE))
                .flatMap(ActivationSourceType.R::fromValueO)
                .orElse((request.isVoiceSession() || request.getClientInfo().isYaSmartDevice()) ?
                        ActivationSourceType.DIRECT :
                        ActivationSourceType.UNDETECTED);
    }

    private Optional<SkillInfo> detectSkillByInput(SemanticFrame frame, MegaMindRequest<DialogovoState> request) {
        Optional<String> fixedSkillIdO = frame.getSlotValueO(SkillsSemanticSlotTypes.FIXED_SKILL_ID);

        if (fixedSkillIdO.isEmpty()) {
            return Optional.empty();
        }

        String fixedSkillId = fixedSkillIdO.get();
        // TODO: tmp special case for PASKILLS-4643 - remove after 9 may
        if (fixedSkillId.equals("9da87669-14dc-4fb2-8a8f-861e9a4190bf")) {
            return detectByFixedSkillId("80c21412-9f56-4867-bd1a-5ea671b7f718",
                    requestContext.getCurrentUserId(), request);
        }

        if (request.hasExperiment(Experiments.EXTERNAL_SKILL_FIXED_ACTIVATE_DISABLED)) {
            return Optional.empty();
        }

        return detectByFixedSkillId(fixedSkillId, requestContext.getCurrentUserId(), request);
    }

    private Optional<SkillInfo> detectByFixedSkillId(String fixedSkillId, @Nullable String currentUserId,
                                                     MegaMindRequest<DialogovoState> request) {
        var skillActivateSourceType =
                getSkillActivateSourceType(request.getSemanticFrameO(EXTERNAL_SKILL_FIXED_ACTIVATE), request);
        return skillProvider.getSkill(fixedSkillId)
                .filter(
                        VerbosePredicate.logMismatch(
                                "EXTERNAL_SKILL_FIXED_ACTIVATE_DISABLED",
                                skillInfo -> !skillInfo.hasFeatureFlag(
                                        SkillFeatureFlag.EXTERNAL_SKILL_FIXED_ACTIVATE_DISABLED)))
                .filter(SkillFilters.VALID_FOR_RECOMMENDATIONS_VERBOSE
                        .or(skill -> (skill.isAccessibleBy(currentUserId, request) &&
                                skill.hasFeatureFlag(SkillFeatureFlag.ALLOW_PRIVATE_SKILL_WITH_FIXED_ACTIVATION))
                                || skillActivateSourceType == ActivationSourceType.SOCIAL_SHARING
                                || skillActivateSourceType == ActivationSourceType.WIDGET_GALLERY));
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

    @Override
    public Collection<String> getSemanticFrames() {
        return List.of(EXTERNAL_SKILL_FIXED_ACTIVATE, EXTERNAL_SKILL_FIXED_ACTIVATE_FAST);
    }
}
