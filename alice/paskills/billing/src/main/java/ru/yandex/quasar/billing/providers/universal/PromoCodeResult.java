package ru.yandex.quasar.billing.providers.universal;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

@Data
@Builder
@JsonInclude(JsonInclude.Include.NON_NULL)
class PromoCodeResult {
    private final Status status;
    @JsonProperty("activated_item_id")
    @Nullable
    private final String activateItemId;
    @JsonProperty("trial_period")
    @Nullable
    private final String trialPeriod;

    @JsonProperty("pricing_after_trial")
    @Nullable
    private final ProductPrice pricingAfterTrial;
    @Nullable
    @JsonProperty("error_text")
    private final String errorText;

    /*public static PromoCodeResultBuilder builder(String activateItemId) {
        return new PromoCodeResultBuilder().activateItemId(activateItemId);
    }*/

    public enum Status {
        SUCCESS,
        ERROR_ALREADY_ACTIVATED,
        ERROR_EXPIRED,
        ERROR_OTHER
    }

}
