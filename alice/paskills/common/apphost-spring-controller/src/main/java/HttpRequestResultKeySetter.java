package ru.yandex.alice.paskills.common.apphost.spring;

import com.fasterxml.jackson.databind.ObjectMapper;

import ru.yandex.alice.paskills.common.apphost.http.HttpRequest;
import ru.yandex.alice.paskills.common.apphost.http.HttpRequestConverter;
import ru.yandex.web.apphost.api.request.ApphostResponseBuilder;

public class HttpRequestResultKeySetter implements IResultKeySetter {
    private final String key;
    private final HttpRequestConverter converter;

    public HttpRequestResultKeySetter(String key, ObjectMapper objectMapper) {
        this.key = key;
        this.converter = new HttpRequestConverter(objectMapper);
    }

    @Override
    public void set(ApphostResponseBuilder context, Object result) {
        context.addProtobufItem(key, converter.httpRequestToProto((HttpRequest<?>) result));
    }
}
