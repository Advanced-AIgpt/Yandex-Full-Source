package ru.yandex.quasar.billing.services.promo;

import java.text.MessageFormat;

import ru.yandex.quasar.billing.exception.InternalServerErrorException;

import static ru.yandex.quasar.billing.services.mediabilling.MediaBillingClient.MusicPromoActivationResult;

public class UnknownPromoActivationResultException extends InternalServerErrorException {
    public UnknownPromoActivationResultException(
            DeviceId deviceId,
            PromoProvider promoProvider,
            MusicPromoActivationResult reason
    ) {
        super(MessageFormat.format(
                "Device {0}:{1} promo {2} unknown activation result: {3}",
                deviceId.getPlatform(),
                deviceId.getId(),
                promoProvider,
                reason.name()
        ));
    }
}
