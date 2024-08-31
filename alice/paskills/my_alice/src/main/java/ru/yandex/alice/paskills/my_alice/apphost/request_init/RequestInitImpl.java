package ru.yandex.alice.paskills.my_alice.apphost.request_init;

import java.util.Optional;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskills.my_alice.blackbox.SessionId;
import ru.yandex.web.apphost.api.request.RequestContext;
import ru.yandex.web.apphost.api.request.RequestItem;

@Component
public class RequestInitImpl implements RequestInit {
    private static final Logger logger = LogManager.getLogger();

    public Optional<Request> getRequest(RequestContext ctx) {
        return ctx.getSingleRequestItemO(Request.ITEM_TYPE)
                .flatMap(this::getRequest);
    }

    public Optional<Request> getRequest(RequestItem item) {
        return Optional.ofNullable(item.getJsonData(Request.Raw.class))
                .map(Request::fromRaw);
    }

    public SessionId.Response getSessionId(RequestContext ctx) {
        return ctx.getSingleRequestItemO(SESSION_ID_ITEM_TYPE)
                .map(this::getSessionId)
                .orElse(SessionId.Response.EMPTY);
    }

    public SessionId.Response getSessionId(RequestItem item) {
        SessionId.RawResponse raw = null;
        try {
            raw = item.getJsonData(SessionId.RawResponse.class);
        } catch (Exception e) {
            logger.error("Failed to parse BB response (from request_init)", e);
        }

        if (raw == null) {
            return SessionId.Response.EMPTY;
        }

        return SessionId.Response.fromRaw(raw);
    }
}
