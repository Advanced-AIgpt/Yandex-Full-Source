package ru.yandex.quasar.billing.controller;

import java.io.IOException;
import java.math.BigDecimal;
import java.net.URI;
import java.sql.Timestamp;
import java.time.Instant;
import java.time.LocalDate;
import java.time.Month;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.Collection;
import java.util.Collections;
import java.util.Currency;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;

import javax.annotation.Nullable;
import javax.servlet.http.HttpServletRequest;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.TreeNode;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.util.Strings;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.server.ResponseStatusException;

import ru.yandex.quasar.billing.beans.ContentItem;
import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptions;
import ru.yandex.quasar.billing.beans.PromoDuration;
import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.controller.GetCardsListResponse.CardInfo;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.exception.BadRequestException;
import ru.yandex.quasar.billing.exception.ExpiredException;
import ru.yandex.quasar.billing.exception.NotFoundException;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;
import ru.yandex.quasar.billing.exception.UnauthorizedException;
import ru.yandex.quasar.billing.filter.TvmRequired;
import ru.yandex.quasar.billing.services.AuthorizationService;
import ru.yandex.quasar.billing.services.BillingService;
import ru.yandex.quasar.billing.services.ProviderPurchaseService;
import ru.yandex.quasar.billing.services.TakeoutService;
import ru.yandex.quasar.billing.services.TransactionInfo;
import ru.yandex.quasar.billing.services.TransactionInfoV2;
import ru.yandex.quasar.billing.services.UserPurchaseLockService;
import ru.yandex.quasar.billing.services.content.ActiveSubscriptionSummary;
import ru.yandex.quasar.billing.services.content.ContentAvailabilityInfo;
import ru.yandex.quasar.billing.services.content.ContentService;
import ru.yandex.quasar.billing.services.content.PurchasedContentInfo;
import ru.yandex.quasar.billing.services.processing.trust.PaymentMethod;
import ru.yandex.quasar.billing.services.promo.BackendDeviceInfo;
import ru.yandex.quasar.billing.services.promo.DeviceInfo;
import ru.yandex.quasar.billing.services.promo.Platform;
import ru.yandex.quasar.billing.services.promo.PromoProvider;
import ru.yandex.quasar.billing.services.promo.QuasarBackendException;
import ru.yandex.quasar.billing.services.promo.QuasarPromoService;
import ru.yandex.quasar.billing.services.promo.RequestPlusResult;

import static java.util.Optional.ofNullable;
import static java.util.stream.Collectors.toList;
import static java.util.stream.Collectors.toMap;
import static java.util.stream.Collectors.toSet;
import static ru.yandex.quasar.billing.controller.PersonalCard.convertToPersonalCard;
import static ru.yandex.quasar.billing.services.processing.trust.TrustBillingService.SECURITY_TOKEN_PARAM;

@RestController
@RequestMapping("billing")
public class BillingController {

    private static final Logger log = LogManager.getLogger();
    private static final DateTimeFormatter FORMATTER = DateTimeFormatter
            .ofPattern("yyyy-MM-dd")
            .withZone(ZoneId.of("UTC"));

    private final AuthorizationService authorizationService;
    private final UserPurchasesDAO userPurchasesDAO;
    private final ObjectMapper objectMapper;
    private final UserPurchaseLockService userPurchaseLockService;
    private final ContentService contentService;
    private final ProviderPurchaseService providerPurchaseService;
    private final BillingService billingService;
    private final QuasarPromoService quasarPromoService;
    private final TakeoutService takeoutService;

    @SuppressWarnings("ParameterNumber")
    @Autowired
    public BillingController(
            AuthorizationService authorizationService,
            UserPurchasesDAO userPurchasesDAO,
            ObjectMapper objectMapper,
            UserPurchaseLockService userPurchaseLockService,
            ContentService contentService,
            ProviderPurchaseService providerPurchaseService,
            BillingService billingService,
            QuasarPromoService quasarPromoService,
            TakeoutService takeoutService) {
        this.userPurchasesDAO = userPurchasesDAO;
        this.billingService = billingService;
        this.quasarPromoService = quasarPromoService;
        this.authorizationService = authorizationService;
        this.objectMapper = objectMapper;
        this.contentService = contentService;
        this.providerPurchaseService = providerPurchaseService;

        this.userPurchaseLockService = userPurchaseLockService;
        this.takeoutService = takeoutService;
    }

    /**
     * Content availability check called from devices
     *
     * @param contentItem contentItem to be checked
     * @param request     http request
     */
    @GetMapping(path = "content_available")
    public AvailabilityResult contentAvailable(
            @RequestParam("content_item") ContentItem contentItem,
            HttpServletRequest request
    ) {
        String uid = authorizationService.getSecretUid(request);
        String userIp = authorizationService.getUserIp(request);
        String userAgent = authorizationService.getUserAgent(request).orElse(null);

        Map<String, ContentAvailabilityInfo> contentAvailabilityInfo = contentService.checkContentAvailable(uid,
                contentItem, userIp, userAgent);

        return AvailabilityResult.create(contentAvailabilityInfo);
    }

    /**
     * When user asks Alice to play Music BASS checks if the user has plus promo available we'll send push and offer
     * to activate it otherwise BASS says that user should purchase Yandex.Plus
     * Request is sent only if  music_check_plus_promo flag is set on device
     * <p>
     * TODO: need to fix that PP comes with session but BASS comes with oauth token
     * switch to TVM after QUASAR-302
     */
    @PostMapping(path = "requestPlus")
    public RequestPlusResponse requestPlus(
            @RequestParam(value = "sendPersonalCards", required = false, defaultValue = "true")
            boolean sendPersonalCards,
            HttpServletRequest request
    ) {
        String uid = authorizationService.getSecretUid(request);

        RequestPlusResult requestPlusResult = quasarPromoService.requestPlus(uid, sendPersonalCards);

        return new RequestPlusResponse(
                requestPlusResult.getPromoState(),
                convertToPersonalCard(requestPlusResult.getOfferCardData()),
                requestPlusResult.getActivatePromoUri(),
                requestPlusResult.getExpirationTime() != null ?
                        FORMATTER.format(requestPlusResult.getExpirationTime()) : null,
                requestPlusResult.getExperiment()
        );
    }

    /**
     * фактически нет запросов. Единицы в месяц
     *
     * @param contentItem
     * @return
     * @throws BadRequestException
     */
    @Deprecated(forRemoval = true)
    @RequestMapping(path = "getContentMetaInfo", method = RequestMethod.GET)
    public ContentMetaInfo getContentMetaInfo(@RequestParam("contentItem") ContentItem contentItem)
            throws BadRequestException {
        return contentService.getContentMetaInfo(contentItem);
    }


    /**
     * Не существует
     */
    @Deprecated(forRemoval = true)
    @RequestMapping(path = "pricingOptions", method = RequestMethod.GET)
    public PricingOptions getPricingOptions(
            @RequestParam("contentItem") ContentItem contentItem,
            HttpServletRequest request
    ) throws BadRequestException {
        String uid = authorizationService.getSecretUid(request);
        return contentService.getPricingOptions(uid, contentItem);
    }


    @RequestMapping(path = "getCardsList", method = RequestMethod.GET, produces = "application/json; charset=UTF-8")
    public GetCardsListResponse getCardsListPP(
            @RequestParam(value = "processor", required = false) @Nullable PaymentProcessor processor,
            HttpServletRequest request
    ) {
        return getCardsList(Objects.requireNonNullElse(processor, PaymentProcessor.TRUST), request);
    }

    @RequestMapping(path = "promo/tv/get_cards_list",
            method = RequestMethod.GET,
            produces = "application/json; charset=UTF-8"
    )
    public GetCardsListResponse getCardsListTv(
            @RequestParam(value = "processor", defaultValue = "MEDIABILLING") PaymentProcessor processor,
            HttpServletRequest request
    ) {
        return getCardsList(Objects.requireNonNullElse(processor, PaymentProcessor.MEDIABILLING), request);
    }

    public GetCardsListResponse getCardsList(PaymentProcessor processor, HttpServletRequest request) {
        String uid = authorizationService.getSecretUid(request);
        String userIp = authorizationService.getUserIp(request);

        List<PaymentMethod> cardsList = billingService.getCardsList(uid, processor, userIp);
        List<CardInfo> cards = cardsList.stream()
                .map(card -> CardInfo.builder()
                        .id(card.getId())
                        .paymentMethod(card.getPaymentMethod())
                        .paymentSystem(card.getPaymentSystem())
                        .account(card.getAccount())
                        .system(card.getSystem())
                        .expired(card.getExpired())
                        .build())
                .collect(toList());
        return new GetCardsListResponse(cards);
    }

    /**
     * Initialize purchase process (start transaction) for native provider purchase
     *
     * @param contentItem       item user interested in
     * @param selectedOptionStr pricing option to access the item
     * @param paymentCardId     paymentCardId from trust
     * @param userEmail         user email
     * @param request           {@link HttpServletRequest}
     * @return response object with purchase token from Trust to check it's status
     * @throws ResponseStatusException       with 409 code (CONFLICT) if purchase is already in progress
     * @throws ProviderUnauthorizedException (401 code) if authorization in provider expired
     * @throws ExpiredException              (410 code, GONE) if selected PricingOption has expired
     */
    @PostMapping(path = {"initPurchaseProcess", "initProviderPurchaseProcess"}, params = "!purchaseOfferUuid")
    public InitPurchaseProcessResponse initPurchaseProcess(
            @RequestParam(value = "contentItem", required = false) @Nullable ContentItem contentItem,
            @RequestParam("selectedOption") String selectedOptionStr,
            @RequestParam("paymentCardId") String paymentCardId,
            @RequestParam("userEmail") String userEmail,
            HttpServletRequest request
    ) throws IOException {

        String uid = authorizationService.getSecretUid(request);
        String userIp = authorizationService.getUserIp(request);

        PricingOption selectedOption = objectMapper.readValue(selectedOptionStr, PricingOption.class);

        ProviderContentItem providerContentItem;
        if (contentItem != null) {
            providerContentItem = ofNullable(contentItem.getProviderInfo(selectedOption.getProvider()))
                    // use purchasingItem if dummy content item was passed
                    .map(it -> "default".equals(it.getId()) ? selectedOption.getPurchasingItem() : it)
                    .orElseThrow(() -> new BadRequestException("No provider ProviderContentItem found"));
        } else {
            providerContentItem = selectedOption.getPurchasingItem();
        }

        try {
            // find last purchase if we ask in retry
            // if not found - create new one
            List<PurchaseInfo> lastContentPurchases = userPurchasesDAO.getLastPurchases(Long.valueOf(uid),
                    Timestamp.from(Instant.now().minusSeconds(60L)));
            Optional<String> recentlyPurchasedToken = lastContentPurchases.stream()
                    .filter(purchase -> Objects.equals(providerContentItem, purchase.getContentItem()) &&
                            purchase.getSelectedOption().equalsForBilling(selectedOption))
                    .findFirst()
                    .map(PurchaseInfo::getPurchaseToken);

            String purchaseToken = recentlyPurchasedToken.orElseGet(() ->
                    userPurchaseLockService.processWithLock(Long.valueOf(uid), selectedOption.getProvider(),
                            () -> providerPurchaseService.initPurchaseItem(uid, selectedOption.getProvider(),
                                    providerContentItem, selectedOption, userIp, paymentCardId, userEmail)
                    )
            );
            return new InitPurchaseProcessResponse(purchaseToken, null);
        } catch (UserPurchaseLockService.UserPurchaseLockException e) {
            throw new ResponseStatusException(HttpStatus.CONFLICT, e.getMessage());
        }
    }

    /**
     * not used any more
     * https://nda.ya.ru/t/cGe_j0KR4tA8qi
     *
     * @param purchaseId
     * @param securityToken
     * @param request
     * @return
     */
    @Deprecated(forRemoval = true)
    @RequestMapping(path = "onBillingHoldApplied", method = RequestMethod.POST, params = {"mode!=refund_result"})
    public SimpleResult onBillingHoldApplied(
            @RequestParam("purchaseId") String purchaseId,
            @RequestParam(value = SECURITY_TOKEN_PARAM, required = false) @Nullable String securityToken,
            HttpServletRequest request
    ) {

        try {
            BillingService.ProcessPurchaseResult processPurchaseResult =
                    billingService.processPurchaseCallback(purchaseId, securityToken,
                            authorizationService.getUserIp(request));

            return new SimpleResult(processPurchaseResult.name());
        } catch (UserPurchaseLockService.UserPurchaseLockException e) {
            throw new ResponseStatusException(HttpStatus.CONFLICT, e.getMessage());
        }

    }

    // not used any more: https://nda.ya.ru/t/B_40Y5Q-4tA8w2

    /**
     * Process refund callback sent by Trust after {@code /unhold}
     * Trust retries (exponentially) for 1 day SkillsServiceImplTestuntil we answer with 200
     *
     * @param purchaseToken purchase token
     * @return result
     */
    @Deprecated(forRemoval = true)
    @RequestMapping(path = {"onBillingHoldApplied", "onBillingHoldForSubscriptionApplied"}, method =
            RequestMethod.POST, params = {"mode=refund_result"})
    public SimpleResult onTrustRefund(@RequestParam("purchase_token") String purchaseToken) {
        return new SimpleResult(billingService.processRefundCallback(purchaseToken).name());
    }

    @RequestMapping(path = "getPurchaseStatus", method = RequestMethod.GET)
    public GetPurchaseStatusResponse getPurchaseStatus(
            @RequestParam("purchaseToken") String purchaseToken,
            HttpServletRequest request
    ) {
        String secretUid = authorizationService.getSecretUid(request);
        PurchaseInfo.Status status = userPurchasesDAO.getPurchaseStatus(Long.valueOf(secretUid), purchaseToken);

        return new GetPurchaseStatusResponse(status);
    }

    @RequestMapping(path = "getSubscriptionsInfo", method = RequestMethod.GET)
    public GetSubscriptionsInfoResponse getSubscriptionsInfo(
            @RequestParam("provider") String provider,
            HttpServletRequest request
    ) {

        String uid = authorizationService.getSecretUid(request);

        Collection<ActiveSubscriptionSummary> activeSubscriptionSummaries =
                contentService.getProviderActiveSubscriptionsSummaries(uid, provider);

        List<GetSubscriptionsInfoResponse.Item> activeItems = activeSubscriptionSummaries.stream()
                .map(subscriptionSummary -> GetSubscriptionsInfoResponse.Item
                        .builder(provider, subscriptionSummary.getProviderContentItem(), subscriptionSummary.getTitle())
                        .activeTill(subscriptionSummary.getActiveTill())
                        .cancellable(subscriptionSummary.isRenewEnabled())
                        .subscriptionId(subscriptionSummary.getSubscriptionId())
                        .nextPaymentDate(subscriptionSummary.getNextPaymentDate())
                        .providerLoginRequired(subscriptionSummary.isProviderLoginRequired())
                        .build())
                .collect(toList());

        return new GetSubscriptionsInfoResponse(activeItems);

    }

    /**
     * Calculate sorting weight for pricing option subscription period
     */
    private int getSubscriptionQuasiDuration(@Nullable PricingOption it) {
        return Optional.ofNullable(it)
                .map(PricingOption::getSubscriptionPeriod)
                .map(subscriptionPeriod -> subscriptionPeriod.getYears() * 365 +
                        subscriptionPeriod.getMonths() * 30 +
                        subscriptionPeriod.getDays())
                .orElse(Integer.MAX_VALUE);
    }

    // ALMOST not used. check quasarui https://nda.ya.ru/t/yITpxf274tA9MR
    @Deprecated(forRemoval = true)
    @RequestMapping(path = "getPurchasedContent", method = RequestMethod.GET)
    public GetContentPurchasesResponse getPurchasedContent(
            HttpServletRequest request
    ) {
        String uid = authorizationService.getSecretUid(request);

        List<PurchasedContentInfo> lastPurchasedContentInfo = contentService.getLastPurchasedContentInfo(uid);

        List<GetContentPurchasesResponse.Item> result = lastPurchasedContentInfo.stream()
                .map(item -> new GetContentPurchasesResponse.Item(
                        item.getContentMetaInfo(),
                        item.getProvider(),
                        item.getContentItem()
                ))
                .collect(toList());

        return new GetContentPurchasesResponse(result);
    }

    @RequestMapping(path = "getTransactionsHistory", method = RequestMethod.GET)
    public GetTransactionsHistoryResponse getTransactionsHistory(
            HttpServletRequest request
    ) {
        String uid = authorizationService.getSecretUid(request);
        String userIp = authorizationService.getUserIp(request);

        List<TransactionInfo> purchases = billingService.getTransactionHistory(uid, userIp);

        List<GetTransactionsHistoryResponse.Item> result = purchases.stream()
                .map(transaction -> new GetTransactionsHistoryResponse.Item(
                        transaction.getProvider(),
                        transaction.getTitle(),
                        transaction.getAccount(),
                        transaction.getCardType(),
                        transaction.getPaymentDate(),
                        transaction.getAmount(),
                        transaction.getCurrency()
                ))
                .collect(toList());

        return new GetTransactionsHistoryResponse(result);
    }

    @GetMapping(path = "v2/getTransactionsHistory")
    public GetTransactionsHistoryV2Response getTransactionsHistoryV2(
            @RequestParam("limit") long limit,
            @RequestParam("offset") long offset,
            HttpServletRequest request
    ) {
        String uid = authorizationService.getSecretUid(request);
        String userIp = authorizationService.getUserIp(request);

        List<TransactionInfoV2> purchases = billingService.getTransactionHistoryV2(uid, userIp, limit, offset);

        return GetTransactionsHistoryV2Response.from(purchases);
    }

    @GetMapping(path = "v2/transaction/{purchase_id}")
    public TransactionItem getTransactionsHistoryV2(
            @PathVariable("purchase_id") long purchaseId,
            HttpServletRequest request
    ) {
        String uid = authorizationService.getSecretUid(request);
        String userIp = authorizationService.getUserIp(request);

        Optional<TransactionInfoV2> purchase = billingService.getTransactionV2(uid, userIp, purchaseId);

        return purchase.map(p -> TransactionItem.getTransactionItem().apply(p))
                .orElseThrow(() -> new NotFoundException("purchase_id " + purchaseId + " not found"));
    }


    record DevicePromoAvailabilityResponse(boolean basic, boolean premium) {
    }

    @GetMapping("device_promo_available")
    @TvmRequired
    public DevicePromoAvailabilityResponse devicePromoAvailable(HttpServletRequest request) {
        try {
            Collection<DeviceInfo> userDevices;
            try {
                String uid = authorizationService.getSecretUid(request);
                userDevices = quasarPromoService.getUserDevices(uid);
            } catch (UnauthorizedException | QuasarBackendException e) {
                userDevices = Collections.emptySet();
            }

            Set<PromoProvider> providers = userDevices.stream()
                    .map(DeviceInfo::getAvailablePromos)
                    .flatMap(Collection::stream)
                    .collect(toSet());

            boolean hasKp = providers.contains(PromoProvider.kinopoisk);
            boolean hasPlus = providers.contains(PromoProvider.yandexplus);

            return new DevicePromoAvailabilityResponse(hasKp || hasPlus, hasKp);
        } catch (Exception e) {
            log.error("device_promo_available call failed", e);
            throw e;
        }
    }

    public record MobilePromoAvailabilityResponse(boolean available) {
    }

    @GetMapping("promo/mobile/promo_period_available")
    public MobilePromoAvailabilityResponse mobilePromoAvailable(HttpServletRequest request) {
        try {
            Collection<DeviceInfo> userDevices;
            try {
                String uid = authorizationService.getSecretUid(request);
                userDevices = quasarPromoService.getUserDevices(uid);
            } catch (UnauthorizedException | QuasarBackendException e) {
                userDevices = Collections.emptySet();
            }

            boolean promoAvailable = userDevices.stream()
                    .map(DeviceInfo::getAvailablePromos)
                    .flatMap(Collection::stream)
                    .anyMatch(provider -> provider == PromoProvider.kinopoisk || provider == PromoProvider.yandexplus);

            return new MobilePromoAvailabilityResponse(promoAvailable);
        } catch (Exception e) {
            log.error("mobile promo_period_available call failed", e);
            throw e;
        }
    }

    public record MobilePromoAvailabilityDetails() {
    }

    /*@GetMapping("promo/mobile/promo_period_available_details")
    public MobilePromoAvailabilityResponse mobilePromoAvailableDetails(HttpServletRequest request) {
        String uid = authorizationService.getSecretUid(request);

        Function<DeviceInfo, PromoInfoResponse.DeviceShortInfo> createShortInfo =
                (device) -> new PromoInfoResponse.DeviceShortInfo(device.getDeviceId(),
                        device.getPlatform(),
                        device.getName(),
                        // we've checked is not empty
                        device.getAvailablePromoTypes().values().iterator().next()
                );

        Collection<DeviceInfo> userDevices = quasarPromoService.getUserDevices(uid);


        Map<Boolean, Set<PromoInfoResponse.DeviceShortInfo>> allDevicesWithPromo =
                quasarPromoService.getUserDevices(uid).stream()
                        .filter(device -> !device.getAvailablePromoTypes().isEmpty())
                        .map(createShortInfo)
                        .sorted(Comparator.<PromoInfoResponse.DeviceShortInfo>comparingInt(deviceInfo ->
                                deviceInfo.getPromo().getDuration()).reversed())
                        .collect(partitioningBy(deviceShortInfo -> deviceShortInfo.getPromo().isMulti(), toSet()));

        var devicesWithMultiPromo = allDevicesWithPromo.get(true);
        var devicesWithPersonalPromo = allDevicesWithPromo.get(false);

        return new MobilePromoAvailabilityResponse();
    }*/

    /**
     * used in TV SUW process to check if promo available.
     */
    @GetMapping(path = "promo/tv/promo_period_available")
    @TvmRequired
    @SuppressWarnings("ParameterNumber")
    public PromoPeriodInfo promoPeriodAvailable(
            HttpServletRequest request,
            @RequestParam("platform") Platform platform,
            @RequestParam("device_id") String deviceId,
            @RequestParam(value = "activation_region", required = false) @Nullable String activationRegion,
            @RequestParam(value = "serial", required = false) @Nullable String serialNumber,
            @RequestParam("wifi_mac") String wifiMac,
            @RequestParam("ethernet_mac") String ethernetMac,
            @RequestParam(value = "tag", required = false) Set<String> tags
    ) {
        @Nullable String uid = authorizationService.getSecretUid(request, true).orElse(null);

        BackendDeviceInfo backendDevice = BackendDeviceInfo.create(
                deviceId,
                platform,
                Objects.requireNonNullElse(tags, Set.of()),
                activationRegion,
                serialNumber,
                wifiMac,
                ethernetMac
        );

        DeviceInfo deviceInfo = quasarPromoService.getUserDevice(uid, backendDevice);

        boolean promoAvailable = !deviceInfo.getAvailablePromos().isEmpty();
        if (promoAvailable) {
            PromoType promo =
                    Objects.requireNonNullElseGet(deviceInfo.getAvailablePromoTypes().get(PromoProvider.kinopoisk),
                            () -> deviceInfo.getAvailablePromoTypes().get(PromoProvider.yandexplus));


            LocalDate expirationDate = LocalDate.of(2022, Month.DECEMBER, 31);
            String defaultText = "Оформляя подписку, вы принимаете «Условия просмотра»\nya.cc/4y4UX и «Условия " +
                    "подписки Яндекс.Плюс» ya.cc/4SaRO.\nПодписка будет продлена автоматически.\nОтключить подписку" +
                    " можно в любой момент";
            String plus149ExpText = defaultText + ".\nИндивидуальное предложение для случайным образом выбранных\n" +
                    "пользователей, не использовавших подписку Яндекс Плюс, полученную\n" +
                    "при покупке ТВ. Предложение до 15.03.2022";

            // Move price to PromoType before TV international sales
            return PromoPeriodInfo.available(new PromoPeriodInfo.PromoDetails(
                    "Яндекс.Плюс",
                    URI.create("https://yandex.ru/legal/tv_mobile_agreement_smarttv/"),
                    null, //LocalDate.now().plusDays(promo.getDuration()),
                    promo.getPrice(),
                    promo.getCurrency(),
                    promo.getLogicalDuration().getValue(),
                    promo.getLogicalDuration().getText(),
                    promo.getDuration(),
                    expirationDate,
                    promo == PromoType.plus90_149rub ? plus149ExpText : defaultText
            ));

        } else {
            return PromoPeriodInfo.notAvailable();
        }

    }

    /**
     * used in TV SUW process to activate promo.
     */
    @PostMapping(path = "promo/tv/activate_promo_period")
    @TvmRequired
    @SuppressWarnings("ParameterNumber")
    public ResponseEntity<DevicePromoActivationResponse> activatePromoPeriod(
            HttpServletRequest request,
            @RequestParam("platform") Platform platform,
            @RequestParam("device_id") String deviceId,
            @RequestParam(value = "activation_region", required = false) @Nullable String activationRegion,
            @RequestParam(value = "serial", required = false) @Nullable String serialNumber,
            @RequestParam("wifi_mac") String wifiMac,
            @RequestParam("ethernet_mac") String ethernetMac,
            @RequestParam(value = "tag", required = false) Set<String> tags,
            @RequestBody PromoActivationRequestBody body
    ) {
        String uid = authorizationService.getSecretUid(request);
        String userIp = authorizationService.getUserIp(request);

        BackendDeviceInfo backendDeviceInfo = BackendDeviceInfo.create(
                deviceId,
                platform,
                Objects.requireNonNullElse(tags, Set.of()),
                Strings.trimToNull(activationRegion),
                serialNumber,
                wifiMac,
                ethernetMac
        );

        var activationResult = quasarPromoService.activatePromoPeriodFromTv(
                PromoProvider.yandexplus,
                uid,
                userIp,
                backendDeviceInfo,
                body.paymentMethodId
        );

        return ResponseEntity.ok(DevicePromoActivationResponse.create(activationResult));
    }

    /**
     * @param paymentMethodId nullable for old TV firmwares
     */

    public record PromoActivationRequestBody(@JsonProperty("payment_method_id") String paymentMethodId) {
    }


    public record PromoPeriodInfo(
            boolean available,
            @JsonProperty("promo_details")
            @Nullable
            BillingController.PromoPeriodInfo.PromoDetails promoDetails
    ) {
        static final PromoPeriodInfo FALSE = new PromoPeriodInfo(false, null);

        static PromoPeriodInfo available(PromoDetails promoDetails) {
            return new PromoPeriodInfo(true, promoDetails);
        }

        static PromoPeriodInfo notAvailable() {
            return FALSE;
        }

        /**
         * @param firstPaymentDate fields are nullable for current plus subscribers
         */
        public record PromoDetails(
                @JsonProperty("subscription_name") String subscriptionName,
                @JsonProperty("user_agreement_url") URI userAgreementUrl,
                @Nullable @JsonProperty("first_payment_date") LocalDate firstPaymentDate,
                @Nullable @JsonProperty("regular_payment_amount") BigDecimal regularPaymentAmount,
                @Nullable @JsonProperty("payment_currency") Currency paymentCurrency,
                @JsonProperty("logical_duration") PromoDuration.PromoDurationValue logicalDuration,
                @JsonProperty("logical_duration_text") String logicalDurationText,
                @JsonProperty("fact_duration") int factDuration,
                @JsonProperty("promo_expiration_date")
                @JsonFormat(shape = JsonFormat.Shape.STRING, pattern = "yyyy-MM-dd")
                LocalDate promoExpirationDate,
                @JsonProperty("disclaimer_text") String disclaimerText
        ) {
        }
    }

    @PostMapping("takeout")
    @TvmRequired()
    public TakeoutResponse takeout(
            @RequestParam("uid") String uid,
            @RequestParam("unixtime") String unixtime) {

        log.info("takeout request for uid - ");
        Map<String, List<?>> data = takeoutService.getUserData(uid);

        Map<String, String> stringifiedData = data.entrySet()
                .stream()
                .collect(toMap(entry -> entry.getKey().toLowerCase() + ".json",
                        entry -> jsonToString(entry.getValue())));
        return TakeoutResponse.ok(stringifiedData);
    }

    private String jsonToString(Object object) {
        try {
            return objectMapper.writeValueAsString(object);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }

    public static class AnythingToString extends JsonDeserializer<String> {

        @Override
        public String deserialize(JsonParser jp, DeserializationContext ctxt) throws IOException {
            TreeNode tree = jp.getCodec().readTree(jp);
            return tree.toString();
        }
    }

}
