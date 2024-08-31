package ru.yandex.quasar.billing.services.processing.trust;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class TrustCreateSubscriptionResponse {

    private final String status;

    @JsonProperty("status_code")
    private final String statusCode;

    public static TrustCreateSubscriptionResponse created() {
        return new TrustCreateSubscriptionResponse("success", "created");
    }
}
