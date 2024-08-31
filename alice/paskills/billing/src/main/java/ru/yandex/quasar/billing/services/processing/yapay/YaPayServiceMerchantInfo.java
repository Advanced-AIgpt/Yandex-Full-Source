package ru.yandex.quasar.billing.services.processing.yapay;

import java.util.Map;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

@Data
@Builder
@Nullable
public class YaPayServiceMerchantInfo {
    @JsonProperty("service_merchant_id")
    private final long serviceMerchantId;
    @JsonProperty("entity_id")
    private final String entityId;
    private final Organization organization;
    private final String description;
    private final boolean deleted;
    private final boolean enabled;
    private final Map<AddressType, Address> addresses;

    public enum AddressType {
        legal,
        post
    }

    @Data
    @Builder
    @Nullable
    public static class Address {
        private final String street;
        private final String home;
        private final String city;
        private final String country;
        private final String zip;
    }
}
