package ru.yandex.quasar.billing.services.promo;

import ru.yandex.quasar.billing.exception.BadRequestException;

public class NoCardBoundException extends BadRequestException {
    public NoCardBoundException() {
        super("No card bound to account");
    }
}
