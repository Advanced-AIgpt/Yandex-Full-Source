package ru.yandex.quasar.billing.providers;


import java.math.BigDecimal;
import java.time.Instant;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;

import ru.yandex.quasar.billing.beans.PricingOption;

@Data
@AllArgsConstructor(access = AccessLevel.PRIVATE)
public class AvailabilityInfo {

    private final boolean available;
    private final boolean requiresAccountBinding;
    private final List<PricingOption> pricingOptions;

    // date for temporal availability (like rent) but not subscription end time
    @Nullable
    private final Instant availableUntil;

    private final StreamData streamData;

    @Nullable
    private final RejectionReason rejectionReason;

    public static AvailabilityInfo available(boolean requiresAccountBinding, @Nullable Instant availableUntil,
                                             @Nullable StreamData streamData) {
        return new AvailabilityInfo(
                true,
                requiresAccountBinding,
                Collections.emptyList(),
                availableUntil,
                streamData != null ? streamData : StreamData.EMPTY,
                null
        );
    }

    public static AvailabilityInfo unavailable(boolean requiresAccountBinding, List<PricingOption> pricingOptions,
                                               RejectionReason rejectionReason) {
        return new AvailabilityInfo(
                false,
                requiresAccountBinding,
                pricingOptions,
                null,
                StreamData.EMPTY,
                Objects.requireNonNull(rejectionReason)
        );
    }

    @JsonProperty("minPrice")
    @Nullable
    public BigDecimal getMinPrice() {
        return pricingOptions.stream().map(PricingOption::getUserPrice).min(Comparator.naturalOrder()).orElse(null);
    }

    @JsonProperty("minPriceCurrency")
    @Nullable
    public String getMinPriceCurrency() {
        return pricingOptions.stream()
                .sorted(Comparator.comparing(PricingOption::getUserPrice))
                .map(PricingOption::getCurrency)
                .findFirst()
                .orElse(null);
    }

}
