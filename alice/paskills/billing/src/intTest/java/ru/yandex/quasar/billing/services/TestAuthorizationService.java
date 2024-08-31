package ru.yandex.quasar.billing.services;

import java.util.Arrays;
import java.util.Map;
import java.util.Optional;

import javax.annotation.Nullable;
import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletRequest;

import org.apache.logging.log4j.util.Strings;
import org.springframework.context.annotation.Primary;
import org.springframework.stereotype.Component;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;

import ru.yandex.quasar.billing.exception.UnauthorizedException;
import ru.yandex.quasar.billing.services.tvm.TestTvmClientImpl;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;
import ru.yandex.quasar.billing.util.AuthHelper;

import static java.util.Optional.ofNullable;

@Component
@Primary
public class TestAuthorizationService implements AuthorizationService {

    public static final String OAUTH_TOKEN = "oauth_token";
    public static final String SESSION_ID = "sessionId";
    public static final String UID = TestTvmClientImpl.UID;
    public static final String IP = "127.0.0.1";
    public static final String PROVIDER_TOKEN = "PROVIDER_TOKEN";
    public static final String USER_TICKET = TestTvmClientImpl.USER_TICKET;
    public static final String SERVICE_TICKET = TestTvmClientImpl.SERVICE_TICKET;


    private final AuthorizationContext authorizationContext;

    private final Map<String, String> oauthTokenToUid = Map.of(OAUTH_TOKEN, UID);
    private final Map<String, String> userTicketToUid = Map.of(USER_TICKET, UID);
    private final Map<String, String> sessionToUid = Map.of(SESSION_ID, UID);
    private final Map<String, String> uidToTestProviderToken = Map.of(UID, PROVIDER_TOKEN);

    public TestAuthorizationService(AuthorizationContext authorizationContext) {
        this.authorizationContext = authorizationContext;
    }

    public static MultiValueMap<String, String> getSessionCookie() {
        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        headers.add("Cookie", "Session_id=" + TestAuthorizationService.SESSION_ID);
        return headers;
    }

    @Override
    public String getSecretUid(HttpServletRequest request) {
        return getSecretUid(request, false).get();
    }

    @Override
    public Optional<String> getSecretUid(HttpServletRequest request, boolean allowAnonymous)
            throws UnauthorizedException {
        authorizationContext.clearUserContext();
        authorizationContext.setUserIp(getUserIp(request));
        authorizationContext.setUserAgent(getUserAgent(request).orElse(null));

        String authorization = request.getHeader("Authorization");
        String uid;
        String userTicket;
        String sessionId;
        if (authorization != null) {
            String token = authorization.replace("OAuth ", "");
            uid = oauthTokenToUid.get(token);
            authorizationContext.setOauthToken(token);
            if (token.equals(TestAuthorizationService.OAUTH_TOKEN)) {
                authorizationContext.setCurrentUserTicket(TestAuthorizationService.USER_TICKET);
            }
        } else if ((userTicket = request.getHeader(TvmHeaders.USER_TICKET_HEADER)) != null) {
            String serviceTicket = request.getHeader(TvmHeaders.SERVICE_TICKET_HEADER);
            if (!SERVICE_TICKET.equals(serviceTicket)) {
                throw new UnauthorizedException("Service ticket is required for TVM authorization");
            }
            uid = userTicketToUid.get(userTicket);
            authorizationContext.setCurrentUserTicket(userTicket);
        } else if ((sessionId = getSessionIdHeader(request)) != null) {
            uid = sessionToUid.get(sessionId);
            authorizationContext.setSessionId(sessionId);
            if (sessionId.equals(TestAuthorizationService.SESSION_ID)) {
                authorizationContext.setCurrentUserTicket(TestAuthorizationService.USER_TICKET);
            }
        } else if (allowAnonymous) {
            uid = null;
        } else {
            throw new UnauthorizedException("Either OAuth token or sessionId must be present");
        }
        authorizationContext.setCurrentUid(uid);
        return Optional.ofNullable(uid);
    }

    @Override
    public Optional<String> getProviderTokenByUid(String uid, String providerSocialName) {
        return ofNullable(uidToTestProviderToken.get(UID));
    }

    @Override
    public Optional<String> getUserAgent(HttpServletRequest request) {
        return Optional.of(AuthHelper.STATION_USER_AGENT);
    }

    @Nullable
    @Override
    public String getTvmUserTicket() {
        return USER_TICKET;
    }

    @Override
    public boolean hasAuthInfo(HttpServletRequest request) {
        return Strings.isNotEmpty(request.getHeader("Authorization")) ||
                Strings.isNotEmpty(getSessionIdHeader(request)) ||
                Strings.isNotEmpty(request.getHeader(TvmHeaders.USER_TICKET_HEADER));
    }

    @Override
    public String getUserIp(HttpServletRequest request) {
        return IP;
    }

    @Override
    public boolean userHasPlus() {
        return false;
    }

    @Nullable
    private String getSessionIdHeader(HttpServletRequest request) {
        if (request.getCookies() == null) {
            return null;
        }
        return Arrays.stream(request.getCookies())
                .filter(it -> "Session_id".equals(it.getName()))
                .map(Cookie::getValue)
                .findFirst()
                .orElse(null);
    }
}
