package ru.yandex.quasar.billing.services.processing.trust;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class TrustRefundInfo {
    private final RefundStatus status;
    @JsonProperty("status_desc")
    @Nullable
    private final String statusDescription;
    @JsonProperty("fiscal_receipt_url")
    @Nullable
    private final String fiscalReceiptUrl;
}
