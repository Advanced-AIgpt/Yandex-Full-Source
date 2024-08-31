package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.geolocation;

import java.time.Instant;
import java.util.Optional;
import java.util.function.Predicate;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.ApplyNeededResponse;
import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.input.UtteranceInput;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType;
import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.domain.UserFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ApplyingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.processor.GeolocationSharingOptions;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestGeolocationSharingApplyArguments;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.MegaMindRequestSkillApplier;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.RequestSkillEvents;

import static ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor.getSkillId;
import static ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType.REQUEST_GEOLOCATION_SHARING;

@Component
public class RequestGeolocationSharingProcessor
        implements ApplyingRunProcessor<DialogovoState, RequestGeolocationSharingApplyArguments> {
    private static final Logger logger = LogManager.getLogger();

    private final SkillProvider skillProvider;
    private final MegaMindRequestSkillApplier skillApplier;

    public RequestGeolocationSharingProcessor(
            SkillProvider skillProvider,
            MegaMindRequestSkillApplier skillApplier
    ) {
        this.skillProvider = skillProvider;
        this.skillApplier = skillApplier;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return IS_IN_SKILL
                .and(hasGeolocationSharingFlag())
                .and(req -> req.getInput() instanceof UtteranceInput)
                .and(hasGeolocationRequestedState())
                .test(request);
    }

    @Override
    public RunRequestProcessorType getType() {
        return REQUEST_GEOLOCATION_SHARING;
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        var input = (UtteranceInput) request.getInput();

        return new ApplyNeededResponse(
                new RequestGeolocationSharingApplyArguments(
                        getSkillId(request).get(),
                        input.getNormalizedUtterance()
                )
        );
    }

    @Override
    public Class<RequestGeolocationSharingApplyArguments> getApplyArgsType() {
        return RequestGeolocationSharingApplyArguments.class;
    }

    @Override
    public ScenarioResponseBody<DialogovoState> apply(
            MegaMindRequest<DialogovoState> request,
            Context context,
            RequestGeolocationSharingApplyArguments applyArguments
    ) {
        var requestSkillEventsBuilder = RequestSkillEvents.builder();
        String utterance = applyArguments.getNormalizedUtterance();

        if (hasFrame(SemanticFrames.ALICE_EXTERNAL_SKILL_ALLOW_GEOSHARING).test(request)) {
            var testIntervalsEnables = request.hasExperiment(Experiments.GEOSHARING_ENABLE_TEST_INTERVALS);
            long millis = request.getSemanticFrameO(SemanticFrames.ALICE_EXTERNAL_SKILL_ALLOW_GEOSHARING)
                    .map(it -> it.getSlotValue(SemanticSlotType.ALLOW_TIME.getValue()))
                    .map(GeolocationSharingOptions::byCode)
                    .filter(it -> it.isAvailableInProduction() || testIntervalsEnables)
                    .map(GeolocationSharingOptions::getAllowedPeriodAsMillis)
                    .orElse(-1L);
            logger.info("Geolocation shared for " + millis);
            if (millis >= 0) {
                Instant untilTime = request.getServerTime().plusMillis(millis);
                var geolocationSharingAllowEvent = Optional.of(new GeolocationSharingAllowedEvent(untilTime));
                requestSkillEventsBuilder.geolocationSharingAllowedEvent(geolocationSharingAllowEvent);
            }
        } else if (hasFrame(SemanticFrames.ALICE_EXTERNAL_SKILL_DO_NOT_ALLOW_GEOSHARING).test(request)) {
            logger.info("Geosharing rejected");
            var rejectedEvent = Optional.of(new GeolocationSharingRejectedEvent());
            requestSkillEventsBuilder.geolocationSharingRejectedEvent(rejectedEvent);
        } else {
            for (var option : GeolocationSharingOptions.values()) {
                if (utterance.equalsIgnoreCase(option.getTitle())) {
                    var testIntervalsEnables = request.hasExperiment(Experiments.GEOSHARING_ENABLE_TEST_INTERVALS);
                    if (option.isOptionAllowSharing() &&
                            (testIntervalsEnables || option.isAvailableInProduction())
                    ) {
                        Instant untilTime = request.getServerTime().plusMillis(option.getAllowedPeriodAsMillis());
                        var geolocationSharingAllowEvent = Optional.of(new GeolocationSharingAllowedEvent(untilTime));
                        requestSkillEventsBuilder.geolocationSharingAllowedEvent(geolocationSharingAllowEvent);
                    } else {
                        var rejectedEvent = Optional.of(new GeolocationSharingRejectedEvent());
                        requestSkillEventsBuilder.geolocationSharingRejectedEvent(rejectedEvent);
                    }
                    break;
                }
            }
        }

        return skillApplier.processApply(request, context, getApplyArgs(applyArguments),
                Optional.of(requestSkillEventsBuilder.build()));
    }

    private RequestSkillApplyArguments getApplyArgs(RequestGeolocationSharingApplyArguments applyArguments) {
        return RequestSkillApplyArguments.create(
                applyArguments.getSkillId(),
                Optional.empty(),
                Optional.empty(),
                Optional.empty());
    }

    private Predicate<MegaMindRequest<DialogovoState>> hasGeolocationSharingFlag() {
        return request -> getSkillId(request).flatMap(skillProvider::getSkill)
                .map(skillInfo -> skillInfo.hasUserFeatureFlag(UserFeatureFlag.GEOLOCATION_SHARING))
                .orElse(false);
    }

    private Predicate<MegaMindRequest<DialogovoState>> hasGeolocationRequestedState() {
        return request -> request.getStateO()
                .flatMap(DialogovoState::getGeolocationSharingStateO)
                .map(DialogovoState.GeolocationSharingState::isRequested)
                .orElse(false);
    }
}
