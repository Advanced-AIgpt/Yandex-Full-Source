package ru.yandex.quasar.billing.services.processing.trust;

import lombok.Data;

@Data
public class NewBindingInfo {
    private final String bindingUrl;
    private final String purchaseToken;
}
