package ru.yandex.quasar.billing.services.processing.trust;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;

@Data
@AllArgsConstructor(access = AccessLevel.PACKAGE)
public class CreateBasketResponse {

    private final String status;
    @JsonProperty("purchase_token")
    private final String purchaseToken;

    @JsonIgnore
    public static CreateBasketResponse success(String purchaseToken) {
        return new CreateBasketResponse("success", purchaseToken);
    }

}
