package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerError;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerEventRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.audio.AudioPlayerFailedRequest;
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.AudioPlayerCallbackAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.OnPlayFailedCallbackDirective;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaEventSender;

@Component
public class ProcessOnAudioPlayFailedCallbackProcessor
        extends CommonAudioPlayerEventCallbackProcessor<OnPlayFailedCallbackDirective> {

    public ProcessOnAudioPlayFailedCallbackProcessor(SkillProvider skillProvider,
                                                     SkillRequestProcessor processor,
                                                     AppMetricaEventSender appMetricaEventSender,
                                                     ObjectMapper objectMapper) {
        super(skillProvider, processor, appMetricaEventSender, objectMapper);
    }

    @Override
    protected Class<OnPlayFailedCallbackDirective> getCallbackClazz() {
        return OnPlayFailedCallbackDirective.class;
    }

    @Override
    AudioPlayerCallbackAction.CallbackType getCallbackAnalyticsAction() {
        return AudioPlayerCallbackAction.CallbackType.ON_FAILED;
    }

    @Override
    public RunRequestProcessorType getType() {
        return RunRequestProcessorType.ON_AUDIO_PLAY_FAILED;
    }

    @Override
    protected AudioPlayerEventRequest getAudioPlayerEvent() {
        // no error detalisation yet from protocol
        return new AudioPlayerFailedRequest(AudioPlayerError.DEFAULT);
    }
}
