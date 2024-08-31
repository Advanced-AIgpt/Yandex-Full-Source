package ru.yandex.quasar.billing.providers;

import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.ResponseStatus;

import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.exception.AbstractHTTPException;

@ResponseStatus(HttpStatus.NOT_FOUND)
public class ContentNotAvailableException extends AbstractHTTPException {
    private final ProviderContentItem providerContentItem;

    public ContentNotAvailableException(ProviderContentItem providerContentItem) {
        super("Content not available: " + providerContentItem);
        this.providerContentItem = providerContentItem;
    }

    public ContentNotAvailableException(Throwable cause, ProviderContentItem providerContentItem) {
        super("Content not available" + providerContentItem, cause);
        this.providerContentItem = providerContentItem;
    }

    public ProviderContentItem getProviderContentItem() {
        return providerContentItem;
    }
}
