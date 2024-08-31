package ru.yandex.quasar.billing.services.mediabilling;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;


public record BindCardResult(
        @Nonnull String status,
        @Nullable Integer bindingCardId,
        @Nullable String bindingUrl,
        @Nullable String purchaseToken
) {

    public record Wrapper(
            InvocationInfo invocationInfo,
            BindCardResult result
    ) {
    }

}
