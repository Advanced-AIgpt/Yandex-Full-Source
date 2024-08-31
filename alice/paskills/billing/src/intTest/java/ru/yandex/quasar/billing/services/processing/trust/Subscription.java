package ru.yandex.quasar.billing.services.processing.trust;

import java.math.BigDecimal;
import java.time.LocalDateTime;
import java.time.ZoneId;

import javax.annotation.Nullable;

import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
public class Subscription extends Order {

    @Nullable
    private volatile LocalDateTime finishTs;
    private volatile LocalDateTime subsUntilTs;

    Subscription(String orderId, String uid, CreateProductRequest product, String region, BigDecimal price,
                 LocalDateTime subsUntilTs) {
        super(orderId, uid, product, region, price, BigDecimal.ONE, product.getFiscalNds(), product.getName(), null,
                true);
        this.finishTs = null;
        this.subsUntilTs = subsUntilTs;
    }

    public static Subscription createSubscription(String orderId, String uid, CreateProductRequest product,
                                                  LocalDateTime subsUntilTs) {
        return new Subscription(orderId, uid, product, String.valueOf(product.getPrices().get(0).getRegionId()),
                product.getPrices().get(0).getPrice(), subsUntilTs);
    }

    public String getSubscriptionId() {
        return getOrderId();
    }

    SubscriptionShortInfo toSubscriptionShortInfo() {
        return new SubscriptionShortInfo("success",
                subsUntilTs.atZone(ZoneId.systemDefault()).toInstant(),
                finishTs != null ? finishTs.atZone(ZoneId.systemDefault()).toInstant() : null);
    }

    Subscription initializeSubscription() {
        this.initializeOrder(getProduct().getPrices().get(0).getPrice(), BigDecimal.ONE, getProduct().getFiscalNds(),
                getProduct().getFiscalTitle(), null);
        return this;
    }
}
