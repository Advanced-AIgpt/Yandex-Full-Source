package ru.yandex.quasar.billing.services.processing.yapay;

class AccessRequestConflictException extends Exception {
    private final long serviceMerchantId;

    AccessRequestConflictException(long serviceMerchantId) {
        this.serviceMerchantId = serviceMerchantId;
    }

    public long getServiceMerchantId() {
        return serviceMerchantId;
    }
}
