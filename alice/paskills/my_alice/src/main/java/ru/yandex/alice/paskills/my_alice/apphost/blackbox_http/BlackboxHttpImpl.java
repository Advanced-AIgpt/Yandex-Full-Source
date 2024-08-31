package ru.yandex.alice.paskills.my_alice.apphost.blackbox_http;

import java.util.Optional;

import NAppHostHttp.Http;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;
import org.springframework.util.MimeType;

import ru.yandex.alice.paskills.my_alice.apphost.http.ApphostHttp;
import ru.yandex.alice.paskills.my_alice.apphost.request_init.Request;
import ru.yandex.alice.paskills.my_alice.blackbox.Blackbox;
import ru.yandex.alice.paskills.my_alice.blackbox.SessionId;
import ru.yandex.web.apphost.api.request.RequestContext;
import ru.yandex.web.apphost.api.request.RequestItem;

@Component
class BlackboxHttpImpl implements BlackboxHttp {
    private static final Logger logger = LogManager.getLogger();

    private final Blackbox blackbox;
    private final ApphostHttp apphostHttp;

    BlackboxHttpImpl(Blackbox blackbox, ApphostHttp apphostHttp) {
        this.blackbox = blackbox;
        this.apphostHttp = apphostHttp;
    }

    public Optional<Http.THttpRequest> buildSessionIdRequest(Request request) {
        return blackbox.buildSessionIdRequest(request)
                .map(apphostHttp::buildRequest);
    }

    public SessionId.Response getSessionIdResponse(RequestContext context) {
        var item = context.getSingleRequestItemO(SESSION_ID_ITEM_TYPE);
        if (item.isEmpty()) {
            logger.info("No {}", SESSION_ID_ITEM_TYPE);
            return SessionId.Response.EMPTY;
        }

        return getSessionIdResponse(item.get());
    }

    public SessionId.Response getSessionIdResponse(RequestItem requestItem) {
        var responseO = apphostHttp.parseResponse(requestItem);
        if (responseO.isEmpty()) {
            logger.info("Not HTTP_RESPONSE");
            return SessionId.Response.EMPTY;
        }

        if (responseO.get().getCode() != 200) {
            logger.info("Code != 200:  {}", () -> responseO.get().getCode().toString());
            return SessionId.Response.EMPTY;
        }

        var contentType = responseO.get().getHeaders().getContentType();
        if (contentType != null && !contentType.isCompatibleWith(MimeType.valueOf("application/json"))) {
            logger.info("Bad content-type: {}", contentType.toString());
            return SessionId.Response.EMPTY;
        }

        if (responseO.get().getContent() == null) {
            logger.info("getContent() == null");
            return SessionId.Response.EMPTY;
        }

        return blackbox.parseSessionIdJsonResponse(responseO.get().getContent());
    }
}
