package ru.yandex.quasar.billing.controller;

import java.net.URI;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.stream.Collectors;

import javax.servlet.http.HttpServletRequest;
import javax.validation.Valid;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.server.ResponseStatusException;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.paskills.common.billing.model.api.CreatedPurchaseOfferResponse;
import ru.yandex.alice.paskills.common.billing.model.api.PurchaseOfferPaymentInfoResponse;
import ru.yandex.alice.paskills.common.billing.model.api.PurchaseOfferStatusResponse;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductActivationRequest;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductActivationResult;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductsResult;
import ru.yandex.quasar.billing.beans.DeliveryInfo;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.PurchaseOffer;
import ru.yandex.quasar.billing.dao.PurchaseOfferDao;
import ru.yandex.quasar.billing.dao.PurchaseOfferStatus;
import ru.yandex.quasar.billing.dao.SkillInfo;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.exception.BadRequestException;
import ru.yandex.quasar.billing.exception.ExpiredException;
import ru.yandex.quasar.billing.exception.ForbiddenException;
import ru.yandex.quasar.billing.exception.InternalErrorException;
import ru.yandex.quasar.billing.exception.NotFoundException;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;
import ru.yandex.quasar.billing.filter.TvmRequired;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.AuthorizationService;
import ru.yandex.quasar.billing.services.BillingService;
import ru.yandex.quasar.billing.services.PaymentInfo;
import ru.yandex.quasar.billing.services.UserPurchaseLockService;
import ru.yandex.quasar.billing.services.UserSkillProductActivationData;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;
import ru.yandex.quasar.billing.services.processing.trust.BindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.NewBindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.TemplateTag;
import ru.yandex.quasar.billing.services.skills.MerchantInfo;
import ru.yandex.quasar.billing.services.skills.SkillsService;
import ru.yandex.quasar.billing.services.tvm.TvmClientName;
import ru.yandex.quasar.billing.util.ParallelHelper;

import static ru.yandex.quasar.billing.beans.PaymentProcessor.MEDIABILLING;

@RestController
class AliceBillingController {
    private static final Set<String> YAPAY_STATUS_TO_PROCESS_IN_CALLBACK =
            Set.of("held", "in_moderation", "rejected", "canceled", "moderation_negative");

    private final SkillsService skillsService;
    private final BillingService billingService;
    private final AuthorizationService authorizationService;
    private final UserPurchaseLockService userPurchaseLockService;
    private final ParallelHelper parallelHelper;
    private final PurchaseOfferDao purchaseOfferDao;
    private final UserPurchasesDAO userPurchasesDAO;
    private final ObjectMapper objectMapper;

    @SuppressWarnings("ParameterNumber")
    AliceBillingController(
            SkillsService skillsService,
            BillingService billingService,
            AuthorizationService authorizationService,
            UserPurchaseLockService userPurchaseLockService,
            AuthorizationContext authorizationContext,
            PurchaseOfferDao purchaseOfferDao,
            UserPurchasesDAO userPurchasesDAO,
            ObjectMapper objectMapper
    ) {
        this.skillsService = skillsService;
        this.billingService = billingService;
        this.authorizationService = authorizationService;
        this.userPurchaseLockService = userPurchaseLockService;
        this.parallelHelper = new ParallelHelper(
                Executors.newCachedThreadPool(
                        new ThreadFactoryBuilder()
                                .setNameFormat("alice-billing-controller-%d")
                                .build()
                ), authorizationContext);
        this.purchaseOfferDao = purchaseOfferDao;
        this.userPurchasesDAO = userPurchasesDAO;
        this.objectMapper = objectMapper;
    }

    @TvmRequired
    @GetMapping("billing/skill/{skill_uuid}/merchants")
    public SkillMerchantResponse getAllMerchantAccess(
            @PathVariable("skill_uuid") String skillUuid
    ) {
        List<MerchantInfo> merchants = skillsService.getMerchants(skillUuid);
        return new SkillMerchantResponse(merchants.stream()
                .map(ServiceMerchant::fromMerchantInfo)
                .collect(Collectors.toList())
        );
    }

    @TvmRequired
    @PutMapping("billing/skill/{skill_uuid}")
    public SimpleResult registerSkill(
            @PathVariable("skill_uuid") String skillUuid,
            @RequestBody @Valid RegisterSkillRequest registerSkillRequest,
            HttpServletRequest request
    ) {
        String uid = authorizationService.getSecretUid(request);
        try {
            skillsService.registerSkill(skillUuid, Long.valueOf(uid), registerSkillRequest.getSlug());
        } catch (SkillsService.SkillAccessViolationException e) {
            throw new ForbiddenException("skill access violation");
        }
        return SimpleResult.OK;
    }

    @TvmRequired
    @GetMapping("billing/skill/{skill_uuid}/public_key")
    @Deprecated //TODO remove in PASKILLS-5195
    public SkillPublicKeyResponse skillPublicKey(
            @PathVariable("skill_uuid") String skillUuid
    ) {
        SkillInfo skillInfo = skillsService.getSkillByUuid(skillUuid);
        return new SkillPublicKeyResponse(skillInfo.getPublicKey());
    }

    @TvmRequired()
    @PutMapping("billing/skill/{skill_uuid}/request_merchant_access")
    public ServiceMerchant requestMerchantAccess(
            @PathVariable("skill_uuid") String skillUuid,
            @RequestBody @Valid CreateMerchantAccessRequest payload
    ) {
        try {
            return ServiceMerchant.fromMerchantInfo(
                    skillsService.requestMerchantAccess(skillUuid, payload.getToken(), payload.getDescription()));
        } catch (SkillsService.BadSkillAccessRequestTokenException e) {
            throw new BadRequestException("Bad token");
        }
    }

    @PostMapping(path = "billing/skill_purchase_callback")
    @TvmRequired(allowed = {TvmClientName.ya_pay, TvmClientName.self})
    public SimpleResult onSkillPurchaseCallback(
            @RequestBody SkillPurchaseCallbackRequest body,
            HttpServletRequest request
    ) {
        if (SkillPurchaseCallbackRequest.CallbackType.ORDER_STATUS_UPDATED != body.getType()) {
            throw new BadRequestException("wrong type");
        }

        String purchaseToken = body.getData().getServiceMerchantId() + "|" + body.getData().getOrderId().toString();
        String newStatus = body.getData().getNewStatus();

        if (YAPAY_STATUS_TO_PROCESS_IN_CALLBACK.contains(newStatus)) {
            try {
                BillingService.ProcessPurchaseResult result =
                        billingService.processYaPayPurchaseCallback(purchaseToken,
                                authorizationService.getUserIp(request));

                return new SimpleResult(result.name());
            } catch (UserPurchaseLockService.UserPurchaseLockException e) {
                throw new ResponseStatusException(HttpStatus.CONFLICT, "Purchase is already processing");
            }
        } else {
            return SimpleResult.OK;
        }
    }

    /**
     * start purchase scenario for external skill, send push to PP
     */
    @TvmRequired//({bass, self})
    @PostMapping({"billing/createSkillPurchaseOffer", "billing/purchase_offer"})
    public CreatedPurchaseOfferResponse offerSkillPurchase(
            @Valid @RequestBody OfferSkillPurchaseRequest data,
            HttpServletRequest request
    ) {
        String uid;
        if (authorizationService.hasAuthInfo(request)) {
            uid = authorizationService.getSecretUid(request);
        } else {
            uid = null;
        }

        OfferSkillPurchaseRequest.SkillInfo purchaseSkillInfo = data.getSkillInfo();
        SkillInfo skillInfo = skillsService.getSkillByUuid(purchaseSkillInfo.getSkillUuid());

        // TODO: check if retry protection is needed
        var skillOfferParams = new BillingService.SkillOfferParams(
                purchaseSkillInfo.getName(),
                purchaseSkillInfo.getImageUrl(),
                purchaseSkillInfo.getCallbackUrl()
        );

        BillingService.CreatedOffer purchaseOffer = billingService.createSkillPurchaseOffer(
                uid,
                skillInfo,
                skillOfferParams,
                data.getDeviceId(),
                data.getPurchaseRequest().toSkillPurchaseOffer(),
                data.getSessionId(),
                data.getUserId(),
                data.getWebhookRequest() != null ? data.getWebhookRequest().toString() : null
        );

        return new CreatedPurchaseOfferResponse(purchaseOffer.getUuid(), URI.create(purchaseOffer.getUrl()));
    }

    /**
     * Purchase initialization method called from PP
     *
     * @param purchaseOfferUuid purchase offer uuid
     * @param paymentCardId     paymentCardId from trust
     * @param userEmail         user email
     * @param request           {@link HttpServletRequest}
     * @return response object with purchase token from Trust to check it's status
     * @throws ResponseStatusException       with 409 code (CONFLICT) if purchase is already in progress
     * @throws ProviderUnauthorizedException (401 code) if authorization in provider expired
     * @throws ExpiredException              (410 code, GONE) if selected PricingOption has expired
     */
    //@PostMapping(path = "billing/initPurchaseProcess", params = "purchaseOfferUuid")
    @PostMapping(path = "billing/purchase_offer/{purchaseOfferUuid}/start")
    public InitPurchaseProcessResponse initPurchaseProcess(
            @PathVariable("purchaseOfferUuid") String purchaseOfferUuid,
            @RequestParam("selectedOptionUuid") String selectedOptionUuid,
            @RequestParam(value = "paymentCardId", required = false) String paymentCardId,
            @RequestParam("userEmail") String userEmail,
            @RequestParam(value = "testPayment", required = false, defaultValue = "false") boolean testPayment,
            HttpServletRequest request
    ) {

        String uid = authorizationService.getSecretUid(request);
        String userIp = authorizationService.getUserIp(request);

        // TODO: protect from retries
        try {
            // if purchase offer is anonymous (not bound to a user) if wouldn't be found and NotFoundException is raised
            PurchaseOffer purchaseOffer = billingService.getPurchaseOffer(uid, purchaseOfferUuid);

            SkillInfo skill = skillsService.getSkillById(purchaseOffer.getSkillInfoId());

            var purchase = userPurchaseLockService.processWithLock(Long.valueOf(uid), skill.getSkillUuid(),
                    () -> billingService.initPurchaseOffer(uid, purchaseOffer, selectedOptionUuid, skill, userIp,
                            paymentCardId, userEmail, testPayment)
            );
            return new InitPurchaseProcessResponse(null, purchase.getRedirectUrl());

        } catch (UserPurchaseLockService.UserPurchaseLockException e) {
            throw new ResponseStatusException(HttpStatus.CONFLICT, e.getMessage());
        }
    }

    @GetMapping("billing/purchase_offer/{purchaseOfferUuid}")
    public PurchaseOfferData purchaseOfferData(
            @PathVariable("purchaseOfferUuid") String purchaseOfferUuid,
            HttpServletRequest request
    ) throws ExecutionException, InterruptedException {
        String uid = authorizationService.getSecretUid(request);

        return getPurchaseOfferData(purchaseOfferUuid, uid);
    }

    // todo: switch to purchase_token
    @GetMapping("billing/purchase_offer/{purchaseOfferUuid}/payment")
    public PurchaseOfferPaymentInfoResponse getPurchaseOfferPaymentInfo(
            @PathVariable("purchaseOfferUuid") String purchaseOfferUuid,
            HttpServletRequest request
    ) {
        String uid = authorizationService.getSecretUid(request);

        var purchaseOfferF = parallelHelper.async(() -> purchaseOfferDao.findByUidAndUuid(uid, purchaseOfferUuid));
        var purchasePaymentInfosF = parallelHelper.async(() -> billingService.getPurchaseOfferPayments(uid,
                purchaseOfferUuid));

        PurchaseOffer purchaseOffer = purchaseOfferF.join()
                .orElseThrow(() -> new NotFoundException("PurchaseOffer by params uid={" + uid + "}, " +
                        "purchaseOfferUuid={" + purchaseOfferUuid + "} not found"));

        PaymentInfo purchasePaymentInfo = purchasePaymentInfosF.join()
                .stream()
                .findFirst()
                .orElseThrow(() -> new NotFoundException("PaymentInfo by params uid={" + uid + "}, " +
                        "purchaseOfferUuid={" + purchaseOfferUuid + "} not found"));

        Optional<PurchaseInfo> purchaseInfoO =
                userPurchasesDAO.findByPurchaseTokenAndPaymentProcessor(purchasePaymentInfo.getPurchaseToken(),
                        PaymentProcessor.YANDEX_PAY);

        PurchaseInfo purchaseInfo = purchaseInfoO
                .orElseThrow(() -> new NotFoundException("PurchaseInfo not found"));

        ObjectNode purchasePayload;
        try {
            purchasePayload = objectMapper.readValue(purchaseInfo.getSelectedOption().getProviderPayload(),
                    ObjectNode.class);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }

        return new PurchaseOfferPaymentInfoResponse(
                purchaseOffer.getPurchaseRequestId(),
                purchaseOffer.getUuid(),
                convert(purchaseOffer.getStatus()),
                convert(purchasePaymentInfo.getStatus()),
                purchasePayload
        );
    }

    @GetMapping("billing/purchase_offer/{purchaseOfferUuid}/status")
    public PurchaseOfferStatusResponse purchaseOfferStatus(
            @PathVariable("purchaseOfferUuid") String purchaseOfferUuid,
            HttpServletRequest request
    ) {
        String uid = authorizationService.getSecretUid(request);

        var future = parallelHelper.async(() -> purchaseOfferDao.findByUidAndUuid(uid, purchaseOfferUuid));

        PaymentInfo.Status paymentStatus = billingService.getPurchaseOfferPayments(uid, purchaseOfferUuid).stream()
                .map(PaymentInfo::getStatus)
                .findFirst()
                .orElse(PaymentInfo.Status.ERROR_UNKNOWN);

        PurchaseOffer purchaseOffer = future.join().orElseThrow(() -> new NotFoundException("Purchase not found"));

        return new PurchaseOfferStatusResponse(convert(purchaseOffer.getStatus()), convert(paymentStatus));
    }

    private ru.yandex.alice.paskills.common.billing.model.PaymentStatus convert(PaymentInfo.Status paymentStatus) {
        switch (paymentStatus) {
            case CLEARED:
                return ru.yandex.alice.paskills.common.billing.model.PaymentStatus.CLEARED;
            case AUTHORIZED:
                return ru.yandex.alice.paskills.common.billing.model.PaymentStatus.AUTHORIZED;
            case ERROR_NOT_ENOUGH_FUNDS:
                return ru.yandex.alice.paskills.common.billing.model.PaymentStatus.ERROR_NOT_ENOUGH_FUNDS;
            case ERROR_EXPIRED_CARD:
                return ru.yandex.alice.paskills.common.billing.model.PaymentStatus.ERROR_EXPIRED_CARD;
            case ERROR_LIMIT_EXCEEDED:
                return ru.yandex.alice.paskills.common.billing.model.PaymentStatus.ERROR_LIMIT_EXCEEDED;
            case ERROR_TRY_LATER:
                return ru.yandex.alice.paskills.common.billing.model.PaymentStatus.ERROR_TRY_LATER;
            case ERROR_DO_NOT_TRY_LATER:
                return ru.yandex.alice.paskills.common.billing.model.PaymentStatus.ERROR_DO_NOT_TRY_LATER;
            case ERROR_UNKNOWN:
                return ru.yandex.alice.paskills.common.billing.model.PaymentStatus.ERROR_UNKNOWN;
            case REFUND:
                return ru.yandex.alice.paskills.common.billing.model.PaymentStatus.REFUND;
            default:
                throw new IllegalStateException("can't find common mode PaymentStatus for " + paymentStatus);
        }
    }

    private ru.yandex.alice.paskills.common.billing.model.PurchaseOfferStatus convert(PurchaseOfferStatus offerStatus) {
        switch (offerStatus) {
            case NEW:
                return ru.yandex.alice.paskills.common.billing.model.PurchaseOfferStatus.NEW;
            case STARTED:
                return ru.yandex.alice.paskills.common.billing.model.PurchaseOfferStatus.STARTED;
            case SUCCESS:
                return ru.yandex.alice.paskills.common.billing.model.PurchaseOfferStatus.SUCCESS;
            case FAILURE:
                return ru.yandex.alice.paskills.common.billing.model.PurchaseOfferStatus.FAILURE;
            default:
                throw new IllegalStateException("can't find common mode PurchaseOfferStatus for " + offerStatus);
        }
    }

    @PostMapping("billing/purchase_offer/{purchaseOfferUuid}/bind")
    public PurchaseOfferData bindPurchaseOfferToUser(
            @PathVariable("purchaseOfferUuid") String purchaseOfferUuid,
            HttpServletRequest request
    ) throws ExecutionException, InterruptedException {
        String uid = authorizationService.getSecretUid(request);
        billingService.bindPurchaseOfferToUser(purchaseOfferUuid, uid);

        return getPurchaseOfferData(purchaseOfferUuid, uid);
    }

    // for Tv
    @PostMapping(path = "billing/promo/tv/bind_card")
    public StartCardBindingResponse startBindingTv(
            @RequestParam("return_url") String returnUrl,
            @RequestParam(value = "processor", defaultValue = "MEDIABILLING") PaymentProcessor processor,
            @RequestParam(value = "wrap_store", defaultValue = "false") boolean wrapStore,
            HttpServletRequest request
    ) {
        StartCardBindingResponse cardBinding = startCardBinding(Objects.requireNonNullElse(processor,
                MEDIABILLING), returnUrl, TemplateTag.STARTTV, request, wrapStore);
        return cardBinding.withCookieUri(URI.create("https://yandex.ru/"));
    }

    // for PP
    @PostMapping(path = {"billing/startBinding"})
    public StartCardBindingResponse startBinding(
            @RequestParam(value = "processor", defaultValue = "TRUST") PaymentProcessor processor,
            @RequestParam("return_url") String returnUrl,
            @RequestParam(value = "template", defaultValue = "MOBILE") TemplateTag template,
            HttpServletRequest request
    ) {
        return startCardBinding(processor, returnUrl, Objects.requireNonNullElse(template, TemplateTag.MOBILE),
                request, false);
    }

    private StartCardBindingResponse startCardBinding(
            PaymentProcessor processor,
            String returnUrl,
            TemplateTag template,
            HttpServletRequest request,
            boolean warpStore
    ) {
        String uid = authorizationService.getSecretUid(request);
        String userIp = authorizationService.getUserIp(request);

        NewBindingInfo result = billingService.startBinding(
                uid, userIp, processor, TrustCurrency.RUB, returnUrl, template);
        String url = warpStore ?
                UriComponentsBuilder.fromUriString("https://dialogs.yandex.ru/store/tv_frame")
                        .queryParam("src", result.getBindingUrl())
                        .toUriString() :
                result.getBindingUrl();
        return new StartCardBindingResponse(url, result.getPurchaseToken());
    }

    @GetMapping(path = "billing/getBindingInfo")
    public BindingInfoResponse getBindingInfo(
            @RequestParam(value = "processor", defaultValue = "TRUST") PaymentProcessor processor,
            @RequestParam("purchase_token") String purchaseToken,
            HttpServletRequest request
    ) {
        return bindingInfo(processor, purchaseToken, request);
    }

    @GetMapping(path = "billing/promo/tv/binding_info")
    public BindingInfoResponse getBindingInfoTv(
            @RequestParam(value = "processor", defaultValue = "MEDIABILLING") PaymentProcessor processor,
            @RequestParam("purchase_token") String purchaseToken,
            HttpServletRequest request
    ) {
        return bindingInfo(processor, purchaseToken, request);
    }

    private BindingInfoResponse bindingInfo(
            PaymentProcessor processor,
            String purchaseToken,
            HttpServletRequest request
    ) {
        String uid = authorizationService.getSecretUid(request);
        String userIp = authorizationService.getUserIp(request);

        BindingInfo bindingInfo = billingService.getBindingInfo(uid, userIp, processor, purchaseToken);
        return new BindingInfoResponse(bindingInfo.getStatus().name(), bindingInfo.getPaymentMethodId());
    }

    @TvmRequired
    @GetMapping("billing/user/skill_product/{skill_uuid}")
    public UserSkillProductsResult getUserSkillProduct(
            @PathVariable("skill_uuid") String skillUuid,
            HttpServletRequest request
    ) {
        String uid = authorizationService.getSecretUid(request);

        var userProducts = billingService.getAllUserSkillProductItems(uid, skillUuid);

        return new UserSkillProductsResult(userProducts);
    }

    @TvmRequired
    @PostMapping("billing/user/skill_product/{skill_uuid}/{token_code}")
    public UserSkillProductActivationResult activateUserSkillProduct(
            @PathVariable("skill_uuid") String skillUuid,
            @PathVariable("token_code") String tokenCode,
            @Valid @RequestBody UserSkillProductActivationRequest data,
            HttpServletRequest request
    ) {
        String uid = authorizationService.getSecretUid(request);
        var activationData = new UserSkillProductActivationData(skillUuid, data.getSkillName(),
                data.getSkillImageUrl());
        return billingService.activateUserSkillProduct(uid, activationData, tokenCode);
    }

    @TvmRequired
    @DeleteMapping("billing/user/skill_product/{skill_uuid}/{product_uuid}")
    public SimpleResult deleteUserSkillProduct(
            @PathVariable("skill_uuid") String skillUuid,
            @PathVariable("product_uuid") String productUuid,
            HttpServletRequest request
    ) {
        String uid = authorizationService.getSecretUid(request);

        billingService.deleteUserSkillProduct(uid, skillUuid, UUID.fromString(productUuid));

        return SimpleResult.OK;
    }

    private PurchaseOfferData getPurchaseOfferData(String purchaseOfferUuid, String uid) throws InterruptedException,
            ExecutionException {
        PurchaseOffer purchaseOffer = billingService.getPurchaseOffer(uid, purchaseOfferUuid);
        if (purchaseOffer.getPricingOptions().size() > 1) {
            throw new InternalErrorException("Compound purchase offers are not supported");

        }

        PricingOption pricingOption = purchaseOffer.getPricingOptions().get(0);

        Optional<DeliveryInfo> purchaseOfferDeliveryInfo = purchaseOffer.getDeliveryInfo();
        DeliveryInfo delivery = purchaseOfferDeliveryInfo.orElse(null);
        String deliveryProductId = delivery != null ? delivery.getProductId() : null;

        PurchaseOfferData.PurchaseDeliveryInfo deliveryInfo = null;
        List<PurchaseOfferData.PurchaseOfferProductData> productDataList =
                new ArrayList<>(pricingOption.getItems().size());
        for (PricingOption.PricingOptionLine it : pricingOption.getItems()) {
            // extract delivery pricing option item to different object
            if (deliveryProductId == null || !deliveryProductId.equals(it.getProductId())) {
                PurchaseOfferData.PurchaseOfferProductData productData =
                        PurchaseOfferData.PurchaseOfferProductData.builder()
                                .productId(it.getProductId())
                                .title(it.getTitle())
                                .userPrice(it.getUserPrice())
                                .price(it.getPrice())
                                .quantity(it.getQuantity())
                                .ndsType(it.getNdsType())
                                .build();
                productDataList.add(productData);
            } else {
                deliveryInfo = PurchaseOfferData.PurchaseDeliveryInfo.builder()
                        .productId(it.getProductId())
                        .city(delivery.getCity())
                        .settlement(delivery.getSettlement())
                        .index(delivery.getIndex())
                        .street(delivery.getStreet())
                        .house(delivery.getHouse())
                        .housing(delivery.getHousing())
                        .building(delivery.getBuilding())
                        .porch(delivery.getPorch())
                        .floor(delivery.getFloor())
                        .flat(delivery.getFlat())
                        .price(it.getUserPrice())
                        .build();
            }
        }

        if (deliveryProductId != null && deliveryInfo == null) {
            throw new RuntimeException("Delivery info references non existing PricingOptionLine productId");
        }


        Future<SkillInfo> skill =
                parallelHelper.async(() -> skillsService.getSkillById(purchaseOffer.getSkillInfoId()));
        Future<MerchantInfo> merchant = parallelHelper.async(() -> {
            try {
                return skillsService.merchantInfo(skill.get().getSkillUuid(), purchaseOffer.getMerchantKey());
            } catch (InterruptedException | ExecutionException e) {
                throw new RuntimeException(e);
            }
        });
        Optional<PaymentInfo> paymentInfo = billingService.getPurchaseOfferPayments(uid, purchaseOfferUuid).stream()
                .findFirst();


        return PurchaseOfferData.
                builder(purchaseOffer.getUuid(), purchaseOffer.getPurchaseRequestId(), pricingOption.getOptionId())
                .name(purchaseOffer.getSkillName())
                .products(productDataList)
                .currency(pricingOption.getCurrency())
                .skillUuid(skill.get().getSkillUuid())
                .imageUrl(purchaseOffer.getSkillImageUrl())
                .merchantInfo(PurchaseOfferData.MerchantInfo.builder()
                        .description(purchaseOffer.getDescription())
                        .name(merchant.get().getOrganization().getName())
                        .inn(merchant.get().getOrganization().getInn())
                        .workingHours(merchant.get().getOrganization().getScheduleText())
                        .address(merchant.get().getLegalAddress())
                        .ogrn(merchant.get().getOrganization().getOgrn())
                        .build())
                .status(purchaseOffer.getStatus())
                .deliveryInfo(deliveryInfo)
                .paymentMethodInfo(paymentInfo
                        .filter(it -> it.getPaymentMethodId() != null)
                        .map(it -> new PurchaseOfferData.PaymentMethodInfo(
                                it.getPaymentMethodId(),
                                it.getAccount(),
                                it.getCardType()
                        ))
                        .orElse(null))
                .build();
    }
}
