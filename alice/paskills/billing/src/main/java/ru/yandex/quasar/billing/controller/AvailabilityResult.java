package ru.yandex.quasar.billing.controller;

import java.math.BigDecimal;
import java.util.Map;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.quasar.billing.services.content.ContentAvailabilityInfo;

import static java.util.stream.Collectors.toMap;

@Data
public class AvailabilityResult {
    @Nonnull
    @JsonProperty("availability_info")
    private final Map<String, ProviderAvailabilityState> availabilityInfo;

    public static AvailabilityResult create(Map<String, ContentAvailabilityInfo> results) {
        Map<String, ProviderAvailabilityState> result = results.entrySet().stream()
                .collect(toMap(Map.Entry::getKey,
                        it -> new ProviderAvailabilityState(Availability.convert(it.getValue().getStatus()),
                                it.getValue().getMinPrice(), it.getValue().getCurrency())));

        return new AvailabilityResult(result);
    }

    // this representation has to be very stable for backward compatibility
    // so separate class is used
    public enum Availability {
        AVAILABLE(ContentAvailabilityInfo.Availability.AVAILABLE),
        PURCHASABLE(ContentAvailabilityInfo.Availability.PURCHASABLE),
        FAILURE(ContentAvailabilityInfo.Availability.FAILURE);

        private final ContentAvailabilityInfo.Availability status;

        Availability(ContentAvailabilityInfo.Availability status) {
            this.status = status;
        }

        static Availability convert(ContentAvailabilityInfo.Availability source) {
            for (Availability value : values()) {
                if (source == value.status) {
                    return value;
                }
            }
            throw new IllegalArgumentException();
        }
    }

    @Data
    public static class ProviderAvailabilityState {

        @Nonnull
        private final Availability status;

        // not null for purchasable
        @Nullable
        @JsonProperty("min_price")
        private final BigDecimal minPrice;

        @Nullable
        private final String currency;
        // not null for purchasable
    }
}
