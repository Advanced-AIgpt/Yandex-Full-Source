package ru.yandex.quasar.billing.services.promo;

import java.time.Instant;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.quasar.billing.beans.PromoType;

@Data
public class DeviceInfo {
    private final String deviceId;
    private final Platform platform;
    private final String name;
    @JsonIgnore
    private final Map<PromoProvider, PromoType> availablePromoTypes;
    @Nullable
    private final Instant firstActivation;

    @JsonProperty("available_promos")
    public Set<PromoProvider> getAvailablePromos() {
        return availablePromoTypes.keySet();
    }

    @JsonIgnore
    public DeviceId getDeviceIdentifier() {
        return DeviceId.create(deviceId, platform);
    }
}
