package ru.yandex.quasar.billing.config;

import java.util.Map;

import lombok.Getter;
import lombok.Setter;

@Setter
@Getter
public class TrustBillingConfig {

    private String apiBaseUrl;

    private String callbackBaseUrl;

    private Map<String, Long> providersMapping;

    private int maxRetriesCount;

    private String tvmClientId;

    private int serviceId;

    private long connectTimeout;

    private long timeout;

    private String serviceToken;
}
