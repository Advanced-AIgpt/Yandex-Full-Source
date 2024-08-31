package ru.yandex.quasar.billing.controller;

import java.net.URI;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.With;

@Data
@AllArgsConstructor
class StartCardBindingResponse {
    @JsonProperty("binding_url")
    private final String bindingUrl;
    @JsonProperty("purchase_token")
    private final String purchaseToken;

    @With
    @JsonProperty("cookie_uri")
    @JsonInclude(JsonInclude.Include.NON_NULL)
    @Nullable
    private final URI cookieUri;

    StartCardBindingResponse(String bindingUrl, String purchaseToken) {
        this.bindingUrl = bindingUrl;
        this.purchaseToken = purchaseToken;
        this.cookieUri = null;
    }
}
