package ru.yandex.quasar.billing.exception;

import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.ResponseStatus;

@ResponseStatus(HttpStatus.UNAUTHORIZED)
public class ProviderUnauthorizedException extends UnauthorizedException {

    private final String providerName;
    private final boolean socialApiSessionFound;

    public ProviderUnauthorizedException(String providerName, boolean socialApiSessionFound) {
        this("Provider's session is invalid. ", providerName, socialApiSessionFound);
    }

    public ProviderUnauthorizedException(String message, String providerName, boolean socialApiSessionFound) {
        super(message);
        this.providerName = providerName;
        this.socialApiSessionFound = socialApiSessionFound;
    }

    public ProviderUnauthorizedException(String providerName, boolean socialApiSessionFound, Throwable cause) {
        this("Provider's session is invalid", providerName, socialApiSessionFound, cause);
    }

    public ProviderUnauthorizedException(String message, String providerName, boolean socialApiSessionFound,
                                         Throwable cause) {
        super(message, cause);
        this.providerName = providerName;
        this.socialApiSessionFound = socialApiSessionFound;
    }

    public String getProviderName() {
        return providerName;
    }

    public boolean isSocialApiSessionFound() {
        return socialApiSessionFound;
    }

    @Override
    public String getMessage() {
        return super.getMessage() + " Provider: " + providerName + " socialApiSessionFound: " + socialApiSessionFound;
    }
}
