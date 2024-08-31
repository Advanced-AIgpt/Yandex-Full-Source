package ru.yandex.quasar.billing.services;

import java.util.Optional;

import javax.annotation.Nullable;
import javax.servlet.http.HttpServletRequest;

import ru.yandex.quasar.billing.exception.UnauthorizedException;

public interface AuthorizationService {

    String X_REQUEST_ID = "X-Request-Id";

    boolean hasAuthInfo(HttpServletRequest request);

    String getUserIp(HttpServletRequest request);

    String getSecretUid(HttpServletRequest request) throws UnauthorizedException;
    Optional<String> getSecretUid(HttpServletRequest request, boolean allowAnonymous) throws UnauthorizedException;

    Optional<String> getUserAgent(HttpServletRequest request);

    Optional<String> getProviderTokenByUid(String uid, String providerSocialName);

    /**
     * check is current user has active Plus subscription
     */
    boolean userHasPlus();

    /**
     * get TVM user ticket for current user
     *
     * @return user ticket or null if current uid is null
     */
    @Nullable
    String getTvmUserTicket();
}
