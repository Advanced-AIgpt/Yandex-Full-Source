package ru.yandex.quasar.billing.controller;

import java.util.Collection;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;

import ru.yandex.quasar.billing.beans.ContentItem;
import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.ProviderContentItem;

@Data
class GetContentPurchasesResponse {

    private final Collection<Item> items;

    @Data
    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    static class Item {

        private final ContentMetaInfo contentMetaInfo;

        private final String provider;

        private final ContentItem contentItem;

        Item(ContentMetaInfo contentMetaInfo, String provider, ProviderContentItem providerContentItem) {
            this.contentMetaInfo = contentMetaInfo;
            this.provider = provider;
            this.contentItem = new ContentItem(provider, providerContentItem);
        }
    }
}
