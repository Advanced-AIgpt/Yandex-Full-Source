package ru.yandex.quasar.billing.providers;

import java.util.Map;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;

/**
 * Если session помечена как {@link Nullable} то допускается вызов от анонимного пользователя
 */
public interface IContentProvider extends IProvider {

    ContentMetaInfo getContentMetaInfo(ProviderContentItem item);

    ProviderPricingOptions getPricingOptions(ProviderContentItem item, @Nullable String session);

    AvailabilityInfo getAvailability(ProviderContentItem providerContentItem, @Nullable String session, String userIp,
                                     String userAgent, boolean withStream);

    /**
     * Request stream data from content provider
     *
     * @return content item stream data
     */
    default StreamData getStream(ProviderContentItem providerContentItem, @Nullable String session, String userIp,
                                 String userAgent) {
        return StreamData.EMPTY;
    }

    void processPurchase(ProviderContentItem purchasingItem, PricingOption selectedOption, String transactionId,
                         String session) throws ProviderPurchaseException;

    /**
     * todo: нужно понимать, какой ProviderContentItem активирован данным промокодом. Если это подписка, нужно будет
     * создавать подписку в трасте на момент после окончания промо-периода
     *
     * @param promoCode promo code
     * @param session   user provider's token
     * @return promo code activation result
     * @throws PromoCodeActivationException and its subtypes if something went wrong while activating promocode
     */
    ProviderPromoCodeActivationResult activatePromoCode(String promoCode, String session);

    /**
     * Get map of active subscriptions with their properties
     *
     * @param session user provider's session
     * @return map of subscription contentItem vs active subscription properties
     */
    Map<ProviderContentItem, ProviderActiveSubscriptionInfo> getActiveSubscriptions(@Nonnull String session);

    /**
     * Check if subscription renewal is cancelled on the provider's side. If so we should stop it on our side
     *
     * @param subscriptionItem provider content item for the subscription
     * @param session          user token
     * @return if subscription is valid for prolongation
     * @throws ProviderUnauthorizedException if token has expired
     * @throws WrongContentTypeException     if passed {@param subscriptionItem} is of the wrong ContentType
     */
    default boolean isSubscriptionRenewalCancelled(ProviderContentItem subscriptionItem, String session)
            throws ProviderUnauthorizedException, WrongContentTypeException {
        if (subscriptionItem.getContentType() != ContentType.SUBSCRIPTION) {
            throw new WrongContentTypeException();
        }
        return false;
    }

}
