package ru.yandex.quasar.billing.services.processing.yapay;

public interface YandexPayMerchantRegistry {
    /**
     * Ask access for purchases for a given Yandex Pay merchant
     * If the access request exists return its current state
     *
     * @param token       Merchant token
     * @param entityId    Unique access request ID
     * @param description Description associated with the request for the merchant to understand who it grants
     * @return access merchant request info
     */
    ServiceMerchantInfo requestMerchantAccess(String token, String entityId, String description) throws TokenNotFound;

    /**
     * Get merchant info for Yandex Pay merchant
     *
     * @param serviceMerchantId service merchant ID
     * @return access merchant request info
     */
    ServiceMerchantInfo merchantInfo(long serviceMerchantId);
}
