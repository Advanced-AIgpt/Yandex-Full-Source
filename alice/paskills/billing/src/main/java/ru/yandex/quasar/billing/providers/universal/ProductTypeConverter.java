package ru.yandex.quasar.billing.providers.universal;

import ru.yandex.quasar.billing.beans.ContentType;

final class ProductTypeConverter {

    private ProductTypeConverter() {
        throw new UnsupportedOperationException();
    }

    static ContentType toContentType(ProductType productType) {
        switch (productType) {
            case MOVIE:
                return ContentType.MOVIE;
            case TV_SHOW:
                return ContentType.TV_SHOW;
            case TV_SHOW_SEASON:
                return ContentType.SEASON;
            case TV_SHOW_EPISODE:
                return ContentType.EPISODE;
            case SUBSCRIPTION:
                return ContentType.SUBSCRIPTION;
            default:
                throw new IllegalArgumentException();
        }
    }

    static ProductType fromContentType(ContentType contentType) {
        switch (contentType) {
            case MOVIE:
                return ProductType.MOVIE;
            case TV_SHOW:
                return ProductType.TV_SHOW;
            case SEASON:
                return ProductType.TV_SHOW_SEASON;
            case EPISODE:
                return ProductType.TV_SHOW_EPISODE;
            case SUBSCRIPTION:
                return ProductType.SUBSCRIPTION;
            default:
                throw new IllegalArgumentException();
        }
    }

}
