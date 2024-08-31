package ru.yandex.quasar.billing.controller;

import java.util.Collections;
import java.util.Set;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.services.promo.Platform;

@Data
class ProviderInfoResponse {
    private final String providerName;
    @Nullable
    private final String socialAPIServiceName;
    @Nullable
    private final String socialAPIClientId;
    @JsonProperty("isAuthorized")
    private final boolean authorized;
    private final Set<DeviceShortInfo> devicesWithPromo;

    static ProviderInfoResponse unauthorized(String providerName) {
        return new ProviderInfoResponse(providerName, null, null, false, Collections.emptySet());
    }

    @Data
    static class DeviceShortInfo {
        private final String deviceId;
        private final Platform platform;
        private final String name;
        private final PromoType promoType;
        @Nullable
        @JsonInclude(JsonInclude.Include.NON_NULL)
        private final String expirationTime;
    }
}
