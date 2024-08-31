package ru.yandex.alice.paskill.dialogovo.external.v1.request.audio;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.InputType;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.RequestBase;

public class AudioPlayerEventRequest extends RequestBase {

    public AudioPlayerEventRequest(InputType type) {
        super(type);
    }
}
