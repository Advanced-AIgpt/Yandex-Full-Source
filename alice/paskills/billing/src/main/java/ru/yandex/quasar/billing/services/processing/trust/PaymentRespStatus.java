package ru.yandex.quasar.billing.services.processing.trust;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;

import ru.yandex.quasar.billing.services.PaymentInfo;

public enum PaymentRespStatus {
    not_enough_funds(PaymentInfo.Status.ERROR_NOT_ENOUGH_FUNDS),

    expired_card(PaymentInfo.Status.ERROR_EXPIRED_CARD),

    limit_exceeded(PaymentInfo.Status.ERROR_LIMIT_EXCEEDED),

    authorization_reject(PaymentInfo.Status.ERROR_TRY_LATER),
    payment_timeout(PaymentInfo.Status.ERROR_TRY_LATER),
    fail_3ds(PaymentInfo.Status.ERROR_TRY_LATER),
    payment_gateway_technical_error(PaymentInfo.Status.ERROR_TRY_LATER),
    timedout_no_success(PaymentInfo.Status.ERROR_TRY_LATER),
    acquirer_payment_timeout(PaymentInfo.Status.ERROR_TRY_LATER),
    incorrect_transaction_status(PaymentInfo.Status.ERROR_TRY_LATER),
    declined_by_issuer(PaymentInfo.Status.ERROR_TRY_LATER),
    invalid_payment_token(PaymentInfo.Status.ERROR_TRY_LATER),
    issuer_technical_error(PaymentInfo.Status.ERROR_TRY_LATER),
    operation_in_progress(PaymentInfo.Status.ERROR_TRY_LATER),
    service_unavailable(PaymentInfo.Status.ERROR_TRY_LATER),
    processing_error(PaymentInfo.Status.ERROR_TRY_LATER),

    transaction_not_permitted(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    invalid_processing_request(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    restricted_card(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    blacklisted(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    promocode_already_used(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    ext_action_required(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    invalid_card_credentials(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    invalid_fiscal_data(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    invalid_signature(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    invalid_terminal_setup(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    invalid_user_credentials(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    operation_cancelled(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    operation_not_permitted(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    technical_error(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    transaction_not_found(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    unclassified_response_code(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    unknown_status(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    user_cancelled(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    wrong_amount(PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER),
    @JsonEnumDefaultValue
    unknown_error(PaymentInfo.Status.ERROR_UNKNOWN);

    private final PaymentInfo.Status purchaseInfoStatus;

    PaymentRespStatus(PaymentInfo.Status purchaseInfoStatus) {
        this.purchaseInfoStatus = purchaseInfoStatus;
    }

    @JsonCreator
    static PaymentRespStatus parse(String value) {
        try {
            return PaymentRespStatus.valueOf(value);
        } catch (NullPointerException | IllegalArgumentException e) {
            return unknown_error;
        }
    }

    public PaymentInfo.Status getPaymentStatus() {
        return purchaseInfoStatus;
    }


}
