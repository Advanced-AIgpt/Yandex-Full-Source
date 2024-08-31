package ru.yandex.quasar.billing.beans;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collector;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;

import ru.yandex.quasar.billing.providers.ProviderPricingOptions;

import static java.util.stream.Collectors.toMap;

@Data
@AllArgsConstructor(access = AccessLevel.PRIVATE)
public class PricingOptions {

    // content is watchable (available at least by one provider
    private final boolean available;

    private final Set<String> providersWhereAvailable;

    private final List<PricingOption> pricingOptions;
    private final Map<String, PromoType> providersWithPromoMap;
    private final Set<String> accountLinkingRequired;

    /**
     * {@link Collector} which aggregates multiple {@link ProviderPricingOptions} from multiple providers into a
     * single instance of {@link PricingOptions}
     *
     * @return aggregated {@link PricingOptions}
     */
    private static Collector<ProviderPricingEntry, ProviderPricingOptionsAccumulator, PricingOptions>
    providerPricingOptionsCollector(Set<PromoType> providersWithPromo) {
        return Collector.of(
                ProviderPricingOptionsAccumulator::new,
                (accumulator, entry) -> {
                    accumulator.pricingOptions.addAll(entry.getPricingOptions().getPricingOptions());
                    if (entry.pricingOptions.isAvailable()) {
                        accumulator.providersWhereAvailable.add(entry.getProviderName());
                    }
                    if (entry.accountLinkingRequired) {
                        accumulator.providersAccountLinkingRequired.add(entry.getProviderName());
                    }
                },
                (v1, v2) -> {
                    v1.providersWhereAvailable.addAll(v2.providersWhereAvailable);
                    v1.pricingOptions.addAll(v2.pricingOptions);
                    v1.providersAccountLinkingRequired.addAll(v2.providersAccountLinkingRequired);
                    return v1;
                },
                accumulator -> new PricingOptions(!accumulator.providersWhereAvailable.isEmpty(),
                        accumulator.providersWhereAvailable,
                        accumulator.pricingOptions,
                        accumulator.providersWhereAvailable.isEmpty() ?
                                providersWithPromo.stream().collect(toMap(x -> x.getProvider().name(),
                                        Function.identity())) :
                                Collections.emptyMap(),
                        accumulator.providersAccountLinkingRequired
                )
        );
    }

    public static ProviderPricingEntry entry(String providerName, ProviderPricingOptions pricingOptions,
                                             boolean accountLinkingRequired) {
        return new ProviderPricingEntry(providerName, pricingOptions, accountLinkingRequired);
    }

    public static PricingOptions collect(Collection<ProviderPricingEntry> entries, Set<PromoType> availablePromos) {
        return entries.stream().collect(providerPricingOptionsCollector(availablePromos));
    }

    @JsonProperty("providersWithPromo")
    public Set<String> getProvidersWithPromo() {
        return providersWithPromoMap.keySet();
    }

    private static class ProviderPricingOptionsAccumulator {
        private final Set<String> providersWhereAvailable = new HashSet<>();
        private final List<PricingOption> pricingOptions = new ArrayList<>();
        private final Set<String> providersAccountLinkingRequired = new HashSet<>();
    }

    @Data
    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    public static class ProviderPricingEntry {
        private final String providerName;
        private final ProviderPricingOptions pricingOptions;
        private final boolean accountLinkingRequired;
    }
}
