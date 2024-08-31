package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.List;
import java.util.Map;
import java.util.Objects;

import javax.annotation.Nullable;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.directive.SetSoundLevelDirective;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameSlot;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames;
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.SessionAnalyticsInfoObject;

import static ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType.SOUND_LEVEL;
import static ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames.SET_SOUND_LEVEL;

@Component
public class SetSoundLevelProcessor extends SoundProcessor {

    private static final Logger logger = LogManager.getLogger();

    private static final Map<String, Integer> VOLUME_MAP = Map.of(
            "minimum", MIN_SOUNDING_VOLUME,
            "very_quiet", VERY_QUIET_VOLUME,
            "quiet", QUIET_VOLUME,
            "middle", MID_VOLUME,
            "high", HIGH_VOLUME,
            "very_high", VERY_HIGH_VOLUME,
            "maximum", MAX_VOLUME
    );

    protected SetSoundLevelProcessor(SuggestButtonFactory suggestButtonFactory,
                                     SkillProvider skillProvider, Phrases phrases) {
        super(SemanticFrames.SET_SOUND_LEVEL, RunRequestProcessorType.SET_SOUND_LEVEL,
                suggestButtonFactory, skillProvider, phrases);
    }


    @Override
    public boolean canProcess(MegaMindRequest<DialogovoState> request) {
        return super.canProcess(request) &&
                request.getSemanticFrameO(SET_SOUND_LEVEL)
                        .map(semanticFrame -> semanticFrame.getFirstSlot(SOUND_LEVEL.getValue()))
                        .filter(slot -> "fst.num".equals(slot.getTypeO().orElse(null)) ||
                                "custom.volume_setting".equals(slot.getTypeO().orElse(null)))
                        .isPresent();
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        SemanticFrame semanticFrame = request.getSemanticFrameO(SET_SOUND_LEVEL).get();
        SemanticFrameSlot slot = Objects.requireNonNull(semanticFrame.getFirstSlot(SOUND_LEVEL.getValue()));

        context.getAnalytics().setIntent(Intents.EXTERNAL_SKILL_SOUND_SET_LEVEL)
                .addObject(new SkillAnalyticsInfoObject(getCurrentSkill(request)))
                .addObject(new SessionAnalyticsInfoObject(
                        request.getStateO()
                                .flatMap(DialogovoState::getSessionO)
                                .map(Session::getSessionId)
                                .orElse(""),
                        request.getStateO()
                                .flatMap(DialogovoState::getSessionO)
                                .map(Session::getActivationSourceType)
                                .orElse(ActivationSourceType.UNDETECTED))
                );

        var layoutBuilder = Layout.builder()
                .shouldListen(true)
                .suggests(List.of(suggestButtonFactory.getStopSuggest(request.getAssistantName())));

        @Nullable
        String type = slot.getTypeO().orElse(null);
        int level;
        if ("fst.num".equals(type)) {
            level = Integer.parseInt(slot.getValue());
            if (level < MIN_VOLUME || level > MAX_VOLUME) {
                logger.info("Sound level out of range: {}", level);

                context.getAnalytics().setIntent(Intents.EXTERNAL_SKILL_SOUND_SET_LEVEL_INCORRECT);
                var text = phrases.getRandom("sound_level.level_out_of_range", request.getRandom());
                return new RunOnlyResponse<>(new ScenarioResponseBody<>(
                        layoutBuilder.textCard(text).outputSpeech(text).build(),
                        request.getStateO(), context.getAnalytics().toAnalyticsInfo(), true));

            }
        } else if ("custom.volume_setting".equals(type)) {
            if (VOLUME_MAP.containsKey(slot.getValue())) {
                level = VOLUME_MAP.get(slot.getValue());
            } else {
                context.getAnalytics().setIntent(Intents.EXTERNAL_SKILL_SOUND_SET_LEVEL_INCORRECT);
                logger.warn("Unknown volume settings slot type value: {}", slot.getValue());

                var text = phrases.getRandom("sound_level.level_out_of_range", request.getRandom());
                return new RunOnlyResponse<>(
                        new ScenarioResponseBody<>(
                                layoutBuilder.textCard(text).outputSpeech(text).build(),
                                request.getStateO(),
                                context.getAnalytics().toAnalyticsInfo(),
                                true));
            }

        } else {
            // never happens as we already check type in #canProcess
            logger.warn("Unknown sound level slot type: {}", slot.getTypeO().orElse("null"));
            throw new RuntimeException("unknown sound level slot type: " + slot.getTypeO().orElse("null"));
        }


        Layout layout = layoutBuilder.directives(List.of(SetSoundLevelDirective.create(level)))
                .outputSpeech("")
                .textCard("")
                .build();
        return new RunOnlyResponse<>(new ScenarioResponseBody<>(layout, request.getStateO(),
                context.getAnalytics().toAnalyticsInfo(), true));

    }

}
