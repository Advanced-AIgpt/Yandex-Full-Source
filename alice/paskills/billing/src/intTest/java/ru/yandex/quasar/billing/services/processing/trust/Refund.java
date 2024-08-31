package ru.yandex.quasar.billing.services.processing.trust;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Data;

@Data
public class Refund {
    @Nonnull
    private final String refundId;
    @Nonnull
    private RefundStatus refundStatus = RefundStatus.wait_for_notification;
    @Nullable
    private String fiscalUrl;
}
