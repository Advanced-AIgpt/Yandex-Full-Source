package ru.yandex.quasar.billing.services.processing;

import java.util.Map;
import java.util.Objects;

import ru.yandex.quasar.billing.beans.PaymentProcessor;

public class ProcessingServiceManager {
    private final Map<PaymentProcessor, ProcessingService> paymentProcessorsMap;

    public ProcessingServiceManager(Map<PaymentProcessor, ProcessingService> paymentProcessorsMap) {
        this.paymentProcessorsMap = paymentProcessorsMap;
    }

    public ProcessingService get(PaymentProcessor paymentProcessor) {
        return Objects.requireNonNull(paymentProcessorsMap.get(paymentProcessor));
    }
}
