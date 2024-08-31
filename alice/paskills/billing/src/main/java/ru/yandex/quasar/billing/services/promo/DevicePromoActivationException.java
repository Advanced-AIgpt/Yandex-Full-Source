package ru.yandex.quasar.billing.services.promo;

import java.text.MessageFormat;

import ru.yandex.quasar.billing.exception.BadRequestException;

public class DevicePromoActivationException extends BadRequestException {
    public DevicePromoActivationException(DeviceId deviceId, PromoProvider promoProvider, Throwable cause) {
        super(MessageFormat.format("Device {0}:{1} promo {2} unknown activation exception", deviceId.getPlatform(),
                deviceId.getId(), promoProvider), cause);
    }
}
