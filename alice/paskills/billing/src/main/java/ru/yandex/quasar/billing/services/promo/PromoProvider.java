package ru.yandex.quasar.billing.services.promo;

import java.util.Optional;

/**
 * Provider's with promo periods for device purchase
 */
public enum PromoProvider {
    // don't delete any of these providers
    // as this enum is used to map existing entries from DB with corresponding values.
    yandexplus,
    ivi,
    amediateka,
    testProvider,
    kinopoisk;

    public static Optional<PromoProvider> byProviderName(String providerName) {
        try {
            return Optional.of(PromoProvider.valueOf(providerName));
        } catch (NullPointerException | IllegalArgumentException e) {
            return Optional.empty();
        }
    }
}
