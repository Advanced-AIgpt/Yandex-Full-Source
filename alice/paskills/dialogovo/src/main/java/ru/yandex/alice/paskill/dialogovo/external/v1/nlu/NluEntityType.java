package ru.yandex.alice.paskill.dialogovo.external.v1.nlu;

import com.fasterxml.jackson.annotation.JsonValue;
import lombok.Getter;

public enum NluEntityType {
    GEO("YANDEX.GEO"),
    FIO("YANDEX.FIO"),
    NUMBER("YANDEX.NUMBER"),
    DATETIME("YANDEX.DATETIME"),
    STRING("YANDEX.STRING");

    @Getter
    @JsonValue
    private final String code;

    NluEntityType(String code) {
        this.code = code;
    }
}
