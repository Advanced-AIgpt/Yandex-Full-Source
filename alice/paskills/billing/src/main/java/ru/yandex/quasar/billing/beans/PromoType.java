package ru.yandex.quasar.billing.beans;

import java.math.BigDecimal;
import java.time.Instant;
import java.time.ZoneId;
import java.time.temporal.ChronoUnit;
import java.util.Currency;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;

import javax.annotation.Nullable;

import ru.yandex.quasar.billing.services.promo.PromoProvider;

import static java.util.Collections.emptyMap;
import static ru.yandex.quasar.billing.beans.ContentType.SUBSCRIPTION;
import static ru.yandex.quasar.billing.beans.PromoDuration.P1M;
import static ru.yandex.quasar.billing.beans.PromoDuration.P1Y;
import static ru.yandex.quasar.billing.beans.PromoDuration.P3M;
import static ru.yandex.quasar.billing.beans.PromoDuration.P4M;
import static ru.yandex.quasar.billing.beans.PromoDuration.P6M;
import static ru.yandex.quasar.billing.services.promo.BackendDeviceInfo.Region.AZ;
import static ru.yandex.quasar.billing.services.promo.BackendDeviceInfo.Region.BY;
import static ru.yandex.quasar.billing.services.promo.BackendDeviceInfo.Region.IL;
import static ru.yandex.quasar.billing.services.promo.BackendDeviceInfo.Region.KZ;
import static ru.yandex.quasar.billing.services.promo.BackendDeviceInfo.Region.UZ;
import static ru.yandex.quasar.billing.services.promo.PromoProvider.kinopoisk;
import static ru.yandex.quasar.billing.services.promo.PromoProvider.yandexplus;

public enum PromoType {
    plus90_by(yandexplus, 90, "YA_PLUS_KP", "BYN", BigDecimal.valueOf(549, 2), P3M),
    plus180_by(yandexplus, 180, "YA_PLUS_KP", "BYN", BigDecimal.valueOf(549, 2), P6M),
    plus180_by_multi(yandexplus, 180, "YA_PLUS_KP", "BYN", BigDecimal.valueOf(959, 2), P6M, true),
    plus360_by(yandexplus, 360, "YA_PLUS_KP", "BYN", BigDecimal.valueOf(549, 2), P1Y),
    plus90_kz(yandexplus, 90, "YA_PLUS", "KZT", BigDecimal.valueOf(849), P3M),
    plus180_kz(yandexplus, 180, "YA_PLUS", "KZT", BigDecimal.valueOf(849), P6M),
    plus180_kz_multi(yandexplus, 180, "YA_PLUS", "KZT", BigDecimal.valueOf(1499), P6M, true),
    plus360_kz(yandexplus, 360, "YA_PLUS", "KZT", BigDecimal.valueOf(849), P1Y),
    plus90_il(yandexplus, 90, "YA_PLUS", "ILS", BigDecimal.valueOf(990, 2), P3M),
    plus180_il(yandexplus, 180, "YA_PLUS", "ILS", BigDecimal.valueOf(990, 2), P6M),
    plus180_il_multi(yandexplus, 180, "YA_PLUS", "ILS", BigDecimal.valueOf(2490, 2), P6M, true),
    plus360_il(yandexplus, 180, "YA_PLUS", "ILS", BigDecimal.valueOf(990, 2), P1Y),
    plus90_az(yandexplus, 90, "YA_PLUS", "AZN", BigDecimal.valueOf(399, 2), P3M),
    plus180_az(yandexplus, 180, "YA_PLUS", "AZN", BigDecimal.valueOf(399, 2), P6M),
    plus180_az_multi(yandexplus, 180, "YA_PLUS", "AZN", BigDecimal.valueOf(659, 2), P6M, true),
    plus360_az(yandexplus, 360, "YA_PLUS", "AZN", BigDecimal.valueOf(399, 2), P1Y),
    plus90_uz(yandexplus, 90, "YA_PLUS", "UZS", BigDecimal.valueOf(15999), P3M),
    plus180_uz(yandexplus, 180, "YA_PLUS", "UZS", BigDecimal.valueOf(15999), P6M),
    plus180_uz_multi(yandexplus, 180, "YA_PLUS", "UZS", BigDecimal.valueOf(24999), P6M, true),
    plus360_uz(yandexplus, 360, "YA_PLUS", "UZS", BigDecimal.valueOf(15999), P1Y),

    plus90(yandexplus, 90, "YA_PLUS", "RUB", BigDecimal.valueOf(199), P3M, false, false,
            Map.of(KZ, plus90_kz, BY, plus90_by, IL, plus90_il, AZ, plus90_az, UZ, plus90_uz)),
    plus180(yandexplus, 180, "YA_PLUS", "RUB", BigDecimal.valueOf(199), P6M, false, false,
            Map.of(KZ, plus180_kz, BY, plus180_by, IL, plus180_il, AZ, plus180_az, UZ, plus180_uz)),
    plus180_multi(yandexplus, 180, "YA_PLUS", "RUB", BigDecimal.valueOf(299), P6M, true, false,
            Map.of(KZ, plus180_kz_multi,
                    BY, plus180_by_multi,
                    IL, plus180_il_multi,
                    AZ, plus180_az_multi,
                    UZ, plus180_uz_multi)),
    plus360(yandexplus, 360, "YA_PLUS", "RUB", BigDecimal.valueOf(199), P1Y, false, false,
            Map.of(KZ, plus360_kz, BY, plus360_by, AZ, plus360_az, UZ, plus360_uz, IL, plus360_il)),

    // experiments
    plus120(yandexplus, 120, "YA_PLUS", "RUB", BigDecimal.valueOf(199), P4M, false, true),
    plus90_149rub(yandexplus, 90, "YA_PLUS", "RUB", BigDecimal.valueOf(149), P3M, false, false,
            Map.of(KZ, plus90_kz, BY, plus90_by, IL, plus90_il, AZ, plus90_az, UZ, plus90_uz)),
    plus30_multi(yandexplus, 30, "YA_PLUS", "RUB", BigDecimal.valueOf(299), P1M, true, false),
    plus90_multi(yandexplus, 90, "YA_PLUS", "RUB", BigDecimal.valueOf(299), P3M, true, false),
    kinopoisk_a3m_multi(kinopoisk, 90, "YA_PREMIUM", "RUB", BigDecimal.valueOf(299), P3M, true, false),
    kinopoisk_a3m_to_kpa(kinopoisk, 90, "YA_PREMIUM", "RUB", BigDecimal.valueOf(699), P3M, true, false),
    kinopoisk_a6m_to_kpa(kinopoisk, 180, "YA_PREMIUM", "RUB", BigDecimal.valueOf(699), P6M, true, false),

    amediateka90(kinopoisk, 90, "YA_PREMIUM", "RUB", BigDecimal.valueOf(299), P3M),
    kinopoisk_a3m(kinopoisk, 90, "YA_PREMIUM", "RUB", BigDecimal.valueOf(299), P3M),
    kinopoisk_a6m(kinopoisk, 180, "YA_PREMIUM", "RUB", BigDecimal.valueOf(299), P6M, true, false,
            Map.of(KZ, plus180_kz_multi,
                    BY, plus180_by_multi,
                    IL, plus180_il_multi,
                    AZ, plus180_az_multi,
                    UZ, plus180_uz_multi)),
    kinopoisk_a6m_plus6m(kinopoisk, 180, "YA_PREMIUM", "RUB", BigDecimal.valueOf(299), P1Y, true, false,
            Map.of(KZ, plus360_kz, BY, plus360_by, IL, plus180_il, AZ, plus360_az, UZ, plus360_uz)),

    // for tests only
    @Deprecated
    test_promo(PromoProvider.testProvider, 90, "dummy", "RUB", BigDecimal.valueOf(199), P3M),
    @Deprecated
    test_promo2(PromoProvider.testProvider, 180, "dummy", "RUB", BigDecimal.valueOf(199), P6M);

    /**
     * Tag on device in backend that is associated with promo {@link PromoType} granted by {@link PromoProvider}
     */
    private final PromoProvider provider;
    /**
     * Effective duration of the promo to compare between them. The value is not used for any kind of processing,
     * it only shows which promo grants a bigger subscription period, so that we can promote the best promo in UI
     */
    private final int duration;

    private final ProviderContentItem promoItem;

    private final Map<String, PromoType> regionAlternatives;
    private final Currency currency;

    private final BigDecimal price;
    private final PromoDuration logicalDuration;
    // Мульти версия подписки для всей семьи
    private final boolean multi;
    // plus120 experiment
    private final boolean isExperiment;


    @SuppressWarnings("ParameterNumber")
    PromoType(PromoProvider provider, int duration, String promoItemId, String currencyCode, BigDecimal price,
              PromoDuration logicalDuration) {
        this(provider, duration, promoItemId, currencyCode, price, logicalDuration, false, false, emptyMap());
    }

    @SuppressWarnings("ParameterNumber")
    PromoType(PromoProvider provider, int duration, String promoItemId, String currencyCode, BigDecimal price,
              PromoDuration logicalDuration, boolean multi) {
        this(provider, duration, promoItemId, currencyCode, price, logicalDuration, multi, false, emptyMap());
    }


    @SuppressWarnings("ParameterNumber")
    PromoType(PromoProvider provider, int duration, String promoItemId, String currencyCode, BigDecimal price,
              PromoDuration logicalDuration,
              boolean multi, boolean experiment) {
        this(provider, duration, promoItemId, currencyCode, price, logicalDuration, multi, experiment, emptyMap());
    }

    @SuppressWarnings("ParameterNumber")
    PromoType(PromoProvider provider, int duration, String promoItemId, String currencyCode, BigDecimal price,
              PromoDuration logicalDuration,
              boolean multi, boolean experiment,
              Map<String, PromoType> regionAlternatives) {
        this.provider = provider;
        this.duration = duration;
        this.promoItem = subscription(promoItemId);
        this.currency = Currency.getInstance(currencyCode);
        this.price = price;
        this.logicalDuration = logicalDuration;
        this.multi = multi;
        this.isExperiment = experiment;
        this.regionAlternatives = Objects.requireNonNull(regionAlternatives);
    }

    @Nullable
    public static PromoType getByTag(String tag) {
        try {
            return PromoType.valueOf(tag);
        } catch (IllegalArgumentException e) {
            return null;
        }
    }

    private static ProviderContentItem subscription(String subId) {
        return ProviderContentItem.create(SUBSCRIPTION, subId);
    }

    public PromoProvider getProvider() {
        return provider;
    }

    public int getDuration() {
        return duration;
    }

    public ProviderContentItem getPromoItem() {
        return promoItem;
    }

    public Currency getCurrency() {
        return currency;
    }

    public boolean isExperiment() {
        return isExperiment;
    }

    public PromoDuration getLogicalDuration() {
        return logicalDuration;
    }

    public BigDecimal getPrice() {
        return price;
    }

    public boolean isMulti() {
        return multi;
    }

    public PromoType forRegion(@Nullable String region) {
        if (region == null) {
            return this;
        }

        return regionAlternatives.getOrDefault(region.toUpperCase(), this);
    }

    public static Optional<Instant> getExpirationTime(
            PromoType promoType,
            @Nullable Instant firstActivation
    ) {
        if (firstActivation == null) {
            return Optional.empty();
        }
        switch (promoType) {
            case plus120:
                return Optional.of(firstActivation
                        .plus(15, ChronoUnit.DAYS)
                        .atZone(ZoneId.of("UTC"))
                        .withHour(0)
                        .withMinute(0)
                        .withSecond(0)
                        .withNano(0)
                        .toInstant());
            default:
                return Optional.empty();
        }
    }
}
