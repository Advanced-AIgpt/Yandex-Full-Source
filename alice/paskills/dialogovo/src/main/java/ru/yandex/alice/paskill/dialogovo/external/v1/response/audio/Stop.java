package ru.yandex.alice.paskill.dialogovo.external.v1.response.audio;

import lombok.Data;
import lombok.EqualsAndHashCode;

@Data
@EqualsAndHashCode(callSuper = true)
public class Stop extends AudioPlayerAction {

    @Override
    public AudioPlayerActionType getAction() {
        return AudioPlayerActionType.STOP;
    }
}
