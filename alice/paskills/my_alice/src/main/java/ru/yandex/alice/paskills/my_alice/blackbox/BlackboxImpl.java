package ru.yandex.alice.paskills.my_alice.blackbox;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

import com.google.gson.Gson;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.util.Strings;
import org.springframework.stereotype.Component;
import org.springframework.web.util.UriComponents;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.paskills.my_alice.apphost.request_init.Request;

@Component
class BlackboxImpl implements Blackbox {

    private static final Logger logger = LogManager.getLogger();
    private final Gson gson;

    BlackboxImpl(Gson gson) {
        this.gson = gson;
    }

    @Override
    public Optional<UriComponents> buildSessionIdRequest(Request request) {
        String sessionId = request.getCookiesParsed().get("Session_id");
        if (sessionId == null || sessionId.isBlank() || sessionId.startsWith("noauth:")) {
            logger.warn("cookie Session_id is empty");
            return Optional.empty();
        }

        if (request.getIp() == null || request.getIp().isBlank()) {
            logger.warn("user IP is empty");
            return Optional.empty();
        }

        String sslSessionId = request.getCookiesParsed().get("sessionid2");

        List<String> dbfields = new ArrayList<>(List.of(
                SessionId.DbFields.SUID_MAIL,
                SessionId.DbFields.SUID_BETATEST,
                SessionId.DbFields.SUID_YASTAFF,
                SessionId.DbFields.USERINFO_FIRSTNAME,
                SessionId.DbFields.USERINFO_LASTNAME
        ));

        List<String> attributes = List.of(
                SessionId.Attributes.NORMALIZED_LOGIN,
                SessionId.Attributes.HAVE_PLUS,
                SessionId.Attributes.KINOPOISK_OTT_SUBSCRIPTION_NAME
        );

        List<String> phoneAttributes = List.of(
                SessionId.PhoneAttributes.E164_NUMBER,
                SessionId.PhoneAttributes.IS_DEFAULT,
                SessionId.PhoneAttributes.IS_SECURED
        );

        List<String> aliases = List.of(
                SessionId.Aliases.YANDEXOID
        );

        // https://docs.yandex-team.ru/blackbox/methods/sessionid
        UriComponentsBuilder uriComponents = UriComponentsBuilder.newInstance();
        uriComponents.queryParam("method", "sessionid");
        uriComponents.queryParam("sessionid", sessionId);
        uriComponents.queryParam("userip", request.getIp());
        uriComponents.queryParam("host", request.getHostname());
        if (sslSessionId != null && !sslSessionId.isBlank()) {
            uriComponents.queryParam("sslsessionid", sslSessionId);
        }
        uriComponents.queryParam("dbfields", Strings.join(dbfields, ','));
        uriComponents.queryParam("attributes", Strings.join(attributes, ','));
        uriComponents.queryParam("emails", "getdefault");
        uriComponents.queryParam("getphones", "bound");
        uriComponents.queryParam("phone_attributes", Strings.join(phoneAttributes, ','));
        uriComponents.queryParam("regname", "yes");
        uriComponents.queryParam("aliases", Strings.join(aliases, ','));
        uriComponents.queryParam("format", "json");
        uriComponents.queryParam("get_user_ticket", "yes");

        return Optional.of(uriComponents.build());
    }

    @Override
    public SessionId.Response parseSessionIdJsonResponse(String json) {
        SessionId.RawResponse raw;
        try {
            raw = gson.fromJson(json, SessionId.RawResponse.class);
        } catch (Exception e) {
            return new SessionId.Response(
                    SessionId.Response.Status.INVALID,
                    e.getMessage(),
                    null,
                    null,
                    null
            );
        }

        if (raw == null) {
            return new SessionId.Response(
                    SessionId.Response.Status.INVALID,
                    "Response is null",
                    null,
                    null,
                    null
            );
        }

        return SessionId.Response.fromRaw(raw);
    }

}

