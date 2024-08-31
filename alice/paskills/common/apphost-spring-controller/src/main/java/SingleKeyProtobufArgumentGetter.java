package ru.yandex.alice.paskills.common.apphost.spring;

import javax.annotation.Nullable;

import com.google.protobuf.GeneratedMessageV3;

import ru.yandex.web.apphost.api.request.RequestContext;

public class SingleKeyProtobufArgumentGetter implements IArgumentGetter<GeneratedMessageV3> {

    private final String key;
    private final boolean nullable;
    private final GeneratedMessageV3 defaultInstance;

    public SingleKeyProtobufArgumentGetter(String key, boolean nullable, Class<? extends GeneratedMessageV3> clazz) {
        this.key = key;
        this.nullable = nullable;
        this.defaultInstance = ProtoDefaultsCache.getDefaultProtoInstance(clazz);
    }

    @Nullable
    @Override
    public GeneratedMessageV3 get(RequestContext requestContext) {
        if (nullable) {
            return requestContext.getSingleRequestItemO(key)
                    .map(item -> item.getProtobufData(defaultInstance))
                    .orElse(null);
        } else {
            return requestContext.getSingleRequestItem(key).getProtobufData(defaultInstance);
        }
    }
}
