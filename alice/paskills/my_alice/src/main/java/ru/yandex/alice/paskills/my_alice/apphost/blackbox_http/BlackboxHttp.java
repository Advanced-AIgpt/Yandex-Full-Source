package ru.yandex.alice.paskills.my_alice.apphost.blackbox_http;

import java.util.Optional;

import NAppHostHttp.Http;

import ru.yandex.alice.paskills.my_alice.apphost.request_init.Request;
import ru.yandex.alice.paskills.my_alice.blackbox.SessionId;
import ru.yandex.web.apphost.api.request.RequestContext;
import ru.yandex.web.apphost.api.request.RequestItem;

public interface BlackboxHttp {
    String SESSION_ID_ITEM_TYPE = "blackbox_http_response";

    Optional<Http.THttpRequest> buildSessionIdRequest(Request request);

    SessionId.Response getSessionIdResponse(RequestContext context);

    SessionId.Response getSessionIdResponse(RequestItem requestItem);
}
