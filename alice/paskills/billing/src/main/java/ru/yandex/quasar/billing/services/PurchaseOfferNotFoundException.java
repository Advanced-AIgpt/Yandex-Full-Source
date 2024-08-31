package ru.yandex.quasar.billing.services;

import java.text.MessageFormat;

import ru.yandex.quasar.billing.exception.NotFoundException;

class PurchaseOfferNotFoundException extends NotFoundException {
    PurchaseOfferNotFoundException(String uid, String purchaseOfferUuid) {
        super(MessageFormat.format("Purchase offer {1} for user {0} not found", uid, purchaseOfferUuid));
    }
}
