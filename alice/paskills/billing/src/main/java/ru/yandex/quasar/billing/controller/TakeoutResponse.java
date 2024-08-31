package ru.yandex.quasar.billing.controller;

import java.util.Map;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;

/**
 * Format specification
 * https://wiki.yandex-team.ru/passport/takeout/integration/
 */
@Data
@AllArgsConstructor(access = AccessLevel.PRIVATE)
class TakeoutResponse {
    @Nonnull
    private final Status status;
    @JsonInclude(JsonInclude.Include.NON_EMPTY)
    @Nullable
    private final String error;
    @JsonInclude(JsonInclude.Include.NON_EMPTY)
    private final Map<String, String> data;

    public static TakeoutResponse noData() {
        return new TakeoutResponse(Status.no_data, null, null);
    }

    public static TakeoutResponse error(String errorText) {
        return new TakeoutResponse(Status.error, errorText, null);
    }

    public static TakeoutResponse ok(Map<String, String> data) {
        return new TakeoutResponse(Status.ok, null, data);
    }

    enum Status {
        ok,
        no_data,
        pending,
        error
    }
}
