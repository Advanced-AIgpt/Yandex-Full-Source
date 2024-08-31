package ru.yandex.alice.paskills.my_alice.blackbox;

import java.util.Optional;

import org.springframework.web.util.UriComponents;

import ru.yandex.alice.paskills.my_alice.apphost.request_init.Request;

public interface Blackbox {
    Optional<UriComponents> buildSessionIdRequest(Request request);

    SessionId.Response parseSessionIdJsonResponse(String json);
}
