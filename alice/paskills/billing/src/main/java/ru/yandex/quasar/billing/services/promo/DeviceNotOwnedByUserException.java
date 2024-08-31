package ru.yandex.quasar.billing.services.promo;

import java.text.MessageFormat;

import ru.yandex.quasar.billing.exception.BadRequestException;

class DeviceNotOwnedByUserException extends BadRequestException {

    DeviceNotOwnedByUserException(String uid, DeviceId devices) {
        super(MessageFormat.format("Device {0} is not owned by user: {1} ", uid, devices));
    }
}
