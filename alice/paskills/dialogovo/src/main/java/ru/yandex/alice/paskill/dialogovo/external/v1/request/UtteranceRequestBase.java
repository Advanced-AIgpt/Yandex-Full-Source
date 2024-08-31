package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import lombok.Getter;

import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Nlu;

@Getter
public abstract class UtteranceRequestBase extends RequestBase {
    private final Nlu nlu;

    public UtteranceRequestBase(InputType type, Nlu nlu) {
        super(type);
        this.nlu = nlu;
    }
}
