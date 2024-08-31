package ru.yandex.quasar.billing.providers.amediateka.model;

import java.util.Collection;

import ru.yandex.quasar.billing.beans.ProviderContentItem;

/**
 * Interface for all the entities that can be converted to a `{@link ProviderContentItem}` (with nested ones)
 */
public interface ProviderContentIterable {
    /**
     * Subclasses should implement this to return a stream with their own `PricingItem` as well as any nested items.
     *
     * @return a Collection with `{@link ProviderContentItem}` for this object as well as for all nested objects.
     */
    Collection<ProviderContentItem> getProviderContentItems();
}
