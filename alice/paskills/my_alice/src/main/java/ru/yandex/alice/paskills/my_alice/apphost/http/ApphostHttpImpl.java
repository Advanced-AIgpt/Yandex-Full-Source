package ru.yandex.alice.paskills.my_alice.apphost.http;

import java.util.Objects;
import java.util.Optional;

import NAppHostHttp.Http;
import com.google.protobuf.ByteString;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.HttpHeaders;
import org.springframework.lang.Nullable;
import org.springframework.stereotype.Component;
import org.springframework.web.util.UriComponents;

import ru.yandex.web.apphost.api.request.RequestItem;

@Component
public class ApphostHttpImpl implements ApphostHttp {

    private static final Logger logger = LogManager.getLogger();

    public Http.THttpRequest buildRequest(Method method, UriComponents uri, HttpHeaders headers,
                                          @Nullable ByteString body) {
        var requestBuilder = Http.THttpRequest.newBuilder();

        requestBuilder.setMethod(method.value);

        String path = Objects.requireNonNullElse(uri.getPath(), "");
        String query = Objects.requireNonNullElse(uri.getQuery(), "");
        requestBuilder.setPath(path + (query.isBlank() ? "" : "?" + query));

        for (var header : headers.entrySet()) {
            for (var value : header.getValue()) {
                requestBuilder.addHeaders(
                        Http.THeader.newBuilder()
                                .setName(header.getKey())
                                .setValue(value)
                                .build()
                );
            }
        }

        if (body != null) {
            requestBuilder.setContent(body);
        }

        return requestBuilder.build();
    }

    public Optional<Response> parseResponse(RequestItem requestItem) {
        var protoResponse = requestItem.getProtobufData(Http.THttpResponse.getDefaultInstance());
        if (protoResponse.equals(Http.THttpResponse.getDefaultInstance())) {
            logger.warn("Empty response, maybe wrong Request item format?");
            return Optional.empty();
        }
        return Optional.of(Response.fromProto(protoResponse));
    }
}
