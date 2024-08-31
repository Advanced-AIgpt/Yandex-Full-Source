package ru.yandex.quasar.billing.services.processing.yapay;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class StartOrderResponse {
    @JsonProperty("trust_url")
    private final String trustURl;
}
