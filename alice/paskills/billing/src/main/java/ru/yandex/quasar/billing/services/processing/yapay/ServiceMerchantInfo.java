package ru.yandex.quasar.billing.services.processing.yapay;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

@Data
@Builder
public class ServiceMerchantInfo {
    @JsonProperty("service_merchant_id")
    private final long serviceMerchantId;
    @JsonProperty("entity_id")
    private final String entityId;
    private final Organization organization;
    private final String description;
    private final boolean deleted;
    private final boolean enabled;
    @Nullable
    private final String legalAddress;
}
