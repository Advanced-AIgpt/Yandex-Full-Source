package ru.yandex.quasar.billing.services.processing.yapay;

public interface YandexPayClient {
    YaPayMerchant getMerchantByKey(String key);

    Order createOrder(long serviceMerchantId, CreateOrderRequest newOrder);

    Order getOrder(long serviceMerchantId, long orderId);

    StartOrderResponse startOrder(long serviceMerchantId, long orderId, StartOrderRequest startOrder);

    void clearOrder(long serviceMerchantId, long orderId);

    void unholdOrder(long serviceMerchantId, long orderId);

    /**
     * @param token
     * @param entityId
     * @param description
     * @return
     * @throws AccessRequestConflictException, TokenNotFound
     */
    ServiceMerchantInfo requestMerchantAccess(String token, String entityId, String description)
            throws AccessRequestConflictException, TokenNotFound;

    ServiceMerchantInfo merchantInfo(long serviceMerchantId);
}
