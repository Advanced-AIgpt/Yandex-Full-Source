package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import javax.annotation.Nonnull;

import lombok.Getter;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction;
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TAudioPlayerControlActionPayload;
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TAudioPlayerControlActionPayload.TActionType;
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TAction;

public class AudioPlayerControlAction extends AnalyticsInfoAction {

    private final AudioPlayerControlActionType actionType;

    public AudioPlayerControlAction(AudioPlayerControlActionType actionType) {
        super("external_skill.audio_player.control", actionType.type,
                String.format("Управление аудио плейером - команда %s", actionType.type));
        this.actionType = actionType;
    }

    public AudioPlayerControlActionType getActionType() {
        return actionType;
    }

    @Nonnull
    @Override
    protected TAction.Builder fillProtoField(@Nonnull TAction.Builder protoBuilder) {
        return protoBuilder.setAudioPlayerControlAction(TAudioPlayerControlActionPayload.newBuilder()
                .setActionType(mapAudioPlayerActionType(actionType))
        );
    }

    @Getter
    public enum AudioPlayerControlActionType {
        PLAY("play"),
        STOP("stop"),
        REWIND("rewind");

        private final String type;

        AudioPlayerControlActionType(String type) {
            this.type = type;
        }
    }

    private static TActionType mapAudioPlayerActionType(AudioPlayerControlActionType actionType) {
        switch (actionType) {
            case PLAY:
                return TActionType.Play;
            case STOP:
                return TActionType.Stop;
            case REWIND:
                return TActionType.Rewind;
            default:
                return TActionType.UNRECOGNIZED;
        }
    }
}
