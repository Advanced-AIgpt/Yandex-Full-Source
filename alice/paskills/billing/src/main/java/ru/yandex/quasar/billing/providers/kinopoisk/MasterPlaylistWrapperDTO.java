package ru.yandex.quasar.billing.providers.kinopoisk;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Data;

import ru.yandex.quasar.billing.controller.BillingController;
import ru.yandex.quasar.billing.providers.RejectionReason;

@Data
public class MasterPlaylistWrapperDTO {
    private final MasterPlaylistStatus status;
    @JsonDeserialize(using = BillingController.AnythingToString.class)
    private final String masterPlaylist;
    @Nullable
    private final WatchingRejectionReason watchingRejectionReason;

    public enum MasterPlaylistStatus {
        APPROVED,
        REJECTED,
        UNKNOWN;

        @JsonCreator
        public static MasterPlaylistStatus fromString(String val) {
            for (MasterPlaylistStatus value : MasterPlaylistStatus.values()) {
                if (value.toString().equals(val)) {
                    return value;
                }
            }
            return UNKNOWN;
        }
    }

    public enum WatchingRejectionReason {
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
        UNKNOWN(RejectionReason.UNKNOWN);

        private final RejectionReason rejectionReason;

        WatchingRejectionReason(RejectionReason rejectionReason) {
            this.rejectionReason = rejectionReason;
        }

        @JsonCreator
        public static WatchingRejectionReason fromString(String val) {
            for (WatchingRejectionReason value : WatchingRejectionReason.values()) {
                if (value.toString().equals(val)) {
                    return value;
                }
            }
            return UNKNOWN;
        }

        public RejectionReason getRejectionReason() {
            return rejectionReason;
        }
    }

}
