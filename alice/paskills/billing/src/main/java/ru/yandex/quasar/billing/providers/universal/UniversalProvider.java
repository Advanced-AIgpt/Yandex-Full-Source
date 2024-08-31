package ru.yandex.quasar.billing.providers.universal;

import java.net.URI;
import java.time.Duration;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.function.BiConsumer;
import java.util.function.Supplier;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Stopwatch;
import com.google.common.base.Strings;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.HttpServerErrorException;
import org.springframework.web.client.ResourceAccessException;
import org.springframework.web.client.RestClientException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.ContentQuality;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.EpisodeProviderContentItem;
import ru.yandex.quasar.billing.beans.LogicalPeriod;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.config.ProviderConfig;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;
import ru.yandex.quasar.billing.providers.AvailabilityInfo;
import ru.yandex.quasar.billing.providers.BasicProvider;
import ru.yandex.quasar.billing.providers.ContentNotAvailableException;
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
import ru.yandex.quasar.billing.providers.WrongContentTypeException;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.UnistatService;
import ru.yandex.quasar.billing.util.ParallelHelper;

import static java.util.Optional.ofNullable;
import static java.util.stream.Collectors.toMap;

public class UniversalProvider extends BasicProvider implements IContentProvider {

    private static final Logger logger = LogManager.getLogger();
    protected final AuthorizationContext context;
    private final UniversalProviderClient client;
    private final ParallelHelper parallelHelper;
    private final UnistatService unistatService;
    private final String signalPrefix;

    @SuppressWarnings("ParameterNumber")
    public UniversalProvider(String name, String socialAPIName, String socialClientId, String clientId, String secret,
                             String baseUri, RestTemplateBuilder restTemplateBuilder,
                             AuthorizationContext context, ExecutorService executorService,
                             UnistatService unistatService) {
        super(name, socialAPIName, socialClientId, true, 30);
        this.context = context;
        UriComponentsBuilder baseUriBuilder = UriComponentsBuilder.fromUriString(baseUri)
                .queryParam("client_id", clientId)
                .queryParam("client_secret", secret);

        BiConsumer<HttpHeaders, String> defaultAuthFiller =
                (httpHeaders, token) -> {
                    if (!Strings.isNullOrEmpty(token)) {
                        httpHeaders.add(HttpHeaders.AUTHORIZATION, "Bearer: " + token);
                    }
                };
        this.parallelHelper = new ParallelHelper(executorService, context);
        this.client = new Client(baseUriBuilder, restTemplateBuilder, defaultAuthFiller);
        this.unistatService = unistatService;
        this.signalPrefix = "quasar_billing_universal_provider_" + getProviderName();
    }

    @SuppressWarnings("ParameterNumber")
    public UniversalProvider(String providerName, ProviderConfig providerConfig, String baseUri,
                             RestTemplateBuilder restTemplateBuilder,
                             AuthorizationContext context, BiConsumer<HttpHeaders, String> fillAuthHeaders,
                             ExecutorService executorService, UnistatService unistatService) {
        super(providerName, providerConfig);
        this.client = new Client(UriComponentsBuilder.fromUriString(baseUri), restTemplateBuilder, fillAuthHeaders);
        this.context = context;
        this.parallelHelper = new ParallelHelper(executorService, context);
        this.unistatService = unistatService;
        this.signalPrefix = "quasar_billing_universal_provider_" + getProviderName();
    }

    UniversalProviderClient getClient() {
        return client;
    }

    @Override
    public ContentMetaInfo getContentMetaInfo(ProviderContentItem item) {
        if (item.getContentType().isConsumableContent()) {
            ContentItemInfo contentItemInfo = client.contentItemInfo(toContentId(item), null);
            return ContentMetaInfo.builder(contentItemInfo.getTitle())
                    .ageRestriction(ofNullable(contentItemInfo.getMinAge()).map(it -> it + "+").orElse(null))
                    .country(contentItemInfo.getCountry())
                    .description(contentItemInfo.getDescription())
                    .durationMinutes(contentItemInfo.getDuration() != null ? contentItemInfo.getDuration() / 60 : null)
                    .imageUrl(ofNullable(contentItemInfo.getCoverUrl2x3())
                            .or(() -> ofNullable(contentItemInfo.getCoverUrl16x9()))
                            .or(() -> ofNullable(contentItemInfo.getThumbnailUrl2x3Small()))
                            .or(() -> ofNullable(contentItemInfo.getThumbnailUrl16x9Small()))
                            .or(() -> ofNullable(contentItemInfo.getThumbnailUrl16x9()))
                            .orElse(null))
                    .year(contentItemInfo.getReleaseYear())
                    .seasonNumber(contentItemInfo.getContentType() == ContentItemType.TV_SHOW_SEASON ?
                            contentItemInfo.getSequenceNumber() : null)
                    .build();
        } else {
            ProductItem productItem = client.productInfo(toContentId(item), null);
            return ContentMetaInfo.builder(productItem.getTitle())
                    .description(productItem.getDescription())
                    .build();
        }

    }

    @Override
    public ProviderPricingOptions getPricingOptions(ProviderContentItem item, @Nullable String session) {

        String itemId = toContentId(item);

        boolean available;
        List<ProductItem> productItems;
        if (item.getContentType().isConsumableContent()) {
            CompletableFuture<ContentAvailable> contentAvailable = async(() -> client.contentAvailable(itemId, false,
                    session, context.getUserIp(), context.getUserAgent()));
            CompletableFuture<ProductItems> contentPurchaseOptions =
                    async(() -> client.contentPurchaseOptions(itemId, session));

            available = contentAvailable.join().isAvailable();
            productItems = available ? Collections.emptyList() : contentPurchaseOptions.join().getProducts();

        } else {

            CompletableFuture<PurchasedItems> purchasedItems =
                    async(() -> client.listPurchasedItems(ProductType.SUBSCRIPTION, session));
            CompletableFuture<ProductItem> productItem = async(() -> client.productInfo(itemId, session));

            available = purchasedItems.join().getPurchasedProducts().stream()
                    .anyMatch(it -> itemId.equals(it.getProductId()));
            productItems = available ? Collections.emptyList() : List.of(productItem.join());
        }

        List<PricingOption> pricingOptions = getListOfPrices(productItems);
        return ProviderPricingOptions.create(available, pricingOptions, null);
    }

    @Override
    public AvailabilityInfo getAvailability(ProviderContentItem providerContentItem, @Nullable String session,
                                            String userIp, String userAgent, boolean withStream) {
        String contentItemId = toContentId(providerContentItem);
        String idForPricingOptions = contentItemIdForPricingOptions(providerContentItem, contentItemId);
        CompletableFuture<List<PricingOption>> prices =
                async(() -> getListOfPrices(client.contentPurchaseOptions(idForPricingOptions, session).getProducts()));
        ContentAvailable contentAvailable = client.contentAvailable(contentItemId, withStream, session, userIp,
                userAgent);

        if (!contentAvailable.isAvailable()) {
            RejectionReason rejectionReason = contentAvailable.getRejectionReason() != null ?
                    contentAvailable.getRejectionReason().getRejectionReason() :
                    RejectionReason.PURCHASE_NOT_FOUND;
            unistatService.incrementStatValue(signalPrefix + "_content_availability_" +
                    rejectionReason.name().toLowerCase());
            return AvailabilityInfo.unavailable(session == null, prices.join(), rejectionReason);
        } else {
            unistatService.incrementStatValue(signalPrefix + "_content_availability_available");
            return AvailabilityInfo.available(session == null, null,
                    contentAvailable.getStreamData());
        }
    }

    private String contentItemIdForPricingOptions(
            ProviderContentItem providerContentItem,
            String defaultContentItemId
    ) {
        if (providerContentItem.getContentType() == ContentType.EPISODE) {
            var episodeContentItem = providerContentItem.narrowTo(EpisodeProviderContentItem.class);

            return episodeContentItem.getSeasonId() != null
                    ? toContentId(episodeContentItem.toSeason())
                    : toContentId(episodeContentItem.toTvShow());
        }
        return defaultContentItemId;
    }

    @Override
    public void processPurchase(ProviderContentItem purchasingItem, PricingOption selectedOption,
                                String transactionId, String session) throws ProviderPurchaseException {
        PurchaseRequest purchaseRequest = new PurchaseRequest(transactionId, selectedOption.getProviderPayload(),
                "", "");
        String priceId = Objects.requireNonNullElse(selectedOption.getOptionId(),
                String.valueOf(selectedOption.hashCode()));
        PurchaseProcessResult result;
        try {
            result = client.purchaseProduct(toContentId(purchasingItem), priceId, purchaseRequest, session);
        } catch (Exception e) {
            unistatService.incrementStatValue("quasar_billing_universal_provider_purchase_call_exception_dmmm");
            unistatService.incrementStatValue(signalPrefix + "_purchase_call_exception_dmmm");
            throw new ProviderPurchaseException(PurchaseInfo.Status.ERROR_UNKNOWN, e);
        }
        if (result.getStatus() != PurchaseStatus.OK) {
            PurchaseInfo.Status status;
            switch (result.getStatus()) {
                case ALREADY_AVAILABLE:
                    status = PurchaseInfo.Status.ALREADY_AVAILABLE;
                    break;
                case ERROR_TRY_LATER:
                    status = PurchaseInfo.Status.ERROR_TRY_LATER;
                    break;
                case ERROR_DO_NOT_TRY_LATER:
                    status = PurchaseInfo.Status.ERROR_DO_NOT_TRY_LATER;
                    break;
                case ERROR_NOT_AUTHORIZED:
                    status = PurchaseInfo.Status.ERROR_NO_PROVIDER_ACC;
                    break;
                case ERROR_PAYMENT_OPTION_OBSOLETE:
                    status = PurchaseInfo.Status.ERROR_PAYMENT_OPTION_OBSOLETE;
                    break;
                case ERROR_UNKNOWN:
                default:
                    status = PurchaseInfo.Status.ERROR_UNKNOWN;
                    break;
            }

            unistatService.incrementStatValue(signalPrefix + "_purchase_" + status.name().toLowerCase() + "_dmmm");
            unistatService.incrementStatValue("quasar_billing_universal_provider_purchase_" +
                    status.name().toLowerCase() + "_dmmm");
            throw new ProviderPurchaseException(status);
        }
    }

    @Override
    public ProviderPromoCodeActivationResult activatePromoCode(@Nonnull String promoCode, @Nonnull String session)
            throws PromoCodeActivationException {
        PromoCodeResult promoCodeResult = client.activatePromoCode(promoCode, session);

        switch (promoCodeResult.getStatus()) {
            case SUCCESS:
                PricingOptionType pricingOptionType = ofNullable(promoCodeResult.getPricingAfterTrial())
                        .map(ProductPrice::getPurchaseType)
                        .map(Enum::name)
                        .map(PricingOptionType::valueOf)
                        .orElse(null);

                Integer subscriptionPeriodDays = ofNullable(promoCodeResult.getTrialPeriod())
                        .map(Duration::parse)
                        .map(Duration::toDays)
                        .map(Math::toIntExact)
                        .orElse(null);

                ProductItem productItem = client.productInfo(promoCodeResult.getActivateItemId(), session);
                PricingOption pricingOption = createPricingOption(productItem, promoCodeResult.getPricingAfterTrial(),
                        false);
                return new ProviderPromoCodeActivationResult(pricingOptionType, subscriptionPeriodDays, pricingOption);
            case ERROR_EXPIRED:
                throw new PromoCodeExpiredException();
            case ERROR_ALREADY_ACTIVATED:
                throw new PromoCodeAlreadyActivatedException();
            case ERROR_OTHER:
            default:
                throw new PromoCodeActivationException();
        }
    }

    @Override
    public Map<ProviderContentItem, ProviderActiveSubscriptionInfo> getActiveSubscriptions(@Nonnull String session) {
        PurchasedItems purchasedItems = client.listPurchasedItems(ProductType.SUBSCRIPTION, session);
        return purchasedItems.getPurchasedProducts().stream()
                .filter(it -> it.getActiveTill() != null)
                .collect(toMap(
                        it -> toProviderItem(it.getProductId()),
                        it -> ProviderActiveSubscriptionInfo.builder(toProviderItem(it.getProductId()))
                                .title(it.getTitle())
                                .description(it.getDescription())
                                .descriptionShort(it.getDescriptionShort())
                                .activeTill(it.getActiveTill())
                                .nextPaymentDate(it.getNextPaymentDate())
                                .build(),
                        //deduplicate, take one tha lasts longer
                        (u, v) -> u.getActiveTill() != null &&
                                (v.getActiveTill() == null || u.getActiveTill().compareTo(v.getActiveTill()) > 0) ?
                                u : v
                ));
    }

    @Override
    public boolean isSubscriptionRenewalCancelled(ProviderContentItem subscriptionItem, String session)
            throws ProviderUnauthorizedException, WrongContentTypeException {
        PurchasedItems purchasedItems = client.listPurchasedItems(ProductType.SUBSCRIPTION, session);

        return purchasedItems.getPurchasedProducts().stream()
                .filter(it -> toContentId(subscriptionItem).equals(it.getProductId()))
                .allMatch(PurchasedItem::isRenewDisabled);
    }

    @Override
    public StreamData getStream(ProviderContentItem contentItem, @Nullable String session, String userIp,
                                String userAgent) throws ProviderUnauthorizedException, ContentNotAvailableException {
        return client.contentStream(toContentId(contentItem), session, userIp, userAgent);
    }

    @Nonnull
    private PricingOption createPricingOption(ProductItem product, ProductPrice option, boolean specialCommission) {

        LogicalPeriod subscriptionPeriod = option.getPeriod() != null ? LogicalPeriod.parse(option.getPeriod()) : null;
        LogicalPeriod subscriptionTrialPeriod;

        if (option.getDiscountType() == ProductPrice.DiscountType.FIRST_PAYMENT) {
            subscriptionTrialPeriod = option.getFirstPaymentPeriod() != null ?
                    LogicalPeriod.parse(option.getFirstPaymentPeriod()) : subscriptionPeriod;
        } else if (option.getTrialPeriod() != null) {
            subscriptionTrialPeriod = LogicalPeriod.parse(option.getTrialPeriod());
        } else {
            subscriptionTrialPeriod = null;
        }

        return PricingOption.builder(option.getTitle(), option.getPurchaseType().getPricingOptionType(),
                option.getUserPrice(),
                option.getPrice(),
                option.getCurrency().getCurrencyCode())
                .providerPayload(option.getPurchasePayload())
                .quality(ofNullable(option.getQuality()).map(Enum::name).map(ContentQuality::valueOf)
                        .orElse(null))
                .provider(getProviderName())
                .subscriptionPeriod(subscriptionPeriod)
                .subscriptionTrialPeriod(subscriptionTrialPeriod)
                .specialCommission(specialCommission)
                .purchasingItem(toProviderItem(product.getProductId()))
                .optionId(option.getId())
                .processor(option.getProcessing())
                .imageUrl(product.getImageUrl())
                .discountType(option.getDiscountType() != null ? option.getDiscountType().getDiscountType() : null)
                .build();
    }

    private List<PricingOption> getListOfPrices(@Nullable List<ProductItem> productItems) {

        return ofNullable(productItems).orElse(Collections.emptyList())
                .stream()
                .flatMap(purchasableItem -> purchasableItem.getPrices().stream()
                        .map(option -> createPricingOption(purchasableItem, option, false)))
                .collect(Collectors.toList());
    }

    ProviderContentItem toProviderItem(String contentItemId) {
        String[] strings = contentItemId.split(":");
        Map<String, String> map = new LinkedHashMap<>();
        for (int i = 0; i < strings.length; i += 2) {
            map.put(strings[i], strings[i + 1]);
        }

        if (map.containsKey(ContentType.EPISODE.getUniversalProviderName())) {
            return ProviderContentItem.create(ContentType.EPISODE,
                    map.get(ContentType.EPISODE.getUniversalProviderName()));
            //} else if (map.containsKey(ContentType.SEASON.getUniversalProviderName())) {
            //return ProviderContentItem.createSeason(map.get(ContentType.SEASON.getUniversalProviderName()), map.get
            // (ContentType.TV_SHOW.getUniversalProviderName()));
        } else {
            ContentType type = ContentType.forUniversalProviderName(strings[0]);
            if (type == null) {
                throw new IllegalArgumentException("Undefined content type: " + strings[0]);
            }
            return ProviderContentItem.create(type, Stream.of(strings)
                    .skip(1).collect(Collectors.joining(":")));
        }
    }

    String toContentId(ProviderContentItem providerContentItem) {
        return providerContentItem.getContentType().getUniversalProviderName() + ":" + providerContentItem.getId();
    }

    private <T> CompletableFuture<T> async(Supplier<T> supplier) {
        return parallelHelper.async(supplier);
    }


    class Client implements UniversalProviderClient {

        private final RestTemplate restTemplate;
        private final UriComponentsBuilder uriBuilder;
        private final BiConsumer<HttpHeaders, String> authHeadersFiller;

        private Client(UriComponentsBuilder baseUriBuilder, RestTemplateBuilder restTemplateBuilder,
                       BiConsumer<HttpHeaders, String> authHeadersFiller) {
            this.uriBuilder = baseUriBuilder;
            this.restTemplate = restTemplateBuilder.build();
            this.authHeadersFiller = authHeadersFiller;
        }

        RestTemplate getRestTemplate() {
            return restTemplate;
        }

        private UriComponentsBuilder url() {
            return uriBuilder.cloneBuilder();
        }

        private <T> T withAuthHandled(Supplier<T> runnable, @Nullable String session) {
            try {
                return runnable.get();
            } catch (HttpClientErrorException e) {
                if (e.getStatusCode() == HttpStatus.UNAUTHORIZED) {
                    throw new ProviderUnauthorizedException(UniversalProvider.this.getProviderName(),
                            session != null, e);
                } else {
                    throw e;
                }
            }
        }

        @Override
        public AllContentItems allContent() {
            return get(url().pathSegment("content", "all").build().toUri(),
                    new HttpHeaders(),
                    AllContentItems.class,
                    null)
                    .getBody();
        }

        @Override
        public ContentItemInfo contentItemInfo(String contentItemId, @Nullable String session) {

            return get(url().pathSegment("content", contentItemId).build().encode().toUri(),
                    new HttpHeaders(),
                    ContentItemInfo.class,
                    session)
                    .getBody();
        }

        @Override
        public ContentAvailable contentAvailable(String contentItemId, boolean requestStream,
                                                 @Nullable String session, String userIp, String userAgent) {
            URI url = url()
                    .pathSegment("content", contentItemId, "available")
                    .queryParam("stream_data", requestStream)
                    .build()
                    .encode()
                    .toUri();

            HttpHeaders headers = new HttpHeaders();
            //headers.add(X_FORWARDED_FOR, userIp); //get it from context
            headers.add(HttpHeaders.USER_AGENT, userAgent);

            return get(url, headers, ContentAvailable.class, session).getBody();
        }

        @Override
        public ProductItems contentPurchaseOptions(String contentItemId, @Nullable String session) {
            ResponseEntity<ProductItems> entity = get(
                    url().pathSegment("content", contentItemId, "options").build().encode().toUri(),
                    new HttpHeaders(),
                    ProductItems.class,
                    session
            );
            if (entity.getStatusCode() == HttpStatus.OK) {
                return entity.getBody();
            } else {
                throw new RestClientException("Error calling universal provider API: " + entity.getStatusCode() +
                        (entity.getBody() != null ? "\nbody: " + entity.getBody() : ""));
            }
        }

        @Override
        public StreamData contentStream(String contentItemId, @Nullable String session, String userIp,
                                        String userAgent) {

            HttpHeaders headers = new HttpHeaders();
            //headers.add(X_FORWARDED_FOR, userIp);
            headers.add(HttpHeaders.USER_AGENT, userAgent);

            return withAuthHandled(() -> get(
                    url().pathSegment("content", contentItemId, "stream").build().encode().toUri(),
                    headers,
                    StreamData.class,
                    session
            ).getBody(), session);
        }

        @Override
        public ProductItem productInfo(String productId, @Nullable String session) {
            return get(
                    url().pathSegment("products", productId).build().encode().toUri(),
                    new HttpHeaders(),
                    ProductItem.class,
                    session
            ).getBody();
        }

        @Override
        public ProductPrice productPriceInfo(String productId, String priceId, @Nullable String session) {
            return get(
                    url().pathSegment("products", productId, priceId).build().encode().toUri(),
                    new HttpHeaders(),
                    ProductPrice.class,
                    session
            ).getBody();
        }

        @Override
        public PurchaseProcessResult purchaseProduct(String productId, String priceId,
                                                     PurchaseRequest purchaseRequest, String session) {

            return withAuthHandled(() -> post(
                    url().pathSegment("products", productId, priceId, "purchase").build().encode().toUri(),
                    new HttpHeaders(),
                    purchaseRequest,
                    PurchaseProcessResult.class,
                    session).getBody(),
                    session);
        }

        @Override
        public PurchasedItems listPurchasedItems(@Nullable ProductType type, String session) {
            return withAuthHandled(() -> {

                UriComponentsBuilder builder = url().pathSegment("purchased");
                if (type != null) {
                    builder.queryParam("product_type", type);
                }

                ResponseEntity<PurchasedItems> entity = get(
                        builder.build().encode().toUri(),
                        new HttpHeaders(),
                        PurchasedItems.class,
                        session
                );

                return entity.getBody();

            }, session);
        }

        @Override
        public PromoCodeResult activatePromoCode(String promocode, String session) {
            return withAuthHandled(() -> post(
                    url().pathSegment("promo", promocode).build().encode().toUri(),
                    new HttpHeaders(),
                    null,
                    PromoCodeResult.class,
                    session
            ).getBody(), session);
        }

        protected <T> ResponseEntity<T> post(URI uri, HttpHeaders headers, @Nullable Object body, Class<T> clazz,
                                             @Nullable String token) {
            authHeadersFiller.accept(headers, token);
            return restTemplate.postForEntity(uri, new HttpEntity<>(body, headers), clazz);
        }

        protected <T> ResponseEntity<T> get(URI uri, HttpHeaders headers, Class<T> clazz, @Nullable String token) {
            authHeadersFiller.accept(headers, token);
            ResponseEntity<T> entity = restTemplate.exchange(uri, HttpMethod.GET, new HttpEntity<>(headers), clazz);
            /*withRetry(3, () -> restTemplate.exchange(uri, HttpMethod.GET, new HttpEntity<>(headers), clazz));*/
            if (entity.getStatusCode() == HttpStatus.OK) {
                return entity;
            } else {
                throw new RestClientException("Error calling universal provider API: " + entity.getStatusCode() +
                        (entity.getBody() != null ? "\nbody: " + entity.getBody() : ""));
            }
        }

        private <T> T withRetry(int retryLimit, Supplier<T> call) {
            RuntimeException ex = null;
            int i = 0;
            do {
                var sw = Stopwatch.createStarted();
                try {
                    T t = call.get();
                    sw.stop();
                    return t;
                } catch (HttpServerErrorException | ResourceAccessException e) {
                    sw.stop();
                    ex = e;
                    logger.error("Provider request failed after " + sw.elapsed().toMillis() + " ms", e);
                }
                i++;
            } while (i < retryLimit);
            throw ex;
        }
    }

}
