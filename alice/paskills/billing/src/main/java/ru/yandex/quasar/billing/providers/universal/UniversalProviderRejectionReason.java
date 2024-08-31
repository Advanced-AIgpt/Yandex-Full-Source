package ru.yandex.quasar.billing.providers.universal;

import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;

import ru.yandex.quasar.billing.providers.RejectionReason;

public enum UniversalProviderRejectionReason {
    PURCHASE_NOT_FOUND(RejectionReason.PURCHASE_NOT_FOUND),
    PURCHASE_EXPIRED(RejectionReason.PURCHASE_EXPIRED),
    SUBSCRIPTION_NOT_FOUND(RejectionReason.SUBSCRIPTION_NOT_FOUND),
    GEO_CONSTRAINT_VIOLATION(RejectionReason.GEO_CONSTRAINT_VIOLATION),
    LICENSES_NOT_FOUND(RejectionReason.LICENSES_NOT_FOUND),
    SERVICE_CONSTRAINT_VIOLATION(RejectionReason.SERVICE_CONSTRAINT_VIOLATION),
    SUPPORTED_STREAMS_NOT_FOUND(RejectionReason.SUPPORTED_STREAMS_NOT_FOUND),
    UNEXPLAINABLE(RejectionReason.UNEXPLAINABLE),
    PRODUCT_CONSTRAINT_VIOLATION(RejectionReason.PRODUCT_CONSTRAINT_VIOLATION),
    STREAMS_NOT_FOUND(RejectionReason.STREAMS_NOT_FOUND),
    MONETIZATION_MODEL_CONSTRAINT_VIOLATION(RejectionReason.MONETIZATION_MODEL_CONSTRAINT_VIOLATION),
    AUTH_TOKEN_SIGNATURE_FAILED(RejectionReason.AUTH_TOKEN_SIGNATURE_FAILED),
    INTERSECTION_BETWEEN_LICENSE_AND_STREAMS_NOT_FOUND(
            RejectionReason.INTERSECTION_BETWEEN_LICENSE_AND_STREAMS_NOT_FOUND),
    UNAUTHORIZED(RejectionReason.UNAUTHORIZED),
    VIDEOERROR(RejectionReason.VIDEOERROR),
    WRONG_SUBSCRIPTION(RejectionReason.PURCHASE_NOT_FOUND),
    @JsonEnumDefaultValue
    UNKNOWN(RejectionReason.UNKNOWN);

    private final RejectionReason rejectionReason;

    UniversalProviderRejectionReason(RejectionReason rejectionReason) {
        this.rejectionReason = rejectionReason;
    }

    public RejectionReason getRejectionReason() {
        return rejectionReason;
    }

}
