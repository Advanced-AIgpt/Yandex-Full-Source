package ru.yandex.quasar.billing.providers;

import java.math.BigDecimal;
import java.time.Duration;
import java.time.Instant;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import org.json.JSONObject;
import org.springframework.stereotype.Component;

import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.ContentQuality;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.LogicalPeriod;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionTestUtil;
import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;
import ru.yandex.quasar.billing.services.TestAuthorizationService;

import static java.util.stream.Collectors.toMap;
import static ru.yandex.quasar.billing.services.mediabilling.TestMediaBillingClient.YA_PREMIUM_SUB_ID;

@Component
public class IntegrationTestProvider extends BasicProvider implements IContentProvider, PricingOptionTestUtil {

    public static final String PROVIDER_NAME = "ITProvider";
    public static final Long PARTNER_ID = 40095249L; //ivi's id. defined in /configs/dev/quasar-billing.cfg
    public static final String PROVIDER_SOCIAL_NAME = "ITProviderSocial";

    public static final ProviderContentItem PAID_FILM = ProviderContentItem.create(ContentType.MOVIE, "film_1");
    public static final ProviderContentItem PAID_TV_SHOW = ProviderContentItem.create(ContentType.TV_SHOW, "TV_SHOW_1");
    public static final ProviderContentItem PAID_SEASON = ProviderContentItem.createSeason("SEASON_1", "TV_SHOW_1");
    public static final ProviderContentItem PAID_EPISOD = ProviderContentItem.createEpisode("EPISOD_1", "SEASON_1",
            "TV_SHOW_1");
    public static final ProviderContentItem PAID_SUBSCRIPTION = ProviderContentItem.create(ContentType.SUBSCRIPTION,
            "SUBS_1");
    public static final ProviderContentItem FILM_BY_SUBSCRIPTION = ProviderContentItem.create(ContentType.MOVIE,
            "film_by_sub");

    public static final ProviderContentItem AVAILABLE_FILM = ProviderContentItem.create(ContentType.MOVIE, "film_2");
    public static final ProviderContentItem AVAILABLE_TV_SHOW = ProviderContentItem.create(ContentType.TV_SHOW,
            "TV_SHOW_2");
    public static final ProviderContentItem AVAILABLE_SEASON = ProviderContentItem.createSeason("SEASON_2",
            "TV_SHOW_2");
    public static final ProviderContentItem AVAILABLE_EPISOD = ProviderContentItem.createEpisode("EPISOD_2",
            "SEASON_2", "TV_SHOW_2");
    public static final ProviderContentItem AVAILABLE_SUBSCRIPTION =
            ProviderContentItem.create(ContentType.SUBSCRIPTION, "SUBS_2");

    public static final ProviderContentItem MEDIABILLING_SUB_FILM = ProviderContentItem.create(ContentType.MOVIE,
            "MEDIA_MOVIE_1");
    public static final ProviderContentItem OTT_TVOD_FILM = ProviderContentItem.create(ContentType.MOVIE,
            "OTT_MOVIE_1");
    public static final ProviderContentItem YA_SUBSCRIPTION_PREMIUM =
            ProviderContentItem.create(ContentType.SUBSCRIPTION, "sub:YA_PREMIUM");

    public static final String SUBSCRIPTION_PROMO_CODE = "SUBS";
    public static final String FILM_PROMO_CODE = "FILM";

    public static final String EXPIRED_PROMO_CODE = "EXPIRED_PROMO_CODE";
    public static final String ALREADY_ACTIVATED_PROMO_CODE = "ALREADY_ACTIVATED_PROMO_CODE";
    public static final int SUBSCRIPTION_PERIOD = 1;
    public static final int LARGE_SUBSCRIPTION_PERIOD = 2;

    private final Set<ProviderContentItem> purchasedItems = new HashSet<>();
    private final Map<ProviderContentItem, Instant> purchasedSubscriptions = new HashMap<>();

    public IntegrationTestProvider() {
        super(PROVIDER_NAME, PROVIDER_SOCIAL_NAME, null, true, 5);
    }

    public void reset() {
        purchasedItems.clear();
        purchasedSubscriptions.clear();

        purchasedItems.add(AVAILABLE_FILM);
        purchasedItems.add(AVAILABLE_TV_SHOW);
        purchasedItems.add(AVAILABLE_SEASON);
        purchasedItems.add(AVAILABLE_EPISOD);
        purchasedItems.add(AVAILABLE_SUBSCRIPTION);
        purchasedSubscriptions.put(AVAILABLE_SUBSCRIPTION, Instant.now().plus(Duration.ofDays(365L)));
    }

    public Set<ProviderContentItem> getPurchasedItems() {
        return purchasedItems;
    }

    public Map<ProviderContentItem, Instant> getPurchasedSubscriptions() {
        return purchasedSubscriptions;
    }

    public void updateSubscriptionActiveTill(ProviderContentItem subscription, Duration change) {
        if (!purchasedSubscriptions.containsKey(subscription)) {
            throw new IllegalArgumentException("Subscription " + subscription + " not found");
        }
        purchasedSubscriptions.computeIfPresent(subscription, (key, activeTill) -> activeTill.plus(change));
    }

    /**
     * sets subscription expiration moment to now
     *
     * @param subscription subscription content item
     */
    public void expireSubscription(ProviderContentItem subscription) {
        if (!purchasedSubscriptions.containsKey(subscription)) {
            throw new IllegalArgumentException("Subscription " + subscription + " not found");
        }
        purchasedSubscriptions.computeIfPresent(subscription, (key, activeTill) -> Instant.now());
    }

    @Override
    public String getProviderName() {
        return PROVIDER_NAME;
    }

    @Override
    public String getSocialAPIServiceName() {
        return PROVIDER_SOCIAL_NAME;
    }

    @Nullable
    @Override
    public String getSocialAPIClientId() {
        return null;
    }

    @Override
    public ContentMetaInfo getContentMetaInfo(ProviderContentItem item) {

        Integer seasonNumber = Set.of(PAID_EPISOD, PAID_SEASON, AVAILABLE_EPISOD, AVAILABLE_SEASON,
                FILM_BY_SUBSCRIPTION).contains(item) ? 1 : null;
        return new ContentMetaInfo(item.getId(), "url", 2018, 120, "18", "RUS", seasonNumber, item.toString());
    }

    @Override
    public ProviderPricingOptions getPricingOptions(ProviderContentItem item, @Nullable String session) {
        if (item.equals(FILM_BY_SUBSCRIPTION)) {
            return ProviderPricingOptions.create(TestAuthorizationService.PROVIDER_TOKEN.equals(session)
                            && isItemAvailable(item),
                    List.of(PricingOption.builder(PAID_SUBSCRIPTION.toString(), PricingOptionType.SUBSCRIPTION,
                            BigDecimal.valueOf(99L),
                            BigDecimal.valueOf(99L),
                            "RUB")
                            .providerPayload("{}")
                            .quality(ContentQuality.HD)
                            .provider(PROVIDER_NAME)
                            .subscriptionPeriod(LogicalPeriod.ofDays(SUBSCRIPTION_PERIOD))
                            .specialCommission(false)
                            .purchasingItem(PAID_SUBSCRIPTION)
                            .optionId("sub1")
                            .build()
                    ),
                    null);
        } else if (item.equals(MEDIABILLING_SUB_FILM)) {

            return ProviderPricingOptions.create(TestAuthorizationService.PROVIDER_TOKEN.equals(session)
                            && isItemAvailable(item),
                    List.of(PricingOption.builder("КиноПоиск + Амедиатека", PricingOptionType.SUBSCRIPTION,
                            BigDecimal.valueOf(5), BigDecimal.valueOf(5), "RUB")
                            .providerPayload(new JSONObject().put("billing_product_id", YA_PREMIUM_SUB_ID).toString())
                            .provider(PROVIDER_NAME)
                            .processor(PaymentProcessor.MEDIABILLING)
                            .subscriptionPeriod(LogicalPeriod.ofDays(1))
                            .optionId(YA_PREMIUM_SUB_ID)
                            .purchasingItem(YA_SUBSCRIPTION_PREMIUM)
                            .build()
                    ),
                    null);
        } else if (item.equals(OTT_TVOD_FILM)) {
            return ProviderPricingOptions.create(TestAuthorizationService.PROVIDER_TOKEN.equals(session)
                            && isItemAvailable(item),
                    List.of(PricingOption.builder(item.toString(), PricingOptionType.BUY, BigDecimal.valueOf(5),
                            BigDecimal.valueOf(5), "RUB")
                            .providerPayload(new JSONObject().toString())
                            .provider(PROVIDER_NAME)
                            .optionId("OTT_TVOD_FILM_OPTION_ID")
                            .quality(ContentQuality.HD)
                            .purchasingItem(item)
                            .build()
                    ),
                    null);
        } else {
            return ProviderPricingOptions.create(TestAuthorizationService.PROVIDER_TOKEN.equals(session)
                            && isItemAvailable(item),
                    List.of(PricingOption.builder(item.toString(),
                            Set.of(AVAILABLE_SUBSCRIPTION, PAID_SUBSCRIPTION, FILM_BY_SUBSCRIPTION).contains(item) ?
                                    PricingOptionType.SUBSCRIPTION : PricingOptionType.BUY,
                            BigDecimal.valueOf(99L),
                            BigDecimal.valueOf(99L),
                            "RUB")
                                    .providerPayload("{}")
                                    .quality(ContentQuality.HD)
                                    .provider(PROVIDER_NAME)
                                    .subscriptionPeriod(Set.of(AVAILABLE_SUBSCRIPTION, PAID_SUBSCRIPTION,
                                            FILM_BY_SUBSCRIPTION).contains(item) ?
                                            LogicalPeriod.ofDays(SUBSCRIPTION_PERIOD) : null)
                                    .purchasingItem(item)
                                    .optionId("sub1")
                                    .build(),
                            PricingOption.builder(item.toString(),
                                    Set.of(AVAILABLE_SUBSCRIPTION, PAID_SUBSCRIPTION, FILM_BY_SUBSCRIPTION)
                                            .contains(item) ? PricingOptionType.SUBSCRIPTION : PricingOptionType.BUY,
                                    BigDecimal.valueOf(199L),
                                    BigDecimal.valueOf(199L),
                                    "RUB")
                                    .providerPayload("{}")
                                    .quality(ContentQuality.HD)
                                    .provider(PROVIDER_NAME)
                                    .subscriptionPeriod(Set.of(AVAILABLE_SUBSCRIPTION, PAID_SUBSCRIPTION,
                                            FILM_BY_SUBSCRIPTION).contains(item) ?
                                            LogicalPeriod.ofDays(LARGE_SUBSCRIPTION_PERIOD) : null)
                                    .purchasingItem(item)
                                    .optionId("sub2")
                                    .build()
                    ),
                    null);
        }
    }

    @Override
    public AvailabilityInfo getAvailability(ProviderContentItem item, @Nullable String session, String userIp,
                                            String userAgent, boolean withStream) {
        boolean available = TestAuthorizationService.PROVIDER_TOKEN.equals(session)
                && isItemAvailable(item);
        if (available) {
            return AvailabilityInfo.available(session == null,
                    null,
                    getStream(item, session, userIp, userAgent));
        } else {
            return AvailabilityInfo.unavailable(session == null,
                    List.of(createPricingOption(PROVIDER_NAME, BigDecimal.ONE, PricingOptionType.BUY, item)),
                    RejectionReason.PURCHASE_NOT_FOUND);
        }
    }

    private boolean isItemAvailable(ProviderContentItem item) {
        return (purchasedItems.contains(item) || (item.equals(FILM_BY_SUBSCRIPTION) &&
                purchasedItems.contains(PAID_SUBSCRIPTION)));
    }

    @Override
    public void processPurchase(ProviderContentItem purchasingItem, PricingOption selectedOption,
                                String transactionId, String session) throws ProviderPurchaseException {
        if (session == null) {
            throw new ProviderPurchaseException(PurchaseInfo.Status.ERROR_UNKNOWN);
        } else if (!TestAuthorizationService.PROVIDER_TOKEN.equals(session)) {
            throw new ProviderUnauthorizedException(PROVIDER_NAME, true);
        }
        purchasedItems.add(purchasingItem);

        if (purchasingItem.getContentType() == ContentType.SUBSCRIPTION) {
            purchasedSubscriptions.put(purchasingItem, Instant.now().plusSeconds(60 * 60));
        }
    }

    @Override
    public Map<ProviderContentItem, ProviderActiveSubscriptionInfo> getActiveSubscriptions(@Nonnull String session) {
        if (!TestAuthorizationService.PROVIDER_TOKEN.equals(session)) {
            throw new ProviderUnauthorizedException(PROVIDER_NAME, session != null);
        }
        return purchasedSubscriptions.entrySet().stream()
                .filter(entry -> entry.getValue().isAfter(Instant.now()))
                .collect(toMap(Map.Entry::getKey, t -> ProviderActiveSubscriptionInfo.builder(t.getKey())
                        .title(getContentMetaInfo(t.getKey()).getTitle())
                        .activeTill(t.getValue())
                        .description(getContentMetaInfo(t.getKey()).getDescription())
                        .build()));
    }

    @Override
    public ProviderPromoCodeActivationResult activatePromoCode(String promoCode, String session)
            throws PromoCodeActivationException {
        if (TestAuthorizationService.PROVIDER_TOKEN.equals(session)) {
            if (EXPIRED_PROMO_CODE.equals(promoCode)) {
                throw new PromoCodeExpiredException();
            } else if (ALREADY_ACTIVATED_PROMO_CODE.equals(promoCode)) {
                throw new PromoCodeAlreadyActivatedException();
            } else if (promoCode.equals(SUBSCRIPTION_PROMO_CODE)) {
                purchasedSubscriptions.put(PAID_SUBSCRIPTION, Instant.now().plusSeconds(60 * 60));
                return new ProviderPromoCodeActivationResult(PricingOptionType.SUBSCRIPTION, 1,
                        getPricingOptions(PAID_SUBSCRIPTION, session).getPricingOptions().get(0));
            } else {
                return new ProviderPromoCodeActivationResult(PricingOptionType.BUY, null, null);
            }
        } else {
            throw new ProviderUnauthorizedException(PROVIDER_NAME, session != null);
        }
    }
}
