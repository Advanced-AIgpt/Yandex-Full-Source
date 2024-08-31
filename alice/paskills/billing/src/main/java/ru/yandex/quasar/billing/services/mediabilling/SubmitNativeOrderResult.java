package ru.yandex.quasar.billing.services.mediabilling;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;
import com.fasterxml.jackson.annotation.JsonValue;
import lombok.Data;

@Data
public class SubmitNativeOrderResult {


    @Nullable
    private final Long orderId;
    @Nullable
    private final String trustPaymentId;

    private final Status status;
    @Nullable
    private final String description;
    @Nullable
    private final String errorText;

    public static SubmitNativeOrderResult success(long orderId, String trustPaymentId) {
        return new SubmitNativeOrderResult(orderId, trustPaymentId, Status.SUCCESS, null, null);
    }

    public static SubmitNativeOrderResult alreadyPurchased(@Nullable Long orderId) {
        return new SubmitNativeOrderResult(orderId, null, Status.ALREADY_PURCHASED, null, null);
    }

    public static SubmitNativeOrderResult alreadyPending() {
        return new SubmitNativeOrderResult(null, null, Status.ALREADY_PENDING, null, null);
    }

    public static SubmitNativeOrderResult error(@Nullable Long orderId, @Nullable String description,
                                                @Nullable String errorText) {
        return new SubmitNativeOrderResult(orderId, null, Status.ERROR, description, errorText);
    }

    public enum Status {

        SUCCESS("success"),
        NEED_SUPPLY_PAYMENT_DATA("needsupplypaymentdata"),
        ALREADY_PURCHASED("alreadypurchased"),
        ALREADY_PENDING("alreadypending"),
        @JsonEnumDefaultValue
        ERROR("error");

        private final String code;

        Status(String code) {
            this.code = code;
        }

        @JsonValue
        public String getCode() {
            return code;
        }
    }

    @Data
    static class Wrapper {
        private final InvocationInfo invocationInfo;
        private final SubmitNativeOrderResult result;
    }

}
