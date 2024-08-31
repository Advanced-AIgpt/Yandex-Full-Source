package ru.yandex.quasar.billing.config;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;

@Getter
@Setter
@NoArgsConstructor
@AllArgsConstructor
public class ProviderConfig {
    private String socialAPIServiceName;
    @Nullable
    private String socialClientId;
    private boolean showInYandexApp;
    private Integer priority;
}
