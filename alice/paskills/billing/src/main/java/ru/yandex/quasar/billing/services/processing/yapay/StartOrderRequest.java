package ru.yandex.quasar.billing.services.processing.yapay;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class StartOrderRequest {
    @Nullable
    @JsonProperty("payment_completion_action")
    private final PurchaseCompletionAction purchaseCompletionAction;

    @Nullable
    @JsonProperty("yandexuid")
    private final String yandexuid;
}
