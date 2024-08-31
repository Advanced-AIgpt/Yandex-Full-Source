package ru.yandex.quasar.billing.controller;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

import ru.yandex.quasar.billing.services.skills.MerchantInfo;

@Data
@Builder
class ServiceMerchant {
    @JsonProperty("entity_id")
    private final String entityId;
    @Nonnull
    private final String token;
    //private final Organization organization;
    @Nonnull
    @JsonProperty("organization_name")
    private final String organizationName;
    private final String description;
    private final boolean deleted;
    private final boolean enabled;

    static ServiceMerchant fromMerchantInfo(MerchantInfo merchantInfo) {
        return ServiceMerchant.builder()
                .entityId(merchantInfo.getEntityId())
                .token(merchantInfo.getToken())
                .organizationName(merchantInfo.getOrganization().getName())
                .description(merchantInfo.getDescription())
                .deleted(merchantInfo.isDeleted())
                .enabled(merchantInfo.isEnabled())
                .build();
    }
}
