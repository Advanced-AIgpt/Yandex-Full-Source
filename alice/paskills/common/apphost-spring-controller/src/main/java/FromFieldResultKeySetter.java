package ru.yandex.alice.paskills.common.apphost.spring;

import java.lang.reflect.Field;

import org.springframework.util.ReflectionUtils;

import ru.yandex.web.apphost.api.request.ApphostResponseBuilder;

public class FromFieldResultKeySetter implements IResultKeySetter {
    private final Field field;
    private final IResultKeySetter delegate;

    public FromFieldResultKeySetter(Field field, IResultKeySetter delegate) {
        this.field = field;
        this.delegate = delegate;
    }

    @Override
    public void set(ApphostResponseBuilder context, Object result) {
        Object fieldValue = ReflectionUtils.getField(field, result);
        if (fieldValue != null) {
            delegate.set(context, fieldValue);
        }
    }
}
