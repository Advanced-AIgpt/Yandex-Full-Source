package ru.yandex.quasar.billing.providers.amediateka.model;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import lombok.Getter;

import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;

import static com.google.common.collect.Lists.newArrayList;

/**
 * Item for serial, contains sub-items of serials
 */
@Getter
public class SerialBundleItem extends BundleItem {
    static final String OBJECT = "serial";

    private List<SeasonBundleItem> seasons;

    @Override
    public Collection<ProviderContentItem> getProviderContentItems() {

        ArrayList<ProviderContentItem> items = newArrayList(ProviderContentItem.create(ContentType.TV_SHOW, getId()));


        for (SeasonBundleItem season : getSeasons()) {
            items.addAll(season.getProviderContentItems());
        }

        return items;
    }
}
