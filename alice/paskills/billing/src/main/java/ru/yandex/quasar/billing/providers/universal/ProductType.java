package ru.yandex.quasar.billing.providers.universal;

import java.util.Optional;

import com.fasterxml.jackson.annotation.JsonValue;

public enum ProductType {
    MOVIE("movie"),
    SUBSCRIPTION("subscription"),
    TV_SHOW("tv_show"),
    TV_SHOW_SEASON("tv_show_season"),
    TV_SHOW_EPISODE("tv_show_episode"),
    ANTHOLOGY_MOVIE("anthology_movie"),
    BUNDLE("bundle");

    private final String code;

    ProductType(String code) {
        this.code = code;
    }

    public static Optional<ProductType> getByCode(String code) {
        for (ProductType value : ProductType.values()) {
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
