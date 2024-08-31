package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio;

import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.ApplyNeededResponse;
import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.domain.AudioPlayerActivityState;
import ru.yandex.alice.kronstadt.core.input.Input;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ApplyingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticSlotEntityType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.MegaMindRequestSkillApplier;

import static java.util.function.Predicate.not;

@Component
public class AudioPlayerIntentToSkillProxyProcessor
        implements ApplyingRunProcessor<DialogovoState, RequestSkillApplyArguments> {
    private static final Logger logger = LogManager.getLogger();
    private static final Set<String> FORBIDDEN_PLAYER_TYPES = List.of(
            PlayerTypeEntity.MUSIC, PlayerTypeEntity.VIDEO, PlayerTypeEntity.RADIO)
            .stream()
            .map(PlayerTypeEntity::getValue)
            .collect(Collectors.toSet());
    private static final Set<String> FORBIDDEN_PLAYER_ACTION_TYPES = List.of(
            PlayerActionTypeEntity.WATCH)
            .stream()
            .map(PlayerActionTypeEntity::getValue)
            .collect(Collectors.toSet());
    private static final Set<String> ALLOWED_PLAYER_ENTITY_TYPES = List.of(
            PlayerEntityTypeEntity.BOOK)
            .stream()
            .map(PlayerEntityTypeEntity::getValue)
            .collect(Collectors.toSet());

    private final SkillProvider skillProvider;
    private final MegaMindRequestSkillApplier skillApplier;

    public AudioPlayerIntentToSkillProxyProcessor(SkillProvider skillProvider,
                                                  MegaMindRequestSkillApplier skillApplier) {
        this.skillProvider = skillProvider;
        this.skillApplier = skillApplier;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return IS_SMART_SPEAKER_OR_TV
                .and(HAS_SKILL_PLAYER_OWNER)
                .and(not(IS_IN_SKILL).or(IS_IN_SKILL.and(IS_IN_PLAYER_OWNER_SKILL)))
                .and(AUDIO_PLAYER_IS_LAST_MEDIA_PLAYER_ACTIVE)
                .and(
                        hasAnyOfFrames(
                                SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_PLAYER_NEXT_TRACK,
                                SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_PLAYER_PREVIOUS_TRACK)
                                .and(inAudioPlayerStates(AudioPlayerActivityState.PLAYING))

                                .or(hasFrame(
                                        SemanticFrames.PERSONAL_ASSISTANT_SCENARIOS_PLAYER_CONTINUE)
                                        // there is no custom player type(among existing: video, radio, music) set in
                                        // any slot
                                        .and(inAudioPlayerStates(
                                                AudioPlayerActivityState.IDLE,
                                                AudioPlayerActivityState.STOPPED,
                                                AudioPlayerActivityState.FINISHED,
                                                AudioPlayerActivityState.PAUSED))
                                        .and(not(hasOneOfValuesInSlotEntityTypeInAnyFrame(
                                                SemanticSlotType.PLAYER_TYPE.getValue(),
                                                SemanticSlotEntityType.CUSTOM_PLAYER_TYPE,
                                                FORBIDDEN_PLAYER_TYPES)))
                                        .and(not(hasOneOfValuesInSlotEntityTypeInAnyFrame(
                                                SemanticSlotType.PLAYER_ACTION_TYPE.getValue(),
                                                SemanticSlotEntityType.PLAYER_ACTION_TYPE,
                                                FORBIDDEN_PLAYER_ACTION_TYPES)))
                                        .and(hasOneOfValuesInSlotEntityTypeInAnyFrameOrEmpty(
                                                SemanticSlotType.PLAYER_ENTITY_TYPE.getValue(),
                                                SemanticSlotEntityType.PLAYER_ENTITY_TYPE,
                                                ALLOWED_PLAYER_ENTITY_TYPES))))
                .test(request);
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.AUDIO_PLAYER_INTENT_TO_SKILL_PROXY;
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

        Optional<ActivationSourceType> activationSourceType = request.getStateO()
                .flatMap(DialogovoState::getSessionO)
                .map(Session::getActivationSourceType)
                .filter(source -> source != ActivationSourceType.UNDETECTED);

        context.getAnalytics()
                .setIntent(Intents.AUDIO_PLAYER_SKILL_USER_REQUEST);

        String originalUtterance = "";
        String normalizedUtterance = "";
        if (request.getInput() instanceof Input.Text) {
            Input.Text input = (Input.Text) request.getInput();
            originalUtterance = input.getOriginalUtterance();
            normalizedUtterance = input.getNormalizedUtterance();
        }

        return new ApplyNeededResponse(RequestSkillApplyArguments.create(
                skillInfo.getId(),
                Optional.of(normalizedUtterance),
                Optional.of(originalUtterance),
                activationSourceType),
                FeaturesUtilKt.capturePlayer(request));
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
