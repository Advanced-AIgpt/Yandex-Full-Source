package ru.yandex.quasar.billing.providers.universal;

import java.util.Optional;

import com.fasterxml.jackson.annotation.JsonValue;

enum ContentItemType {
    MOVIE("movie"),
    TV_SHOW("tv_show"),
    TV_SHOW_SEASON("tv_show_season"),
    TV_SHOW_EPISODE("tv_show_episode"),
    ANTHOLOGY_MOVIE("anthology_movie"),
    TRAILER("trailer");

    private final String code;

    ContentItemType(String code) {
        this.code = code;
    }

    //@JsonCreator
    public static Optional<ContentItemType> getByCode(String code) {
        for (ContentItemType value : ContentItemType.values()) {
            if (value.getCode().equals(code)) {
                return Optional.of(value);
            }
        }
        return Optional.empty();
    }

    @JsonValue
    public String getCode() {
        return code;
    }

}
