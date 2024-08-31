package ru.yandex.quasar.billing.exception;

import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.ResponseStatus;

/**
 * Means that some kind of resource expired beyond usability and can not be accessed anymore.
 * <p>
 * For example, a {@link ru.yandex.quasar.billing.beans.PricingOption} is not available any longer at a provider.
 */
@ResponseStatus(HttpStatus.GONE)
public class ExpiredException extends AbstractHTTPException {

    public ExpiredException(String message) {
        super(message);
    }
}
