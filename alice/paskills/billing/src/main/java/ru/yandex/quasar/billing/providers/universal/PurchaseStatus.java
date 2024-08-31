package ru.yandex.quasar.billing.providers.universal;

import ru.yandex.quasar.billing.dao.PurchaseInfo;

enum PurchaseStatus {
    OK,
    ALREADY_AVAILABLE,
    ERROR_PAYMENT_OPTION_OBSOLETE,
    ERROR_TRY_LATER,
    ERROR_DO_NOT_TRY_LATER,
    ERROR_NOT_AUTHORIZED,
    ERROR_UNKNOWN;

    static PurchaseStatus map(PurchaseInfo.Status status) {
        if (status == null) {
            return null;
        }
        switch (status) {
            case ERROR_TRY_LATER:
                return ERROR_TRY_LATER;

            case ERROR_PAYMENT_OPTION_OBSOLETE:
                return ERROR_PAYMENT_OPTION_OBSOLETE;

            case ERROR_DO_NOT_TRY_LATER:
                return ERROR_DO_NOT_TRY_LATER;

            case ERROR_NO_PROVIDER_ACC:
                return ERROR_NOT_AUTHORIZED;

            case ALREADY_AVAILABLE:
                return ALREADY_AVAILABLE;

            case ERROR_NOT_ENOUGH_FUNDS:
            case ERROR_EXPIRED_CARD:
            case ERROR_LIMIT_EXCEEDED:
            case ERROR_UNKNOWN:
                return ERROR_UNKNOWN;

            case CLEARED:
            case STARTED:
            case REFUNDED:
            case PROCESSED:
                return OK;
            default:
                throw new IllegalArgumentException();
        }
    }
}
