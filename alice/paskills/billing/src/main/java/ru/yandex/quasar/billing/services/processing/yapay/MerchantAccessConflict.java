package ru.yandex.quasar.billing.services.processing.yapay;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class MerchantAccessConflict {
    private final String message;
    private final Params params;

    @Data
    static class Params {
        @JsonProperty("service_merchant_id")
        private final Long serviceMerchantId;
    }
}
