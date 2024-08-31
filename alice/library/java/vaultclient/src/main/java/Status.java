package ru.yandex.alice.vault;

import java.text.MessageFormat;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonValue;

public enum Status {
    OK("ok"),
    WARNING("warning"),
    ERROR("error");
    private final String value;

    Status(String value) {
        this.value = value;
    }

    @JsonCreator
    public static Status fromString(String value) {
        for (Status candidate : Status.values()) {
            if (candidate.value.equals(value)) {
                return candidate;
            }
        }
        throw new IllegalArgumentException(MessageFormat.format("Unknown value {0} for enum Status", value));
    }

    @JsonValue
    public String getValue() {
        return value;
    }
}
