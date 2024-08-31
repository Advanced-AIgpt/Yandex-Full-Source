package ru.yandex.quasar.billing.controller;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonValue;
import lombok.Builder;
import lombok.Data;

@Data
@Builder
public class SkillPurchaseCallbackRequest {

    static final String ORDER_STATUS_UPDATE = "order_status_updated";
    @Nonnull
    private final CallbackType type;
    @Nonnull
    private final Payload data;

    public enum CallbackType {
        ORDER_STATUS_UPDATED("order_status_updated"),
        @JsonEnumDefaultValue
        UNKNOWN("unknown");

        private final String code;

        CallbackType(String code) {
            this.code = code;
        }

        @JsonValue
        public String getCode() {
            return code;
        }
    }

    @Data
    @Builder
    public static class Payload {
        @JsonProperty("service_merchant_id")
        @Nonnull
        private final Long serviceMerchantId;
        @JsonProperty("order_id")
        @Nonnull
        private final Long orderId;
        @JsonProperty("new_status")
        @Nonnull
        private final String newStatus;
    }
}
