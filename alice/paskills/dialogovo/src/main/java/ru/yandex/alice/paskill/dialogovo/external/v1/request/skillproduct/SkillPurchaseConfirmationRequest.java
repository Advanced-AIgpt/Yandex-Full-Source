package ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.node.ObjectNode;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.InputType;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.RequestBase;
import ru.yandex.alice.paskill.dialogovo.service.logging.MaskSensitiveData;

public class SkillPurchaseConfirmationRequest extends RequestBase {
    @JsonProperty("purchase_request_id")
    private final String purchaseRequestId;

    @JsonProperty("purchase_token")
    private final String purchaseToken;

    @JsonProperty("order_id")
    private final String orderId;

    @JsonProperty("purchase_timestamp")
    private final long purchaseTimestamp;

    @JsonProperty("purchase_payload")
    private final ObjectNode purchasePayload;

    @JsonProperty("signed_data")
    private final String signedData;

    @JsonProperty("signature")
    @MaskSensitiveData
    private final String signature;

    public SkillPurchaseConfirmationRequest(
            String purchaseRequestId,
            String purchaseToken,
            String orderId,
            long purchaseTimestamp,
            ObjectNode purchasePayload,
            String signedData,
            String signature
    ) {
        super(InputType.PURCHASE_CONFIRMATION);
        this.purchaseRequestId = purchaseRequestId;
        this.purchaseToken = purchaseToken;
        this.orderId = orderId;
        this.purchaseTimestamp = purchaseTimestamp;
        this.purchasePayload = purchasePayload;
        this.signedData = signedData;
        this.signature = signature;
    }
}
