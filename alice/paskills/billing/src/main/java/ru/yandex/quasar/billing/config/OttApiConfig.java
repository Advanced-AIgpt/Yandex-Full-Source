package ru.yandex.quasar.billing.config;

import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
public class OttApiConfig {
    private String apiBaseUrl;
    // Mediabilling uses production trust to get card list
    private String trustBaseApi;
}
