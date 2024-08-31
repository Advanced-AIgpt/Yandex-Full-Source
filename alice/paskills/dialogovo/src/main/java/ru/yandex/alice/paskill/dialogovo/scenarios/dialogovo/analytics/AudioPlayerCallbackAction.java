package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import javax.annotation.Nonnull;

import lombok.Getter;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction;
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TAudioPlayerCallbackActionPayload;
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TAudioPlayerCallbackActionPayload.TCallbackType;
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TAction;

public class AudioPlayerCallbackAction extends AnalyticsInfoAction {

    private final CallbackType callbackType;

    public AudioPlayerCallbackAction(CallbackType callbackType) {
        super("external_skill.audio_player.control", callbackType.type,
                String.format("Событие [%s] от аудио плейера", callbackType.type));
        this.callbackType = callbackType;
    }

    @Nonnull
    @Override
    protected TAction.Builder fillProtoField(@Nonnull TAction.Builder protoBuilder) {
        return protoBuilder.setAudioPlayerCallbackAction(TAudioPlayerCallbackActionPayload.newBuilder()
                .setCallbackType(mapAudioPlayerCallbackType(callbackType))
        );
    }

    private static TCallbackType mapAudioPlayerCallbackType(CallbackType callbackType) {
        switch (callbackType) {
            case ON_STARTED:
                return TCallbackType.OnStarted;
            case ON_STOPPED:
                return TCallbackType.OnStopped;
            case ON_FINISHED:
                return TCallbackType.OnFinished;
            case ON_FAILED:
                return TCallbackType.OnFailed;
            case ON_GET_NEXT:
                return TCallbackType.OnGetNext;
            default:
                return TCallbackType.UNRECOGNIZED;
        }
    }

    @Getter
    public enum CallbackType {
        ON_STARTED("on_started"),
        ON_FINISHED("on_finished"),
        ON_STOPPED("on_stopped"),
        ON_FAILED("on_failed"),
        ON_GET_NEXT("on_get_next");

        private final String type;

        CallbackType(String type) {
            this.type = type;
        }
    }
}
