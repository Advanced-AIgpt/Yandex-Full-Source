package ru.yandex.quasar.billing.providers;

import ru.yandex.quasar.billing.dao.PurchaseInfo;

public class ProviderPurchaseException extends Exception {

    private final PurchaseInfo.Status purchaseStatus;

    public ProviderPurchaseException(PurchaseInfo.Status purchaseStatus) {
        this.purchaseStatus = purchaseStatus;
    }

    public ProviderPurchaseException(PurchaseInfo.Status purchaseStatus, String message) {
        super(message);
        this.purchaseStatus = purchaseStatus;
    }

    public ProviderPurchaseException(PurchaseInfo.Status purchaseStatus, Throwable cause) {
        super(cause);
        this.purchaseStatus = purchaseStatus;
    }

    public PurchaseInfo.Status getPurchaseStatus() {
        return purchaseStatus;
    }
}
