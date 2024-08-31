package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import lombok.Data;

@Data
public class RequestBase {
    private final InputType type;

    public RequestBase(InputType type) {
        this.type = type;
    }
}
