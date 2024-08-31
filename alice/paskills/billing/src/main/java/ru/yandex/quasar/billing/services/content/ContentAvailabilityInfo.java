package ru.yandex.quasar.billing.services.content;

import java.math.BigDecimal;
import java.time.Instant;

import javax.annotation.Nullable;

import lombok.Data;

@Data
public class ContentAvailabilityInfo {
    private static final ContentAvailabilityInfo PERMANENTLY_AVAILABLE =
            new ContentAvailabilityInfo(Availability.AVAILABLE, null, null, null);
    private static final ContentAvailabilityInfo ERROR = new ContentAvailabilityInfo(Availability.FAILURE,
            null, null, null);
    private final Availability status;
    // not null for purchasable
    @Nullable
    private final BigDecimal minPrice;
    @Nullable
    private final Instant availableUntil;
    @Nullable
    private final String currency;

    public static ContentAvailabilityInfo available(@Nullable Instant until) {
        return until != null ? new ContentAvailabilityInfo(Availability.AVAILABLE, null, until, null) :
                PERMANENTLY_AVAILABLE;
    }

    public static ContentAvailabilityInfo error() {
        return ERROR;
    }

    public static ContentAvailabilityInfo purchasable(BigDecimal minPrice, String currency) {
        return new ContentAvailabilityInfo(Availability.PURCHASABLE, minPrice, null, null);
    }


    public enum Availability {
        AVAILABLE,
        PURCHASABLE,
        FAILURE
    }

}
