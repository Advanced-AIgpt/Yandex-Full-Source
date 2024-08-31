package ru.yandex.alice.paskills.common.apphost.spring;

import ru.yandex.web.apphost.api.request.ApphostResponseBuilder;

public abstract class AbstractTypedResultSetter<T> implements IResultKeySetter {

    @SuppressWarnings("unchecked")
    @Override
    public void set(ApphostResponseBuilder context, Object result) {
        doSet(context, (T) result);
    }

    protected abstract void doSet(ApphostResponseBuilder context, T result);
}
