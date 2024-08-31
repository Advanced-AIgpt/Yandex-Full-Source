package ru.yandex.quasar.billing.providers;

import java.util.Collections;
import java.util.Map;
import java.util.Optional;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;

public abstract class DelegatingContentProvider extends BasicProvider implements IContentProvider {


    private IContentProvider delegate;

    public DelegatingContentProvider(String providerName, String providerSocialName, @Nullable String socialCliendId) {
        super(providerName, providerSocialName, socialCliendId, true, 1);
    }

    public void setDelegate(IContentProvider delegate) {
        this.delegate = delegate;
    }

    @Override
    public Integer getPriority() {
        return delegate != null ? delegate.getPriority() : 1;
    }

    @Override
    public boolean showInYandexApp() {
        return delegate != null ? delegate.showInYandexApp() : true;
    }

    @Override
    public ContentMetaInfo getContentMetaInfo(ProviderContentItem item) {
        var defaultItem = new ContentMetaInfo(item.toString(), "url", 2018, 150, "18", "RUS",
                item.getContentType() == ContentType.SEASON || item.getContentType() == ContentType.EPISODE ? 1 :
                        null, "description");

        return Optional.ofNullable(delegate)
                .flatMap(it -> Optional.ofNullable(it.getContentMetaInfo(item)))
                .orElse(defaultItem);
    }

    @Override
    public ProviderPricingOptions getPricingOptions(ProviderContentItem item, @Nullable String session) {
        return delegate != null ? delegate.getPricingOptions(item, session) : null;
    }

    @Override
    public AvailabilityInfo getAvailability(ProviderContentItem providerContentItem, @Nullable String session,
                                            String userIp, String userAgent, boolean withStream) {
        return delegate != null ? delegate.getAvailability(providerContentItem, session, userIp, userAgent,
                withStream) : null;
    }

    @Override
    public void processPurchase(ProviderContentItem purchasingItem, PricingOption selectedOption,
                                String transactionId, String session)
            throws ProviderPurchaseException {
        if (delegate != null) {
            processPurchase(purchasingItem, selectedOption, transactionId, session);
        }
    }

    @Override
    public ProviderPromoCodeActivationResult activatePromoCode(@Nonnull String promoCode, @Nonnull String session)
            throws PromoCodeActivationException {
        return delegate != null ? delegate.activatePromoCode(promoCode, session) : null;
    }

    @Override
    public boolean isSubscriptionRenewalCancelled(ProviderContentItem subscriptionItem, String session)
            throws ProviderUnauthorizedException, WrongContentTypeException {
        return delegate != null ? delegate.isSubscriptionRenewalCancelled(subscriptionItem, session) : false;
    }

    @Override
    public Map<ProviderContentItem, ProviderActiveSubscriptionInfo> getActiveSubscriptions(@Nonnull String session) {
        return delegate != null ? delegate.getActiveSubscriptions(session) : Collections.emptyMap();
    }

}
