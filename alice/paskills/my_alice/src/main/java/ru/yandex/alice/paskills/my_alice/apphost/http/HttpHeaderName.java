package ru.yandex.alice.paskills.my_alice.apphost.http;

public class HttpHeaderName {

    public static final String USER_TICKET = "X-Ya-User-Ticket";
    public static final String REQUEST_ID = "X-Request-Id";

    private HttpHeaderName() {
        throw new UnsupportedOperationException();
    }

}
