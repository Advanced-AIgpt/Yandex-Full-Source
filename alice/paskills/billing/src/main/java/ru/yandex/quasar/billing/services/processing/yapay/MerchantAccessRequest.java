package ru.yandex.quasar.billing.services.processing.yapay;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class MerchantAccessRequest {
    @Nonnull
    private final String token;
    @Nonnull
    @JsonProperty("entity_id")
    private final String entityId;
    @Nonnull
    private final String description;
}
