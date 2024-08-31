package ru.yandex.quasar.billing.services.processing.trust;

import java.util.List;

public interface TrustBillingClient extends PaymentMethodsClient {
    CreateBasketResponse createBasket(String uid, String userIp, CreateBasketRequest createBasketRequest);

    // запускаем корзину на оплату
    void startPayment(String uid, String userIp, String purchaseToken);

    TrustPaymentShortInfo getPaymentShortInfo(String uid, String userIp, String purchaseToken);

    void clearPayment(
            String uid,
            String userIp,
            String purchaseToken
    );

    void unholdPayment(
            String uid,
            String userIp,
            String purchaseToken
    );

    void createProduct(CreateProductRequest createProductRequest);

    SubscriptionShortInfo getSubscriptionShortInfo(String uid, String userIp, String subscriptionId);

    /**
     * Create new refund
     *
     * @param uid                 user identified
     * @param ip                  IP address for Trust
     * @param createRefundRequest refund request data
     * @return refund creation response
     */
    CreateRefundResponse createRefund(String uid, String ip, SubscriptionPaymentRefundParams createRefundRequest);

    /**
     * start refund
     *
     * @param uid      user identifier
     * @param ip       AP address for Trust
     * @param refundId refund identifier to start
     * @return refund info
     */
    TrustRefundInfo startRefund(String uid, String ip, String refundId);

    /**
     * get refund info
     *
     * @param refundId refund identifier to start
     * @return refund info
     */
    TrustRefundInfo getRefundStatus(String refundId);

    /**
     * create new orders for given product ID and user
     *
     * @param uid        user identifier
     * @param userIp     user IP address
     * @param productIds list of product IDs to create orders for
     * @return list of created order IDs
     */
    List<String> createOrdersBatch(String uid, String userIp, List<String> productIds);

    /**
     * create new order for given product ID and user
     *
     * @param uid       user identifier
     * @param userIp    user IP address
     * @param productId product ID to create orders for
     * @return created order ID
     */
    String createOrder(String uid, String userIp, String productId);
}
