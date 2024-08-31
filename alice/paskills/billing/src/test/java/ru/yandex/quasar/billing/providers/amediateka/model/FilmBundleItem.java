package ru.yandex.quasar.billing.providers.amediateka.model;

import java.util.Collection;
import java.util.Collections;

import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;

/**
 * An item for a basic film. It's only `PricingItem` is the film itself
 */
public class FilmBundleItem extends BundleItem {
    static final String OBJECT = "film";

    @Override
    public Collection<ProviderContentItem> getProviderContentItems() {
        return Collections.singletonList(ProviderContentItem.create(ContentType.MOVIE, this.getId()));
    }
}
