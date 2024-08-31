package ru.yandex.quasar.billing.services.processing.trust;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.List;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Data;

import ru.yandex.quasar.billing.services.processing.TrustCurrency;

import static java.util.stream.Collectors.toList;

@Data
@Builder(toBuilder = true)
public class Purchase {
    private final String purchaseToken;
    private final String uid;
    private final String productId;
    private final TrustCurrency currency;
    private final String paymethodId;
    private final String userEmail;
    private final String callBackUrl;
    private final String commissionCategory;
    private final String trustPaymentId;
    private final List<Order> orders;
    @Nullable
    private final String submerchantId;
    private PaymentStatus paymentStatus;
    @Nullable
    private PaymentRespStatus paymentRespStatus;
    private PaymentState status;
    @Nullable
    private volatile Instant paymentTs;
    @Nullable
    private volatile Refund refund;
    @Nullable
    private volatile Instant clearDate;

    public List<Subscription> getSubscriptions() {
        return orders.stream()
                .filter(it -> it instanceof Subscription)
                .map(Subscription.class::cast)
                .collect(toList());
    }

    public BigDecimal getAmount() {
        return !"trial_payment".equals(paymethodId) ? orders.stream()
                .map(order -> order.getPrice().multiply(order.getQuantity()))
                .reduce(BigDecimal.ZERO, BigDecimal::add) : BigDecimal.ZERO;
    }

    public enum PaymentState {
        created,
        started,
        success
    }

    public enum PaymentStatus {
        cleared,
        authorized,
        not_authorized,
        canceled
    }

}
