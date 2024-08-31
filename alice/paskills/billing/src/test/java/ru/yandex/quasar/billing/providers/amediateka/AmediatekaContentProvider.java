package ru.yandex.quasar.billing.providers.amediateka;

import java.io.IOException;
import java.math.BigDecimal;
import java.time.Duration;
import java.time.Instant;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Executors;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.collect.Maps;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import org.json.JSONException;
import org.json.JSONObject;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;
import org.springframework.web.client.HttpClientErrorException;

import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.LogicalPeriod;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.exception.InternalErrorException;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;
import ru.yandex.quasar.billing.providers.AvailabilityInfo;
import ru.yandex.quasar.billing.providers.BasicProvider;
import ru.yandex.quasar.billing.providers.IContentProvider;
import ru.yandex.quasar.billing.providers.PromoCodeActivationException;
import ru.yandex.quasar.billing.providers.PromoCodeAlreadyActivatedException;
import ru.yandex.quasar.billing.providers.PromoCodeExpiredException;
import ru.yandex.quasar.billing.providers.ProviderActiveSubscriptionInfo;
import ru.yandex.quasar.billing.providers.ProviderPricingOptions;
import ru.yandex.quasar.billing.providers.ProviderPromoCodeActivationResult;
import ru.yandex.quasar.billing.providers.ProviderPurchaseException;
import ru.yandex.quasar.billing.providers.RejectionReason;
import ru.yandex.quasar.billing.providers.StreamData;
import ru.yandex.quasar.billing.providers.amediateka.AmediatekaAPI.AmediatekaType;
import ru.yandex.quasar.billing.providers.amediateka.model.Bundle;
import ru.yandex.quasar.billing.providers.amediateka.model.Price;
import ru.yandex.quasar.billing.providers.amediateka.model.PromoCode;
import ru.yandex.quasar.billing.providers.amediateka.model.ProviderPayload;
import ru.yandex.quasar.billing.providers.amediateka.model.Subscription;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.util.ParallelHelper;

/**
 * Покупка в медиатека не идемпотентная, не защищает от двойной покупки, не возвращает никакого идентификатору ИД.
 */
@Component("amediatekaContentProvider")
public class AmediatekaContentProvider extends BasicProvider implements IContentProvider {
    public static final String PROVIDER_NAME = "amediateka";
    /*
    01.10.2018 Амедиатеки объединила подписки: https://blog.amediateka.ru/theone
    в результате ABC вошла внутрь основной подписки, а это значит, что если у пользователя уже есть основная, то ему
    не нужно показывать ни ABC ни "Основная + ABC"
    */
    static final String DEFAULT_BUNDLE_SLUG = "amediateka-osnovnaya-podpiska";
    static final String ABC_BUNDLE_SLUG = "abc-studios";
    static final String DEFAULT_PLUS_ABC_BUNDLE_SLUG = "amediateka-abc-studios";
    private final AmediatekaBundlesCache amediatekaBundlesCache;
    private final AmediatekaAPI amediatekaAPI;
    private final ParallelHelper parallelHelper;
    private final ObjectMapper objectMapper;

    @Autowired
    public AmediatekaContentProvider(AmediatekaBundlesCache amediatekaBundlesCache, AmediatekaAPI amediatekaAPI,
                                     ObjectMapper objectMapper, AuthorizationContext authorizationContext,
                                     BillingConfig billingConfig) {
        super(PROVIDER_NAME, billingConfig.getProvidersConfig().get(PROVIDER_NAME));
        this.amediatekaBundlesCache = amediatekaBundlesCache;
        this.amediatekaAPI = amediatekaAPI;
        this.objectMapper = objectMapper;
        this.parallelHelper = new ParallelHelper(
                Executors.newCachedThreadPool(
                        new ThreadFactoryBuilder()
                                .setNameFormat("amediateka-content-provider-%d")
                                .build()
                ),
                authorizationContext
        );
    }

    @Override
    public ContentMetaInfo getContentMetaInfo(ProviderContentItem item) {
        AmediatekaType amediatekaType = AmediatekaType.forContentType(item.getContentType());
        return amediatekaType.getContentMetaInfo(amediatekaAPI, item);
    }

    /**
     * Gets pricing options from amediateka -- bundles that contain given `contentId / contentType` pair
     * <p>
     * TODO: check for user-specific prices via `GET /v1/bundles/promo_prices.json`
     */
    @Override
    public ProviderPricingOptions getPricingOptions(ProviderContentItem item, @Nullable String session) {
        boolean available = isAvailable(item, session);
        return ProviderPricingOptions.create(available, available ? Collections.emptyList() :
                getPricingOptionsInternal(item), null);
    }

    private ProviderContentItem getPurchasingItem(ProviderContentItem item) {
        ProviderContentItem pricingContentItem;
        if (item.getContentType() != ContentType.SUBSCRIPTION) {
            AmediatekaType amediatekaType = AmediatekaType.forContentType(item.getContentType());
            pricingContentItem = amediatekaType.getPurchasingContentItem(amediatekaAPI, item);
        } else {
            pricingContentItem = item;
        }
        return pricingContentItem;
    }

    @Override
    public AvailabilityInfo getAvailability(ProviderContentItem providerContentItem, @Nullable String session,
                                            String userIp, String userAgent, boolean withStream) {
        // maps item to Availability info
        AvailabilityInfo availabilityInfo;
        CompletableFuture<List<PricingOption>> pricingOptions = parallelHelper.async(
                () -> getPricingOptionsInternal(providerContentItem));
        try {
            String url = amediatekaAPI.getStreamUrl(providerContentItem, session, userIp);
            if (url != null) {
                availabilityInfo = AvailabilityInfo.available(
                        session == null,
                        null,
                        withStream ? StreamData.byUrl(url) : StreamData.EMPTY);
            } else {
                availabilityInfo = AvailabilityInfo.unavailable(
                        session == null,
                        pricingOptions.join(),
                        RejectionReason.PURCHASE_NOT_FOUND);
            }
        } catch (ProviderUnauthorizedException e) {
            throw e;
        } catch (Exception e) {
            availabilityInfo = AvailabilityInfo.unavailable(
                    session == null,
                    pricingOptions.join(),
                    RejectionReason.PURCHASE_NOT_FOUND);
        }
        return availabilityInfo;
    }

    @Override
    public StreamData getStream(ProviderContentItem providerContentItem, @Nullable String session, String userIp,
                                String userAgent) {
        if (session == null) {
            throw new ProviderUnauthorizedException(PROVIDER_NAME, false);
        }
        String url = amediatekaAPI.getStreamUrl(providerContentItem, session, userIp);
        return StreamData.byUrl(url);
    }

    private List<PricingOption> getPricingOptionsInternal(ProviderContentItem item) {
        ProviderContentItem pricingContentItem = getPurchasingItem(item);
        List<Bundle> bundlesFor = amediatekaBundlesCache.getBundlesFor(pricingContentItem);

        return bundlesFor.stream()
                // we should not offer purchase of ABC and Default+ABD subscriptions
                .filter(bundle -> !bundle.getSlug().equals(ABC_BUNDLE_SLUG) &&
                        !bundle.getSlug().equals(DEFAULT_PLUS_ABC_BUNDLE_SLUG))
                .map(this::bundleToPricingOptions)
                .flatMap(Collection::stream)
                .collect(Collectors.toList());
    }

    private Collection<PricingOption> bundleToPricingOptions(Bundle bundle) {
        return bundle.getOffers()
                .stream()
                .map(Bundle.Offer::getPrices)
                .flatMap(Collection::stream)
                // all bundles are
                .map(price -> createPricingOption(bundle, price))
                .collect(Collectors.toList());
    }

    private PricingOption createPricingOption(Bundle bundle, Price price) {
        try {
            BigDecimal userPrice = price.getValue() == null ? null : new BigDecimal(price.getValue());
            return PricingOption.builder(bundle.getName(), PricingOptionType.SUBSCRIPTION, userPrice,
                            price.getOriginalValue() == null ? userPrice : new BigDecimal(price.getOriginalValue()),
                            price.getCurrency().toUpperCase())
                    .providerPayload(objectMapper.writeValueAsString(new ProviderPayload(price.getUid(),
                            bundle.getId())))
                    .provider(PROVIDER_NAME)
                    .subscriptionPeriod(price.getPeriod() != null ? LogicalPeriod.ofDays(price.getPeriod()) : null)
                    .specialCommission(!DEFAULT_BUNDLE_SLUG.equals(bundle.getSlug()))
                    .purchasingItem(ProviderContentItem.create(ContentType.SUBSCRIPTION, bundle.getId()))
                    .optionId(bundle.getSlug() + "#" + (price.getId() != null ? price.getId().toString() : null))
                    .build();
        } catch (JsonProcessingException e) {
            // this should never happen -- our ProviderPayload is simple enough
            throw new InternalErrorException("Failed to write provider payload", e);
        }
    }

    /**
     * consider content available if there is a stream url for it
     */
    private boolean isAvailable(ProviderContentItem contentItem, @Nullable String session) {
        if (contentItem.getContentType() == ContentType.SUBSCRIPTION) {
            return session != null && getActiveSubscriptions(session).containsKey(contentItem);
        } else {
            return amediatekaAPI.isContentAvailable(contentItem, session);
        }
    }

    @Override
    public void processPurchase(ProviderContentItem purchasingItem, PricingOption selectedOption,
                                String transactionId, String session) throws ProviderPurchaseException {
        ProviderPricingOptions freshOptions = getPricingOptions(purchasingItem, session);

        boolean selectedOptionStillPresent = freshOptions.getPricingOptions().stream().anyMatch(
                (PricingOption pricingOption) -> pricingOption.equalsForBilling(selectedOption)
                        && Objects.equals(selectedOption.getProviderPayload(), pricingOption.getProviderPayload())
        );

        if (!selectedOptionStillPresent) {
            throw new ProviderPurchaseException(PurchaseInfo.Status.ERROR_PAYMENT_OPTION_OBSOLETE);
        }

        ProviderPayload providerPayload;
        try {
            providerPayload = new ObjectMapper().readValue(selectedOption.getProviderPayload(), ProviderPayload.class);
        } catch (IOException e) {
            throw new InternalErrorException("Failed to read provider payload", e);
        }

        amediatekaAPI.payExternal(providerPayload.getPriceUid(), providerPayload.getBundleId(), session);
    }

    /**
     * Промокод всегда превращается в подписку поэтому опираемся на главный сценарий - активация кода из коробки станции
     * на основную подписку. Остальные коды сейчас вообще не обрабатываются. Нужно договариваться с провайдером,
     * чтобы он отдавал, что активировалось кодом.
     * TODO: QUASAR-1740
     *
     * @param promoCode promo code
     * @param session   user token
     * @return result of activation process
     */
    @Override
    public ProviderPromoCodeActivationResult activatePromoCode(String promoCode, String session)
            throws PromoCodeActivationException {
        try {
            PromoCode promoCodeResult = amediatekaAPI.activatePromoCode(promoCode, session);

            PricingOption subscriptionPricingOption;
            if (promoCodeResult.getBundleId() != null && promoCodeResult.getPrice() != null) {
                Optional<Bundle> bundleOptional = amediatekaBundlesCache.getBundleById(promoCodeResult.getBundleId());
                subscriptionPricingOption = bundleOptional.map(bundle -> createPricingOption(bundle,
                        promoCodeResult.getPrice())).orElse(null);
            } else {
                subscriptionPricingOption = null;
            }
            return new ProviderPromoCodeActivationResult(PricingOptionType.SUBSCRIPTION,
                    promoCodeResult.getPeriodInDays(), subscriptionPricingOption);
        } catch (HttpClientErrorException e) {
            JSONObject errorJson;
            try {
                errorJson = new JSONObject(e.getResponseBodyAsString());
            } catch (JSONException ignored) {
                throw new PromoCodeActivationException(e);
            }

            String errorCode = errorJson.optString("id");
            switch (errorCode) {
                case "code_already_redeemed":
                    throw new PromoCodeAlreadyActivatedException();
                case "code_expired":
                    throw new PromoCodeExpiredException();
                default:
                    throw new PromoCodeActivationException(e);
            }
        }
    }

    @Override
    public Map<ProviderContentItem, ProviderActiveSubscriptionInfo> getActiveSubscriptions(@Nonnull String session) {
        List<Subscription> subscriptions = amediatekaAPI.getActiveSubscriptionsState(session);

        Map<ProviderContentItem, ProviderActiveSubscriptionInfo> result =
                Maps.newHashMapWithExpectedSize(subscriptions.size());

        for (Subscription subscription : subscriptions) {
            if (subscription.isStatus() && subscription.getBundleUid() != null &&
                    subscription.getSubscriptionPeriod() != null) {
                ProviderContentItem contentItem = ProviderContentItem.create(ContentType.SUBSCRIPTION,
                        subscription.getBundleUid());
                var metaInfo = getContentMetaInfo(contentItem);
                result.put(
                        contentItem,
                        ProviderActiveSubscriptionInfo.builder(contentItem)
                                .title(metaInfo.getTitle())
                                .description(metaInfo.getDescription())
                                .activeTill(Instant.now().plus(Duration.ofDays(subscription.getSubscriptionPeriod())))
                                .build()
                );
            }
        }

        return result;
    }
}
