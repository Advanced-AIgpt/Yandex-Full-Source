package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.InputType;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerEventRequest;
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.AudioPlayerCallbackAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.OnPlayFinishedCallbackDirective;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;

@Component
public class ProcessOnAudioPlayFinishedCallbackProcessor
        extends CommonAudioPlayerEventCallbackProcessor<OnPlayFinishedCallbackDirective> {

    public ProcessOnAudioPlayFinishedCallbackProcessor(SkillProvider skillProvider,
                                                       SkillRequestProcessor processor,
                                                       AppMetricaEventSender appMetricaEventSender,
                                                       ObjectMapper objectMapper) {
        super(skillProvider, processor, appMetricaEventSender, objectMapper);
    }

    @Override
    protected Class<OnPlayFinishedCallbackDirective> getCallbackClazz() {
        return OnPlayFinishedCallbackDirective.class;
    }

    @Override
    AudioPlayerCallbackAction.CallbackType getCallbackAnalyticsAction() {
        return AudioPlayerCallbackAction.CallbackType.ON_FINISHED;
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.ON_AUDIO_PLAY_FINISHED;
    }

    @Override
    protected AudioPlayerEventRequest getAudioPlayerEvent() {
        return new AudioPlayerEventRequest(InputType.AUDIO_PLAYER_PLAYBACK_FINISHED);
    }
}