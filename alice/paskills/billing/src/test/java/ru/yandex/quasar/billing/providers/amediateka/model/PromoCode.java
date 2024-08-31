package ru.yandex.quasar.billing.providers.amediateka.model;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;

@Getter
public class PromoCode {

    @JsonProperty("period_in_days")
    private int periodInDays;
    @JsonProperty("bundle_id")
    @Nullable
    private String bundleId;
    @Nullable
    private Price price;

    @Getter
    public static class PromoCodeDTO extends Metadated {
        @JsonProperty("promo_code")
        private PromoCode promoCode;
    }
}
