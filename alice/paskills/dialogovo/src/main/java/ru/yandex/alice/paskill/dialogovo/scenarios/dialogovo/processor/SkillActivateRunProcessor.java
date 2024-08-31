package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.Comparator;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionException;
import java.util.function.Predicate;

import javax.annotation.Nullable;

import lombok.Data;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.input.UtteranceInput;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
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
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticSlotEntityTypes;
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticSlotTypes;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ActivateSkillAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;
import ru.yandex.alice.paskill.dialogovo.service.SkillDetector;
import ru.yandex.alice.paskill.dialogovo.service.SurfaceChecker;
import ru.yandex.alice.paskill.dialogovo.service.penguinary.PenguinaryResult;
import ru.yandex.alice.paskill.dialogovo.service.penguinary.PenguinaryService;

import static java.util.function.Predicate.not;
import static ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticFrames.ALICE_EXTERNAL_SKILL_ACTIVATE;

@Component
public class SkillActivateRunProcessor
        implements ApplyingRunProcessor<DialogovoState, RequestSkillApplyArguments>,
        SingleSemanticFrameRunProcessor<DialogovoState> {
    private static final Logger logger = LogManager.getLogger();
    private final SkillDetector skillDetector;
    private final SkillProvider skillProvider;
    private final PenguinaryService penguinaryService;
    private final SurfaceChecker surfaceChecker;
    private final ProcessRunSkillRequestService processRunSkillRequestService;
    private final double dssmThreshold;
    private final SkillLayoutsFactory skillLayoutsFactory;
    private final RequestContext requestContext;
    private final MegaMindRequestSkillApplier skillApplier;
    private final Predicate<MegaMindRequest<DialogovoState>> allowActivateAnotherSkill;

    @SuppressWarnings("ParameterNumber")
    protected SkillActivateRunProcessor(
            @Qualifier("skillDetector") SkillDetector skillDetector,
            SkillProvider skillProvider,
            PenguinaryService penguinaryService,
            SurfaceChecker surfaceChecker,
            ProcessRunSkillRequestService processRunSkillRequestService,
            SkillLayoutsFactory skillLayoutsFactory,
            RequestContext requestContext,
            MegaMindRequestSkillApplier skillApplier,
            @Value("${dssmThreshold}") double dssmThreshold) {
        this.skillDetector = skillDetector;
        this.skillProvider = skillProvider;
        this.penguinaryService = penguinaryService;
        this.surfaceChecker = surfaceChecker;
        this.processRunSkillRequestService = processRunSkillRequestService;
        this.dssmThreshold = dssmThreshold;
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
        return RunRequestProcessorType.EXTERNAL_SKILL_ACTIVATE;
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        logger.info("Process activations request with semanticFrameType={}",
                ALICE_EXTERNAL_SKILL_ACTIVATE);

        // as already checked
        SemanticFrame semanticFrame = request.getSemanticFrameO(getSemanticFrame()).get();
        String utterance =
                Objects.requireNonNull(semanticFrame.getSlotValue(SemanticSlotType.ACTIVATION_PHRASE.getValue()));
        CompletableFuture<Optional<SkillInfo>> future = tryDetectUsingPenguinary(request, utterance);
        Optional<SkillInfo> skillRO = detectSkillByInput(utterance, requestContext.getCurrentUserId(), request);
        if (skillRO.isPresent()) {
            logger.info("Detected skill using pg");
        } else if (future.join().isPresent()) {
            skillRO = future.join();
            logger.info("Detected skill using penguinary");
        } else {
            logger.info("Unable to detect skill by semanticFrame [{}]", semanticFrame);
            return DefaultIrrelevantResponse.create(Intents.EXTERNAL_SKILL_IRRELEVANT, request.getRandom(),
                    request.isVoiceSession());
        }

        SkillInfo skillInfo = skillRO.get();

        //here we already not IS_IN_SKILL OR allowActivateAnotherSkill
        //for the case not(IS_IN_SKILL) we test that the skill activation  hasn't be made from the same skill
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
        return processRunSkillRequestService.activateSkill(context, request, skillInfo,
                Optional.of(getSkillActivateSourceType(semanticFrame, request)));
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

    private ActivationSourceType getSkillActivateSourceType(SemanticFrame semanticFrame,
                                                            MegaMindRequest<DialogovoState> request) {
        return semanticFrame.getTypedEntityValueO(
                        SkillsSemanticSlotTypes.ACTIVATION_SOURCE_TYPE,
                        SkillsSemanticSlotEntityTypes.ACTIVATION_SOURCE_TYPE)
                .flatMap(ActivationSourceType.R::fromValueO)
                .orElse((request.isVoiceSession() || request.getClientInfo().isYaSmartDevice()) ?
                        ActivationSourceType.DIRECT :
                        ActivationSourceType.UNDETECTED);
    }

    private Optional<SkillInfo> detectSkillByInput(String utterance, @Nullable String currentUserId,
                                                   MegaMindRequest<DialogovoState> request) {
        return skillDetector.detectSkills(utterance, false).stream()
                .map(detectedSkill -> skillProvider.getSkill(detectedSkill.getSkillId()))
                .flatMap(Optional::stream)
                .filter(skill -> skill.isAccessibleBy(currentUserId, request))
                .findFirst();
    }

    private CompletableFuture<Optional<SkillInfo>> tryDetectUsingPenguinary(
            MegaMindRequest<DialogovoState> baseRequest, String input
    ) {
        // no utterance to work with
        // use dssm only for simple activation without suffix as we cant separate suffix part to pass to the skill
        if (!(baseRequest.getInput() instanceof UtteranceInput)) {
            return CompletableFuture.completedFuture(Optional.empty());
        }
        logger.info("Try detect skill using penguinary dssm. threshold = {}", dssmThreshold);
        return penguinaryService.findSkillsByUtteranceAsync(input)
                .thenApply(result -> processPenguinaryResponse(baseRequest, result))
                .exceptionally(e -> {
                    var ex = e instanceof CompletionException ? Objects.requireNonNullElse(e.getCause(), e) : e;
                    logger.error("Failed calling Penguinary", ex);
                    return Optional.empty();
                });
    }

    private Optional<SkillInfo> processPenguinaryResponse(
            MegaMindRequest<DialogovoState> baseRequest,
            PenguinaryResult result
    ) {
        logger.info("Penguinary found {} candidates: {}", result.getCandidates().size(), result);
        final Comparator<DssmCandidateWithSkill> comparator = getCandidateComparator(baseRequest);
        Optional<SkillInfo> detectedSkillResult = result.getCandidates().stream()
                // take only good skills
                .filter(candidate -> candidate.getDistance() < dssmThreshold)
                .flatMap(candidate ->
                        skillProvider.getSkill(candidate.getDocumentId())
                                .map(skill -> new DssmCandidateWithSkill(candidate, skill))
                                .stream()
                )
                .filter(c -> SkillFilters.CONTAINS_RESTRICTED_EXPLICIT_CONTENT.negate()
                        .or(SkillFilters.SKILL_CAN_PROCESS_FILTRATION_LEVEL_VERBOSE)
                        .test(baseRequest, c.skill))
                .filter(c -> surfaceChecker.isSkillSupported(baseRequest.getClientInfo(), c.skill.getSurfaces()))
                // filter out unsupported skills as we increase activation flux with dssm
                .filter(c -> c.skill.isValidForRecommendations())
                .min(comparator)
                .map(it -> it.skill);
        logger.info("Penguinary detected skill ID: {}",
                detectedSkillResult.map(SkillInfo::getId).orElse("null"));
        return detectedSkillResult;
    }

    private Comparator<DssmCandidateWithSkill> getCandidateComparator(MegaMindRequest<DialogovoState> baseRequest) {
        if (baseRequest.hasExperiment(Experiments.DSSM_RERANK_BY_STORE_SCORE)) {
            double maxDssmDistance = baseRequest.getExperimentWithValue(
                    Experiments.DSSM_RERANK_BY_STORE_SCORE_MIN_DSSM_DISTANCE,
                    0.35);
            return new SkillScoreComparator(maxDssmDistance);
        } else {
            return DssmDistanceComparator.INSTANCE;
        }
    }

    @Override
    public String getSemanticFrame() {
        return ALICE_EXTERNAL_SKILL_ACTIVATE;
    }

    private static class DssmDistanceComparator implements Comparator<DssmCandidateWithSkill> {

        private static final DssmDistanceComparator INSTANCE = new DssmDistanceComparator();

        private DssmDistanceComparator() {
        }

        @Override
        public int compare(DssmCandidateWithSkill a, DssmCandidateWithSkill b) {
            return Double.compare(a.candidate.getDistance(), b.candidate.getDistance());
        }

    }

    private static class SkillScoreComparator implements Comparator<DssmCandidateWithSkill> {

        private final double maxDssmDistance;
        private final DssmDistanceComparator dssmDistanceComparator;

        SkillScoreComparator(final double maxDssmDistance) {
            this.maxDssmDistance = maxDssmDistance;
            this.dssmDistanceComparator = new DssmDistanceComparator();
        }

        @Override
        public int compare(DssmCandidateWithSkill a, DssmCandidateWithSkill b) {
            if (a.candidate.getDistance() < maxDssmDistance && b.candidate.getDistance() < maxDssmDistance) {
                return -1 * Double.compare(a.skill.getScore(), b.skill.getScore());
            } else {
                return dssmDistanceComparator.compare(a, b);
            }
        }

    }

    @Data
    private static class DssmCandidateWithSkill {
        final PenguinaryResult.Candidate candidate;
        final SkillInfo skill;

        DssmCandidateWithSkill(PenguinaryResult.Candidate candidate, SkillInfo skill) {
            this.candidate = candidate;
            this.skill = skill;
        }
    }
}
