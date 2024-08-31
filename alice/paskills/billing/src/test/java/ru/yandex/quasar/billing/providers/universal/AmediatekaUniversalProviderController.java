package ru.yandex.quasar.billing.providers.universal;

import java.util.Collection;
import java.util.Collections;
import java.util.Currency;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;
import javax.servlet.http.HttpServletRequest;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.http.HttpHeaders;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestHeader;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.ContentQuality;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.EpisodeProviderContentItem;
import ru.yandex.quasar.billing.beans.LogicalPeriod;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.beans.SeasonProviderContentItem;
import ru.yandex.quasar.billing.exception.NotFoundException;
import ru.yandex.quasar.billing.providers.AvailabilityInfo;
import ru.yandex.quasar.billing.providers.IContentProvider;
import ru.yandex.quasar.billing.providers.PromoCodeActivationException;
import ru.yandex.quasar.billing.providers.PromoCodeAlreadyActivatedException;
import ru.yandex.quasar.billing.providers.PromoCodeExpiredException;
import ru.yandex.quasar.billing.providers.ProviderActiveSubscriptionInfo;
import ru.yandex.quasar.billing.providers.ProviderPromoCodeActivationResult;
import ru.yandex.quasar.billing.providers.ProviderPurchaseException;
import ru.yandex.quasar.billing.providers.StreamData;
import ru.yandex.quasar.billing.services.AuthorizationService;

import static java.util.Optional.ofNullable;
import static java.util.stream.Collectors.groupingBy;
import static ru.yandex.quasar.billing.providers.universal.UniversalProviderRejectionReason.PURCHASE_NOT_FOUND;

//TODO: extract authentication info
@RestController
@RequestMapping("/provider/amediateka/")
public class AmediatekaUniversalProviderController implements UniversalProviderClient {

    private static final Logger log = LogManager.getLogger(AmediatekaUniversalProviderController.class);

    private final IContentProvider provider;
    private final AuthorizationService authorizationService;

    public AmediatekaUniversalProviderController(
            @Qualifier("amediatekaContentProvider") IContentProvider provider,
            AuthorizationService authorizationService) {
        this.provider = provider;
        this.authorizationService = authorizationService;
    }

    @Override
    public AllContentItems allContent() {
        return new AllContentItems(Collections.emptyList());
    }

    @Override
    @GetMapping("content/{contentItemId}")
    public ContentItemInfo contentItemInfo(
            @PathVariable String contentItemId,
            @RequestHeader(name = HttpHeaders.AUTHORIZATION, required = false) @Nullable String session
    ) {

        ProviderContentItem item = contentIdToProviderItem(contentItemId);

        ContentMetaInfo contentMetaInfo = provider.getContentMetaInfo(item);
        return ContentItemInfo.builder(contentItemId, ContentTypeConverter.fromContentType(item.getContentType()),
                        contentMetaInfo.getTitle())
                .minAge(ofNullable(contentMetaInfo.getAgeRestriction())
                        .map(it -> Integer.valueOf(it.replace("+", ""))).orElse(null))
                .country(contentMetaInfo.getCountry())
                .description(contentMetaInfo.getDescription())
                .duration(ofNullable(contentMetaInfo.getDurationMinutes()).map(it -> it * 60).orElse(null))
                .coverUrl2x3(contentMetaInfo.getImageUrl())
                .releaseYear(contentMetaInfo.getYear())
                .sequenceNumber(contentMetaInfo.getSeasonNumber())
                .build();
    }


    @GetMapping(path = "content/{contentItemId}/available")
    public ContentAvailable contentAvailable(
            @PathVariable String contentItemId,
            boolean requestStream,
            @RequestHeader(name = HttpHeaders.AUTHORIZATION, required = false) @Nullable String session,
            HttpServletRequest request) {

        String userIp = authorizationService.getUserIp(request);
        String userAgent = authorizationService.getUserAgent(request).orElse(null);

        return contentAvailable(contentItemId, requestStream, extractSession(session), userIp, userAgent);
    }

    @Override
    public ContentAvailable contentAvailable(
            String contentItemId,
            boolean requestStream,
            @Nullable String session,
            String userIp,
            String userAgent) {
        ProviderContentItem providerContentItem = contentIdToProviderItem(contentItemId);
        AvailabilityInfo availability = provider.getAvailability(providerContentItem, session, userIp, userAgent,
                requestStream);

        return new ContentAvailable(availability.isAvailable(),
                null,
                availability.isAvailable() ? availability.getStreamData() : null,
                availability.isAvailable() ? null : PURCHASE_NOT_FOUND);
    }

    @Override
    @GetMapping(path = "content/{contentItemId}/options")
    public ProductItems contentPurchaseOptions(
            @PathVariable String contentItemId,
            @RequestHeader(name = HttpHeaders.AUTHORIZATION, required = false) @Nullable String session
    ) {
        List<PricingOption> pricingOptions = provider.getPricingOptions(contentIdToProviderItem(contentItemId),
                        extractSession(session))
                .getPricingOptions();
        return new ProductItems(getProducts(pricingOptions));
    }


    @GetMapping(path = "content/{contentItemId}/stream")
    public StreamData contentStream(
            @PathVariable String contentItemId,
            @RequestHeader(name = HttpHeaders.AUTHORIZATION) String session,
            HttpServletRequest request) {
        String userIp = authorizationService.getUserIp(request);
        String userAgent = authorizationService.getUserAgent(request).orElse(null);

        return contentStream(contentItemId, extractSession(session), userIp, userAgent);

    }


    @Override
    public StreamData contentStream(String contentItemId, String session, String userIp, String userAgent) {
        StreamData stream = provider.getStream(contentIdToProviderItem(contentItemId), session, userIp, userAgent);
        return StreamData.create(stream.getUrl(), ofNullable(stream.getPayload()).orElse("{}"));
    }

    @Override
    @GetMapping("products/{productId}")
    public ProductItem productInfo(
            @PathVariable String productId,
            @RequestHeader(name = HttpHeaders.AUTHORIZATION, required = false) @Nullable String session
    ) {
        ProviderContentItem purchasableContentItem = productIdToProviderItem(productId);
        List<PricingOption> pricingOptions = provider.getPricingOptions(purchasableContentItem, extractSession(session))
                .getPricingOptions()
                .stream()
                .filter(option -> purchasableContentItem.equals(option.getPurchasingItem()))
                .collect(Collectors.toList());

        return toProduct(purchasableContentItem, pricingOptions);
    }

    @Override
    @GetMapping("products/{productId}/{priceId}")
    public ProductPrice productPriceInfo(
            @PathVariable String productId,
            @PathVariable String priceId,
            @RequestHeader(name = HttpHeaders.AUTHORIZATION, required = false) @Nullable String session
    ) {
        ProductItem productItem = productInfo(productId, extractSession(session));
        return productItem.getPrices().stream()
                .filter(price -> price.getId().equals(priceId))
                .findFirst()
                .orElseThrow(() -> {
                    throw new NotFoundException("Price " + priceId + " for purchasable item " + productId + " not " +
                            "found");
                });
    }

    @Override
    @PostMapping("products/{productId}/{priceId}/purchase")
    public PurchaseProcessResult purchaseProduct(
            @PathVariable String productId,
            @PathVariable String priceId,
            @RequestBody PurchaseRequest purchaseRequest,
            @RequestHeader(name = HttpHeaders.AUTHORIZATION) String auth
    ) {
        String session = extractSession(auth);
        ProductItem productItem = productInfo(productId, session);
        return productInfo(productId, session).getPrices().stream()
                .filter(price -> price.getId().equals(priceId))
                .findFirst()
                .map(option -> processPurchase(purchaseRequest, session, productItem, option))
                .map(PurchaseProcessResult::new)
                .orElseThrow(() -> new NotFoundException("Price " + priceId + " for purchasable item " + productId +
                        " not found"));
    }

    private String extractSession(String authToken) {
        return authToken != null ? authToken.replaceAll("Bearer: ", "") : null;
    }

    @Override
    @GetMapping("purchased")
    public PurchasedItems listPurchasedItems(
            @RequestParam(value = "product_type", required = false) @Nullable ProductType type,
            @RequestHeader(name = HttpHeaders.AUTHORIZATION) String auth
    ) {
        String session = extractSession(auth);
        Map<ProviderContentItem, ProviderActiveSubscriptionInfo> activeSubscriptions =
                provider.getActiveSubscriptions(session);

        List<PurchasedItem> productItems = activeSubscriptions.entrySet().stream()
                .filter(it -> type == null || type == ProductType.SUBSCRIPTION)
                .map(entry -> {
                    var purchasableItemInfo = productInfo(toContentItemId(entry.getKey()), session);
                    //TODO determine price
                    return PurchasedItem.builder(purchasableItemInfo, /*purchasableItemInfo.getPrice().stream()
                    .findFirst().orElse(null)*/null)
                            .title(entry.getValue().getTitle())
                            .activeTill(entry.getValue().getActiveTill())
                            .purchasedAt(null) // TODO fix
                            .renewDisabled(provider.isSubscriptionRenewalCancelled(entry.getKey(), session))
                            .build();
                })
                .collect(Collectors.toList());
        return new PurchasedItems(productItems);
    }

    @Override
    @PostMapping("promo/{promocode}")
    public PromoCodeResult activatePromoCode(
            @PathVariable String promocode,
            @RequestHeader(name = HttpHeaders.AUTHORIZATION) String auth
    ) {
        String session = extractSession(auth);
        var builder = PromoCodeResult.builder();

        try {
            ProviderPromoCodeActivationResult promoCodeActivationResult = provider.activatePromoCode(promocode,
                    session);
            builder.status(PromoCodeResult.Status.SUCCESS);

            if (promoCodeActivationResult.getType() == PricingOptionType.SUBSCRIPTION) {

                if (promoCodeActivationResult.getSubscriptionPricingOption() != null) {
                    builder.activateItemId(toContentItemId(
                            promoCodeActivationResult.getSubscriptionPricingOption().getPurchasingItem()
                    ));
                    builder.pricingAfterTrial(toProductPrice(promoCodeActivationResult.getSubscriptionPricingOption()));
                }

                builder.trialPeriod("P" + promoCodeActivationResult.getSubscriptionPeriodDays().toString() + "D");
            }
        } catch (PromoCodeAlreadyActivatedException e) {
            builder.status(PromoCodeResult.Status.ERROR_ALREADY_ACTIVATED);
        } catch (PromoCodeExpiredException e) {
            builder.status(PromoCodeResult.Status.ERROR_EXPIRED);
        } catch (PromoCodeActivationException e) {
            builder.status(PromoCodeResult.Status.ERROR_OTHER);
        } catch (Exception e) {
            builder.status(PromoCodeResult.Status.ERROR_OTHER);
            builder.errorText(e.getMessage());
            log.error("Error activating promocode", e);
        }
        return builder.build();
    }

    private PurchaseStatus processPurchase(PurchaseRequest purchaseRequest, String session, ProductItem productItem,
                                           ProductPrice option) {
        try {
            provider.processPurchase(productIdToProviderItem(productItem.getProductId()),
                    PricingOption.builder(option.getTitle(),
                                    PricingOptionType.valueOf(option.getPurchaseType().name()),
                                    option.getUserPrice(),
                                    option.getPrice(),
                                    "RUB")
                            .providerPayload(purchaseRequest.getPurchasePayload())
                            .quality(ContentQuality.valueOf(option.getQuality().name()))
                            .provider(provider.getProviderName())
                            .subscriptionPeriod(ofNullable(option.getPeriod()).map(LogicalPeriod::parse)
                                    .orElse(null))
                            .specialCommission(false)
                            .purchasingItem(productIdToProviderItem(productItem.getProductId()))
                            .optionId(option.getId())
                            .build(),
                    purchaseRequest.getTransactionId(),
                    session);
            return PurchaseStatus.OK;
        } catch (ProviderPurchaseException e) {
            return PurchaseStatus.map(e.getPurchaseStatus());
        } catch (Exception e) {
            log.error(e.getMessage(), e);
            return PurchaseStatus.ERROR_UNKNOWN;
        }
    }

    @Nonnull
    private List<ProductItem> getProducts(List<PricingOption> pricingOptions) {
        Map<ProviderContentItem, List<PricingOption>> purchasingItems = pricingOptions
                .stream()
                .collect(groupingBy(PricingOption::getPurchasingItem));

        return purchasingItems.entrySet().stream()
                .map(entry -> toProduct(entry.getKey(), entry.getValue()))
                .collect(Collectors.toList());
    }


    private ProductItem toProduct(ProviderContentItem providerContentItem, Collection<PricingOption> pricingOptions) {
        ContentMetaInfo contentMetaInfo = provider.getContentMetaInfo(providerContentItem);
        String itemId = toContentItemId(providerContentItem);

        List<ProductPrice> prices = pricingOptions.stream()
                .map(this::toProductPrice)
                .collect(Collectors.toList());

        return ProductItem.builder(itemId, contentMetaInfo.getTitle(),
                        ProductTypeConverter.fromContentType(providerContentItem.getContentType()))
                .description(contentMetaInfo.getDescription())
                .descriptionShort(null)
                .prices(prices)
                .build();
    }

    private ProductPrice toProductPrice(PricingOption option) {
        return ProductPrice.builder(getPricingOptionId(option), option.getUserPrice(),
                        PurchaseType.valueOf(option.getType().name()))
                .purchasePayload(option.getProviderPayload())
                .currency(Currency.getInstance("RUB"))
                .title(option.getTitle())
                .period(option.getSubscriptionPeriod() != null ? option.getSubscriptionPeriod().toString() : null)
                .quality(option.getQuality() != null ? Quality.valueOf(option.getQuality().name()) : null)
                .processing(option.getProcessor())
                .build();
    }

    private String getPricingOptionId(PricingOption pricingOption) {
        return "price:" + pricingOption.getPurchasingItem().getId() + "-" + pricingOption.hashCode();
    }


    private ProviderContentItem productIdToProviderItem(String productId) {
        String[] strings = productId.split(":");
        Map<String, String> map = idPartsToMap(strings);

        if (map.containsKey(ContentType.EPISODE.getUniversalProviderName())) {
            return ProviderContentItem.createEpisode(map.get(ContentType.EPISODE.getUniversalProviderName()),
                    map.get(ContentType.SEASON.getUniversalProviderName()),
                    map.get(ContentType.TV_SHOW.getUniversalProviderName()));
        } else if (map.containsKey(ProductType.TV_SHOW_SEASON.getCode())) {
            return ProviderContentItem.createSeason(map.get(ContentType.SEASON.getUniversalProviderName()),
                    map.get(ContentType.TV_SHOW.getUniversalProviderName()));
        } else {
            ContentType type =
                    Optional.ofNullable(ContentType.forUniversalProviderName(strings[0]))
                            .orElseThrow(() -> new IllegalArgumentException("Undefined content type: " + strings[0]));
            return ProviderContentItem.create(type, strings[1]);
        }
    }

    private ProviderContentItem contentIdToProviderItem(String contentItemId) {
        String[] strings = contentItemId.split(":");
        Map<String, String> map = idPartsToMap(strings);

        if (map.containsKey(ContentItemType.TV_SHOW_EPISODE.getCode())) {
            return ProviderContentItem.createEpisode(map.get(ContentItemType.TV_SHOW_EPISODE.getCode()),
                    map.get(ContentItemType.TV_SHOW_SEASON.getCode()),
                    map.get(ContentItemType.TV_SHOW.getCode()));
        } else if (map.containsKey(ContentItemType.TV_SHOW_SEASON.getCode())) {
            return ProviderContentItem.createSeason(map.get(ContentItemType.TV_SHOW_SEASON.getCode()),
                    map.get(ContentItemType.TV_SHOW.getCode()));
        } else {
            ContentItemType type = ContentItemType.getByCode(strings[0])
                    .orElseThrow(() -> new IllegalArgumentException("Undefined content type: " + strings[0]));
            return ProviderContentItem.create(ContentTypeConverter.toContentType(type), strings[1]);
        }
    }

    private Map<String, String> idPartsToMap(String[] strings) {

        Map<String, String> map = new LinkedHashMap<>(strings.length / 2 + 1);
        for (int i = 0; i < strings.length; i += 2) {
            map.put(strings[i], strings[i + 1]);
        }
        return map;
    }

    private String toContentItemId(ProviderContentItem providerContentItem) {
        StringBuilder buf = new StringBuilder();

        if (providerContentItem instanceof EpisodeProviderContentItem) {
            EpisodeProviderContentItem seasonProviderContentItem = (EpisodeProviderContentItem) providerContentItem;
            buf.append(ContentType.TV_SHOW.getUniversalProviderName()).append(":")
                    .append(seasonProviderContentItem.getTvShowId()).append(":");
            buf.append(ContentType.SEASON.getUniversalProviderName()).append(":")
                    .append(seasonProviderContentItem.getSeasonId()).append(":");
        } else if (providerContentItem instanceof SeasonProviderContentItem) {
            SeasonProviderContentItem seasonProviderContentItem = (SeasonProviderContentItem) providerContentItem;
            buf.append(ContentType.TV_SHOW.getUniversalProviderName()).append(":")
                    .append(seasonProviderContentItem.getTvShowId()).append(":");
        }

        buf.append(providerContentItem.getContentType().getUniversalProviderName())
                .append(":").append(providerContentItem.getId());

        return buf.toString();
    }

}
