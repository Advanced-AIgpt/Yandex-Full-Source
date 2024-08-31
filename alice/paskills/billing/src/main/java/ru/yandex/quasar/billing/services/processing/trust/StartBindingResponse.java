package ru.yandex.quasar.billing.services.processing.trust;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class StartBindingResponse {
    private final String status;
    @JsonProperty("binding_url")
    private final String bindingUrl;
}
