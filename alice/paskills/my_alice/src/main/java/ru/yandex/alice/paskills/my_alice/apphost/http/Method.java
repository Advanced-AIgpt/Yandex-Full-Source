package ru.yandex.alice.paskills.my_alice.apphost.http;

import NAppHostHttp.Http;

public enum Method {
    GET(Http.THttpRequest.EMethod.Get),
    POST(Http.THttpRequest.EMethod.Post),
    PUT(Http.THttpRequest.EMethod.Put),
    DELETE(Http.THttpRequest.EMethod.Delete),
    HEAD(Http.THttpRequest.EMethod.Head),
    OPTIONS(Http.THttpRequest.EMethod.Options),
    PATCH(Http.THttpRequest.EMethod.Patch);

    protected final Http.THttpRequest.EMethod value;

    Method(final Http.THttpRequest.EMethod value) {
        this.value = value;
    }
}
