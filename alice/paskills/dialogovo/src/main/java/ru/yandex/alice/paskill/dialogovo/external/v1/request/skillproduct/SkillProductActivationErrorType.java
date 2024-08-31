package ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct;

import com.fasterxml.jackson.annotation.JsonValue;

import ru.yandex.alice.kronstadt.core.utils.StringEnum;

public enum SkillProductActivationErrorType implements StringEnum {
    MUSIC_NOT_RECOGNIZED("music_not_recognized"),
    MUSIC_NOT_PLAYING("music_not_playing"),
    MEDIA_ERROR_UNKNOWN("media_error_unknown");

    private final String code;

    SkillProductActivationErrorType(String code) {
        this.code = code;
    }

    @Override
    @JsonValue
    public String value() {
        return code;
    }
}
