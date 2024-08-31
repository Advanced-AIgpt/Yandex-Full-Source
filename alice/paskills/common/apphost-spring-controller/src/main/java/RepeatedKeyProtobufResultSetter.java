package ru.yandex.alice.paskills.common.apphost.spring;

import java.util.List;

import com.google.protobuf.Message;

import ru.yandex.web.apphost.api.request.ApphostResponseBuilder;

public class RepeatedKeyProtobufResultSetter extends AbstractTypedResultSetter<List<Message>> {

    private final String key;

    public RepeatedKeyProtobufResultSetter(String key) {
        this.key = key;
    }

    @Override
    protected void doSet(ApphostResponseBuilder context, List<Message> results) {
        results.forEach(
                result -> context.addProtobufItem(key, result)
        );
    }
}
