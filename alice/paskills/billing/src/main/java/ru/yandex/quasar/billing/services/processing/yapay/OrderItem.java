package ru.yandex.quasar.billing.services.processing.yapay;

import java.math.BigDecimal;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

import ru.yandex.quasar.billing.services.processing.NdsType;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;

@Data
@Builder
public class OrderItem {
    // название товара
    @Nonnull
    private final String name;
    // валюта заказа
    @Nonnull
    private final TrustCurrency currency;
    // ставка НДС
    @Nonnull
    private final NdsType nds;
    // цена
    @Nonnull
    @JsonFormat(shape = JsonFormat.Shape.STRING)
    private final BigDecimal price;
    // количество
    @Nonnull
    private final BigDecimal amount;
    @Nullable
    @JsonProperty("product_id")
    private final Long productId;
}
