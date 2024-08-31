package ru.yandex.quasar.billing.services.promo;

import java.text.MessageFormat;

import ru.yandex.quasar.billing.exception.BadRequestException;

public class DevicePromoAlreadyUsedException extends BadRequestException {
    public DevicePromoAlreadyUsedException(DeviceId deviceId, PromoProvider promoProvider) {
        super(MessageFormat.format("Device {0}:{1} promo {2} is already used", deviceId.getPlatform(),
                deviceId.getId(), promoProvider));
    }

    public DevicePromoAlreadyUsedException(DeviceId deviceId, PromoProvider promoProvider, Throwable e) {
        super(MessageFormat.format("Device {0}:{1} promo {2} is already used", deviceId.getPlatform(),
                deviceId.getId(), promoProvider), e);
    }
}
