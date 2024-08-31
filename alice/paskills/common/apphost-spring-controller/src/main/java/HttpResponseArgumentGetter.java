package ru.yandex.alice.paskills.common.apphost.spring;

import javax.annotation.Nullable;

import NAppHostHttp.Http;
import com.fasterxml.jackson.databind.ObjectMapper;

import ru.yandex.alice.paskills.common.apphost.http.HttpRequestConverter;
import ru.yandex.alice.paskills.common.apphost.http.HttpResponse;
import ru.yandex.web.apphost.api.request.RequestContext;

public class HttpResponseArgumentGetter implements IArgumentGetter<HttpResponse<?>> {
    private static final Http.THttpResponse DEFAULT_INSTANCE = Http.THttpResponse.getDefaultInstance();

    private final String key;
    private final boolean nullable;
    private final Class<?> bodyClazz;
    private final HttpRequestConverter converter;


    public HttpResponseArgumentGetter(String key, boolean nullable, Class<?> bodyClazz, ObjectMapper jsonMapper) {
        this.key = key;
        this.nullable = nullable;
        this.bodyClazz = bodyClazz;
        this.converter = new HttpRequestConverter(jsonMapper);
    }

    @Nullable
    @Override
    public HttpResponse<?> get(RequestContext requestContext) {
        if (nullable) {
            return requestContext.getSingleRequestItemO(key)
                    .map(item -> converter.protoToHttpResponse(item.getProtobufData(DEFAULT_INSTANCE), bodyClazz))
                    .orElse(null);
        } else {
            Http.THttpResponse httpResponseProto =
                    requestContext.getSingleRequestItem(key).getProtobufData(DEFAULT_INSTANCE);
            return converter.protoToHttpResponse(httpResponseProto, bodyClazz);
        }
    }
}
