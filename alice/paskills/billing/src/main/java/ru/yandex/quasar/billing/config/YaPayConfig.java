package ru.yandex.quasar.billing.config;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
@AllArgsConstructor
public class YaPayConfig {
    @Nonnull
    private String apiBaseUrl;
    private int retryLimit;
}
