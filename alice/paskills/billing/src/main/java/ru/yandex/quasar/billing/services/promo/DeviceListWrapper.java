package ru.yandex.quasar.billing.services.promo;

import java.util.List;

import lombok.Data;

@Data
class DeviceListWrapper {
    private final List<BackendDeviceInfo> devices;
    private final String status;
}
