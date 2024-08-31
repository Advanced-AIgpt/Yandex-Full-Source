package ru.yandex.quasar.billing.providers;

import javax.annotation.Nullable;

public interface IProvider {
    /**
     * Provider name
     * Must be constant for a given provider
     */
    String getProviderName();

    /**
     * Get application_name for social API requests
     */
    String getSocialAPIServiceName();

    /**
     * Get client_id for social API authorization. Used by Yandex app to open native authorization screen
     */
    @Nullable
    String getSocialAPIClientId();

    /**
     * Get provider's priority to sort them in list
     */
    Integer getPriority();

    /**
     * is provider visible in Yandex app webview home screan
     */
    boolean showInYandexApp();
}
