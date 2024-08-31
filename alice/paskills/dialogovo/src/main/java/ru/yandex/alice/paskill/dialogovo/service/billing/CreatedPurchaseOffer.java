package ru.yandex.alice.paskill.dialogovo.service.billing;

import java.net.URI;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class CreatedPurchaseOffer {
    @JsonProperty("order_id")
    private final String orderId;
    private final URI url;
}
