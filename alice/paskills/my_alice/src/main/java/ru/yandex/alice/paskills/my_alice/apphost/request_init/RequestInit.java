package ru.yandex.alice.paskills.my_alice.apphost.request_init;

import java.util.Optional;

import ru.yandex.alice.paskills.my_alice.blackbox.SessionId;
import ru.yandex.web.apphost.api.request.RequestContext;
import ru.yandex.web.apphost.api.request.RequestItem;

public interface RequestInit {
    String SESSION_ID_ITEM_TYPE = "blackbox";

    Optional<Request> getRequest(RequestContext ctx);

    Optional<Request> getRequest(RequestItem item);

    SessionId.Response getSessionId(RequestContext ctx);

    SessionId.Response getSessionId(RequestItem item);
}
