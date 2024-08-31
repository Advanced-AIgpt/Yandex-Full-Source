package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import com.fasterxml.jackson.annotation.JsonValue;

public enum FilteringMode {
    NO_FILTER("NO_FILTER"),
    MODERATE("MODERATE"),
    SAFE("SAFE");

    private final String code;

    FilteringMode(String code) {
        this.code = code;
    }

    @JsonValue
    public String getCode() {
        return code;
    }
}
