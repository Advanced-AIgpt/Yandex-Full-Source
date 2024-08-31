package ru.yandex.alice.paskills.common.apphost.spring;

import javax.annotation.Nullable;

import ru.yandex.web.apphost.api.request.RequestContext;

public interface IArgumentGetter<T> {

    @Nullable
    T get(RequestContext context);

}
