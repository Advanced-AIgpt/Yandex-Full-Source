package ru.yandex.quasar.billing.providers.universal;

import ru.yandex.quasar.billing.beans.ContentType;

final class ContentTypeConverter {

    private ContentTypeConverter() {
        throw new UnsupportedOperationException();
    }

    static ContentType toContentType(ContentItemType contentItemType) {
        switch (contentItemType) {
            case MOVIE:
                return ContentType.MOVIE;
            case TV_SHOW:
                return ContentType.TV_SHOW;
            case TV_SHOW_SEASON:
                return ContentType.SEASON;
            case TV_SHOW_EPISODE:
                return ContentType.EPISODE;
            case TRAILER:
                return ContentType.TRAILER;
            default:
                throw new IllegalArgumentException();
        }
    }

    static ContentItemType fromContentType(ContentType contentType) {
        switch (contentType) {
            case MOVIE:
                return ContentItemType.MOVIE;
            case TV_SHOW:
                return ContentItemType.TV_SHOW;
            case SEASON:
                return ContentItemType.TV_SHOW_SEASON;
            case EPISODE:
                return ContentItemType.TV_SHOW_EPISODE;
            case TRAILER:
                return ContentItemType.TRAILER;
            default:
                throw new IllegalArgumentException();
        }
    }

}
