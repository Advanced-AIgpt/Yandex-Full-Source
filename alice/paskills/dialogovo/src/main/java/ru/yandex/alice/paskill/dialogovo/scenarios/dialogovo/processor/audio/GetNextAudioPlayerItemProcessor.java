package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio;

import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.ApplyNeededResponse;
import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ApplyingRunProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.RequestSkillApplyArguments;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.GetNextAudioPlayerItemCallbackDirective;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.MegaMindRequestSkillApplier;

import static java.util.function.Predicate.not;

@Component
public class GetNextAudioPlayerItemProcessor implements ApplyingRunProcessor<DialogovoState,
        RequestSkillApplyArguments> {
    private static final Logger logger = LogManager.getLogger();

    private final SkillProvider skillProvider;
    private final MegaMindRequestSkillApplier skillApplier;

    public GetNextAudioPlayerItemProcessor(SkillProvider skillProvider, MegaMindRequestSkillApplier skillApplier) {
        this.skillProvider = skillProvider;
        this.skillApplier = skillApplier;
    }

    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return onCallbackDirective(GetNextAudioPlayerItemCallbackDirective.class)
                .and(HAS_SKILL_PLAYER_OWNER)
                //TODO: use not(IS_IN_SKILL) only after player modality enables
                .and(not(IS_IN_SKILL).or(IS_IN_SKILL.and(IS_IN_PLAYER_OWNER_SKILL)))
                .test(request);
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.GET_NEXT_AUDIO_PLAYER_ITEM;
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        var directive = request.getInput().getDirective(GetNextAudioPlayerItemCallbackDirective.class);

        // provided by SKILL_PLAYER_OWNER
        String skillIdFromDeviceState = request.getAudioPlayerOwnerSkillIdO().get();
        String callbackInputSkillId = directive.getSkillId();

        if (!skillIdFromDeviceState.equals(callbackInputSkillId)) {
            logger.info("Player skill owner in device_state: [{}] differs from callback: [{}]",
                    skillIdFromDeviceState, callbackInputSkillId);
            return DefaultIrrelevantResponse.create(Intents.EXTERNAL_SKILL_IRRELEVANT, request.getRandom(),
                    request.isVoiceSession());
        }

        Optional<SkillInfo> skillO = skillProvider.getSkill(callbackInputSkillId);
        if (skillO.isEmpty()) {
            logger.info("Unable to detect skill id [{}] from audio player callback", callbackInputSkillId);
            return DefaultIrrelevantResponse.create(Intents.EXTERNAL_SKILL_IRRELEVANT, request.getRandom(),
                    request.isVoiceSession());
        }
        SkillInfo skillInfo = skillO.get();

        Optional<ActivationSourceType> activationSourceType = request.getStateO()
                .flatMap(DialogovoState::getSessionO)
                .map(Session::getActivationSourceType)
                .filter(source -> source != ActivationSourceType.UNDETECTED);

        return new ApplyNeededResponse(RequestSkillApplyArguments.create(
                skillInfo.getId(),
                Optional.empty(),
                Optional.empty(),
                activationSourceType));
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
