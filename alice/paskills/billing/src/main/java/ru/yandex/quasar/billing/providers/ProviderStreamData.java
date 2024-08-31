package ru.yandex.quasar.billing.providers;

import javax.annotation.Nullable;

import lombok.Data;

@Data
public class ProviderStreamData {
    private final String url;
    @Nullable
    private final String payload;
}
