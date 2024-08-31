package ru.yandex.quasar.billing.config;


import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
public class UniversalProviderConfig {
    private String baseUrl;
    private String clientId;
    private String socialAPIServiceName;
    private long connectionTimeoutMs;
    private long readTimeoutMs;
}
