package ru.yandex.quasar.billing.services.mediabilling;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.Currency;

import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;

record PromoCodeSubscriptionFeatureDto(OfferShortDto offer, @Nullable Invoice firstInvoice) {

    record Invoice(
            @JsonDeserialize(converter = LongToInstantConverter.class) Instant timestamp,
            Price totalPrice) {
    }

    record Price(Currency currency, BigDecimal amount) {
    }
}
