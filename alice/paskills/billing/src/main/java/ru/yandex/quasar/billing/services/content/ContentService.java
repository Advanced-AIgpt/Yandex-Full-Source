package ru.yandex.quasar.billing.services.content;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Comparator;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.util.concurrent.ThreadFactoryBuilder;
import lombok.Data;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.stereotype.Service;

import ru.yandex.quasar.billing.beans.ContentItem;
import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptions;
import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.SubscriptionInfo;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.dao.UserSubscriptionsDAO;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;
import ru.yandex.quasar.billing.providers.AvailabilityInfo;
import ru.yandex.quasar.billing.providers.IContentProvider;
import ru.yandex.quasar.billing.providers.IProvider;
import ru.yandex.quasar.billing.providers.ProviderActiveSubscriptionInfo;
import ru.yandex.quasar.billing.providers.ProviderManager;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.AuthorizationService;
import ru.yandex.quasar.billing.services.promo.DeviceInfo;
import ru.yandex.quasar.billing.services.promo.PromoProvider;
import ru.yandex.quasar.billing.services.promo.QuasarPromoService;
import ru.yandex.quasar.billing.util.ParallelHelper;

import static java.util.Collections.emptyMap;
import static java.util.Comparator.comparing;
import static java.util.stream.Collectors.groupingBy;
import static java.util.stream.Collectors.maxBy;
import static java.util.stream.Collectors.toList;
import static java.util.stream.Collectors.toMap;
import static java.util.stream.Collectors.toSet;
import static ru.yandex.quasar.billing.util.ParallelHelper.TaskResult;

@Service
public class ContentService {

    private static final Logger log = LogManager.getLogger();

    private final AuthorizationService authorizationService;
    private final ParallelHelper parallelHelper;
    private final ProviderManager providerManager;
    private final UserSubscriptionsDAO userSubscriptionsDAO;
    private final UserPurchasesDAO userPurchasesDao;
    private final QuasarPromoService quasarPromoService;

    @SuppressWarnings("ParameterNumber")
    ContentService(AuthorizationService authorizationService,
                   AuthorizationContext authorizationContext,
                   ProviderManager providerManager,
                   UserSubscriptionsDAO userSubscriptionsDAO,
                   UserPurchasesDAO userPurchasesDao,
                   QuasarPromoService quasarPromoService,
                   @Qualifier("contentExecutorService") ExecutorService executorService) {
        this.authorizationService = authorizationService;
        this.providerManager = providerManager;
        this.userSubscriptionsDAO = userSubscriptionsDAO;
        this.userPurchasesDao = userPurchasesDao;
        this.quasarPromoService = quasarPromoService;
        this.parallelHelper = new ParallelHelper(
                executorService,
                authorizationContext
        );
    }

    /**
     * Check if content item is available for the user or has to be purchased first
     *
     * @param uid         user identifier
     * @param contentItem content item
     * @param userIp      user IP
     * @param userAgent   user agent of the device
     * @return availability information per provider from contentItem
     */
    @Nonnull
    public Map<String, ContentAvailabilityInfo> checkContentAvailable(String uid, ContentItem contentItem,
                                                                      String userIp, String userAgent) {

        return checkAvailabilityByProvider(uid, contentItem, false, userIp, userAgent)
                .entrySet()
                .stream()
                .collect(toMap(Map.Entry::getKey, this::toContentAvailabilityInfo));
    }


    public ContentMetaInfo getContentMetaInfo(ContentItem contentItem) {
        return contentItem.getProviderEntries().stream()
                .map(entry -> providerManager.getContentProvider(entry.getProviderName())
                        .getContentMetaInfo(entry.getProviderContentItem()))
                .reduce(ContentMetaInfo::merge)
                .orElse(null);
    }

    /**
     * Get active subscriptions for a given provider
     *
     * @param uid          user identifier
     * @param providerName provider name
     * @return all active subscriptions summaries for a given provider name
     */
    public Collection<ActiveSubscriptionSummary> getProviderActiveSubscriptionsSummaries(String uid,
                                                                                         String providerName) {
        IContentProvider provider = providerManager.getContentProvider(providerName);
        String session =
                authorizationService.getProviderTokenByUid(uid, provider.getSocialAPIServiceName()).orElse(null);
        return getProviderActiveSubscriptionsSummaries(uid, provider, session, true);
    }

    /**
     * get list of last purchases of the user
     *
     * @param uid user identifier
     * @return list of purchased item info
     */
    public List<PurchasedContentInfo> getLastPurchasedContentInfo(String uid) {
        List<PurchaseInfo> lastContentPurchases = userPurchasesDao.getLastContentPurchases(Long.valueOf(uid));
        Set<String> providers =
                providerManager.getAllContentProviders().stream().map(IProvider::getProviderName).collect(toSet());

        // use toSet as user might have bought (rented) similar item multiple times
        Set<Map.Entry<String, ProviderContentItem>> items = lastContentPurchases.stream()
                .filter(item -> providers.contains(item.getProvider()))
                // provider and contentItem is not null for video purchases
                .map(purchaseInfo -> Map.entry(purchaseInfo.getProvider(), purchaseInfo.getContentItem()))
                // LinkedHashSet is needed to preserve order of fetched purchases as they are sorted by
                // purchase date desc
                .collect(Collectors.toCollection(LinkedHashSet::new));

        // if we cant get metadata for an item just skip it
        return parallelHelper.processParallelAsTasks(
                items,
                item -> getPurchasedContentInfo(item.getKey(), item.getValue())
        )
                .stream()
                .filter(TaskResult::isSuccessful)
                .map(TaskResult::getResult)
                .collect(toList());

    }

    public PricingOptions getPricingOptions(String uid, ContentItem contentItem) {

        CompletableFuture<Set<PromoType>> availablePromoProvidersTask =
                parallelHelper.async(() -> getUsersAvailablePromos(uid));

        List<PricingOptions.ProviderPricingEntry> providersPricing =
                parallelHelper.processParallel(contentItem.getProviderEntries(),
                        entry -> {
                            IContentProvider contentProvider =
                                    providerManager.getContentProvider(entry.getProviderName());
                            String session = authorizationService.getProviderTokenByUid(uid,
                                    contentProvider.getSocialAPIServiceName())
                                    .orElse(null);
                            return PricingOptions.entry(contentProvider.getProviderName(),
                                    contentProvider.getPricingOptions(entry.getProviderContentItem(), session),
                                    session == null);
                        });

        // if we can obtain the purchasing item from Pricing option by promo we should offer user
        // to activate the promo
        Set<PromoType> availablePromos = Set.of();
        if (providersPricing.stream().noneMatch(it -> it.getPricingOptions().isAvailable())) {
            try {
                Set<PromoType> availablePromoProviders = availablePromoProvidersTask.join();

                List<PricingOption> options = providersPricing.stream()
                        .map(it -> it.getPricingOptions().getPricingOptions())
                        .flatMap(Collection::stream)
                        .collect(toList());

                Map<PromoProvider, Optional<PromoType>> promoTypes = promosAlternativesForPricingOptions(options,
                        availablePromoProviders)
                        .stream()
                        .collect(groupingBy(PromoType::getProvider,
                                maxBy(Comparator.comparing(PromoType::getDuration))));


                availablePromos =
                        promoTypes.values().stream().filter(Optional::isPresent).map(Optional::get).collect(toSet());
            } catch (Exception e) {
                availablePromos = Set.of();
            }
        }

        return PricingOptions.collect(providersPricing, availablePromos);
    }

    @Nonnull
    private Set<PromoType> getUsersAvailablePromos(String uid) {
        return quasarPromoService.getUserDevices(uid).stream()
                .map(DeviceInfo::getAvailablePromoTypes)
                .map(Map::values)
                .flatMap(Collection::stream)
                .collect(toSet());
    }

    /**
     * Determine which of available promos can be used to obtain any of the provided options for free
     *
     * @param options pricing options for a content item
     * @param promos  available users promos
     * @return promos that provide alternative to any of the pricings
     */
    private Set<PromoType> promosAlternativesForPricingOptions(List<PricingOption> options, Set<PromoType> promos) {
        var freeItems = promos.stream().collect(groupingBy(PromoType::getPromoItem, toSet()));

        // check if promo item is present among purchasing items
        return options.stream()
                .map(PricingOption::getPurchasingItem)
                .map(freeItems::get)
                .filter(Objects::nonNull)
                .flatMap(Collection::stream)
                .collect(toSet());
    }

    @Nonnull
    private PurchasedContentInfo getPurchasedContentInfo(String provider, ProviderContentItem contentItem) {
        ContentMetaInfo contentMetaInfo = getContentMetaInfo(new ContentItem(provider, contentItem));
        return new PurchasedContentInfo(contentMetaInfo, provider, contentItem);
    }

    private Collection<ActiveSubscriptionSummary> getProviderActiveSubscriptionsSummaries(String uid,
                                                                                          IContentProvider provider,
                                                                                          @Nullable String session,
                                                                                          boolean failOnBadToken) {

        // subscriptions purchased through us
        // Map of ProviderContentItem of subscriptions to SubscriptionId
        Map<ProviderContentItem, List<SubscriptionInfo>> ourActiveSubscriptions =
                userSubscriptionsDAO.getActiveSubscriptions(Long.valueOf(uid)).stream()
                        .filter(sub -> provider.getProviderName().equals(sub.getProvider()))
                        .filter(it -> Objects.nonNull(it.getPurchasedContentItem()))
                        // we may have multiple similar subscriptions if user for example bought one then switched
                        // provider's account and bought another one
                        .collect(groupingBy(SubscriptionInfo::getPurchasedContentItem));

        // active subscriptions on provider's side
        Map<ProviderContentItem, ProviderActiveSubscriptionInfo> activeSubscriptions;
        boolean providerLoginRequired;
        try {
            activeSubscriptions = session != null ? provider.getActiveSubscriptions(session) : emptyMap();
            providerLoginRequired = session == null;
        } catch (ProviderUnauthorizedException e) {
            if (failOnBadToken) {
                throw e;
            } else {
                activeSubscriptions = emptyMap();
                providerLoginRequired = true;
            }
        }

        List<ActiveSubscriptionSummary> summaries = new ArrayList<>();

        // display all our subscriptions even if they are not presented in provider's info (bad token or user changed
        // linked provider's account)
        // we do so as we are going to charge user for these subscriptions! he has to be able to cancel them even
        // without provider's account
        for (Map.Entry<ProviderContentItem, List<SubscriptionInfo>> providerContentItemListEntry :
                ourActiveSubscriptions.entrySet()) {
            ProviderContentItem providerContentItem = providerContentItemListEntry.getKey();
            for (SubscriptionInfo subscriptionInfo : providerContentItemListEntry.getValue()) {
                // activeTill info from provider
                var activeSubscriptionInfo = activeSubscriptions.get(providerContentItem);

                String title = activeSubscriptionInfo != null ? activeSubscriptionInfo.getTitle() :
                        provider.getContentMetaInfo(providerContentItem).getTitle();
                Instant activeTill = activeSubscriptionInfo != null ? activeSubscriptionInfo.getActiveTill() : null;
                var summaryItem = ActiveSubscriptionSummary.builder()
                        // .provider(provider.getProviderName())
                        .providerContentItem(providerContentItem)
                        .providerLoginRequired(providerLoginRequired)
                        .title(title)
                        .activeTill(activeTill)
                        .subscriptionId(subscriptionInfo.getSubscriptionId())
                        .renewEnabled(subscriptionInfo.getStatus() == SubscriptionInfo.Status.ACTIVE)
                        .nextPaymentDate(subscriptionInfo.getStatus() == SubscriptionInfo.Status.ACTIVE ?
                                subscriptionInfo.getActiveTill().toInstant() : null)
                        .build();
                summaries.add(summaryItem);
            }
        }

        // add subscriptions present on provider's side but not managed by us
        activeSubscriptions.values().stream()
                .filter(item -> !ourActiveSubscriptions.containsKey(item.getContentItem()))
                .map(subscriptionEntry -> ActiveSubscriptionSummary.builder()
                        .providerLoginRequired(false)
                        .providerContentItem(subscriptionEntry.getContentItem())
                        .title(subscriptionEntry.getTitle())
                        .activeTill(subscriptionEntry.getActiveTill())
                        .renewEnabled(false)
                        .nextPaymentDate(null)
                        .build())
                .forEach(summaries::add);

        // sort by Title
        summaries.sort(comparing(ActiveSubscriptionSummary::getTitle));
        return summaries;
    }

    @Nonnull
    private ContentAvailabilityInfo toContentAvailabilityInfo(Map.Entry<String, AvailabilityCheckResult> entry) {
        if (entry.getValue().getStatus() == AvailabilityCheckStatus.OK) {
            var availabilityInfo = Objects.requireNonNull(entry.getValue().getAvailabilityInfo());
            if (availabilityInfo.isAvailable()) {
                return ContentAvailabilityInfo.available(availabilityInfo.getAvailableUntil());
            } else if (availabilityInfo.getMinPrice() != null) {
                // PURCHASABLE with min_price=null leads to Station failure so treat it as error instead
                return ContentAvailabilityInfo.purchasable(availabilityInfo.getMinPrice(),
                        availabilityInfo.getMinPriceCurrency());
            } else {
                return ContentAvailabilityInfo.error();
            }
        } else {
            return ContentAvailabilityInfo.error();
        }
    }

    @Nonnull
    private Map<String, AvailabilityCheckResult> checkAvailabilityByProvider(String uid, ContentItem contentItem,
                                                                             boolean obtainStream, String userIp,
                                                                             String userAgent) {
        // perform parallel availability checks
        return parallelHelper.processParallel(contentItem.getProviderEntries(),
                entry -> checkAvailabilityByProvider(uid, entry, obtainStream, userIp, userAgent))
                .stream()
                .collect(Collectors.toMap(Map.Entry::getKey, Map.Entry::getValue));
    }

    /**
     * Perform provider availability check and handle probable exceptions
     *
     * @param uid       user identifier
     * @param entry     {@link ContentItem.ProviderContentItemEntry} entry
     * @param userIp    user IP
     * @param userAgent useAgent of the initial call
     * @return result of the availability check by provider name
     */
    private Map.Entry<String, AvailabilityCheckResult> checkAvailabilityByProvider(
            String uid,
            ContentItem.ProviderContentItemEntry entry,
            boolean obtainStream, String userIp, String userAgent) {

        IContentProvider provider = providerManager.getContentProvider(entry.getProviderName());
        @Nullable
        String session =
                authorizationService.getProviderTokenByUid(uid, provider.getSocialAPIServiceName()).orElse(null);

        AvailabilityCheckResult result;
        try {
            AvailabilityInfo availability = provider.getAvailability(entry.getProviderContentItem(), session, userIp,
                    userAgent, obtainStream);

            result = new AvailabilityCheckResult(availability, AvailabilityCheckStatus.OK);
        } catch (ProviderUnauthorizedException e) {
            result = new AvailabilityCheckResult(null, e.isSocialApiSessionFound() ?
                    AvailabilityCheckStatus.TOKEN_EXPIRED : AvailabilityCheckStatus.UNAUTHORIZED);
        } catch (Exception e) {
            result = new AvailabilityCheckResult(null, AvailabilityCheckStatus.FAILED);
        }

        return Map.entry(entry.getProviderName(), result);
    }

    public enum AvailabilityCheckStatus {
        OK,
        UNAUTHORIZED,
        TOKEN_EXPIRED,
        FAILED
    }

    @Data
    private static final class AvailabilityCheckResult {

        @Nullable
        private final AvailabilityInfo availabilityInfo;
        private final AvailabilityCheckStatus status;
    }

    @Configuration
    static class ContentServiceExecutorConfig {
        @Bean(value = "contentExecutorService", destroyMethod = "shutdownNow")
        public ExecutorService contentExecutorService() {
            return Executors.newCachedThreadPool(
                    new ThreadFactoryBuilder()
                            .setNameFormat("content-service-%d")
                            .build()
            );
        }
    }
}
