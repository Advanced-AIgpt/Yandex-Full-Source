package ru.yandex.quasar.billing.services;

import java.math.BigDecimal;
import java.sql.Timestamp;
import java.time.Instant;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import javax.annotation.Nullable;
import javax.servlet.http.HttpServletRequest;

import com.google.common.base.Stopwatch;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.dao.DuplicateKeyException;
import org.springframework.data.relational.core.conversion.DbActionExecutionException;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskills.common.billing.model.api.SkillProductItem;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductActivationResult;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.SkillProductKeyConfig;
import ru.yandex.quasar.billing.controller.BillingController;
import ru.yandex.quasar.billing.dao.ProductTokenDao;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.PurchaseOffer;
import ru.yandex.quasar.billing.dao.PurchaseOfferDao;
import ru.yandex.quasar.billing.dao.PurchaseOfferStatus;
import ru.yandex.quasar.billing.dao.SkillInfo;
import ru.yandex.quasar.billing.dao.SkillInfoDAO;
import ru.yandex.quasar.billing.dao.SkillProduct;
import ru.yandex.quasar.billing.dao.SkillProductDao;
import ru.yandex.quasar.billing.dao.SubscriptionInfo;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.dao.UserSkillProduct;
import ru.yandex.quasar.billing.dao.UserSkillProductDao;
import ru.yandex.quasar.billing.dao.UserSubscriptionsDAO;
import ru.yandex.quasar.billing.exception.BadRequestException;
import ru.yandex.quasar.billing.exception.ForbiddenException;
import ru.yandex.quasar.billing.exception.InternalErrorException;
import ru.yandex.quasar.billing.exception.NotFoundException;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;
import ru.yandex.quasar.billing.exception.RetryPurchaseException;
import ru.yandex.quasar.billing.providers.ProviderPurchaseException;
import ru.yandex.quasar.billing.services.processing.NdsType;
import ru.yandex.quasar.billing.services.processing.ProcessingService;
import ru.yandex.quasar.billing.services.processing.ProcessingServiceManager;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;
import ru.yandex.quasar.billing.services.processing.trust.BindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.NewBindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.PaymentMethod;
import ru.yandex.quasar.billing.services.processing.trust.TemplateTag;
import ru.yandex.quasar.billing.services.processing.trust.TrustBillingService;
import ru.yandex.quasar.billing.services.processing.yapay.YaPayBillingService;
import ru.yandex.quasar.billing.services.promo.QuasarBackendClient;
import ru.yandex.quasar.billing.services.skills.MerchantInfo;
import ru.yandex.quasar.billing.services.skills.SkillsService;
import ru.yandex.quasar.billing.services.sup.MobilePushService;
import ru.yandex.quasar.billing.util.ParallelHelper;
import ru.yandex.quasar.billing.util.UuidHelper;

import static java.util.Collections.emptyList;
import static java.util.Objects.requireNonNull;
import static java.util.Objects.requireNonNullElse;
import static java.util.Optional.ofNullable;
import static java.util.stream.Collectors.toList;
import static ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductActivationResult.ActivationResult;
import static ru.yandex.quasar.billing.services.processing.yapay.PurchaseCompletionAction.REDIRECT_ALWAYS;
import static ru.yandex.quasar.billing.services.processing.yapay.YaPayBillingService.SKILL_PROVIDER_NAME;

@Component
public class BillingServiceImpl implements BillingService {

    private static final Logger log = LogManager.getLogger();

    private final UserSubscriptionsDAO userSubscriptionsDao;
    private final UserPurchasesDAO userPurchasesDAO;
    private final PurchaseOfferDao purchaseOfferDao;
    private final TrustBillingService trustBillingService;
    private final MobilePushService mobilePushService;
    private final UnistatService unistatService;
    private final SkillsService skillsService;
    private final Map<String, Long> providersMap;
    private final Map<String, List<SkillProductKeyConfig>> getDeviceTagToProvidedSkillProduct;
    private final AuthorizationContext authorizationContext;
    private final UserSkillProductDao userSkillProductDao;
    private final ProductTokenDao productTokenDao;
    private final SkillProductDao skillProductDao;
    private final ProcessingServiceManager processingServiceManager;
    private final YaPayBillingService yaPayBillingService;
    private final SkillInfoDAO skillInfoDao;
    private final QuasarBackendClient quasarBackendClient;

    private final int maxRetriesForProviderFailure;
    private final Map<String, ProviderOnPurchaseCallback> registeredCallbacks = new HashMap<>();
    private final UserPurchaseLockService userPurchaseLockService;
    private final ParallelHelper parallelHelper;

    @SuppressWarnings("ParameterNumber")
    public BillingServiceImpl(MobilePushService mobilePushService,
                              UserSubscriptionsDAO userSubscriptionsDao,
                              TrustBillingService trustBillingService,
                              UserPurchasesDAO userPurchasesDAO,
                              UnistatService unistatService,
                              PurchaseOfferDao purchaseOfferDao,
                              SkillsService skillsService,
                              AuthorizationContext authorizationContext,
                              BillingConfig config,
                              ProcessingServiceManager processingServiceManager,
                              UserSkillProductDao userSkillProductDao,
                              ProductTokenDao productTokenDao,
                              SkillProductDao skillProductDao,
                              YaPayBillingService yaPayBillingService,
                              SkillInfoDAO skillInfoDao,
                              UserPurchaseLockService userPurchaseLockService,
                              QuasarBackendClient quasarBackendClient,
                              @Qualifier("billingServiceExecutor") ExecutorService executorService) {
        this.mobilePushService = mobilePushService;
        this.userSubscriptionsDao = userSubscriptionsDao;
        this.trustBillingService = trustBillingService;
        this.userPurchasesDAO = userPurchasesDAO;
        this.unistatService = unistatService;
        this.purchaseOfferDao = purchaseOfferDao;
        this.skillsService = skillsService;
        this.authorizationContext = authorizationContext;
        this.maxRetriesForProviderFailure = config.getTrustBillingConfig().getMaxRetriesCount();
        this.providersMap = config.getTrustBillingConfig().getProvidersMapping();
        this.getDeviceTagToProvidedSkillProduct = config.getDeviceTagToProvidedSkillProduct();
        this.processingServiceManager = processingServiceManager;
        this.userSkillProductDao = userSkillProductDao;
        this.productTokenDao = productTokenDao;
        this.skillProductDao = skillProductDao;
        this.yaPayBillingService = yaPayBillingService;
        this.skillInfoDao = skillInfoDao;
        this.userPurchaseLockService = userPurchaseLockService;
        this.quasarBackendClient = quasarBackendClient;
        this.parallelHelper = new ParallelHelper(executorService, authorizationContext);
    }

    @Override
    public void registerCallback(String providerName, ProviderOnPurchaseCallback callbackFunction)
            throws ProviderCallbackAlreadyRegisteredException {
        if (!registeredCallbacks.containsKey(providerName)) {
            registeredCallbacks.put(providerName, callbackFunction);
        } else {
            throw new ProviderCallbackAlreadyRegisteredException(providerName);
        }
    }

    @Override
    @SuppressWarnings("ParameterNumber")
    public CreatedOffer createSkillPurchaseOffer(
            @Nullable String uid,
            SkillInfo skill,
            SkillOfferParams skillOfferParams,
            @Nullable String deviceId,
            SkillPurchaseOffer purchaseRequest,
            String sessionId,
            String userId,
            @Nullable String webhookRequest
    ) {

        if (!skill.getMerchants().containsKey(purchaseRequest.getMerchantKey())) {
            throw new BadRequestException("Wrong merchant key");
        }

        Optional<PurchaseOffer> existingRequest =
                purchaseOfferDao.findBySkillIdAndPurchaseRequestId(skill.getSkillInfoId(),
                        purchaseRequest.getPurchaseRequestId());
        if (existingRequest.isPresent()) {
            throw new BadRequestException("Purchase request duplicate");
        }

        PurchaseOffer offer;
        try {

            offer = purchaseOfferDao.save(
                    PurchaseOffer.builder(
                                    uid,
                                    skill.getSkillInfoId(),
                                    purchaseRequest.getPurchaseRequestId(),
                                    purchaseRequest.getPricingOptions(),
                                    skillOfferParams.getCallbackUrl())
                            .description(purchaseRequest.getDescription()
                            )
                            .title(skillOfferParams.getName())
                            .skillSessionId(sessionId)
                            .skillUserId(userId)
                            .deviceId(deviceId)
                            .skillName(skillOfferParams.getName())
                            .skillImageUrl(skillOfferParams.getImageUrl())
                            .merchantKey(purchaseRequest.getMerchantKey())
                            .deliveryInfo(purchaseRequest.getDeliveryInfo())
                            .webhookRequest(webhookRequest)
                            .testPayment(purchaseRequest.isTestPayment())
                            .build()
            );
        } catch (DbActionExecutionException e) {
            if (e.getCause() instanceof DuplicateKeyException) {
                throw new BadRequestException("Purchase offer duplicate");
            } else {
                throw e;
            }
        }

        String offerUrl = yaPayBillingService.getPurchaseOfferUrl(skill, offer, uid == null, deviceId, false);

        return new CreatedOffer(offer.getUuid(), offerUrl);
    }

    @Override
    public PurchaseOffer getPurchaseOffer(@Nullable String uid, String purchaseOfferUuid)
            throws PurchaseOfferNotFoundException {
        return purchaseOfferDao.findByUidAndUuid(uid, purchaseOfferUuid)
                .orElseThrow(() -> new PurchaseOfferNotFoundException(uid, purchaseOfferUuid));
    }

    @Override
    public void bindPurchaseOfferToUser(String purchaseOfferUuid, String uid) {
        boolean changed = purchaseOfferDao.bindToUser(purchaseOfferUuid, Objects.requireNonNull(uid));
        if (!changed) {
            throw new BadRequestException("Can't bind purchase offer to the user");
        }
    }

    @Override
    public NewBindingInfo startBinding(String uid, String userIp, PaymentProcessor processor, TrustCurrency currency,
                                       String returnUrl, TemplateTag template) {
        ProcessingService processingService = processingServiceManager.get(processor);
        return processingService.startBinding(uid, userIp, currency, returnUrl, template);
    }

    @Override
    public BindingInfo getBindingInfo(String uid, String userIp, PaymentProcessor processor, String purchaseToken) {
        ProcessingService processingService = processingServiceManager.get(processor);
        return processingService.getBindingInfo(uid, userIp, purchaseToken);
    }

    @Override
    public List<TransactionInfo> getTransactionHistory(String uid, String userIp) {

        List<PurchaseInfo> lastPurchases = userPurchasesDAO.getLastPurchases(Long.valueOf(uid))
                .stream()
                // leave only TRUST payments as we don't know parameters like account PaymentMethod and CardType
                .filter(it -> it.getPaymentProcessor() == PaymentProcessor.TRUST)
                .collect(Collectors.toList());

        List<TransactionInfo> transactions = parallelHelper.processParallelAsTasks(
                        lastPurchases,
                        p -> getTransactionsHistoryItem(p, userIp)
                ).stream()
                .filter(ParallelHelper.TaskResult::isSuccessful)
                .map(ParallelHelper.TaskResult::getResult)
                .filter(Objects::nonNull)
                // filter out trial payments
                .filter(item -> item.getAmount().compareTo(BigDecimal.ZERO) > 0)
                .collect(toList());

        return transactions;
    }

    @Override
    public List<TransactionInfoV2> getTransactionHistoryV2(String uid, String userIp, long limit, long offset) {
        List<PurchaseInfo> lastPurchases = userPurchasesDAO.getPurchases(Long.valueOf(uid), limit, offset);

        List<ParallelHelper.TaskResult<TransactionInfoV2>> taskResults = parallelHelper.processParallelAsTasks(
                lastPurchases,
                purchase -> getTransactionsHistoryItemV2(purchase, userIp)
        );
        return taskResults.stream()
                .filter(ParallelHelper.TaskResult::isSuccessful)
                .map(ParallelHelper.TaskResult::getResult)
                .filter(Objects::nonNull)
                .collect(toList());
    }

    @Override
    public Optional<TransactionInfoV2> getTransactionV2(String uid, String userIp, long purchaseId) {
        Optional<PurchaseInfo> purchase = userPurchasesDAO.getPurchaseInfo(Long.valueOf(uid), purchaseId);

        return purchase.map(p -> getTransactionsHistoryItemV2(p, userIp));
    }

    @Override
    public List<SkillProductItem> getAllUserSkillProductItems(String uid, String skillUuid) {
        CompletableFuture<List<SkillProduct>> productsFromDeviceTags = parallelHelper.async(
                () -> getSkillProductsFromUserDeviceTags(uid, skillUuid));

        Set<SkillProduct> userProducts = getUserSkillProducts(uid, skillUuid).stream()
                .map(UserSkillProduct::getSkillProduct)
                .collect(Collectors.toSet());

        userProducts.addAll(productsFromDeviceTags.join());

        return userProducts.stream()
                .map(this::convert)
                .collect(Collectors.toList());
    }

    public SkillProductItem convert(UserSkillProduct userSkillProduct) {
        return new SkillProductItem(
                userSkillProduct.getSkillProduct().getUuid().toString(),
                userSkillProduct.getSkillProduct().getName()
        );
    }

    public SkillProductItem convert(SkillProduct skillProduct) {
        return new SkillProductItem(
                skillProduct.getUuid().toString(),
                skillProduct.getName()
        );
    }

    private List<UserSkillProduct> getUserSkillProducts(String uid, String skillUuid) {
        log.info("Skill product requested for userId = {} and skillUuid = {}", uid, skillUuid);

        return userSkillProductDao.getUserSkillProducts(uid, skillUuid);
    }

    private List<SkillProduct> getSkillProductsFromUserDeviceTags(String uid, String skillUuid) {
        log.info("Skill product from device tags requested for userId = {} and skillUuid = {}", uid, skillUuid);

        Set<UUID> productsFromTags = quasarBackendClient.getUserDeviceTags(uid).values()
                .stream()
                .flatMap(Set::stream)
                .filter(getDeviceTagToProvidedSkillProduct::containsKey)
                .map(getDeviceTagToProvidedSkillProduct::get)
                .flatMap(Collection::stream)
                .filter(skillProductKey -> skillProductKey.getSkillUuid().equals(skillUuid))
                .map(SkillProductKeyConfig::getProductUuid)
                .map(UuidHelper::fromString)
                .flatMap(Optional::stream)
                .collect(Collectors.toSet());

        return productsFromTags.isEmpty() ? emptyList() : skillProductDao.getSkillProducts(productsFromTags, skillUuid);
    }

    @Override
    public UserSkillProductActivationResult activateUserSkillProduct(
            String uid,
            UserSkillProductActivationData activationData,
            String tokenCode
    ) {
        String skillUuid = activationData.getSkillUuid();

        var skillInfoId = skillInfoDao.findBySkillId(skillUuid).map(SkillInfo::getSkillInfoId).orElseThrow(() -> {
            unistatService.incrementStatValue("quasar_billing-user-skill-product_unknown-skill_uuid_calls_dmmm");
            return new BadRequestException("Can't find skill uuid: " + skillUuid);
        });

        var productToken = productTokenDao.getProductToken(tokenCode, skillUuid)
                .orElseThrow(() -> {
                    unistatService.incrementStatValue("quasar_billing-user-skill-product_unknown-token_calls_dmmm");
                    return new BadRequestException("Can't find token by provided code: " + tokenCode
                            + " for skill: " + skillUuid);
                });
        UUID productUuid = productToken.getSkillProduct().getUuid();
        var skillProduct = skillProductDao.getSkillProduct(productUuid, skillUuid)
                .orElseThrow(() -> {
                    unistatService.incrementStatValue("quasar_billing-user-skill-product_unknown-skill_calls_dmmm");
                    return new BadRequestException(
                            "Can't find skill product: " + productUuid + " for skill: " + skillUuid);
                });

        var userSkillProduct = userSkillProductDao.getUserSkillProduct(uid, skillUuid, productUuid);
        if (userSkillProduct.isPresent()) {
            unistatService.incrementStatValue("quasar_billing-user-skill-product_already-activated_calls_dmmm");
            return new UserSkillProductActivationResult(
                    ActivationResult.ALREADY_ACTIVATED,
                    convert(userSkillProduct.get())
            );
        }

        UserSkillProduct newUserSkillProduct = UserSkillProduct.builder()
                .uid(uid)
                .productToken(productToken)
                .skillProduct(skillProduct)
                .build();

        userSkillProductDao.createUserSkillProduct(newUserSkillProduct, activationData);
        Long userProductId = userSkillProductDao.getUserSkillProduct(uid, skillUuid, productUuid).orElseThrow().getId();
        saveFreeProductActivationToUserPurchases(uid, skillInfoId, skillProduct, userProductId);

        unistatService.incrementStatValue("quasar_billing-user-skill-product_success_calls_dmmm");
        return new UserSkillProductActivationResult(
                ActivationResult.SUCCESS,
                convert(newUserSkillProduct)
        );
    }

    private void saveFreeProductActivationToUserPurchases(
            String uid,
            Long skillInfoId,
            SkillProduct skillProduct,
            Long userProductId
    ) {
        Long purchaseId = userPurchasesDAO.getNextPurchaseId();

        PricingOption.PricingOptionLine pricingOptionLine = new PricingOption.PricingOptionLine(
                skillProduct.getUuid().toString(),
                BigDecimal.ZERO,
                BigDecimal.ZERO,
                skillProduct.getName(),
                BigDecimal.ONE,
                NdsType.nds_none);

        var selectedOption = PricingOption.builder(
                        skillProduct.getName(),
                        PricingOptionType.BUY,
                        BigDecimal.ZERO,
                        BigDecimal.ZERO,
                        "RUB")
                .items(List.of(pricingOptionLine))
                .build();

        var purchaseInfo = PurchaseInfo.createFreeSkillProductPayment(
                purchaseId,
                Long.valueOf(uid),
                purchaseId + "|" + skillProduct.getUuid(),
                selectedOption,
                PurchaseInfo.Status.CLEARED,
                userProductId,
                SKILL_PROVIDER_NAME,
                skillInfoId);

        userPurchasesDAO.savePurchaseInfo(purchaseInfo);
    }

    @Override
    public void deleteUserSkillProduct(String uid, String skillUuid, UUID productUuid) {
        if (userSkillProductDao.getUserSkillProduct(uid, skillUuid, productUuid).isEmpty()) {
            var msg = "User " + uid + " doesn't have productUuid = " + productUuid + " for skillUuid = " + skillUuid;
            log.error(msg);
            throw new BadRequestException(msg);
        }
        userSkillProductDao.deleteUserSkillProduct(uid, skillUuid, productUuid);
    }

    private TransactionInfo getTransactionsHistoryItem(PurchaseInfo purchaseInfo, String userIp) {
        PaymentInfo paymentShortInfo;
        try {
            paymentShortInfo = processingServiceManager.get(purchaseInfo.getPaymentProcessor())
                    .getPaymentShortInfo(purchaseInfo.getPurchaseToken(), purchaseInfo.getUid().toString(), userIp);
        } catch (Exception e) {
            throw new InternalErrorException("Failed to get payment info from trust", e);
        }

        return TransactionInfo.builder()
                .purchaseToken(purchaseInfo.getPurchaseToken())
                .provider(purchaseInfo.getProvider())
                .title(purchaseInfo.getSelectedOption().getTitle())
                .account(paymentShortInfo.getAccount())
                .cardType(paymentShortInfo.getCardType())
                .paymentDate(paymentShortInfo.getPaymentDate())
                .amount(paymentShortInfo.getAmount())
                .currency(paymentShortInfo.getCurrency())
                .build();
    }

    private TransactionInfoV2 getTransactionsHistoryItemV2(PurchaseInfo purchaseInfo, String userIp) {
        PaymentInfo paymentShortInfo = null;
        try {
            paymentShortInfo = processingServiceManager.get(purchaseInfo.getPaymentProcessor())
                    .getPaymentShortInfo(purchaseInfo.getPurchaseToken(), purchaseInfo.getUid().toString(), userIp);
        } catch (Exception e) {
            log.error("Failed to get payment info from purchase id = " + purchaseInfo.getPurchaseId());
        }

        TransactionInfoV2.SkillInfo transactionSkillInfo = getTransactionSkillInfo(purchaseInfo);

        return TransactionInfoV2.builder()
                .purchaseId(purchaseInfo.getPurchaseId())
                .purchaseType(purchaseInfo.getProvider())
                .skillInfo(transactionSkillInfo)
                .maskedCardNumber(paymentShortInfo == null ? null : paymentShortInfo.getAccount())
                .paymentSystem(paymentShortInfo == null ? null : paymentShortInfo.getCardType())
                .totalPrice(paymentShortInfo == null ? purchaseInfo.getUserPrice() : paymentShortInfo.getAmount())
                .currency(paymentShortInfo == null ? purchaseInfo.getCurrencyCode() : paymentShortInfo.getCurrency())
                .status(purchaseInfo.getStatus())
                .statusChangeDate(paymentShortInfo == null ? purchaseInfo.getPurchaseDate()
                        : paymentShortInfo.getPaymentDate())
                .pricingType(purchaseInfo.getSelectedOption().getType())
                .basket(purchaseInfo.getSelectedOption().getItems().stream()
                        .map(item -> TransactionInfoV2.Basket.builder()
                                .title(item.getTitle())
                                .userPrice(item.getUserPrice())
                                .price(item.getPrice())
                                .quantity(item.getQuantity())
                                .ndsType(item.getNdsType())
                                .build()
                        ).collect(Collectors.toUnmodifiableList())
                )
                .build();
    }

    @Nullable
    private TransactionInfoV2.SkillInfo getTransactionSkillInfo(PurchaseInfo purchaseInfo) {
        Long skillInfoId = purchaseInfo.getSkillInfoId();
        if (!SKILL_PROVIDER_NAME.equals(purchaseInfo.getProvider()) || skillInfoId == null) {
            return null;
        }

        String slug = skillInfoDao.findBySkillId(skillInfoId)
                .map(SkillInfo::getSlug)
                .orElse("");

        if (purchaseInfo.getPurchaseOfferId() != null) {
            return purchaseOfferDao.findBySkillIdAndPurchaseOfferId(skillInfoId, purchaseInfo.getPurchaseOfferId())
                    .map(purchaseOffer -> TransactionInfoV2.SkillInfo.builder()
                            .name(purchaseOffer.getSkillName())
                            .logoUrl(purchaseOffer.getSkillImageUrl())
                            .slug(slug)
                            .merchantName(purchaseInfo.getMerchantName())
                            .build())
                    .orElse(null);
        } else if (purchaseInfo.getUserSkillProductId() != null) {
            return userSkillProductDao.getUserSkillProductById(purchaseInfo.getUserSkillProductId())
                    .map(userSkillProduct -> TransactionInfoV2.SkillInfo.builder()
                            .name(userSkillProduct.getSkillName())
                            .logoUrl(userSkillProduct.getSkillImageUrl())
                            .slug(slug)
                            .merchantName(purchaseInfo.getMerchantName())
                            .build())
                    .orElse(null);
        }
        return null;
    }

    @Override
    public List<PaymentInfo> getPurchaseOfferPayments(String uid, String purchaseOfferUuid) {
        PurchaseOffer purchaseOffer = purchaseOfferDao.findByUidAndUuid(uid, purchaseOfferUuid)
                .orElseThrow(() -> new PurchaseOfferNotFoundException(uid, purchaseOfferUuid));
        // list is sorted by purchaseDate
        List<PurchaseInfo> purchases = userPurchasesDAO.getAllSucceededByPurchaseOfferId(purchaseOffer.getId());

        List<PaymentInfo> paymentInfos = parallelHelper.processParallel(purchases, this::getPaymentInfo);

        return paymentInfos;
    }

    private PaymentInfo getPaymentInfo(PurchaseInfo purchase) {
        ProcessingService processingService = processingServiceManager.get(purchase.getPaymentProcessor());
        return processingService
                .getPaymentShortInfo(purchase.getPurchaseToken(), purchase.getUid().toString(),
                        authorizationContext.getUserIp());
    }

    @SuppressWarnings("ParameterNumber")
    @Override
    public String initPurchase(String uid, String provider, PricingOption selectedOption,
                               ProviderContentItem initiallyRequestedItem, String userIp, String paymentCardId,
                               String userEmail, @Nullable Integer trialPeriodDays) {
        Long partnerId = requireNonNull(providersMap.get(provider));
        PaymentProcessor paymentProcessor = Objects.requireNonNullElse(selectedOption.getProcessor(),
                PaymentProcessor.TRUST);
        if (selectedOption.getType() == PricingOptionType.SUBSCRIPTION) {
            // Не даём покупать подписку если уже есть активная или только что кончившаяся подписка такого же типа
            Set<ProviderContentItem> pendingSubscriptions =
                    userSubscriptionsDao.getPendingSubscriptions(Long.valueOf(uid))
                            .stream()
                            .map(SubscriptionInfo::getContentItem)
                            .collect(Collectors.toSet());

            if (pendingSubscriptions.contains(initiallyRequestedItem)) {
                return createExceptionalPurchase(uid, provider, PurchaseInfo.Status.ALREADY_AVAILABLE,
                        initiallyRequestedItem, selectedOption, partnerId, paymentProcessor);
            } else {

                return startSubscription(paymentProcessor, uid, selectedOption, ofNullable(trialPeriodDays).orElse(0),
                        partnerId, initiallyRequestedItem, provider, userIp, paymentCardId, userEmail);
            }
        } else {
            return startPayment(paymentProcessor, uid, selectedOption, partnerId, provider, initiallyRequestedItem,
                    userIp, paymentCardId, userEmail).getPurchaseToken();
        }
    }

    @SuppressWarnings("ParameterNumber")
    @Override
    public NewPurchase initPurchaseOffer(String uid, PurchaseOffer purchaseOfferData, String selectedOptionUuid,
                                         SkillInfo skill, String userIp, @Nullable String paymentCardId,
                                         String userEmail, boolean testPayment) {

        PricingOption selectedOption = purchaseOfferData.getPricingOptions().stream()
                .filter(it -> selectedOptionUuid.equals(it.getOptionId()))
                .findFirst()
                .orElseThrow(() -> new ProductNotFoundInPurchaseException(uid, purchaseOfferData.getUuid(),
                        selectedOptionUuid));

        if (selectedOption.getType() == PricingOptionType.SUBSCRIPTION) {
            throw new BadRequestException("Unsupported pricing option type: " + selectedOption.getType());
        } else {
            MerchantInfo serviceMerchantInfo = skillsService.merchantInfo(skill.getSkillUuid(),
                    purchaseOfferData.getMerchantKey());
            if (!serviceMerchantInfo.isEnabled()) {
                throw new ForbiddenException("Wrong merchant token. Access denied");
            }

            setOfferStatus(purchaseOfferData, PurchaseOfferStatus.STARTED);
            try {

                // генерим id покупки
                Long purchaseId = userPurchasesDAO.getNextPurchaseId();

                String purchaseToken = yaPayBillingService.createPayment(uid, skill,
                        purchaseOfferData, selectedOption, paymentCardId, userEmail,
                        serviceMerchantInfo.getServiceMerchantId());

                // сохраняем id корзины у нас
                var purchaseInfo = PurchaseInfo.createSinglePayment(
                        purchaseId,
                        Long.valueOf(uid),
                        purchaseToken,
                        null,
                        selectedOption,
                        PurchaseInfo.Status.STARTED,
                        null,
                        null,
                        purchaseOfferData.getId(),
                        SKILL_PROVIDER_NAME,
                        skill.getSkillInfoId(),
                        PaymentProcessor.YANDEX_PAY,
                        serviceMerchantInfo.getServiceMerchantId(),
                        serviceMerchantInfo.getOrganization().getFullName());
                userPurchasesDAO.savePurchaseInfo(purchaseInfo);

                Optional<String> trustUrl = yaPayBillingService.startPayment(purchaseToken, REDIRECT_ALWAYS);

                //TODO: обработка кода ошибки от trust-а
                return new NewPurchase(purchaseToken, trustUrl.orElse(null));
            } catch (Exception e) {
                setOfferStatus(purchaseOfferData, PurchaseOfferStatus.FAILURE);
                throw e;
            }

        }
    }


    private String generateSecurityToken() {
        return UUID.randomUUID().toString().replace("-", "");
    }

    @SuppressWarnings("ParameterNumber")
    private NewPurchase startPayment(
            PaymentProcessor paymentProcessor,
            String uid,
            PricingOption selectedOption,
            Long partnerId,
            @Nullable String provider,
            @Nullable ProviderContentItem contentItem,
            String userIp,
            String paymentCardId,
            String userEmail
    ) {
        ProcessingService processingService = processingServiceManager.get(paymentProcessor);
        // генерим id покупки
        Long purchaseId = userPurchasesDAO.getNextPurchaseId();

        // формируем ссылку callback-а
        // security token is used to verify trust callback
        // used only for non-subscription purchases as we control creation of the purchase
        String securityToken = generateSecurityToken();

        String purchaseToken = processingService.createPayment(uid, partnerId, purchaseId.toString(), securityToken,
                selectedOption, paymentCardId, userIp, userEmail, null);

        // сохраняем id корзины у нас
        var purchaseInfo = PurchaseInfo.createSinglePayment(
                purchaseId,
                Long.valueOf(uid),
                purchaseToken,
                contentItem,
                selectedOption,
                PurchaseInfo.Status.STARTED,
                securityToken,
                partnerId,
                null,
                provider,
                null,
                paymentProcessor,
                null,
                null);
        userPurchasesDAO.savePurchaseInfo(purchaseInfo);

        processingService.startPayment(uid, userIp, purchaseToken);

        //TODO: обработка кода ошибки от trust-а
        return new NewPurchase(purchaseToken, null);

    }

    @SuppressWarnings("ParameterNumber")
    private String startSubscription(PaymentProcessor paymentProcessor, String uid, PricingOption selectedOption,
                                     int trialPeriodDays, Long partnerId,
                                     @Nullable ProviderContentItem purchasingItem, @Nullable String provider,
                                     String userIp, String paymentCardId, String userEmail) {
        ProcessingService processingService = processingServiceManager.get(paymentProcessor);
        // генерим id подписки
        Long subscriptionId = userSubscriptionsDao.getNextSubscriptionId();
        String securityToken = generateSecurityToken();

        String purchaseToken = processingService.createSubscription(uid, partnerId, subscriptionId.toString(),
                securityToken, selectedOption, trialPeriodDays, paymentCardId, userIp, userEmail, provider);

        // сохраняем подписку у нас
        long currentTimeMillis = System.currentTimeMillis();
        SubscriptionInfo subscriptionInfo = SubscriptionInfo.create(
                subscriptionId,
                Long.valueOf(uid),
                Timestamp.from(Instant.now()),
                purchasingItem,
                selectedOption,
                // mediabilling subscriptions are activated on purchase
                trialPeriodDays != 0 || paymentProcessor == PaymentProcessor.MEDIABILLING ?
                        SubscriptionInfo.Status.ACTIVE :
                        SubscriptionInfo.Status.CREATED,
                new Timestamp(currentTimeMillis + TimeUnit.DAYS.toMillis(trialPeriodDays)),
                securityToken,
                trialPeriodDays,
                /*subscriptionParams.getProductCode()*/null,
                partnerId,
                provider,
                selectedOption.getPurchasingItem(),
                paymentProcessor);
        userSubscriptionsDao.save(subscriptionInfo);

        getOrCreatePurchaseInfoForSubscription(subscriptionInfo, purchaseToken, trialPeriodDays != 0);

        processingService.startSubscription(uid, userIp, purchaseToken);

        return purchaseToken;
    }

    @Override
    public ProcessPurchaseResult processPurchaseCallback(String purchaseId, @Nullable String securityToken,
                                                         String userIp) {
        PurchaseInfo purchaseInfo = userPurchasesDAO.getPurchaseInfo(Long.valueOf(purchaseId))
                .orElseThrow(() -> new NotFoundException(String.format("Could not find purchase with id='%s'",
                        purchaseId)));

        if (!Objects.equals(securityToken, purchaseInfo.getSecurityToken())) {
            throw new NotFoundException(String.format("Could not find purchase with id='%s'. Wrong token.",
                    purchaseId));
        }

        authorizationContext.setCurrentUid(purchaseInfo.getUid().toString());

        // lock to protect from concurrency
        BillingService.ProcessPurchaseResult processPurchaseResult =
                userPurchaseLockService.processWithLock(purchaseInfo.getUid(), purchaseInfo.getPartnerId().toString(),
                        () -> processPurchaseCallback(purchaseInfo, userIp));

        return processPurchaseResult;
    }

    @Override
    public ProcessPurchaseResult processYaPayPurchaseCallback(String purchaseToken, String userIp)
            throws UserPurchaseLockService.UserPurchaseLockException {
        PurchaseInfo purchaseInfo = userPurchasesDAO.findByPurchaseTokenAndPaymentProcessor(purchaseToken,
                        PaymentProcessor.YANDEX_PAY)
                .orElseThrow(() -> new NotFoundException(String.format("Could not find purchase with id='%s'",
                        purchaseToken)));

        authorizationContext.setCurrentUid(purchaseInfo.getUid().toString());

        // lock to protect from concurrency
        BillingService.ProcessPurchaseResult processPurchaseResult =
                userPurchaseLockService.processWithLock(purchaseInfo.getUid(), purchaseInfo.getMerchantId().toString(),
                        () -> processPurchaseCallback(purchaseInfo, userIp));

        return processPurchaseResult;
    }

    private ProcessPurchaseResult processPurchaseCallback(PurchaseInfo purchaseInfo, String userIp) {
        unistatService.incrementStatValue("quasar_billing_payment-callback_calls_dmmm");
        if (purchaseInfo.getCallbackDate() == null) {
            long currentTimeMillis = System.currentTimeMillis();
            userPurchasesDAO.updatePurchaseCallbackDate(purchaseInfo.getPurchaseId(), new Timestamp(currentTimeMillis));
            unistatService.logOperationDurationHist("quasar_billing_payment-callback-first_duration_dhhh",
                    currentTimeMillis - purchaseInfo.getPurchaseDate().toEpochMilli());
            unistatService.incrementStatValue("quasar_billing_payment-callback-first_calls_dmmm");
        }

        if (purchaseInfo.getStatus() == PurchaseInfo.Status.CLEARED ||
                purchaseInfo.getStatus() == PurchaseInfo.Status.WAITING_FOR_CLEARING) {
            return BillingService.ProcessPurchaseResult.ALREADY_PROCESSED;
        }

        ProcessingService processingService = processingServiceManager.get(purchaseInfo.getPaymentProcessor());

        PurchaseOffer purchaseOffer;
        if (purchaseInfo.getPurchaseOfferId() != null) {
            purchaseOffer = purchaseOfferDao.findById(purchaseInfo.getPurchaseOfferId())
                    .orElseThrow(() -> new NotFoundException("Purchase offer not found"));
        } else {
            purchaseOffer = null;
        }

        try {
            // Проверяем что деньги успешно за-hold-ились
            PaymentInfo payment = processingService.getPaymentShortInfo(
                    purchaseInfo.getPurchaseToken(),
                    purchaseInfo.getUid().toString(),
                    userIp);

            if (payment.getStatus() != PaymentInfo.Status.AUTHORIZED &&
                    payment.getStatus() != PaymentInfo.Status.CLEARED &&
                    payment.getStatus() != PaymentInfo.Status.REFUND
            ) {
                PurchaseInfo.Status status;
                switch (payment.getStatus()) {
                    case ERROR_NOT_ENOUGH_FUNDS:
                        status = PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS;
                        break;
                    case ERROR_EXPIRED_CARD:
                        status = PurchaseInfo.Status.ERROR_EXPIRED_CARD;
                        break;
                    case ERROR_LIMIT_EXCEEDED:
                        status = PurchaseInfo.Status.ERROR_LIMIT_EXCEEDED;
                        break;
                    case ERROR_TRY_LATER:
                        status = PurchaseInfo.Status.ERROR_TRY_LATER;
                        break;
                    case ERROR_DO_NOT_TRY_LATER:
                        status = PurchaseInfo.Status.ERROR_DO_NOT_TRY_LATER;
                        break;
                    default:
                        status = PurchaseInfo.Status.ERROR_UNKNOWN;
                        break;
                }
                userPurchasesDAO.updatePurchaseStatus(purchaseInfo.getPurchaseId(), status);
                setOfferStatus(purchaseOffer, PurchaseOfferStatus.FAILURE);
                unistatService.incrementStatValue(
                        "quasar_billing_payment-callback-payment_error_dmmm");
                unistatService.incrementStatValue(
                        "quasar_billing_payment-callback-payment_error_status_"
                                + status +
                                "_dmmm");
                return ProcessPurchaseResult.PAYMENT_ERROR;
            }

            // send callback to provider to process purchase
            if (purchaseInfo.getProvider() != null && registeredCallbacks.containsKey(purchaseInfo.getProvider())) {
                ProviderOnPurchaseCallback callback = registeredCallbacks.get(purchaseInfo.getProvider());
                callback.run(purchaseInfo.getProvider(), purchaseInfo.getUid().toString(),
                        purchaseInfo.getContentItem(), purchaseInfo.getSelectedOption(),
                        purchaseInfo.getPurchaseId().toString(), purchaseInfo.getPurchaseToken());
            } else if (purchaseOffer != null) {
                Stopwatch sw = Stopwatch.createStarted();
                skillsService.executeSkillCallback(purchaseOffer,
                        purchaseInfo.getSelectedOption().getProviderPayload(),
                        purchaseInfo.getPurchaseToken(),
                        purchaseOffer.getWebhookRequest(),
                        SkillsService.PurchaseEventType.PURCHASE);
                unistatService.logOperationDurationHist(
                        "quasar_billing_payment-skill-callback_duration_dhhh", sw.elapsed(TimeUnit.MILLISECONDS)
                );
            } else {
                throw new InternalErrorException("Provider or purchaseOfferId must be specified");
            }

        } catch (ProviderPurchaseException e) {

            log.warn(e.getMessage());

            if (e.getPurchaseStatus() == PurchaseInfo.Status.ERROR_TRY_LATER &&
                    purchaseInfo.getRetriesCount() < maxRetriesForProviderFailure) {
                userPurchasesDAO.incrementPurchaseRetries(purchaseInfo.getPurchaseId());
                userPurchasesDAO.updatePurchaseStatus(purchaseInfo.getPurchaseId(), e.getPurchaseStatus());
                // re-raise exception so that Trust retries the purchase later
                // Если хотим чтобы попытки продолжались то unhold делать не надо
                unistatService.incrementStatValue("quasar_billing_payment-callback_retry_dmmm");
                throw new RetryPurchaseException("Need to retry purchase", e);
            } else {
                // отменяем платёж если его провести на стороне провайдера не получилось и retry-и не помогут
                processingService.unholdPayment(purchaseInfo.getPurchaseToken(), purchaseInfo.getUid().toString(),
                        userIp);
                userPurchasesDAO.updatePurchaseStatus(purchaseInfo.getPurchaseId(), e.getPurchaseStatus());

                unistatService.incrementStatValue("quasar_billing_payment-callback_failures_dmmm");
                setOfferStatus(purchaseOffer, PurchaseOfferStatus.FAILURE);

                return BillingService.ProcessPurchaseResult.CANCELLED;
            }
        } catch (ProviderUnauthorizedException e) {
            log.warn(e.getMessage());
            processingService.unholdPayment(purchaseInfo.getPurchaseToken(), purchaseInfo.getUid().toString(), userIp);
            userPurchasesDAO.updatePurchaseStatus(purchaseInfo.getPurchaseId(),
                    PurchaseInfo.Status.ERROR_NO_PROVIDER_ACC);

            unistatService.incrementStatValue("quasar_billing_payment-callback_failures_dmmm");
            setOfferStatus(purchaseOffer, PurchaseOfferStatus.FAILURE);
            return BillingService.ProcessPurchaseResult.CANCELLED;
        } catch (Exception e) {
            // Если не смогли классифицировать ошибку
            log.warn("Fatal purchase exception: " + e.getMessage());
            processingService.unholdPayment(purchaseInfo.getPurchaseToken(), purchaseInfo.getUid().toString(), userIp);
            userPurchasesDAO.updatePurchaseStatus(purchaseInfo.getPurchaseId(), PurchaseInfo.Status.ERROR_UNKNOWN);

            unistatService.incrementStatValue("quasar_billing_payment-callback_failures_dmmm");
            setOfferStatus(purchaseOffer, PurchaseOfferStatus.FAILURE);
            return BillingService.ProcessPurchaseResult.PAYMENT_ERROR;
        }

        // обновляем статус покупки на обработанный
        userPurchasesDAO.updatePurchaseStatus(purchaseInfo.getPurchaseId(), PurchaseInfo.Status.PROCESSED);

        // делаем clear платежа в биллинге
        processingService.clearPayment(purchaseInfo.getPurchaseToken(), purchaseInfo.getUid().toString(), userIp);

        // обновляем статус покупки на закрытый
        userPurchasesDAO.updatePurchaseStatus(purchaseInfo.getPurchaseId(), PurchaseInfo.Status.WAITING_FOR_CLEARING);

        setOfferStatus(purchaseOffer, PurchaseOfferStatus.SUCCESS);

        return BillingService.ProcessPurchaseResult.OK;
    }

    private void setOfferStatus(@Nullable PurchaseOffer purchaseOffer, PurchaseOfferStatus status) {
        if (purchaseOffer != null) {
            purchaseOffer.setStatus(status);
            purchaseOfferDao.save(purchaseOffer);
        }
    }

    @Override
    public ProcessRefundResult processRefundCallback(String purchaseToken) {
        return userPurchasesDAO.getPurchaseInfo(purchaseToken)
                .map(purchaseInfo -> {
                    // if the purchase succeeded we mark it as refunded but if it failed we leave the error status
                    // unchanged
                    if (!purchaseInfo.getStatus().isError()) {
                        userPurchasesDAO.setRefunded(purchaseInfo.getPurchaseId());
                    }
                    return BillingService.ProcessRefundResult.PURCHASE_REFUNDED;
                })
                .orElse(BillingService.ProcessRefundResult.PURCHASE_NOT_FOUND);
    }

    @Override
    public Optional<Long> getSubscriptionForPurchase(String purchaseToken) {
        return userPurchasesDAO.getPurchaseInfo(purchaseToken)
                .map(PurchaseInfo::getSubscriptionId);
    }

    /**
     * Creates {@link PurchaseInfo} record with generated purchase token to store failed purchase request info and
     * stores it to DB
     * Needed as PP understands purchase state and failures calling
     * {@link BillingController#getPurchaseStatus(String, HttpServletRequest)}
     */
    @Override
    public String createExceptionalPurchase(String uid, String provider, PurchaseInfo.Status status,
                                            @Nullable ProviderContentItem providerContentItem,
                                            PricingOption selectedOption, @Nullable Long partnerId,
                                            PaymentProcessor paymentProcessor) {
        String errorPurchaseToken = "ERR" + UUID.randomUUID().toString();
        PurchaseInfo purchaseInfo = PurchaseInfo.createSinglePayment(userPurchasesDAO.getNextPurchaseId(),
                Long.valueOf(uid),
                errorPurchaseToken,
                providerContentItem,
                selectedOption,
                status,
                null,
                partnerId != null ? partnerId : getPartnerId(provider),
                null,
                provider,
                null,
                paymentProcessor,
                null,
                null);
        userPurchasesDAO.savePurchaseInfo(purchaseInfo);
        return errorPurchaseToken;
    }

    @Override
    public List<PaymentMethod> getCardsList(String uid, @Nullable PaymentProcessor processor, String userIp) {
        if (processor != null) {
            ProcessingService processingService = processingServiceManager.get(processor);
            return processingService.getCardsList(uid, userIp);
        } else {
            // fallback for old API
            return trustBillingService.getCardsList(uid, userIp);
        }
    }


    private PurchaseInfo getOrCreatePurchaseInfoForSubscription(SubscriptionInfo subscriptionInfo,
                                                                String purchaseToken, boolean trial) {
        PurchaseInfo purchaseInfo = userPurchasesDAO.getPurchaseInfo(subscriptionInfo.getUid(), purchaseToken);

        if (purchaseInfo != null) {
            return purchaseInfo;
        }

        Long purchaseId = userPurchasesDAO.getNextPurchaseId();

        PaymentProcessor paymentProcessor = Objects.requireNonNullElse(subscriptionInfo.getPaymentProcessor(),
                PaymentProcessor.TRUST);
        PurchaseInfo result = PurchaseInfo.createSubscriptionPayment(
                purchaseId,
                subscriptionInfo.getUid(),
                purchaseToken,
                subscriptionInfo.getContentItem() != null ? subscriptionInfo.getContentItem() :
                        subscriptionInfo.getPurchasedContentItem(),
                subscriptionInfo.getSelectedOption(),
                paymentProcessor == PaymentProcessor.MEDIABILLING || trial ? PurchaseInfo.Status.CLEARED :
                        PurchaseInfo.Status.STARTED,
                subscriptionInfo.getSubscriptionId(),
                trial,
                requireNonNullElse(subscriptionInfo.getPartnerId(), getPartnerId(subscriptionInfo.getProvider())),
                subscriptionInfo.getProvider(),
                requireNonNullElse(subscriptionInfo.getPaymentProcessor(), PaymentProcessor.TRUST));

        userPurchasesDAO.savePurchaseInfo(result);

        return result;
    }

    private Long getPartnerId(String provider) {
        return requireNonNull(providersMap.get(provider), "Partner id not found for provider " + provider);
    }

    @Configuration
    static class BillingServiceExecutorConfig {
        @Bean("billingServiceExecutor")
        ExecutorService billingServiceExecutor() {
            return Executors.newCachedThreadPool(
                    new ThreadFactoryBuilder()
                            .setNameFormat("billingServ-%d")
                            .build()
            );
        }
    }

}
