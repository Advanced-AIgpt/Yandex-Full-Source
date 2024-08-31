package ru.yandex.quasar.billing.providers.universal;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonRawValue;
import lombok.Data;

@Data
class PurchaseRequest {
    @JsonProperty("transaction_id")
    private final String transactionId;
    @JsonProperty("purchase_payload")
    @JsonRawValue
    private final String purchasePayload;
    @JsonProperty("signed_data")
    private final String signedData;
    @JsonProperty("signature")
    private final String signature;
}
