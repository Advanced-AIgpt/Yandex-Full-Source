package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio;

import java.util.List;
import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.domain.AudioPlayerActivityState;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.Rewind;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.processor.DialogovoDirectiveFactory;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.AudioPlayerControlAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.AudioStreamAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SessionAnalyticsInfoObject;

import static java.util.function.Predicate.not;

@Component
public class AudioPlayerReplayIntentProcessor implements RunRequestProcessor<DialogovoState> {
    private static final Logger logger = LogManager.getLogger();

    private final SkillProvider skillProvider;
    private final DialogovoDirectiveFactory directiveFactory;

    public AudioPlayerReplayIntentProcessor(SkillProvider skillProvider, DialogovoDirectiveFactory directiveFactory) {
        this.skillProvider = skillProvider;
        this.directiveFactory = directiveFactory;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return IS_SMART_SPEAKER_OR_TV
                .and(HAS_SKILL_PLAYER_OWNER)
                .and(AUDIO_PLAYER_IS_LAST_MEDIA_PLAYER_ACTIVE)
                .and(inAudioPlayerStates(AudioPlayerActivityState.PLAYING))
                .and(not(IS_IN_SKILL).or(IS_IN_SKILL.and(IS_IN_PLAYER_OWNER_SKILL)))
                .and(hasAnyOfFrames(
                        SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_PLAYER_REPLAY,
                        SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_PLAYER_REPEAT))
                .test(request);
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.AUDIO_PLAYER_REPLAY;
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        // provided by SKILL_PLAYER_OWNER
        String skillId = request.getAudioPlayerOwnerSkillIdO().get();

        Optional<SkillInfo> skillO = skillProvider.getSkill(skillId);
        if (skillO.isEmpty()) {
            logger.info("Unable to detect skill id [{}] from audio player device state", skillId);
            return DefaultIrrelevantResponse.create(Intents.EXTERNAL_SKILL_IRRELEVANT, request.getRandom(),
                    request.isVoiceSession());
        }
        SkillInfo skillInfo = skillO.get();

        Session session = request.getStateO()
                .flatMap(DialogovoState::getSessionO)
                .map(Session::getNext)
                .orElseGet(() -> Session.create(ActivationSourceType.UNDETECTED, request.getServerTime()));

        boolean isSessionInBackground = request.getStateO().map(DialogovoState::isSessionInBackground)
                .orElse(false);

        context.getAnalytics()
                .setIntent(Intents.AUDIO_PLAYER_CONTROL)
                .addAction(new AudioPlayerControlAction(AudioPlayerControlAction.AudioPlayerControlActionType.REWIND))
                .addObject(new SkillAnalyticsInfoObject(skillInfo))
                .addObject(new SessionAnalyticsInfoObject(
                        session.getSessionId(),
                        session.getActivationSourceType()));

        // provided by SKILL_PLAYER_OWNER
        MegaMindRequest.DeviceState.AudioPlayer audioPlayerState =
                request.getDeviceStateO().map(MegaMindRequest.DeviceState::getAudioPlayerState).get();
        context.getAnalytics().addObject(new AudioStreamAnalyticsInfoObject(
                audioPlayerState.getToken(),
                audioPlayerState.getOffsetMs()));

        DialogovoState skillState = DialogovoState.createSkillState(skillId,
                session,
                request.getServerTime().toEpochMilli(),
                Optional.empty(),
                isSessionInBackground,
                request.getStateO().flatMap(DialogovoState::getGeolocationSharingStateO));

        return new RunOnlyResponse<>(
                new ScenarioResponseBody<>(Layout.builder()
                        .shouldListen(false)
                        .directives(List.of(directiveFactory.audioRewindDirective(Rewind.Type.ABSOLUTE, 0)))
                        .outputSpeech("")
                        .textCard("")
                        .build(),
                        Optional.of(skillState),
                        context.getAnalytics().toAnalyticsInfo(),
                        !isSessionInBackground),
                FeaturesUtilKt.capturePlayer(request));
    }
}
