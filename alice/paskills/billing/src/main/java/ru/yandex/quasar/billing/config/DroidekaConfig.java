package ru.yandex.quasar.billing.config;

import lombok.Data;

@Data
public class DroidekaConfig {
    private final String url;
    private final int retries;
    private final int connectionTimeout;
    private final int readTimeout;
}
