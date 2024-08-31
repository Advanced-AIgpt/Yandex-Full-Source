package ru.yandex.quasar.billing.config;

import lombok.Data;

@Data
public class QuasarBackendConfig {
    private String backendUrl;
    private int retries;
    private int connectionTimeout;
    private int readTimeout;
}
