package ru.yandex.quasar.billing.services.promo;

import lombok.Data;

@Data
public class DeviceId {
    private final String id;
    private final Platform platform;

    public static DeviceId create(String id, Platform platform) {
        return new DeviceId(id, platform);
    }
}
