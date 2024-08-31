package ru.yandex.alice.paskills.common.apphost.spring;

import java.lang.reflect.Method;
import java.util.List;
import java.util.function.BiConsumer;

import org.springframework.util.ReflectionUtils;

import ru.yandex.web.apphost.api.request.RequestContext;

public record ApphostHandlerSpecification(
        String path,
        Method method,
        List<IArgumentGetter<?>> argumentGetters,
        List<IResultKeySetter> resultSetters
) implements BiConsumer<Object, RequestContext> {
    @Override
    public void accept(Object bean, RequestContext requestContext) {
        Object[] args = new Object[argumentGetters().size()];
        int i = 0;
        for (IArgumentGetter<?> getter : argumentGetters()) {
            // apphost provides exception response if required key is missing
            args[i++] = getter.get(requestContext);
        }

        Object result = ReflectionUtils.invokeMethod(method, bean, args);
        if (result != null) {
            resultSetters.forEach(setter -> setter.set(requestContext, result));
        }
    }
}
