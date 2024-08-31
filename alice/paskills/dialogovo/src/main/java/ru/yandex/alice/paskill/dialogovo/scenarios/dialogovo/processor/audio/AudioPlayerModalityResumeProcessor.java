package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio;

import java.time.Duration;
import java.time.Instant;
import java.util.Optional;
import java.util.Set;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.ApplyNeededResponse;
import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.domain.AudioPlayerActivityState;
import ru.yandex.alice.kronstadt.core.input.UtteranceInput;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ApplyingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.MegaMindRequestSkillApplier;
import ru.yandex.alice.paskill.dialogovo.service.MatchedActivationIntentFinder;

import static java.util.function.Predicate.not;

/**
 * Processor intercepts all requests while scenario is thin player owner - provided by MM with
 * IsPlayerOwnerPriorityAllowed: true configuration
 * <p>
 * Processing request if now PLAYING
 * or (RESUME_SESSION_AFTER_PLAYER_STOP_ATTEMPTS requests after STOP
 * and not older than RESUME_SESSION_AFTER_PLAYER_STOP_WINDOW).
 * <p>
 * Processing - is get skill from player meta - check activations grammars - get request if one of its matches
 */
@Component
public class AudioPlayerModalityResumeProcessor
        implements ApplyingRunProcessor<DialogovoState, RequestSkillApplyArguments> {

    private static final int RESUME_SESSION_AFTER_PLAYER_STOP_REQUESTS_MAX = 1;
    private static final Duration RESUME_SESSION_AFTER_PLAYER_STOP_WINDOW = Duration.ofSeconds(30);

    private final MegaMindRequestSkillApplier skillApplier;
    private final MatchedActivationIntentFinder matchedActivationIntentFinder;

    public AudioPlayerModalityResumeProcessor(MegaMindRequestSkillApplier skillApplier,
                                              MatchedActivationIntentFinder matchedActivationIntentFinder) {
        this.skillApplier = skillApplier;
        this.matchedActivationIntentFinder = matchedActivationIntentFinder;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return IS_SMART_SPEAKER_OR_TV
                .and(HAS_SKILL_PLAYER_OWNER)
                .and(not(IS_IN_SKILL))
                // Possible request after deactivate or error
                .and(req -> req.getStateO().isPresent())
                .and(
                        inAudioPlayerStates(AudioPlayerActivityState.PLAYING)
                                .or(inAudioPlayerStates(
                                        AudioPlayerActivityState.STOPPED,
                                        AudioPlayerActivityState.FINISHED,
                                        AudioPlayerActivityState.PAUSED)
                                        .and(AUDIO_PLAYER_IS_LAST_MEDIA_PLAYER_ACTIVE)
                                        .and(this::allowRequestOnStoppedPlayer)))
                .and(req -> req.getInput() instanceof UtteranceInput)
                .test(request);
    }

    private boolean allowRequestOnStoppedPlayer(MegaMindRequest<DialogovoState> req) {
        Optional<Instant> lastStopTimestampO = req.getLastStopTimestampO();
        if (lastStopTimestampO.isEmpty()) {
            return false;
        }

        Instant lastStopTimestamp = lastStopTimestampO.get();

        Duration resumeSessionAfterPlayerStopWindow =
                req.getExperimentWithValue(Experiments.RESUME_SESSION_AFTER_PLAYER_STOP_WINDOW_SEC,
                        RESUME_SESSION_AFTER_PLAYER_STOP_WINDOW,
                        sec -> Duration.ofSeconds(Integer.parseInt(sec)));

        boolean timeSinceStopNotAboveAllowed =
                Duration.between(lastStopTimestamp,
                        req.getServerTime()).compareTo(resumeSessionAfterPlayerStopWindow) < 0;

        boolean resumeSessionAfterPlayerStopRequestsNotExceed = req.getStateO()
                .map(DialogovoState::getResumeSessionAfterPlayerStopRequests)
                .orElse(0) < RESUME_SESSION_AFTER_PLAYER_STOP_REQUESTS_MAX;

        return timeSinceStopNotAboveAllowed && resumeSessionAfterPlayerStopRequestsNotExceed;
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.AUDIO_PLAYER_MODALITY_RESUME;
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        // provided by HAS_SKILL_PLAYER_OWNER
        String skillId = request.getAudioPlayerOwnerSkillIdO().get();

        // provided by canProcess
        var input = (UtteranceInput) request.getInput();

        Set<String> matchedIntents = matchedActivationIntentFinder.find(skillId, input.getOriginalUtterance());

        int resumeSessionAfterPlayerStopRequests = request.getStateO()
                .map(DialogovoState::getResumeSessionAfterPlayerStopRequests)
                .orElse(0);

        // do not count request on playing player
        if (request.inAudioPlayerStates(AudioPlayerActivityState.PLAYING)) {
            resumeSessionAfterPlayerStopRequests = 0;
        } else {
            resumeSessionAfterPlayerStopRequests++;
        }

        if (!matchedIntents.isEmpty()) {
            ApplyNeededResponse applyNeededResponse = new ApplyNeededResponse(RequestSkillApplyArguments.create(
                    skillId,
                    Optional.of(input.getNormalizedUtterance()),
                    Optional.of(input.getOriginalUtterance()),
                    Optional.of(ActivationSourceType.DIRECT),
                    resumeSessionAfterPlayerStopRequests),
                    FeaturesUtilKt.capturePlayer(request));
            return applyNeededResponse;
        }

        return DefaultIrrelevantResponse.create(
                Intents.IRRELEVANT,
                request.getRandom(),
                false);
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
