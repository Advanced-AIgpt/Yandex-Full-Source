package ru.yandex.quasar.billing.providers.amediateka.model;

import java.util.Collection;
import java.util.Collections;

import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;

public class SeasonBundleItem extends BaseObject implements ProviderContentIterable {
    @Override
    public Collection<ProviderContentItem> getProviderContentItems() {
        return Collections.singletonList(ProviderContentItem.create(ContentType.SEASON, this.getId()));
    }
}
