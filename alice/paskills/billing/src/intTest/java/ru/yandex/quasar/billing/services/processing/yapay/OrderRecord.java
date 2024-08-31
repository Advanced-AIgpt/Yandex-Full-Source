package ru.yandex.quasar.billing.services.processing.yapay;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.List;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;

import ru.yandex.quasar.billing.services.processing.TrustCurrency;


@Builder
@Data
@AllArgsConstructor(access = AccessLevel.PRIVATE)
public class OrderRecord {

    @Nonnull
    private final Long serviceMerchantId;

    private final long orderId;
    @Nonnull
    private final String userEmail;
    @Nonnull
    private final BigDecimal price;
    @Nullable
    @JsonInclude(JsonInclude.Include.NON_NULL)
    private final String description;
    private final String receiptUrl;
    @Nonnull
    private final String kind;
    @Nonnull
    private final TrustCurrency currency;
    @Nullable
    private final Instant closed;
    @Nonnull
    private final List<OrderItem> items;
    @Nonnull
    private final Instant created;
    @Nonnull
    private final String caption;
    // merchant uid
    @Nonnull
    private final Long uid;
    @Nullable
    @JsonInclude(JsonInclude.Include.NON_NULL)
    private final String userDescription;
    @Builder.Default
    private boolean active = true;
    private boolean verified;
    @Nonnull
    private String payStatus;
    @Nonnull
    @Builder.Default
    private Integer revision = 1;
    private Instant updated;

    Order toOrder() {
        return Order.builder()
                .active(true)
                .items(items)
                .orderId(orderId)
                .verified(false)
                .userEmail(userEmail)
                .price(price)
                .description(description)
                .receiptUrl(receiptUrl)
                .kind(kind)
                .currency(currency)
                .payStatus(Order.Status.forCode(payStatus))
                .closed(closed)
                .created(created)
                .caption(caption)
                .revision(revision)
                .uid(uid)
                .userDescription(userDescription)
                .updated(updated)
                .build();
    }
}
