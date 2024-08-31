package ru.yandex.quasar.billing.controller;

import ru.yandex.quasar.billing.services.promo.DevicePromoActivationResult;


record DevicePromoActivationResponse(DevicePromoActivationResult result) {
    public static DevicePromoActivationResponse create(DevicePromoActivationResult result) {
        return new DevicePromoActivationResponse(result);
    }

}
