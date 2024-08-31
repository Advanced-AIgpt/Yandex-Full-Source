package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.payment;

import com.fasterxml.jackson.databind.node.ObjectNode;
import lombok.Data;

@Data
public class PurchaseCompleteEvent {
    private final String purchaseRequestId;
    private final String purchaseOfferUuid;
    private final ObjectNode purchasePayload;
}
