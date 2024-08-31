package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio;

import java.util.List;
import java.util.Optional;
import java.util.OptionalLong;

import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.Data;
import lombok.Getter;
import lombok.NoArgsConstructor;
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
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType;
import ru.yandex.alice.kronstadt.core.utils.StringEnum;
import ru.yandex.alice.kronstadt.core.utils.StringEnumResolver;
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
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticSlotEntityType;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.AudioPlayerControlAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.AudioStreamAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SessionAnalyticsInfoObject;

import static java.util.function.Predicate.not;

@Component
public class AudioPlayerRewindIntentProcessor implements RunRequestProcessor<DialogovoState> {
    private static final Logger logger = LogManager.getLogger();

    private static final Long REWIND_AMOUNT_MS_DEFAULT = 5000L;

    private final SkillProvider skillProvider;
    private final ObjectMapper mapper;
    private final DialogovoDirectiveFactory directiveFactory;

    public AudioPlayerRewindIntentProcessor(SkillProvider skillProvider, ObjectMapper mapper,
                                            DialogovoDirectiveFactory directiveFactory) {
        this.skillProvider = skillProvider;
        this.mapper = mapper;
        this.directiveFactory = directiveFactory;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return IS_SMART_SPEAKER_OR_TV
                .and(HAS_SKILL_PLAYER_OWNER)
                .and(AUDIO_PLAYER_IS_LAST_MEDIA_PLAYER_ACTIVE)
                .and(inAudioPlayerStates(AudioPlayerActivityState.PLAYING))
                .and(not(IS_IN_SKILL).or(IS_IN_SKILL.and(IS_IN_PLAYER_OWNER_SKILL)))
                .and(hasFrame(SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_PLAYER_REWIND))
                .test(request);
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.AUDIO_PLAYER_REWIND;
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

        // provided by canProcess
        SemanticFrame rewindSemanticFrame =
                request.getSemanticFrameO(SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_PLAYER_REWIND).get();
        Optional<RewindTypeEntity> rewindTypeO = getRewindTypeO(rewindSemanticFrame);
        Optional<RewindSizeEntity> rewindSizeO = getRewindSizeO(rewindSemanticFrame);
        OptionalLong amountMsO = getAmountMsO(rewindSemanticFrame);

        // if timeMs is not defined - move a little - five seconds
        long amountMs = amountMsO.orElse(REWIND_AMOUNT_MS_DEFAULT);
        // if not specified - move is absolute
        Rewind.Type rewindType = rewindTypeO
                .map(rewindT -> rewindT.rewindType)
                .orElse(amountMsO.isPresent() ?  Rewind.Type.ABSOLUTE : Rewind.Type.FORWARD);

        return new RunOnlyResponse<>(
                new ScenarioResponseBody<>(Layout.builder()
                        .shouldListen(false)
                        .directives(List.of(directiveFactory.audioRewindDirective(rewindType, amountMs)))
                        .outputSpeech("")
                        .textCard("")
                        .build(),
                        Optional.of(skillState),
                        context.getAnalytics().toAnalyticsInfo(),
                        !isSessionInBackground),
                FeaturesUtilKt.capturePlayer(request)
        );
    }

    private Optional<RewindTypeEntity> getRewindTypeO(SemanticFrame semanticFrame) {
        return semanticFrame.getTypedEntityValueO(
                SemanticSlotType.REWIND_TYPE.getValue(),
                SemanticSlotEntityType.CUSTOM_REWIND_TYPE)
                .flatMap(RewindTypeEntity.R::fromValueO);
    }

    private OptionalLong getAmountMsO(SemanticFrame semanticFrame) {
        // provided by canProcess
        String unitsTimeS = semanticFrame.getTypedEntityValueO(
                SemanticSlotType.TIME.getValue(),
                SemanticSlotEntityType.SYS_UNITS_TIME)
                .orElse(null);
        if (unitsTimeS == null) {
            return OptionalLong.empty();
        }
        try {
            UnitsTime unitsTime = mapper.readValue(unitsTimeS, UnitsTime.class);
            return OptionalLong.of(unitsTime.toMilliseconds());
        } catch (Exception ex) {
            logger.error("Error while parsing units_time type : [" + unitsTimeS + "]", ex);
            return OptionalLong.empty();
        }
    }

    private Optional<RewindSizeEntity> getRewindSizeO(SemanticFrame semanticFrame) {
        return semanticFrame.getTypedEntityValueO(
                SemanticSlotType.REWIND_SIZE.getValue(),
                SemanticSlotEntityType.CUSTOM_REWIND_SIZE)
                .flatMap(RewindSizeEntity.R::fromValueO);
    }

    @Data
    @NoArgsConstructor
    private static class UnitsTime {
        private float hours;
        private float minutes;
        private float seconds;

        private long toMilliseconds() {
            return (long) (seconds * 1000 +
                    minutes * 1000 * 60 +
                    hours * 1000 * 60 * 60);
        }
    }

    @Getter
    private enum RewindTypeEntity implements StringEnum {
        FORWARD("forward", Rewind.Type.FORWARD),
        BACKWARD("backward", Rewind.Type.BACKWARD);

        public static final StringEnumResolver<RewindTypeEntity> R =
                StringEnumResolver.resolver(RewindTypeEntity.class);

        @Getter
        private final String type;
        private final Rewind.Type rewindType;

        RewindTypeEntity(String type, Rewind.Type rewindType) {
            this.type = type;
            this.rewindType = rewindType;
        }

        @Override
        public String value() {
            return type;
        }
    }

    @Getter
    private enum RewindSizeEntity implements StringEnum {
        LITTLE("little");

        public static final StringEnumResolver<RewindSizeEntity> R =
                StringEnumResolver.resolver(RewindSizeEntity.class);

        @Getter
        private final String value;

        RewindSizeEntity(String value) {
            this.value = value;
        }

        @Override
        public String value() {
            return value;
        }
    }
}
