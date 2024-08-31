package ru.yandex.quasar.billing.services.processing.trust;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class BindingStatusResponse {
    private final String status;
    @JsonProperty("binding_result")
    private final String bindingResult;
    @JsonProperty("payment_resp_code")
    private final String paymentRespCode;
    @JsonProperty("payment_resp_desc")
    private final String paymentRespDesc;
    @Nullable // null if not started
    @JsonProperty("purchase_token")
    private final String purchaseToken;
    @Nullable
    @JsonFormat(shape = JsonFormat.Shape.STRING)
    private final Integer timeout;
    @Nullable
    @JsonProperty("payment_method_id")
    private final String paymentMethodId;
    @Nullable
    private final String rrn;
    @Nullable
    private final String method;
}
