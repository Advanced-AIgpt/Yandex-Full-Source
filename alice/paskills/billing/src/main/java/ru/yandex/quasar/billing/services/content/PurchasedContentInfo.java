package ru.yandex.quasar.billing.services.content;

import lombok.Data;

import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.ProviderContentItem;

@Data
public class PurchasedContentInfo {

    private final ContentMetaInfo contentMetaInfo;

    private final String provider;

    private final ProviderContentItem contentItem;
}
