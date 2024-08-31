package ru.yandex.quasar.billing.config;

import lombok.Getter;

@Getter
public class LaasConfig {
    String serviceName;
    private String url;
    private int retryLimit;
    private int readTimeout;
    private int connectTimeout;
}
