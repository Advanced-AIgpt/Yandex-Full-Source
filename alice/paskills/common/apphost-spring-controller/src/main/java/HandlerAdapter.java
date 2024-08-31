package ru.yandex.alice.paskills.common.apphost.spring;

import javax.annotation.Nonnull;

import ru.yandex.web.apphost.api.AppHostPathHandler;
import ru.yandex.web.apphost.api.request.RequestContext;

public class HandlerAdapter implements AppHostPathHandler {

    private final Object bean;
    private final ApphostHandlerSpecification handler;

    public HandlerAdapter(Object bean,
                          ApphostHandlerSpecification handler
    ) {
        this.bean = bean;
        this.handler = handler;
    }

    public ApphostHandlerSpecification getHandlerSpecification() {
        return handler;
    }

    @Override
    @Nonnull
    public String getPath() {
        return handler.path();
    }

    @Override
    public void accept(RequestContext requestContext) {
        handler.accept(bean, requestContext);
    }
}
