package ru.yandex.quasar.billing.services.promo;

import java.time.Instant;

import javax.annotation.Nullable;

import lombok.Data;

import ru.yandex.quasar.billing.services.OfferCardData;

@Data
public class RequestPlusResult {
    private final PromoState promoState;
    @Nullable
    private final OfferCardData offerCardData;
    @Nullable
    private final String activatePromoUri;
    @Nullable
    private final Instant expirationTime;
    @Nullable
    private final String experiment;
}
