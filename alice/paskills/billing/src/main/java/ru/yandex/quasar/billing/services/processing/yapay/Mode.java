package ru.yandex.quasar.billing.services.processing.yapay;


import com.fasterxml.jackson.annotation.JsonValue;

public enum Mode {
    PROD("prod"),
    TEST("test");

    @JsonValue
    private final String code;

    Mode(String code) {
        this.code = code;
    }

    public String getCode() {
        return code;
    }
}
