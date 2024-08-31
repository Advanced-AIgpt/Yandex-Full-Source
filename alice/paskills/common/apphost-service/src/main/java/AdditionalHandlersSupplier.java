package ru.yandex.alice.paskills.common.apphost.spring;

import java.util.Collection;

import ru.yandex.web.apphost.api.AppHostPathHandler;

@FunctionalInterface
public interface AdditionalHandlersSupplier {
    Collection<? extends AppHostPathHandler> supply();
}
