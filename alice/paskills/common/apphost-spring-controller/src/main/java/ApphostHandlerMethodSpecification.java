package ru.yandex.alice.paskills.common.apphost.spring;

import java.lang.reflect.Method;
import java.util.List;

public record ApphostHandlerMethodSpecification(
        Method method,
        List<IArgumentGetter<?>> argumentGetters,
        List<IResultKeySetter> resultSetters
) {
    public ApphostHandlerSpecification handler(String path) {
        return new ApphostHandlerSpecification(path, method, argumentGetters, resultSetters);
    }
}
