package ru.yandex.quasar.billing.providers.amediateka.model;

import java.util.Collection;
import java.util.Collections;
import java.util.List;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonSubTypes;
import com.fasterxml.jackson.annotation.JsonTypeInfo;

import ru.yandex.quasar.billing.beans.ProviderContentItem;

@JsonTypeInfo(use = JsonTypeInfo.Id.NAME, include = JsonTypeInfo.As.EXISTING_PROPERTY, property = "object",
        defaultImpl = BundleItem.SimpleBundleItem.class)
@JsonSubTypes(value = {
        @JsonSubTypes.Type(value = FilmBundleItem.class, name = FilmBundleItem.OBJECT),
        @JsonSubTypes.Type(value = SerialBundleItem.class, name = SerialBundleItem.OBJECT)
})
public abstract class BundleItem extends BaseObject implements ProviderContentIterable {

    public static class BundleItemsDTO extends MultipleDTO<BundleItem> {
        private List<BundleItem> items;

        public List<BundleItem> getItems() {
            return items;
        }

        public void setItems(List<BundleItem> items) {
            this.items = items;
        }

        @Override
        @JsonIgnore
        public List<BundleItem> getPayload() {
            return items;
        }
    }

    /**
     * a dummy implementation for unknown / ignored "object"s, i.e. "channel"
     */
    public static class SimpleBundleItem extends BundleItem {

        /**
         * Используется только в AmediatekaBundlesCache, а ему достаточно ProviderContentItem.JustIdProviderContentItem
         *
         * @see ru.yandex.quasar.billing.providers.amediateka.AmediatekaBundlesCache
         */
        @Override
        public Collection<ProviderContentItem> getProviderContentItems() {
            return Collections.emptyList();
        }
    }
}
