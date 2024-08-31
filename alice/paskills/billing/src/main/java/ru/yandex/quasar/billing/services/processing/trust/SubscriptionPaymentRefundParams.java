package ru.yandex.quasar.billing.services.processing.trust;

import java.math.BigDecimal;
import java.util.List;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;

@Data
@AllArgsConstructor(access = AccessLevel.PRIVATE)
public class SubscriptionPaymentRefundParams {
    @JsonProperty("purchase_token")
    @Nonnull
    private final String purchaseToken;

    @JsonProperty("reason_desc")
    @Nonnull
    private final String reason;

    @JsonInclude(JsonInclude.Include.NON_EMPTY)
    @Nullable
    private final List<Order> orders;

    static SubscriptionPaymentRefundParams subscriptionPayment(String purchaseToken, String reason, String orderId,
                                                               BigDecimal amount) {
        return new SubscriptionPaymentRefundParams(purchaseToken, reason, List.of(new Order(amount, orderId)));
    }

    static SubscriptionPaymentRefundParams singlePayment(String purchaseToken, String reason,
                                                         List<TrustPaymentShortInfo.TrustOrderInfo> ordersInfo) {
        List<Order> orders = ordersInfo.stream()
                .map(it -> new Order(it.getOrigAmount(), it.getOrderId()))
                .collect(Collectors.toList());
        return new SubscriptionPaymentRefundParams(purchaseToken, reason, orders);
    }

    @Data
    @JsonInclude(JsonInclude.Include.NON_NULL)
    public static class Order {

        @Nonnull
        @JsonProperty("delta_amount")
        @JsonFormat(shape = JsonFormat.Shape.STRING)
        private final BigDecimal deltaAmount;

        @Nonnull
        @JsonProperty("order_id")
        private final String orderId;
    }
}
