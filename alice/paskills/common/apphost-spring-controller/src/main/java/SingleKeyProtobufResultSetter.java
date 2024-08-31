package ru.yandex.alice.paskills.common.apphost.spring;

import com.google.protobuf.Message;

import ru.yandex.web.apphost.api.request.ApphostResponseBuilder;

public class SingleKeyProtobufResultSetter extends AbstractTypedResultSetter<Message> {

    private final String key;

    public SingleKeyProtobufResultSetter(String key) {
        this.key = key;
    }

    @Override
    protected void doSet(ApphostResponseBuilder context, Message result) {
        context.addProtobufItem(key, result);
    }
}
