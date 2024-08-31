package ru.yandex.alice.paskills.my_alice.apphost.http;

import java.util.Optional;

import NAppHostHttp.Http;
import com.google.protobuf.ByteString;
import org.springframework.http.HttpHeaders;
import org.springframework.lang.Nullable;
import org.springframework.web.util.UriComponents;

import ru.yandex.web.apphost.api.request.RequestItem;

public interface ApphostHttp {
    default Http.THttpRequest buildRequest(UriComponents uri) {
        return buildRequest(Method.GET, uri, new HttpHeaders(), null);
    }

    default Http.THttpRequest buildRequest(Method method, UriComponents uri) {
        return buildRequest(method, uri, new HttpHeaders(), null);
    }

    default Http.THttpRequest buildRequest(Method method, UriComponents uri, HttpHeaders headers) {
        return buildRequest(method, uri, headers, null);
    }

    Http.THttpRequest buildRequest(Method method, UriComponents uri, HttpHeaders headers,
                                   @Nullable ByteString body);

    Optional<Response> parseResponse(RequestItem requestItem);
}
