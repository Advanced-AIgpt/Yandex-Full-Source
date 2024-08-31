package ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.node.ObjectNode;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.InputType;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.RequestBase;

public class SkillPurchaseCompleteRequest extends RequestBase {
    @JsonProperty("purchase_request_id")
    private final String purchaseRequestId;

    @JsonProperty("order_id")
    private final String orderId;

    @JsonProperty("purchase_payload")
    private final ObjectNode purchasePayload;

    public SkillPurchaseCompleteRequest(
            String purchaseRequestId,
            String orderId,
            ObjectNode purchasePayload
    ) {
        super(InputType.PURCHASE_COMPLETE);
        this.purchaseRequestId = purchaseRequestId;
        this.orderId = orderId;
        this.purchasePayload = purchasePayload;
    }
}
