package ru.yandex.alice.paskill.dialogovo.external.v1.response.show;

import com.fasterxml.jackson.annotation.JsonValue;

public enum ShowType {
    MORNING("MORNING");

    private final String code;

    ShowType(String code) {
        this.code = code;
    }

    @JsonValue
    public String getCode() {
        return code;
    }
}
