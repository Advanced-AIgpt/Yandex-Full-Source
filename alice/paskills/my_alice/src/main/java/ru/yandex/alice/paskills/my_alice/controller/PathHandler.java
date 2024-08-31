package ru.yandex.alice.paskills.my_alice.controller;

import ru.yandex.web.apphost.api.request.RequestContext;

public interface PathHandler {
    String getPath();

    void handle(RequestContext ctx);
}
