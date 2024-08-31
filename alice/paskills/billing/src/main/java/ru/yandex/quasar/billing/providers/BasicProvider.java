package ru.yandex.quasar.billing.providers;

import javax.annotation.Nullable;

import ru.yandex.quasar.billing.config.ProviderConfig;

public abstract class BasicProvider implements IProvider {
    private final String providerName;
    private final String socialAPIServiceName;
    @Nullable
    private final String socialAPIClientId;
    private final boolean showInYandexApp;
    private final Integer priority;

    public BasicProvider(String providerName, ProviderConfig providerConfig) {
        this.providerName = providerName;
        this.socialAPIServiceName = providerConfig.getSocialAPIServiceName();
        this.socialAPIClientId = providerConfig.getSocialClientId();
        this.showInYandexApp = providerConfig.isShowInYandexApp();
        this.priority = providerConfig.getPriority();
    }

    public BasicProvider(String providerName, String socialAPIServiceName, @Nullable String socialAPIClientId,
                         boolean showInYandexApp, Integer priority) {
        this.providerName = providerName;
        this.socialAPIServiceName = socialAPIServiceName;
        this.socialAPIClientId = socialAPIClientId;
        this.showInYandexApp = showInYandexApp;
        this.priority = priority;
    }

    @Override
    public String getProviderName() {
        return providerName;
    }

    @Override
    public String getSocialAPIServiceName() {
        return socialAPIServiceName;
    }

    @Override
    @Nullable
    public String getSocialAPIClientId() {
        return socialAPIClientId;
    }

    @Override
    public boolean showInYandexApp() {
        return showInYandexApp;
    }

    @Override
    public Integer getPriority() {
        return priority;
    }
}
