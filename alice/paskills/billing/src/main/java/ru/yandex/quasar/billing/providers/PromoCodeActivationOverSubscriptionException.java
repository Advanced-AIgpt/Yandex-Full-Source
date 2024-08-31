package ru.yandex.quasar.billing.providers;

/**
 * Promocode activation exception when subscription already exists on the user's provider account
 */
public class PromoCodeActivationOverSubscriptionException extends PromoCodeActivationException {
    public PromoCodeActivationOverSubscriptionException() {
    }

    public PromoCodeActivationOverSubscriptionException(Throwable cause) {
        super(cause);
    }
}
