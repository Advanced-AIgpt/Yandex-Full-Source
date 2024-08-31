package ru.yandex.quasar.billing.services.promo;

import ru.yandex.quasar.billing.exception.InternalErrorException;

public class NoPromoCodeLeftException extends InternalErrorException {
    NoPromoCodeLeftException(String provider, String promoType) {
        super("No promocode left for " + provider + " and promotype=" + promoType);
    }
}
