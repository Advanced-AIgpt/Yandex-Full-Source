package ru.yandex.quasar.billing.services.processing.trust;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.UUID;
import java.util.function.Predicate;
import java.util.function.Supplier;

import javax.annotation.Nullable;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.dao.DuplicateKeyException;
import org.springframework.data.relational.core.conversion.DbActionExecutionException;
import org.springframework.transaction.PlatformTransactionManager;
import org.springframework.transaction.support.TransactionTemplate;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.TrustBillingConfig;
import ru.yandex.quasar.billing.dao.GenericProduct;
import ru.yandex.quasar.billing.dao.GenericProductDao;
import ru.yandex.quasar.billing.exception.NotFoundException;
import ru.yandex.quasar.billing.services.PaymentInfo;
import ru.yandex.quasar.billing.services.RefundInfo;
import ru.yandex.quasar.billing.services.processing.ProcessingService;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;

public class TrustBillingService implements ProcessingService {

    public static final String SECURITY_TOKEN_PARAM = "securityToken";
    static final int COUNTRY_CODE_RUSSIA = 225;
    // @sage (from TRUST) said to use this for internal calls.
    static final String INTERNAL_IP = "127.0.0.1";
    private static final Logger log = LogManager.getLogger();
    private final TrustBillingConfig config;

    private final TrustBillingClient trustClient;
    private final GenericProductDao genericProductDao;
    private final TransactionTemplate transactionTemplate;
    private final PaymentProcessor paymentProcessor;

    public TrustBillingService(
            PaymentProcessor paymentProcessor,
            BillingConfig billingConfig,
            TrustBillingClient trustClient,
            GenericProductDao genericProductDao,
            PlatformTransactionManager transactionManager) {
        this.paymentProcessor = paymentProcessor;
        this.config = billingConfig.getTrustBillingConfig();
        this.trustClient = trustClient;
        this.genericProductDao = genericProductDao;
        this.transactionTemplate = new TransactionTemplate(transactionManager);
    }

    public PaymentProcessor getPaymentProcessor() {
        return paymentProcessor;
    }

    @SuppressWarnings("ParameterNumber")
    @Override
    public String createPayment(
            String uid, Long partnerId, String purchaseId, String securityToken,
            PricingOption selectedOption, String paymentCardId, String userIp, String userEmail,
            @Nullable Long submerchantId
    ) {
        // формируем ссылку callback-а
        // security token is used to verify trust callback
        // used only for non-subscription purchases as we control creation of the purchase
        String callbackUrl = UriComponentsBuilder.fromUriString(config.getCallbackBaseUrl() + "/onBillingHoldApplied?")
                .queryParam("purchaseId", purchaseId)
                .queryParam(SECURITY_TOKEN_PARAM, securityToken)
                .toUriString();

        // создаём корзину
        String commission;
        if (selectedOption.getProvider() != null) {
            // predefined products exist only for native providers
            // Commission check is done only for native providers

            // in Balance kinopoisk has COMMISSIONS: {"Default": "1"}
            commission = "1";
        } else {
            commission = null;
        }
        Long productCodeFact = getOrCreateGenericProductCode(partnerId);


        CreateBasketRequest createBasketRequest;
        if (selectedOption.getItems().size() > 1) {
            List<String> productIds = Collections.nCopies(selectedOption.getItems().size(), productCodeFact.toString());
            Iterator<String> orderIdIterator = trustClient.createOrdersBatch(uid, userIp, productIds).iterator();

            List<CreateBasketRequest.Order> orders = new ArrayList<>();
            for (PricingOption.PricingOptionLine item : selectedOption.getItems()) {
                orders.add(new CreateBasketRequest.Order(orderIdIterator.next(),
                        item.getQuantity(),
                        item.getUserPrice(),
                        item.getNdsType(),
                        item.getTitle(),
                        null));
            }

            createBasketRequest = CreateBasketRequest.createMultiItemBasket(
                    TrustCurrency.findByCode(selectedOption.getCurrency()),
                    paymentCardId,
                    orders,
                    userEmail,
                    callbackUrl,
                    commission
            );
        } else {
            createBasketRequest = CreateBasketRequest.createSimpleBasket(
                    selectedOption.getUserPrice(),
                    TrustCurrency.findByCode(selectedOption.getCurrency()),
                    productCodeFact.toString(),
                    paymentCardId,
                    userEmail,
                    callbackUrl,
                    selectedOption.getItems().get(0).getNdsType(),
                    Objects.requireNonNullElseGet(selectedOption.getTitle(),
                            () -> selectedOption.getItems().get(0).getTitle()),
                    commission,
                    null
            );
        }

        CreateBasketResponse createPaymentResponse = trustClient.createBasket(uid, userIp, createBasketRequest);
        //TODO: обработка кода ошибки от trust-а
        return createPaymentResponse.getPurchaseToken();
    }

    @Override
    public Optional<String> startPayment(String uid, String userIp, String purchaseToken) {
        trustClient.startPayment(uid, userIp, purchaseToken);
        return Optional.empty();
    }

    @SuppressWarnings("ParameterNumber")
    @Override
    public String createSubscription(String uid, Long partnerId, String subscriptionId, String securityToken,
                                     PricingOption selectedOption, int trialPeriodDays, String paymentCardId,
                                     String userIp, String userEmail, String provider) {
        throw new RuntimeException("Trust subscription creation is deprecated. This must never happen.");
    }

    @Override
    public void startSubscription(String uid, String userIp, String purchaseToken) {
        //TODO: обработка кода ошибки от trust-а
        trustClient.startPayment(uid, userIp, purchaseToken);
    }

    public TrustPaymentShortInfo getTrustPaymentShortInfo(String purchaseToken, String uid, String userIp) {
        return trustClient.getPaymentShortInfo(uid, userIp, purchaseToken);
    }

    @Override
    public PaymentInfo getPaymentShortInfo(String purchaseToken, String uid, String userIp) {
        TrustPaymentShortInfo paymentShortInfo = getTrustPaymentShortInfo(purchaseToken, uid, userIp);
        PaymentInfo.Status status;
        if ("authorized".equals(paymentShortInfo.getPaymentStatus())) {
            status = PaymentInfo.Status.AUTHORIZED;
        } else if ("cleared".equals(paymentShortInfo.getPaymentStatus())) {
            status = PaymentInfo.Status.CLEARED;
        } else {

            if ("not_authorized".equals(paymentShortInfo.getPaymentStatus())) {
                status = PaymentRespStatus.parse(paymentShortInfo.getPaymentRespCode())
                        .getPaymentStatus();

            } else {
                status = PaymentInfo.Status.ERROR_UNKNOWN;
            }
            log.warn(String.format("Purchase '%s' failed due to code='%s', desc='%s'", purchaseToken,
                    paymentShortInfo.getPaymentRespCode(), paymentShortInfo.getPaymentRespDesc()));
        }

        return new PaymentInfo(
                purchaseToken,
                paymentShortInfo.getPaymethodId(),
                paymentShortInfo.getMaskedCardNumber(),
                paymentShortInfo.getAmount(),
                paymentShortInfo.getCurrency().getCurrencyCode(),
                paymentShortInfo.getPaymentTs(),
                paymentShortInfo.getCardType(),
                status,
                paymentShortInfo.getClearRealTs()
        );
    }

    @Override
    public void clearPayment(String purchaseToken, String uid, String userIp) {
        trustClient.clearPayment(uid, userIp, purchaseToken);
    }

    @Override
    public void unholdPayment(String purchaseToken, String uid, String userIp) {
        trustClient.unholdPayment(uid, userIp, purchaseToken);
    }

    @Override
    public List<PaymentMethod> getCardsList(String uid, String userIp) {
        return trustClient.getCardsList(uid, userIp);
    }

    private Long getOrCreateGenericProductCode(Long partnerId) {
        return genericProductDao.findByPartnerId(partnerId)
                .map(GenericProduct::getId)
                // transaction is committed after the product is registered in trust
                .orElseGet(() -> registerProduct(partnerId));

    }

    /**
     * Register product in DB and trust in a transaction to face concurrency
     * Second thread will wait until first one commits. The first one will commit only after successful call to trust.
     */
    private Long registerProduct(Long partnerId) {
        // execute in transaction
        return transactionTemplate.execute(status -> {
            GenericProduct product;
            try {
                product = genericProductDao.save(GenericProduct.create(partnerId, UUID.randomUUID().toString()));
            } catch (DbActionExecutionException e) {
                if (e.getCause() instanceof DuplicateKeyException) {
                    // in case it was already created by another thread
                    return genericProductDao.findByPartnerId(partnerId).get().getId();
                } else {
                    throw e;
                }
            }
            var request = CreateProductRequest.createGenericProduct(product.getId().toString(),
                    product.getPartnerId(), "generic product for " + product.getPartnerId());
            trustClient.createProduct(request);
            return product.getId();
        });
    }

    @Override
    public SubscriptionShortInfo getSubscriptionShortInfo(String subscriptionId, String uid, String userIp) {
        return trustClient.getSubscriptionShortInfo(uid, userIp, subscriptionId);
    }

    @Override
    public NewBindingInfo startBinding(String uid, String userIp, TrustCurrency currency, String returnUrl,
                                       TemplateTag template) {
        String purchaseToken = trustClient.createBinding(uid, userIp, currency, returnUrl, template);
        String bindingUrl = trustClient.startBinding(uid, userIp, purchaseToken);
        return new NewBindingInfo(bindingUrl, purchaseToken);
    }

    @Override
    public BindingInfo getBindingInfo(String uid, String userIp, String purchaseToken) {
        BindingStatusResponse bindingStatus = trustClient.getBindingStatus(uid, userIp, purchaseToken);

        var status = BindingInfo.Status.forName(bindingStatus.getPaymentRespCode());

        return new BindingInfo(status, bindingStatus.getPaymentMethodId());
    }

    /**
     * Make refund of a given payment and wait till it success or failure
     *
     * @param uid            user identifier
     * @param purchaseToken  purchase token
     * @param subscriptionId subscription ID for subscription purchases
     * @param description    description for the refund
     * @return refund details
     * @throws RefundException if refund failed
     */
    public RefundInfo refundPayment(String uid, String purchaseToken, @Nullable String subscriptionId,
                                    String description) {

        TrustPaymentShortInfo paymentShortInfo = trustClient.getPaymentShortInfo(uid, INTERNAL_IP, purchaseToken);

        CreateRefundResponse refund = startRefundInternal(purchaseToken, uid, paymentShortInfo, subscriptionId,
                description);

        TrustRefundInfo trustRefundInfo =
                ConditionalDelayedTask.execute(() -> trustClient.getRefundStatus(refund.getTrustRefundId()))
                        .withDelay(100)
                        .until(refundInfo -> refundInfo.getStatus() != RefundStatus.wait_for_notification);

        if (trustRefundInfo.getStatus() == RefundStatus.success) {
            return new RefundInfo(purchaseToken, refund.getTrustRefundId(), subscriptionId,
                    trustRefundInfo.getFiscalReceiptUrl(), paymentShortInfo.getAmount(), uid);
        } else {
            throw new RefundException("Failed to refund " + refund.getTrustRefundId() + ". status: " +
                    refund.getStatus());
        }
    }

    /**
     * Make refund of a given payment and wait till it success or failure
     *
     * @param uid            user identifier
     * @param purchaseToken  purchase token
     * @param subscriptionId subscription ID for subscription purchases
     * @param description    description for the refund
     * @return refund details
     * @throws RefundException if refund failed
     */
    public String startRefund(String uid, String purchaseToken, @Nullable String subscriptionId, String description) {
        TrustPaymentShortInfo paymentShortInfo = trustClient.getPaymentShortInfo(uid, INTERNAL_IP, purchaseToken);
        CreateRefundResponse refund = startRefundInternal(purchaseToken, uid, paymentShortInfo, subscriptionId,
                description);
        return refund.getTrustRefundId();
    }

    /**
     * get refund status
     *
     * @param refundId refund ID
     * @return refund detailed status
     * @throws NotFoundException if refund not found
     */
    public TrustRefundInfo getRefundStatus(String refundId) {
        try {
            return trustClient.getRefundStatus(refundId);
        } catch (HttpClientErrorException.NotFound e) {
            throw new NotFoundException("Refund not found", e);
        }
    }

    private CreateRefundResponse startRefundInternal(String purchaseToken, String uid,
                                                     TrustPaymentShortInfo paymentShortInfo,
                                                     @Nullable String subscriptionId, String description) {
        log.warn("refunding purchase token: " + purchaseToken);

        if (!Set.of("cleared", "authorized", "success").contains(paymentShortInfo.getPaymentStatus())) {
            throw new RefundException("Payment in non-refundable status: " + paymentShortInfo.getPaymentStatus());
        }

        SubscriptionPaymentRefundParams request;
        if (subscriptionId == null) {
            request = SubscriptionPaymentRefundParams.singlePayment(purchaseToken, description,
                    paymentShortInfo.getOrders());
        } else {
            request = SubscriptionPaymentRefundParams.subscriptionPayment(purchaseToken, description, subscriptionId,
                    paymentShortInfo.getAmount());
        }

        CreateRefundResponse refund = trustClient.createRefund(uid, INTERNAL_IP, request);

        if (refund.getStatus() != CreateRefundStatus.success) {
            throw new RefundException("Failed to create refund: " + refund.getStatus());
        }

        log.info("Created refund: " + refund.getTrustRefundId());

        trustClient.startRefund(uid, INTERNAL_IP, refund.getTrustRefundId());
        log.info("Refund started: " + refund.getTrustRefundId());

        return refund;
    }

    /**
     * Utility executor that executes a {@link Supplier} task with delay in loop until {@link Predicate} predicate
     * of the supplier's results is met
     */
    static class ConditionalDelayedTask<T> {

        private final Supplier<T> task;
        private Long delay = 0L;

        private ConditionalDelayedTask(Supplier<T> task) {
            this.task = task;
        }

        static <T> ConditionalDelayedTask<T> execute(Supplier<T> task) {
            Objects.requireNonNull(task, "Task not defined");
            return new ConditionalDelayedTask<>(task);
        }

        ConditionalDelayedTask<T> withDelay(long delayMiliseconds) {
            this.delay = delayMiliseconds;
            return this;

        }

        T until(Predicate<T> predicate) {
            Objects.requireNonNull(predicate, "Predicate not specified");
            Objects.requireNonNull(delay, "Delay not specified");

            return execute(task, delay, predicate);
        }


        private T execute(Supplier<T> task, long delay, Predicate<T> predicate) {
            T result = task.get();
            while (!predicate.test(result)) {
                try {
                    Thread.sleep(delay);
                } catch (InterruptedException e) {
                    Thread.interrupted();
                    break;
                }
                result = task.get();
            }
            return result;
        }

    }

}
