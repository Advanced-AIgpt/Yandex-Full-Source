package ru.yandex.quasar.billing.services.processing.trust;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

@Data
@Builder
public class PaymentMethod {
    @JsonProperty("id")
    private final String id;
    @JsonProperty("payment_method")
    private final String paymentMethod;
    private final String account;
    private final String holder;
    private final String system;
    @JsonProperty("region_id")
    private final int regionId;
    @JsonProperty("expiration_month")
    private final Integer expirationMonth;
    @JsonProperty("expiration_year")
    private final Integer expirationYear;
    @JsonProperty("payment_system")
    private final String paymentSystem;
    @JsonProperty("card_bank")
    private final String cardBank;
    private final Boolean expired;
    @Nullable
    @JsonProperty("blocking_reason")
    private final String blockingReason;

    public static PaymentMethodBuilder builder(String cardId, String paymentMethod) {
        return new PaymentMethodBuilder().id(cardId).paymentMethod(paymentMethod);
    }
}
