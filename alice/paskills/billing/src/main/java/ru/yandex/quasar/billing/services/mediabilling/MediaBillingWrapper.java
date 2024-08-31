package ru.yandex.quasar.billing.services.mediabilling;

import javax.annotation.Nullable;

import lombok.Data;

@Data
public class MediaBillingWrapper<T> {
    private final InvocationInfo invocationInfo;
    @Nullable
    private final T result;
}
