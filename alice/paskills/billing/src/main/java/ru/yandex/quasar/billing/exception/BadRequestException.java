package ru.yandex.quasar.billing.exception;

import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.ResponseStatus;

import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;

@ResponseStatus(code = HttpStatus.BAD_REQUEST)
public class BadRequestException extends AbstractHTTPException {

    public BadRequestException() { }

    public BadRequestException(String message) {
        super(message);
    }

    public BadRequestException(String message, Throwable cause) {
        super(message, cause);
    }

    /**
     * Convenience factory method to create a {@link BadRequestException} for an unsupported {@link ProviderContentItem}
     */
    public static BadRequestException unsupportedContentType(ProviderContentItem item) {
        return unsupportedContentType(item.getContentType());
    }

    public static BadRequestException unsupportedContentType(ContentType contentType) {
        return new BadRequestException("Unsupported type " + contentType.getName());
    }
}
