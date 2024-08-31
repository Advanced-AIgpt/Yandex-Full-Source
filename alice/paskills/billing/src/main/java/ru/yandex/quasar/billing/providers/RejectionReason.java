package ru.yandex.quasar.billing.providers;

import com.fasterxml.jackson.annotation.JsonCreator;

public enum RejectionReason {
    PURCHASE_NOT_FOUND,
    PURCHASE_EXPIRED,
    SUBSCRIPTION_NOT_FOUND,
    GEO_CONSTRAINT_VIOLATION,
    LICENSES_NOT_FOUND,
    SERVICE_CONSTRAINT_VIOLATION,
    SUPPORTED_STREAMS_NOT_FOUND,
    UNEXPLAINABLE,
    PRODUCT_CONSTRAINT_VIOLATION,
    STREAMS_NOT_FOUND,
    MONETIZATION_MODEL_CONSTRAINT_VIOLATION,
    AUTH_TOKEN_SIGNATURE_FAILED,
    INTERSECTION_BETWEEN_LICENSE_AND_STREAMS_NOT_FOUND,
    UNAUTHORIZED,
    VIDEOERROR,
    UNKNOWN;

    @JsonCreator
    public static RejectionReason fromString(String val) {
        for (RejectionReason value : RejectionReason.values()) {
            if (value.toString().equals(val)) {
                return value;
            }
        }
        return UNKNOWN;
    }
}
