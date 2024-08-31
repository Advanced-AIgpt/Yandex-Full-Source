package ru.yandex.quasar.billing.services.skills;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;
import lombok.ToString;

import ru.yandex.quasar.billing.services.processing.yapay.Organization;
import ru.yandex.quasar.billing.services.processing.yapay.ServiceMerchantInfo;

@AllArgsConstructor
@Builder
@Getter
@ToString
public class MerchantInfo {
    private final String token;
    //private final Instant requestDate;
    private final long serviceMerchantId;
    private final String entityId;
    private final Organization organization;
    private final String description;
    private final boolean deleted;
    private final boolean enabled;
    @Nullable
    private final String legalAddress;

    static MerchantInfo fromServiceMerchantInfo(String token, ServiceMerchantInfo merchantInfo) {
        return new MerchantInfo(token,
                merchantInfo.getServiceMerchantId(),
                merchantInfo.getEntityId(),
                merchantInfo.getOrganization(),
                merchantInfo.getDescription(),
                merchantInfo.isDeleted(),
                merchantInfo.isEnabled(),
                merchantInfo.getLegalAddress());
    }
}
