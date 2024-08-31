package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.Collections;
import java.util.List;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.BaseRunResponse;
import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.RunOnlyResponse;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.directive.SoundQuieterDirective;
import ru.yandex.alice.kronstadt.core.layout.Layout;
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

@Component
public class QuieterProcessor extends SoundProcessor {
    protected QuieterProcessor(SuggestButtonFactory suggestButtonFactory,
                               SkillProvider skillProvider, Phrases phrases) {
        super(SemanticFrames.SOUND_QUIETER, RunRequestProcessorType.QUIETER,
                suggestButtonFactory, skillProvider, phrases);
    }

    @Override
    public BaseRunResponse process(Context context, MegaMindRequest<DialogovoState> request) {
        Layout layout;
        context.getAnalytics().setIntent(Intents.EXTERNAL_SKILL_SOUND_QUIETER)
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
        if (request.getDeviceStateO().get().getSoundLevel() <= MIN_VOLUME) {
            String text = phrases.getRandom("sound_level.already_min", request.getRandom());
            layout = Layout.builder()
                    .shouldListen(true)
                    .outputSpeech(text)
                    .textCard(text)
                    .directives(Collections.emptyList())
                    .suggests(List.of(suggestButtonFactory.getStopSuggest(request.getAssistantName())))
                    .build();
        } else {
            layout = Layout.builder()
                    .shouldListen(true)
                    .outputSpeech("") //TODO: move to phrases
                    .textCard("")
                    .directives(List.of(SoundQuieterDirective.INSTANCE))
                    .suggests(List.of(suggestButtonFactory.getStopSuggest(request.getAssistantName())))
                    .build();
        }
        return new RunOnlyResponse<>(new ScenarioResponseBody<>(layout, request.getStateO(),
                context.getAnalytics().toAnalyticsInfo(), true));
    }
}
