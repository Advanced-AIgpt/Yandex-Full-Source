package ru.yandex.quasar.billing.services;

import java.text.MessageFormat;

public class ProviderCallbackAlreadyRegisteredException extends RuntimeException {
    public ProviderCallbackAlreadyRegisteredException(String providerName) {
        super(MessageFormat.format("Callback for provider {0} already registered", providerName));
    }
}
