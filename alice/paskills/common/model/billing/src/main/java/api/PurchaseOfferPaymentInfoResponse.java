package ru.yandex.alice.paskills.common.billing.model.api;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonRawValue;
import com.fasterxml.jackson.databind.node.ObjectNode;

import ru.yandex.alice.paskills.common.billing.model.PaymentStatus;
import ru.yandex.alice.paskills.common.billing.model.PurchaseOfferStatus;

public class PurchaseOfferPaymentInfoResponse {

    @JsonProperty("purchase_request_id")
    private final String purchaseRequestId;

    @JsonProperty("purchase_offer_uuid")
    private final String purchaseOfferUuid;

    @JsonProperty("purchase_offer_status")
    private final PurchaseOfferStatus purchaseOfferStatus;

    @JsonProperty("payment_status")
    private final PaymentStatus paymentStatus;

    @JsonRawValue
    @JsonProperty("purchase_payload")
    private final ObjectNode purchasePayload;

    public PurchaseOfferPaymentInfoResponse(String purchaseRequestId,
                                            String purchaseOfferUuid,
                                            PurchaseOfferStatus purchaseOfferStatus,
                                            PaymentStatus paymentStatus,
                                            ObjectNode purchasePayload) {
        this.purchaseRequestId = purchaseRequestId;
        this.purchaseOfferUuid = purchaseOfferUuid;
        this.purchaseOfferStatus = purchaseOfferStatus;
        this.paymentStatus = paymentStatus;
        this.purchasePayload = purchasePayload;
    }

    public String getPurchaseRequestId() {
        return purchaseRequestId;
    }

    public String getPurchaseOfferUuid() {
        return purchaseOfferUuid;
    }

    public PurchaseOfferStatus getPurchaseOfferStatus() {
        return purchaseOfferStatus;
    }

    public PaymentStatus getPaymentStatus() {
        return paymentStatus;
    }

    public ObjectNode getPurchasePayload() {
        return purchasePayload;
    }

    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder("PurchaseOfferPaymentInfoResponse{");
        sb.append("purchaseRequestId='").append(purchaseRequestId).append('\'');
        sb.append(", purchaseOfferUuid='").append(purchaseOfferUuid).append('\'');
        sb.append(", purchaseOfferStatus=").append(purchaseOfferStatus);
        sb.append(", paymentStatus=").append(paymentStatus);
        sb.append(", purchasePayload=").append(purchasePayload);
        sb.append('}');
        return sb.toString();
    }
}
