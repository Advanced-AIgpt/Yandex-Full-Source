package ru.yandex.alice.paskills.my_alice.controller;

import org.springframework.stereotype.Controller;

import ru.yandex.alice.paskills.my_alice.apphost.blackbox_http.BlackboxHttp;
import ru.yandex.alice.paskills.my_alice.apphost.request_init.RequestInit;
import ru.yandex.web.apphost.api.request.RequestContext;

@Controller
public class BlackboxSetupHandler implements PathHandler {
    private final BlackboxHttp blackbox;
    private final RequestInit requestInit;

    BlackboxSetupHandler(BlackboxHttp blackbox, RequestInit requestInit) {
        this.blackbox = blackbox;
        this.requestInit = requestInit;
    }

    @Override
    public String getPath() {
        return "/_setup/blackbox";
    }

    @Override
    public void handle(RequestContext ctx) {
        requestInit.getRequest(ctx)
                .flatMap(blackbox::buildSessionIdRequest)
                .ifPresent(tHttpRequest -> ctx.addProtobufItem("blackbox_http_request", tHttpRequest));
    }
}
