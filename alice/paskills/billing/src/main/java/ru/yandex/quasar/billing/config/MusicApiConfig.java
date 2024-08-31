package ru.yandex.quasar.billing.config;

import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
public class MusicApiConfig {
    private String apiUrl;
    private String mediabillingUrl;
    private int retries;
    private String trustBaseApi;
    private long trustConnectTimeout;
    private long trustTimeout;
    private boolean useTrustToken;
}
