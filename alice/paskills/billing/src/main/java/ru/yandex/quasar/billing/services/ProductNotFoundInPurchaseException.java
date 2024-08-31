package ru.yandex.quasar.billing.services;

import java.text.MessageFormat;

import ru.yandex.quasar.billing.exception.BadRequestException;

class ProductNotFoundInPurchaseException extends BadRequestException {
    ProductNotFoundInPurchaseException(String uid, String purchaseOfferUuid, String selectedProductUuid) {
        super(MessageFormat.format("Product {2} not found in purchase offer {1} for user {0}", uid, purchaseOfferUuid,
                selectedProductUuid));
    }
}
