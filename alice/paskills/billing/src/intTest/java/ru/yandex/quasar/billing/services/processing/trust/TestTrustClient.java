package ru.yandex.quasar.billing.services.processing.trust;

import java.text.MessageFormat;
import java.time.Instant;
import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import org.apache.commons.lang3.StringUtils;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.context.annotation.Lazy;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpStatus;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.controller.SimpleResult;
import ru.yandex.quasar.billing.services.TestAuthorizationService;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;

import static java.util.Objects.requireNonNull;
import static java.util.stream.Collectors.toList;

public class TestTrustClient implements TrustBillingClient {


    public static final String PAYMETHOD_ID = "card-xxxx";
    public static final String CARD_MASKED = "510000****0000";
    public static final String PAYMENT_SYSTEM = "MasterCard";
    // map of purchases by token
    private final Map<String, Purchase> purchases = new ConcurrentHashMap<>();
    private final Map<String, CreateProductRequest> products = new ConcurrentHashMap<>();
    //subscriptions by orderId
    private final Map<String, Order> orders = new ConcurrentHashMap<>();
    private final RestTemplate restTemplate;
    private final BillingConfig billingConfig;
    private AtomicInteger purchaseIdGenerator = new AtomicInteger(1);
    private AtomicInteger orderIdGenerator = new AtomicInteger(10000);
    @Autowired
    @Lazy
    private PostClearingJob postClearingJob;

    TestTrustClient(RestTemplateBuilder restTemplateBuilder, BillingConfig billingConfig) {
        this.restTemplate = restTemplateBuilder.build();
        this.billingConfig = billingConfig;
    }

    public Purchase getPurchase(String purchaseToken) {
        return purchases.get(purchaseToken);
    }

    public Purchase getFirstPurchase() {
        if (purchases.size() == 1) {
            return purchases.values().iterator().next();
        } else {
            throw new RuntimeException("purchase collection is not of size==1 but " + purchases.size());
        }
    }

    public CreateProductRequest getProduct(String productCode) {
        return products.get(productCode);
    }

    public Subscription getSubscription(String subscriptionId) {
        return (Subscription) orders.get(subscriptionId);
    }

    public Order getOrder(String orderId) {
        return orders.get(orderId);
    }

    public List<Subscription> getUserSubscriptions(String uid) {
        return orders.values().stream()
                .filter(it -> it instanceof Subscription)
                .filter(order -> uid.equals(order.getUid()))
                .map(Subscription.class::cast)
                .collect(toList());
    }

    public List<Purchase> getUserPurchases(String uid) {
        return purchases.values().stream()
                .filter(purchase -> uid.equals(purchase.getUid()))
                .collect(toList());
    }

    public Subscription expireSubscription(String subscriptionId) {
        Subscription subscription = (Subscription) orders.get(subscriptionId);
        subscription.setSubsUntilTs(LocalDateTime.now());
        return subscription;
    }

    /**
     * Create new purchase based on existing purchase. Used for subscription renewal
     *
     * @param purchaseToken existing purchase token
     * @return new purchase
     */
    public Purchase createRenewalPurchase(String purchaseToken) {
        Purchase existingPurchase = purchases.get(purchaseToken);

        Purchase newPurchase = existingPurchase.toBuilder()
                .purchaseToken(UUID.randomUUID().toString().replace("-", ""))
                .paymethodId(CARD_MASKED)
                .status(Purchase.PaymentState.started)
                .paymentStatus(Purchase.PaymentStatus.authorized)
                .paymentRespStatus(null)
                .paymentTs(null)
                .build();

        purchases.put(newPurchase.getPurchaseToken(), newPurchase);
        return newPurchase;
    }


    @Override
    public CreateBasketResponse createBasket(String uid, String userIp, CreateBasketRequest createBasketRequest) {
        String purchaseToken = UUID.randomUUID().toString().replace("-", "");
        List<Order> orderList;
        if (createBasketRequest.getOrders() != null) {
            orderList = new ArrayList<>();
            for (CreateBasketRequest.Order it : createBasketRequest.getOrders()) {
                var order = orders.get(it.getOrderId());
                if (order == null) {
                    throw new RuntimeException("Order " + it.getOrderId() + " not found");
                }

                if (order instanceof Subscription) {
                    ((Subscription) order).initializeSubscription();
                } else {
                    order.initializeOrder(it.getPrice(), it.getQty(), it.getFiscalNds(), it.getFiscalTitle(),
                            it.getFiscalInn());
                }

                orderList.add(order);
            }
        } else {
            CreateProductRequest product = requireNonNull(products.get(createBasketRequest.getProductId()), "Product " +
                    "not found");
            Order order = Order.createSingletonOrder(String.valueOf(orderIdGenerator.incrementAndGet()),
                    uid, product,
                    product.getPrices() != null && !product.getPrices().isEmpty() ?
                            String.valueOf(product.getPrices().get(0).getRegionId()) : "225",
                    requireNonNull(createBasketRequest.getAmount()),
                    createBasketRequest.getFiscalNds(),
                    createBasketRequest.getFiscalTitle());
            orders.put(order.getOrderId(), order);
            orderList = Collections.singletonList(order);
        }

        boolean isSubscription = orderList.stream().allMatch(it -> it instanceof Subscription);

        TrustCurrency currency;
        boolean isTrial;

        if (isSubscription) {
            CreateProductRequest product;
            product = orderList.get(0).getProduct();
            isTrial = StringUtils.isNotEmpty(product.getSubsTrialPeriod());
            currency = product.getPrices().get(0).getCurrency();
        } else {
            currency = createBasketRequest.getCurrency();
            isTrial = false;
        }

        Purchase purchase = Purchase.builder()
                .uid(uid)
                .purchaseToken(purchaseToken)
                .productId(createBasketRequest.getProductId())
                .currency(currency)
                .paymethodId(isTrial ? "trial_payment" : createBasketRequest.getPaymethodId())
                .userEmail(createBasketRequest.getUserEmail())
                .callBackUrl(createBasketRequest.getCallBackUrl())
                .commissionCategory(createBasketRequest.getCommissionCategory())
                .trustPaymentId(UUID.randomUUID().toString())
                .status(Purchase.PaymentState.created)
                .paymentTs(isTrial ? Instant.now() : null)
                .orders(orderList)
                .build();

        purchases.put(purchaseToken, purchase);
        orderList.forEach(order -> order.getPurchases().add(purchase));

        return new CreateBasketResponse("success", purchaseToken);

    }

    @Override
    public void startPayment(String uid, String userIp, String purchaseToken) {
        Purchase purchase = purchases.get(purchaseToken);
        purchase.setStatus(Purchase.PaymentState.started);
        purchase.setPaymentStatus(Purchase.PaymentStatus.authorized);
    }

    @Nullable
    public String executeCallback(String purchaseToken, int port) {
        Purchase purchase = purchases.get(purchaseToken);
        if (purchase != null) {

            MultiValueMap<String, String> callbackParams = new LinkedMultiValueMap<>();
            callbackParams.add("purchase_token", purchaseToken);
            callbackParams.add("status", purchase.getPaymentStatus() == Purchase.PaymentStatus.authorized ?
                    "success" : "cancelled");
            callbackParams.add("status_code", purchase.getPaymentStatus() == Purchase.PaymentStatus.authorized ?
                    "success" : purchase.getPaymentRespStatus().name());
            callbackParams.add("user_email", purchase.getUserEmail());
            callbackParams.add("payment_method", purchase.getPaymethodId());
            callbackParams.add("trust_payment_id", purchase.getTrustPaymentId());

            if (purchase.getSubscriptions().isEmpty()) {
                callbackParams.add("service_order_id", String.valueOf(purchaseIdGenerator.getAndIncrement()));
            } else {
                // subscriptionId is added to the original callback url by billing
                purchase.getSubscriptions().forEach(order -> callbackParams.add("service_order_id",
                        order.getSubscriptionId()));
            }

            return restTemplate.postForObject(
                    UriComponentsBuilder.fromUriString(purchase.getCallBackUrl())
                            .scheme("http")
                            .host("localhost")
                            .port(port)
                            .queryParams(callbackParams)
                            .build().toUri(),
                    HttpEntity.EMPTY,
                    SimpleResult.class)
                    .getResult();
        } else {
            return null;
        }
    }

    @Nullable
    public String executeTrialCallback(String purchaseToken, int port) {
        Purchase purchase = purchases.values().iterator().next();

        MultiValueMap<String, String> callbackParams = new LinkedMultiValueMap<>();
        callbackParams.add("purchase_token", purchaseToken);
        callbackParams.add("status", "success");
        callbackParams.add("status_code", "success");
        // callbackParams.add("user_email", purchase.getUserEmail());
        callbackParams.add("payment_method", "trial_payment");

        // subscriptionId is added to the original callback url by billing
        purchase.getSubscriptions().forEach(order -> callbackParams.add("service_order_id", order.getSubscriptionId()));

        return restTemplate.postForObject(
                UriComponentsBuilder.fromUriString(purchase.getCallBackUrl())
                        .scheme("http")
                        .host("localhost")
                        .port(port)
                        .queryParams(callbackParams)
                        .build().toUri(),
                HttpEntity.EMPTY,
                SimpleResult.class)
                .getResult();

    }

    @Nullable
    public String executeRefundCallback(String purchaseToken, int port) {
        Purchase purchase = purchases.get(purchaseToken);
        if (purchase != null) {
            MultiValueMap<String, String> callbackParams = new LinkedMultiValueMap<>();
            callbackParams.add("purchase_token", purchaseToken);
            callbackParams.add("status", "success");
            callbackParams.add("mode", "refund_result");

            return restTemplate.postForObject(
                    UriComponentsBuilder.fromUriString(purchase.getCallBackUrl())
                            .scheme("http")
                            .host("localhost")
                            .port(port)
                            .queryParams(callbackParams)
                            .build().toUri(),
                    HttpEntity.EMPTY,
                    SimpleResult.class)
                    .getResult();
        } else {
            return null;
        }
    }

    @Override
    public TrustPaymentShortInfo getPaymentShortInfo(String uid, String userIp, String purchaseToken) {
        Purchase purchase = purchases.get(purchaseToken);
        return new TrustPaymentShortInfo("dummy test description",
                purchase.getPaymentRespStatus() != null ? purchase.getPaymentRespStatus().name() : null,
                purchase.getPaymentStatus().name(),
                CARD_MASKED,
                PAYMENT_SYSTEM,
                purchase.getAmount(),
                purchase.getCurrency(),
                purchase.getPaymentTs(),
                purchase.getPaymethodId(),
                purchase.getClearDate(),
                purchase.getOrders().stream()
                        .map(order -> new TrustPaymentShortInfo.TrustOrderInfo(order.getOrderId(), order.getPrice()))
                        .collect(toList()));
    }

    @Override
    public void clearPayment(String uid, String userIp, String purchaseToken) {
        Purchase purchase = purchases.get(purchaseToken);
        purchase.setPaymentTs(Instant.now());
        purchase.setStatus(Purchase.PaymentState.success);
        purchase.setPaymentStatus(Purchase.PaymentStatus.cleared);
        purchase.setClearDate(Instant.now());
        if (!purchase.getSubscriptions().isEmpty()) {
            purchase.getSubscriptions().forEach(
                    subscription -> subscription.setSubsUntilTs(LocalDateTime.now().plusHours(1L))
            );
        }
    }

    @Override
    public void unholdPayment(String uid, String userIp, String purchaseToken) {
        Purchase purchase = purchases.get(purchaseToken);
        purchase.setStatus(Purchase.PaymentState.success);
        purchase.setPaymentStatus(Purchase.PaymentStatus.canceled);
    }

    @Override
    public List<PaymentMethod> getCardsList(String uid, String userIp) {
        if (TestAuthorizationService.UID.equals(uid)) {
            return List.of(PaymentMethod.builder(PAYMETHOD_ID, "card")
                    .paymentSystem(PAYMENT_SYSTEM)
                    .expired(false)
                    .build());
        } else {
            return List.of();
        }
    }

    @Override
    public String createBinding(String uid, String userIp, TrustCurrency currency, String backUrl,
                                TemplateTag template) {
        return null;
    }

    @Override
    public String startBinding(String uid, String userIp, String purchaseToken) {
        return null;
    }

    @Override
    public BindingStatusResponse getBindingStatus(String uid, String userIp, String purchaseToken) {
        return null;
    }

    @Override
    public void createProduct(CreateProductRequest createProductRequest) {
        products.put(createProductRequest.getProductId(), createProductRequest);
    }

    @Override
    public SubscriptionShortInfo getSubscriptionShortInfo(String uid, String userIp, String subscriptionId) {
        return ((Subscription) orders.get(subscriptionId)).toSubscriptionShortInfo();
    }

    @Override
    public List<String> createOrdersBatch(String uid, String userIp, List<String> productIds) {
        return productIds.stream()
                .map(it -> Order.createOrderWithoutPrice(String.valueOf(orderIdGenerator.incrementAndGet()), uid,
                        products.get(it)))
                .peek(order -> orders.put(order.getOrderId(), order))
                .map(Order::getOrderId)
                .collect(Collectors.toList());
    }

    @Override
    public String createOrder(String uid, String userIp, String productId) {
        CreateProductRequest productRequest = products.get(productId);
        var order = Order.createOrderWithoutPrice(String.valueOf(orderIdGenerator.incrementAndGet()), uid,
                productRequest);

        orders.put(order.getOrderId(), order);

        return order.getOrderId();
    }

    @Override
    public CreateRefundResponse createRefund(String uid, String ip,
                                             SubscriptionPaymentRefundParams createRefundRequest) {
        if (!purchases.containsKey(createRefundRequest.getPurchaseToken())) {
            throw new HttpClientErrorException(HttpStatus.BAD_REQUEST);
        }

        Purchase purchase = requireNonNull(purchases.get(createRefundRequest.getPurchaseToken()));

        if (purchase.getAmount().compareTo(createRefundRequest.getOrders().get(0).getDeltaAmount()) != 0) {
            throw new RuntimeException(MessageFormat.format("Wrong payment amount: got {0}, expected: {1}",
                    createRefundRequest.getOrders().get(0).getDeltaAmount(),
                    purchase.getAmount()));
        }

        Set<String> purchaseOrders =
                Objects.requireNonNullElse(purchase.getOrders(), Collections.<Order>emptyList()).stream()
                        .map(Order::getOrderId)
                        .collect(Collectors.toSet());

        Set<String> requestOrders = Objects.requireNonNullElse(createRefundRequest.getOrders(),
                Collections.<SubscriptionPaymentRefundParams.Order>emptyList()).stream()
                .map(SubscriptionPaymentRefundParams.Order::getOrderId)
                .collect(Collectors.toSet());

        if (!purchaseOrders.equals(requestOrders)) {
            throw new RuntimeException("Wrong order information passed");
        }

        if (purchase.getRefund() != null) {
            throw new RuntimeException("Purchase is already refunded");
        }

        Refund refund = new Refund(UUID.randomUUID().toString());
        purchase.setRefund(refund);

        return new CreateRefundResponse(CreateRefundStatus.success, refund.getRefundId());
    }

    @Override
    public TrustRefundInfo startRefund(String uid, String ip, String refundId) {
        Refund refundInfo = getRefundById(refundId);

        TrustRefundInfo trustRefundInfo = new TrustRefundInfo(refundInfo.getRefundStatus(),
                refundInfo.getRefundStatus().toString(), refundInfo.getFiscalUrl());

        markRefundSuccess(refundId);

        return trustRefundInfo;
    }

    @Override
    public TrustRefundInfo getRefundStatus(String refundId) {
        Refund refundInfo = getRefundById(refundId);

        return new TrustRefundInfo(refundInfo.getRefundStatus(), refundInfo.getRefundStatus().toString(),
                refundInfo.getFiscalUrl());
    }

    public void markRefundFailed(String refundId) {
        Refund refund = getRefundById(refundId);

        refund.setRefundStatus(RefundStatus.failed);
        refund.setFiscalUrl(null);

    }

    public String markRefundSuccess(String refundId) {
        Purchase purchase = getPurchaseByRefundId(refundId);
        Refund refund = purchase.getRefund();

        refund.setRefundStatus(RefundStatus.success);
        refund.setFiscalUrl(fiscalUrlForRefundId(refundId));

        purchase.setPaymentStatus(Purchase.PaymentStatus.canceled);

        return refund.getFiscalUrl();
    }

    public String fiscalUrlForRefundId(String refundId) {
        return "http://url/" + refundId;
    }

    private Purchase getPurchaseByRefundId(String refundId) {
        return purchases.values().stream()
                .filter(purchase -> purchase.getRefund() != null && refundId.equals(purchase.getRefund().getRefundId()))
                .findFirst()
                .orElseThrow(() -> new HttpClientErrorException(HttpStatus.NOT_FOUND));
    }

    private Refund getRefundById(String refundId) {
        return getPurchaseByRefundId(refundId).getRefund();
    }

    public void clear() {
        purchases.clear();
        products.clear();
        orders.clear();
    }

    public void runClearingJob() {
        this.postClearingJob.postClearing();
    }

}
