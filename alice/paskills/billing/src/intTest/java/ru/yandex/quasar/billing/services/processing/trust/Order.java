package ru.yandex.quasar.billing.services.processing.trust;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;

import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.Setter;
import lombok.ToString;

import ru.yandex.quasar.billing.services.processing.NdsType;

@Getter
@Setter
@ToString
@EqualsAndHashCode(exclude = "purchases")
public class Order {
    private final String orderId;
    private final String uid;
    private final CreateProductRequest product;
    private final List<Purchase> purchases = new ArrayList<>();
    @Nullable
    private volatile String region;
    @Nullable
    private volatile BigDecimal price;
    @Nullable
    private volatile BigDecimal quantity;

    @Nullable
    private volatile NdsType nds;
    @Nullable
    private volatile String title;
    @Nullable
    private volatile String fiscalInn;
    private volatile boolean initialized;

    @SuppressWarnings("ParameterNumber")
    Order(String orderId, String uid, CreateProductRequest product, @Nullable String region,
          @Nullable BigDecimal price, @Nullable BigDecimal quantity, @Nullable NdsType nds, @Nullable String title,
          @Nullable String fiscalInn, boolean initialized) {
        this.orderId = orderId;
        this.uid = uid;
        this.product = product;
        this.region = region;
        this.price = price;
        this.quantity = quantity;
        this.nds = nds;
        this.title = title;
        this.fiscalInn = fiscalInn;
        this.initialized = initialized;
    }


    static Order createSingletonOrder(String orderId, String uid, CreateProductRequest product, String region,
                                      BigDecimal price, NdsType fiscalNds, String fiscalTitle) {
        return new Order(orderId, uid, product,
                region,
                price,
                BigDecimal.ONE,
                Objects.requireNonNull(fiscalNds),
                Objects.requireNonNull(fiscalTitle),
                null,
                true
        );
    }

    public static Order createOrderWithoutPrice(String orderId, String uid, CreateProductRequest product) {
        return new Order(orderId, uid, product, "225", null, null, null, null, null, false);
    }

    Order initializeOrder(BigDecimal price, BigDecimal quantity, NdsType fiscalNds, String fiscalTitle,
                          @Nullable String fiscalInn) {
        this.price = Objects.requireNonNull(price);
        this.quantity = Objects.requireNonNull(quantity);
        this.nds = Objects.requireNonNull(fiscalNds);
        this.title = Objects.requireNonNull(fiscalTitle);
        this.fiscalInn = fiscalInn;

        return this;
    }
}
