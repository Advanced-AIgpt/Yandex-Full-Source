package ru.yandex.quasar.billing.services.processing.trust;

import java.math.BigDecimal;
import java.util.List;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.ser.std.ToStringSerializer;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.Getter;

import ru.yandex.quasar.billing.services.processing.NdsType;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;

@JsonInclude(JsonInclude.Include.NON_NULL)
@Getter
@AllArgsConstructor(access = AccessLevel.PRIVATE)
class CreateProductRequest {
    // внешний (по отношению к Трасту) идентификатор продукта, строка до 100 символов.
    // Должен быть уникальным внутри сервиса. ID продукта генерирует сервис и
    // передает в качестве параметра при создании продукта
    @JsonProperty("product_id")
    private final String productId;

    // идентификатор Партнера, строка с числом
    @JsonProperty("partner_id")
    private final Long partnerId;

    // название продукта
    @JsonProperty("name")
    private final String name;

    @JsonProperty("product_type")
    @Nullable
    private final ProductType productType;

    @JsonProperty("subs_period")
    @Nullable
    private final String subscriptionPeriod;

    @JsonProperty("prices")
    @Nullable
    private final List<Price> prices;

    @JsonProperty("subs_trial_period")
    @Nullable
    private final String subsTrialPeriod;

    @JsonProperty("single_purchase")
    @Nullable
    private final Integer singlePurchase;

    @JsonProperty("fiscal_nds")
    @Nullable
    private final NdsType fiscalNds;

    @JsonProperty("fiscal_title")
    @Nullable
    private final String fiscalTitle;

    static CreateProductRequest createGenericProduct(String productId, Long partnerId, String name) {
        return new CreateProductRequest(productId,
                partnerId,
                name,
                null,
                null,
                null,
                null,
                null,
                null,
                null);
    }

    static CreateProductRequest createSubscriptionProduct(String productId, Long partnerId, String name,
                                                          String subscriptionPeriod, Price price, NdsType fiscalNds,
                                                          String fiscalTitle) {
        return new CreateProductRequest(productId,
                partnerId,
                name,
                ProductType.subs,
                subscriptionPeriod,
                List.of(price),
                null,
                null,
                fiscalNds,
                fiscalTitle);
    }

    @JsonInclude(JsonInclude.Include.NON_NULL)
    @Data
    static class Price {
        @JsonProperty("currency")
        private final TrustCurrency currency;
        @JsonProperty("price")
        @JsonSerialize(using = ToStringSerializer.class)
        private final BigDecimal price;
        @JsonProperty("region_id")
        private final int regionId;
        @JsonProperty("start_ts")
        private final long startTs;

        static Price createPrice(TrustCurrency currency, BigDecimal price, int regionId, long startTs) {
            return new Price(currency, price, regionId, startTs);
        }
    }
}
