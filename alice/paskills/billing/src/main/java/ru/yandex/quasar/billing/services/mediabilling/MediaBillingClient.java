package ru.yandex.quasar.billing.services.mediabilling;

import javax.annotation.Nullable;

import ru.yandex.quasar.billing.services.processing.trust.TemplateTag;
import ru.yandex.quasar.billing.services.promo.PromoInfo;

/**
 * Client to communicate to Media billing which grants Plus promo-period to the user
 *
 * @throws MediaBillingException
 */
public interface MediaBillingClient {

    /**
     * Activate promocode using mediabilling universal handler
     *
     * @param uid             user identifier
     * @param code            promocode
     * @param paymentMethodId TRUST card ID to charge after trial period expired
     */
    PromoCodeActivationResult activatePromoCode(long uid,
                                                String code,
                                                String paymentMethodId,
                                                String origin,
                                                @Nullable Integer region);

    /**
     * Estimate which product will be granter to the user by the prototype
     *
     * @param uid       user identifier
     * @param prototype promocode prototype
     * @param platform  platform (web)
     * @param region    region
     */
    PromoInfo prototypeFeatures(long uid, String prototype, @Nullable String platform, @Nullable Integer region);

    /**
     * Submit order for subsccription on Mediabilling
     *
     * @param uid           user identifier
     * @param productId     mediabilling product id
     * @param paymentCardId TRUST card ID to charge after trial period expired
     * @return order creation result
     */
    SubmitNativeOrderResult submitNativeOrder(String uid, String productId, String paymentCardId);

    /**
     *
     */
    String clonePrototype(String prototype);


    /**
     * Check order info in Mediabilling
     *
     * @param uid     user identifier
     * @param orderId order ID
     * @return order info
     */
    OrderInfo getOrderInfo(String uid, String orderId);

    BindCardResult bindCard(String uid, String userIp, String backUrl, TemplateTag template);

    enum MusicPromoActivationResult {
        SUCCESS("success"),
        FAILED_TO_CREATE_PAYMENT("failed-to-create-payment"),
        CODE_NOT_ALLOWED_IN_CURRENT_REGION("code-not-allowed-in-current-region"),
        USER_TEMPORARY_BANNED("user-temporary-banned"),
        CODE_ALREADY_CONSUMED("code-already-consumed"),
        CODE_EXPIRED("code-expired"),
        CODE_NOT_EXISTS("code-not-exists"),
        USER_HAS_TEMPORARY_CAMPAIGN_RESTRICTIONS("user-has-temporary-campaign-restrictions"),
        CODE_ONLY_FOR_NEW_USERS("code-only-for-new-users"),
        CODE_ONLY_FOR_WEB("code-only-for-web"),
        CODE_ONLY_FOR_MOBILE("code-only-for-mobile"),
        ALLOW_BILLING_PRODUCT("allow-billing-product"),
        CODE_CANT_BE_CONSUMED("code-cant-be-consumed"),
        UNKNOWN("unknown-error");

        private final String status;

        MusicPromoActivationResult(String code) {
            this.status = code;
        }

        static MusicPromoActivationResult byStatus(String status) {
            for (MusicPromoActivationResult value : MusicPromoActivationResult.values()) {
                if (value.status.equals(status)) {
                    return value;
                }
            }
            return UNKNOWN;
        }

        static MusicPromoActivationResult byName(String status) {
            try {
                return valueOf(status);
            } catch (IllegalArgumentException e) {
                return UNKNOWN;
            }
        }
    }
}
