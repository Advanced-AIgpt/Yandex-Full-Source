package ru.yandex.quasar.billing.providers;

import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonRawValue;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;

import ru.yandex.quasar.billing.controller.BillingController;

@Data
@AllArgsConstructor(access = AccessLevel.PACKAGE)
public class StreamData {
    public static final StreamData EMPTY = new StreamData(null, null);
    @Nullable
    private final String url;
    @JsonRawValue
    @JsonDeserialize(using = BillingController.AnythingToString.class)
    @Nullable
    private final String payload;

    public static StreamData byUrl(String url) {
        return new StreamData(url, null);
    }

    public static StreamData create(String url, String payload) {
        Objects.requireNonNull(url, "URL must be specified");
        Objects.requireNonNull(payload, "payload must be specified");
        return new StreamData(url, payload);
    }
}
