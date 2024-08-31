package ru.yandex.quasar.billing.services.processing.trust;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.List;

import ru.yandex.quasar.billing.services.processing.TrustCurrency;

public interface TrustBillingClientTestUtils {
    default TrustPaymentShortInfo clearedPayment(String card, BigDecimal amount) {
        return new TrustPaymentShortInfo(null, null, "cleared",
                card, "MasterCard",
                amount, TrustCurrency.RUB,
                Instant.now(),
                card,
                Instant.now(),
                List.of(new TrustPaymentShortInfo.TrustOrderInfo("orderId", amount)));
    }

    default TrustPaymentShortInfo clearedPayment(String card, BigDecimal amount, Instant paymentTs) {
        return new TrustPaymentShortInfo(null, null, "cleared", card, "MasterCard", amount, TrustCurrency.RUB,
                paymentTs,
                card, Instant.now(), List.of(new TrustPaymentShortInfo.TrustOrderInfo("orderId", amount)));
    }
}
