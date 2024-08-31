package ru.yandex.quasar.billing.services;

import java.util.Locale;
import java.util.Map;
import java.util.Optional;

import javax.annotation.Nullable;
import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletRequest;

import com.fasterxml.jackson.annotation.JsonAutoDetect;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.ThreadContext;
import org.apache.logging.log4j.util.Strings;
import org.json.JSONException;
import org.json.JSONObject;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.ResponseEntity;
import org.springframework.stereotype.Component;
import org.springframework.util.StringUtils;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.passport.tvmauth.CheckedServiceTicket;
import ru.yandex.passport.tvmauth.CheckedUserTicket;
import ru.yandex.passport.tvmauth.TicketStatus;
import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.SocialAPIClientConfig;
import ru.yandex.quasar.billing.exception.ForbiddenException;
import ru.yandex.quasar.billing.exception.UnauthorizedException;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;
import ru.yandex.quasar.billing.util.CSRFHelper;

import static com.google.common.net.HttpHeaders.X_FORWARDED_FOR;
import static ru.yandex.quasar.billing.filter.HeaderModifierFilter.HEADER_X_CSRF_TOKEN;

@Component
public class AuthorizationServiceImpl implements AuthorizationService {


    private static final String PLUS_BLACKBOX_ATTRIBUTE = "1015";
    private static final Logger log = LogManager.getLogger();

    private final SocialAPIClientConfig config;

    private final RestTemplate restTemplate;

    private final AuthorizationContext authorizationContext;

    private final CSRFHelper csrfHelper;

    private final UnistatService unistatService;
    private final TvmClient tvmClient;
    private final UriComponentsBuilder oauthRequestBuilder;
    private final UriComponentsBuilder sessionRequestBuilder;
    private final UriComponentsBuilder checkPlusRequestBuilder;
    private final UriComponentsBuilder newestTokenRequestBuilder;
    private final Map<String, String> userIpStub;

    @Autowired
    public AuthorizationServiceImpl(
            RestTemplateBuilder restTemplateBuilder,
            BillingConfig billingConfig,
            AuthorizationContext authorizationContext,
            @Value("${CSRF_TOKEN_KEY}") String csrfKey,
            UnistatService unistatService,
            TvmClient tvmClient) {
        this.restTemplate = restTemplateBuilder.build();
        this.config = billingConfig.getSocialAPIClientConfig();
        this.authorizationContext = authorizationContext;
        this.csrfHelper = new CSRFHelper(csrfKey.getBytes());
        this.unistatService = unistatService;
        this.tvmClient = tvmClient;
        this.userIpStub = billingConfig.getUserIpStub();
        this.oauthRequestBuilder = UriComponentsBuilder.fromUriString(config.getBlackboxBaseUrl())
                .queryParam("method", "oauth")
                .queryParam("format", "json")
                .queryParam("get_user_ticket", "yes");
        this.sessionRequestBuilder = UriComponentsBuilder.fromUriString(config.getBlackboxBaseUrl())
                .queryParam("method", "sessionid")
                .queryParam("format", "json")
                .queryParam("get_user_ticket", "yes");
        this.checkPlusRequestBuilder = UriComponentsBuilder.fromUriString(config.getBlackboxBaseUrl())
                .queryParam("method", "userinfo")
                .queryParam("format", "json")
                .queryParam("attributes", PLUS_BLACKBOX_ATTRIBUTE);
        this.newestTokenRequestBuilder =
                UriComponentsBuilder.fromUriString(config.getSocialApiBaseUrl() + "/token/newest");
    }

    @Nullable
    private String getCookie(HttpServletRequest request, String cookieName) {
        Cookie[] cookies = request.getCookies();
        if (cookies != null) {
            for (Cookie cookie : cookies) {
                if (cookieName.equals(cookie.getName())) {
                    return Strings.trimToNull(cookie.getValue());
                }
            }
        }

        return null;

    }

    @Nullable
    private String getYandexUID(HttpServletRequest request) {
        return Optional.ofNullable(getCookie(request, "yandexuid"))
                // store can pass yandexuid cookie in header with the same name
                .orElse(getHeader(request, "yandexuid"));
    }

    @Nullable
    private String getHeader(HttpServletRequest request, String header) {
        String yandexuid = request.getHeader(header);
        return Strings.trimToNull(yandexuid);
    }

    RestTemplate restTemplate() {
        return this.restTemplate;
    }

    @Override
    public String getSecretUid(HttpServletRequest request) throws UnauthorizedException {
        return getSecretUid(request, false).get();
    }

    @Override
    public Optional<String> getSecretUid(HttpServletRequest request, boolean allowAnonymous)
            throws UnauthorizedException {
        String oauthToken = getOAuthToken(request);
        String sessionId = getSessionId(request);
        String yandexUID = getYandexUID(request);
        String tvmUserTicket = getTvmUserTicket(request);
        String tvmServiceTicketHeader = getTvmServiceTicket(request);
        String userIp = getUserIp(request);
        String userAgent = getUserAgent(request).orElse(null);
        String forwardedFor = getForwardedFor(request);

        if (tvmServiceTicketHeader != null) {
            CheckedServiceTicket serviceTicket = tvmClient.checkServiceTicket(tvmServiceTicketHeader);
            var status = serviceTicket.getStatus();
            if (status != TicketStatus.OK) {
                if (status == TicketStatus.EXPIRED ||
                        status == TicketStatus.MALFORMED ||
                        status == TicketStatus.INVALID_DST) {
                    throw new ForbiddenException("Bad service ticket status: " + status);
                }
                throw new UnauthorizedException("Bad service ticket status: " + status);
            }
        }

        if (authorizationContext.getCurrentUid() == null) {
            @Nullable
            String hostHeader = request.getHeader("Host");
            UserCredential credentials = getUidAndTicket(oauthToken, sessionId, userIp, tvmUserTicket,
                    tvmServiceTicketHeader, hostHeader, allowAnonymous);

            if (sessionId != null) {
                String method = request.getMethod();
                if ((HttpMethod.POST.matches(method) ||
                        HttpMethod.PUT.matches(method) ||
                        HttpMethod.DELETE.matches(method) ||
                        HttpMethod.PATCH.matches(method)) && !csrfHelper.isValid(
                        getCSRFToken(request),
                        yandexUID,
                        credentials.getUid()
                )) {
                    log.warn(
                            "Bad CSRF token of <{}> in {} {} {} from {}",
                            getCSRFToken(request),
                            request.getMethod(),
                            request.getHeader("host"),
                            request.getRequestURI(),
                            request.getRemoteHost()
                    );
                    unistatService.incrementStatValue("quasar_billing_incorrect_x_csrf_token_dmmm");
                    throw new ForbiddenException("Invalid x-csrf-token");
                }
            }

            authorizationContext.setCurrentUid(credentials.getUid());
            authorizationContext.setCurrentUserTicket(credentials.getUserTicket());
            authorizationContext.setUserIp(userIpStub.getOrDefault(credentials.uid, userIp));
            authorizationContext.setSessionId(sessionId);
            authorizationContext.setOauthToken(oauthToken);
            authorizationContext.setUserAgent(userAgent);
            authorizationContext.setYandexUid(yandexUID);
            authorizationContext.setForwardedFor(userIpStub.getOrDefault(credentials.uid, forwardedFor));
            authorizationContext.setHost(hostHeader != null ? hostHeader : "quasar.yandex.ru");
            ThreadContext.put("uid", credentials.getUid());
        }

        return Optional.ofNullable(authorizationContext.getCurrentUid());
    }

    @Override
    public boolean hasAuthInfo(HttpServletRequest request) {
        return Strings.isNotEmpty(getOAuthToken(request)) ||
                Strings.isNotEmpty(getSessionId(request)) ||
                Strings.isNotEmpty(getTvmUserTicket(request));
    }

    private String getForwardedFor(HttpServletRequest request) {
        return Strings.trimToNull(request.getHeader(X_FORWARDED_FOR));
    }

    @Nullable
    private String getTvmUserTicket(HttpServletRequest request) {
        String ticket = request.getHeader(TvmHeaders.USER_TICKET_HEADER);
        return Strings.trimToNull(ticket);
    }

    @Nullable
    private String getTvmServiceTicket(HttpServletRequest request) {
        String ticket = request.getHeader(TvmHeaders.SERVICE_TICKET_HEADER);
        return Strings.trimToNull(ticket);
    }

    @Override
    public String getUserIp(HttpServletRequest request) {
        String header = request.getHeader(X_FORWARDED_FOR);
        if (StringUtils.hasText(header)) {
            header = header.trim();
            int commaIndex = header.indexOf(',');
            if (commaIndex == -1) {
                return header.trim();
            } else if (commaIndex > 0) {
                return header.substring(0, commaIndex).trim();
            }
        }

        return request.getRemoteAddr();
    }

    @Override
    public Optional<String> getUserAgent(HttpServletRequest request) {
        return Optional.ofNullable(request.getHeader(HttpHeaders.USER_AGENT));
    }

    UserCredential getUidAndTicket(@Nullable String oauthToken, @Nullable String sessionId, String userIp,
                                   @Nullable String tvmUserTicket, @Nullable String tvmServiceTicket,
                                   @Nullable String hostHeader, boolean allowAnonymous) {

        if (tvmUserTicket != null) {
            if (tvmServiceTicket == null) {
                throw new UnauthorizedException("Service ticket is required for TVM authorization");
            }


            CheckedUserTicket userTicket = tvmClient.checkUserTicket(tvmUserTicket);
            if (userTicket.getStatus() != TicketStatus.OK) {
                if (userTicket.getStatus() == TicketStatus.EXPIRED ||
                        userTicket.getStatus() == TicketStatus.MALFORMED) {
                    throw new ForbiddenException("Bad user ticket status: " + userTicket.getStatus());
                }
                throw new UnauthorizedException("Bad user ticket status: " + userTicket.getStatus());
            }

            return new UserCredential(String.valueOf(userTicket.getDefaultUid()), tvmUserTicket);
        } else {
            ResponseEntity<String> xtokenResponse;
            String blackboxServiceTicket = tvmClient.getServiceTicketFor("blackbox");

            HttpHeaders headers = new HttpHeaders();
            headers.add(TvmHeaders.SERVICE_TICKET_HEADER, blackboxServiceTicket);
            if (oauthToken != null) {
                headers.add("Authorization", "OAuth " + oauthToken);

                xtokenResponse = restTemplate.exchange(
                        oauthRequestBuilder.cloneBuilder()
                                .queryParam("userip", userIp)
                                .build().toUri(),
                        HttpMethod.GET,
                        new HttpEntity<>(headers),
                        String.class
                );
                return parseBlackboxResponse(xtokenResponse);
            } else if (sessionId != null) {
                xtokenResponse = restTemplate.exchange(
                        sessionRequestBuilder.cloneBuilder()
                                .queryParam("host", hostHeader != null ? hostHeader : "yandex.ru")
                                .queryParam("userip", userIp)
                                .queryParam("sessionid", sessionId)
                                .build().toUri(),
                        HttpMethod.GET,
                        new HttpEntity<>(headers),
                        String.class
                );
                return parseBlackboxResponse(xtokenResponse);
            } else if (!allowAnonymous) {
                throw new UnauthorizedException("Either OAuth token or sessionId must be present");
            } else {
                return UserCredential.EMPTY;
            }
        }
    }

    private UserCredential parseBlackboxResponse(ResponseEntity<String> xtokenResponse) {
        JSONObject jsonObject = null;

        try {
            jsonObject = new JSONObject(xtokenResponse.getBody());
            String currentUid = jsonObject.getJSONObject("uid").get("value").toString();
            String currentUserTicket = jsonObject.optString("user_ticket");
            return new UserCredential(currentUid, currentUserTicket);
        } catch (JSONException e) {
            String errorMessage = jsonObject != null && jsonObject.has("error") ? jsonObject.getString("error")
                    : "Unhandled blackbox exception: " + xtokenResponse.getBody();
            throw new UnauthorizedException(errorMessage, e);
        }

    }

    @Override
    public Optional<String> getProviderTokenByUid(String uid, String providerSocialName) {
        if (authorizationContext.getProviderTokens().containsKey(providerSocialName)) {
            return Optional.of(authorizationContext.getProviderTokens().get(providerSocialName));
        } else {
            Optional<String> session = getProviderTokenInfoByUid(uid, providerSocialName)
                    .map(TokenInfo::getValue);
            // cache the token
            session.ifPresent(it -> authorizationContext.getProviderTokens().put(providerSocialName, it));
            return session;
        }
    }

    @Override
    public boolean userHasPlus() {
        if (authorizationContext.getCurrentUid() == null) {
            return false;
        }

        try {
            String response = restTemplate.getForObject(
                    checkPlusRequestBuilder.cloneBuilder()
                            .queryParam("uid", authorizationContext.getCurrentUid())
                            .queryParam("userip", authorizationContext.getUserIp())
                            .build().toUri(),
                    String.class
            );

            JSONObject body = new JSONObject(response);
            JSONObject user;
            JSONObject attributes;
            return body.has("users") &&
                    (user = body.getJSONArray("users").getJSONObject(0)).has("attributes") &&
                    (attributes = user.getJSONObject("attributes")).has(PLUS_BLACKBOX_ATTRIBUTE) &&
                    "1".equals(attributes.optString(PLUS_BLACKBOX_ATTRIBUTE));
        } catch (Exception e) {
            return false;
        }
    }

    @Nullable
    @Override
    public String getTvmUserTicket() {
        return authorizationContext.getCurrentUserTicket();
    }

    private Optional<TokenInfo> getProviderTokenInfoByUid(String uid, String providerSocialName) {
        if (providerSocialName.equals("yandex-kinopoisk")) {

            // for yandex return oauth token as temporary solution
            return Optional.ofNullable(uid)
                    .map(it -> new TokenInfo(null, null, null, "dummy"));
        } else {
            try {
                TokenWrapper response = restTemplate.getForObject(
                        newestTokenRequestBuilder.cloneBuilder()
                                .queryParam("uid", uid)
                                .queryParam("application_name", providerSocialName)
                                .build().toUri(),
                        TokenWrapper.class
                );

                return Optional.ofNullable(response).map(TokenWrapper::getToken);
            } catch (HttpClientErrorException e) {
                return Optional.empty();
            }
        }
    }

    private String getCSRFToken(HttpServletRequest request) {
        return request.getHeader(HEADER_X_CSRF_TOKEN);
    }

    @Nullable
    private String getOAuthToken(HttpServletRequest request) {
        String authorization = request.getHeader("Authorization");

        return StringUtils.hasText(authorization) && authorization.toLowerCase(Locale.US).startsWith("oauth ")
                ? authorization.substring(6).trim()
                : null;
    }

    @Nullable
    private String getSessionId(HttpServletRequest request) {
        return getCookie(request, "Session_id");
    }

    private static class TokenWrapper {
        @JsonProperty("token")
        private TokenInfo token;

        TokenInfo getToken() {
            return token;
        }
    }

    @JsonAutoDetect(fieldVisibility = JsonAutoDetect.Visibility.ANY)
    @Data
    private static class TokenInfo {
        @JsonProperty("token_id")
        private final String tokenId;
        private final String secret;
        private final String application;
        @JsonProperty("value")
        private final String value;

    }

    @Data
    static class UserCredential {
        static final UserCredential EMPTY = new UserCredential(null, null);
        @Nullable
        private final String uid;
        @Nullable
        private final String userTicket;
    }
}
