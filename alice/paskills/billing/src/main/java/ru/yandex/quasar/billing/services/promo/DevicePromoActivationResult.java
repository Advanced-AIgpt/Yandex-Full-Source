package ru.yandex.quasar.billing.services.promo;

import ru.yandex.quasar.billing.util.HasCode;

public enum DevicePromoActivationResult implements HasCode<String> {
    SUCCESS("success"),
    ALREADY_ACTIVATED("already_activated"),
    SUBSCRIPTION_EXISTS("subscription_exists"),
    NOT_ALLOWED_IN_CURRENT_REGION("not_allowed_in_current_region"),
    USER_HAS_TEMPORARY_CAMPAIGN_RESTRICTIONS("user_has_temporary_campaign_restrictions"),
    USER_TEMPORARY_BANNED("user_temporary_banned"),
    CODE_EXPIRED("code_expired"),
    NOT_EXISTS("not_exists"),
    ONLY_FOR_NEW_USERS("only_for_new_users"),
    ONLY_FOR_WEB("only_for_web"),
    ONLY_FOR_MOBILE("only_for_mobile"),
    FAILED_TO_CREATE_PAYMENT("failed_to_create_payment"),
    CANT_BE_CONSUMED("cant_be_consumed"),
    ERROR("error");

    private final String code;

    DevicePromoActivationResult(String code) {
        this.code = code;
    }

    @Override
    public String getCode() {
        return this.code;
    }
}
