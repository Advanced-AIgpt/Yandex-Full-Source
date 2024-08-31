package ru.yandex.alice.paskills.common.billing.model.api;

import ru.yandex.alice.paskills.common.billing.model.PaymentStatus;
import ru.yandex.alice.paskills.common.billing.model.PurchaseOfferStatus;

public class PurchaseOfferStatusResponse {
    private final PurchaseOfferStatus purchaseOfferStatus;
    private final PaymentStatus paymentStatus;

    public PurchaseOfferStatusResponse(PurchaseOfferStatus purchaseOfferStatus, PaymentStatus paymentStatus) {
        this.purchaseOfferStatus = purchaseOfferStatus;
        this.paymentStatus = paymentStatus;
    }

    public PurchaseOfferStatus getPurchaseOfferStatus() {
        return purchaseOfferStatus;
    }

    public PaymentStatus getPaymentStatus() {
        return paymentStatus;
    }

    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder("PurchaseOfferStatusResponse{");
        sb.append("purchaseOfferStatus=").append(purchaseOfferStatus);
        sb.append(", paymentStatus=").append(paymentStatus);
        sb.append('}');
        return sb.toString();
    }
}
