package ru.yandex.alice.paskill.dialogovo.scenarios.news.domain;

import com.fasterxml.jackson.annotation.JsonValue;
import lombok.Getter;

import ru.yandex.alice.kronstadt.core.utils.StringEnum;
import ru.yandex.alice.kronstadt.core.utils.StringEnumResolver;

public enum FlashBriefingType implements StringEnum {
    UNKNOWN("unknown"),
    FACTS("facts"),
    // todo: rename to radionews
    RADIONEWS("news"),
    TEXT_NEWS("text_news");

    public static final StringEnumResolver<FlashBriefingType> R =
            StringEnumResolver.resolver(FlashBriefingType.class);

    @Getter
    @JsonValue
    private final String value;

    FlashBriefingType(String value) {
        this.value = value;
    }

    @Override
    public String value() {
        return value;
    }
}
