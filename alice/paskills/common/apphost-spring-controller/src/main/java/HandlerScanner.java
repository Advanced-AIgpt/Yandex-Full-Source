package ru.yandex.alice.paskills.common.apphost.spring;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Parameter;
import java.lang.reflect.ParameterizedType;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.Objects;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.reflect.TypeToken;
import com.google.protobuf.GeneratedMessageV3;
import com.google.protobuf.Message;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.core.annotation.AnnotatedElementUtils;
import org.springframework.stereotype.Component;
import org.springframework.util.ReflectionUtils;

import ru.yandex.alice.paskills.common.apphost.http.HttpRequest;
import ru.yandex.alice.paskills.common.apphost.http.HttpResponse;
import ru.yandex.web.apphost.api.request.ApphostRequest;
import ru.yandex.web.apphost.api.request.ApphostResponseBuilder;
import ru.yandex.web.apphost.api.request.RequestContext;

import static java.util.Objects.requireNonNull;
import static java.util.stream.Collectors.toList;
import static ru.yandex.alice.paskills.common.apphost.spring.NullabilityUtil.isNullable;

@Component
public class HandlerScanner {
    private static final Logger logger = LogManager.getLogger();

    static final String HTTP_REQUEST_APPHOST_KEY = "http_request";
    static final String HTTP_RESPONSE_APPHOST_KEY = "http_response";

    private final ObjectMapper objectMapper;
    private static final TypeToken MESSAGE_LIST_TYPE = new TypeToken<List<Message>>() {
    };

    public HandlerScanner(ObjectMapper jsonMapper) {
        this.objectMapper = jsonMapper;
    }

    public List<ApphostHandlerSpecification> scanHandler(Class<?> clazz) {
        ApphostHandler classAnnotation = AnnotatedElementUtils.findMergedAnnotation(clazz, ApphostHandler.class);
        ApphostController controllerAnnotation =
                AnnotatedElementUtils.findMergedAnnotation(clazz, ApphostController.class);

        String basePath;

        if (classAnnotation != null && !classAnnotation.value().isEmpty()) {
            basePath = classAnnotation.value();
        } else if (controllerAnnotation != null && !controllerAnnotation.value().isEmpty()) {
            basePath = controllerAnnotation.value();
        } else {
            basePath = "";
        }

        List<ApphostHandlerSpecification> handlerMethods = new ArrayList<>();
        ReflectionUtils.doWithMethods(clazz,
                method -> handlerMethods.add(createHandler(method, basePath)),
                method -> method.isAnnotationPresent(ApphostHandler.class));

        return handlerMethods;
    }

    private ApphostHandlerSpecification createHandler(Method method, String basePath) {
        ApphostHandlerMethodSpecification apphostHandlerMethodSpecification = parseMethodSpecification(method);

        ApphostHandler apphostHandler =
                requireNonNull(AnnotatedElementUtils.findMergedAnnotation(method, ApphostHandler.class));

        String path;
        if (basePath.isEmpty()) {
            path = "/" + apphostHandler.value().replaceFirst("^/", "");
        } else {
            path = "/" + basePath.replaceFirst("^/|/$", "") +
                    "/" + apphostHandler.value().replaceFirst("^/", "");
        }
        if (path.endsWith("/")) {
            logger.warn("Suspicious apphost handler path: {} defained in {} contains trailing \"/\"",
                    path, method.getDeclaringClass().getName());
        }
        return apphostHandlerMethodSpecification.handler(path);

    }

    public ApphostHandlerMethodSpecification parseMethodSpecification(Method method) {
        logger.debug("Processing method {}::{}", () -> method.getDeclaringClass().getName(), method::getName);

        Parameter[] parameters = method.getParameters();

        validateHandlerArguments(method, parameters);

        List<IArgumentGetter<?>> getters = createArgumentGetters(parameters);

        List<IResultKeySetter> setters = createResultSetters(method);

        ReflectionUtils.makeAccessible(method);

        return new ApphostHandlerMethodSpecification(method, getters, setters);
    }

    private void validateHandlerArguments(Method method, Parameter[] parameters) {
        int foundHttpResponseArgsWithoutExplicitKeySpecified = 0;
        for (int i = 0; i < parameters.length; i++) {
            var p = parameters[i];
            if (p.isAnnotationPresent(ApphostKey.class) &&
                    (p.getAnnotation(ApphostKey.class).value() == null ||
                            p.getAnnotation(ApphostKey.class).value().isBlank())) {
                throw new ApphostControllerConfigurationException("ApphostKey may not be blank or null");
            }

            if (RequestContext.class.isAssignableFrom(p.getType()) ||
                    ApphostRequest.class.isAssignableFrom(p.getType()) ||
                    ApphostResponseBuilder.class.isAssignableFrom(p.getType()) ||
                    ApphostKeysContainer.class.isAssignableFrom(p.getType())) {
                continue;
            }
            if (GeneratedMessageV3.class.isAssignableFrom(p.getType())) {
                if (!p.isAnnotationPresent(ApphostKey.class)) {
                    throw new ApphostControllerConfigurationException("Parameter " + i + " of method " +
                            method.getDeclaringClass().getName() + "::" + method.getName() +
                            " must be annotated with " + ApphostKey.class.getName());
                }
                continue;
            }
            if (HttpResponse.class.isAssignableFrom(p.getType())) {
                if (!p.isAnnotationPresent(ApphostKey.class)) {
                    if (foundHttpResponseArgsWithoutExplicitKeySpecified > 0) {
                        throw new ApphostControllerConfigurationException(
                                "Only one HttpResponse argument may be specified for method " + method.getName()
                        );
                    } else {
                        foundHttpResponseArgsWithoutExplicitKeySpecified++;
                    }

                }
                continue;
            }

            throw new ApphostControllerConfigurationException("Parameter " + i + " of method " +
                    method.getDeclaringClass().getName() + "::" + method.getName() + " must be protobuf Message");
        }
    }

    @SuppressWarnings("unchecked")
    private List<IResultKeySetter> createResultSetters(Method method) {
        List<IResultKeySetter> setters;
        Class<?> returnType = method.getReturnType();

        @Nullable
        String methodKeyAnnotation = method.isAnnotationPresent(ApphostKey.class) ?
                method.getAnnotation(ApphostKey.class).value() : null;

        if (method.getReturnType().equals(Void.TYPE)) {
            setters = List.of();

        } else if (HttpRequest.class.isAssignableFrom(method.getReturnType())) {
            String resultKey = Objects.requireNonNullElse(methodKeyAnnotation, HTTP_REQUEST_APPHOST_KEY);
            setters = List.of(new HttpRequestResultKeySetter(resultKey, objectMapper));

        } else if (method.isAnnotationPresent(ApphostKey.class)) {
            String resultKey = method.getAnnotation(ApphostKey.class).value();
            if (GeneratedMessageV3.class.isAssignableFrom(returnType)) {
                setters = List.of(new SingleKeyProtobufResultSetter(resultKey));
            } else {
                throw new ApphostControllerConfigurationException("Method " + method.getName() +
                        " is annotated with ApphostKey but result type " + method.getReturnType().getName() +
                        " is unsupported");
            }

        } else {
            Field[] fields = returnType.getDeclaredFields();
            setters = Arrays.stream(fields)
                    .map(this::setterForField)
                    .collect(toList());
        }
        return setters;
    }

    private IResultKeySetter setterForField(Field field) {
        ReflectionUtils.makeAccessible(field);
        if (HttpRequest.class.isAssignableFrom(field.getType())) {
            String key = field.isAnnotationPresent(ApphostKey.class) ?
                    field.getAnnotation(ApphostKey.class).value() :
                    HTTP_REQUEST_APPHOST_KEY;

            return new FromFieldResultKeySetter(field, new HttpRequestResultKeySetter(key, objectMapper));
        }
        if (field.isAnnotationPresent(ApphostKey.class)) {
            String key = field.getAnnotation(ApphostKey.class).value();

            if (Message.class.isAssignableFrom(field.getType())) {
                return new FromFieldResultKeySetter(field, new SingleKeyProtobufResultSetter(key));
            } else if (Collection.class.isAssignableFrom(field.getType())) {
                return new FromFieldResultKeySetter(field, new RepeatedKeyProtobufResultSetter(key));
            } else {
                throw new ApphostControllerConfigurationException("Field " + field.getName() + " of class " +
                        field.getDeclaringClass().getName() + " must implement Message interface");
            }

        } else {
            throw new ApphostControllerConfigurationException("Field " + field.getDeclaringClass().getName() + "."
                    + field.getName() + " must be annotated with ApphostKey");
        }
    }

    private List<IArgumentGetter<?>> createArgumentGetters(Parameter[] parameters) {
        List<IArgumentGetter<?>> getters = Stream.of(parameters)
                .map(this::createGetterForParameter)
                .filter(Objects::nonNull)
                .collect(toList());
        return getters;
    }

    @Nullable
    private IArgumentGetter<?> createGetterForParameter(Parameter param) {
        @Nullable
        String key = param.isAnnotationPresent(ApphostKey.class) ?
                param.getAnnotation(ApphostKey.class).value() :
                null;
        Class<?> paramType = param.getType();
        boolean nullable = isNullable(param, paramType);

        if (GeneratedMessageV3.class.isAssignableFrom(paramType) && key != null) {

            return new SingleKeyProtobufArgumentGetter(key, nullable, (Class<GeneratedMessageV3>) paramType);

        } else if (HttpResponse.class.isAssignableFrom(paramType)) {
            Class<?> genericArgument =
                    (Class<?>) ((ParameterizedType) param.getParameterizedType()).getActualTypeArguments()[0];

            String responseKey = Objects.requireNonNullElse(key, HTTP_RESPONSE_APPHOST_KEY);

            return new HttpResponseArgumentGetter(responseKey, nullable, genericArgument, objectMapper);
        } else if (ApphostKeysContainer.class.isAssignableFrom(paramType)) {
            return new ApphostKeyContainerArgumentGetter(nullable, (Class<ApphostKeysContainer>) paramType);
        } else if (ApphostRequest.class.isAssignableFrom(paramType)) {
            return (IArgumentGetter<ApphostRequest>) context -> context;
        } else if (ApphostResponseBuilder.class.isAssignableFrom(paramType)) {
            return (IArgumentGetter<ApphostResponseBuilder>) context -> context;
        } else {
            return null;
        }
    }

}
