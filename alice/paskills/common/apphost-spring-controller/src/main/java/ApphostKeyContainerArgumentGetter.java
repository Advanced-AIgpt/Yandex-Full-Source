package ru.yandex.alice.paskills.common.apphost.spring;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

import javax.annotation.Nullable;

import com.google.protobuf.GeneratedMessageV3;
import org.springframework.core.annotation.AnnotatedElementUtils;

import ru.yandex.web.apphost.api.request.RequestContext;

import static ru.yandex.alice.paskills.common.apphost.spring.NullabilityUtil.isNullable;

public class ApphostKeyContainerArgumentGetter implements IArgumentGetter<ApphostKeysContainer> {

    private final boolean nullable;
    private final Constructor<? extends ApphostKeysContainer> constructor;
    private final List<? extends IArgumentGetter<?>> getters;

    public ApphostKeyContainerArgumentGetter(boolean nullable, Class<? extends ApphostKeysContainer> paramType) {
        this.nullable = nullable;

        //var fields = List.of(paramType.getDeclaredFields());
        List<Constructor<ApphostKeysContainer>> constructors = Arrays.stream(paramType.getConstructors())
                // exclude Kotlin internal constructor with marker
                .filter(c -> Arrays.stream(c.getParameters())
                        .noneMatch(p -> p.getType().getName().equals("kotlin.jvm.internal.DefaultConstructorMarker"))
                )
                .sorted(Comparator.comparingInt(Constructor::getParameterCount))
                .map(it -> (Constructor<ApphostKeysContainer>) it)
                .toList();
        this.constructor = constructors.get(constructors.size() - 1);

        this.getters = Arrays.stream(this.constructor.getParameters())
                .map(p -> {
                    if (!p.isAnnotationPresent(ApphostKey.class)) {
                        throw new ApphostControllerException("Parameter " + p.getName() + " of constructor of " +
                                paramType.getName() + " is not annotated with ApphostKey");
                    }
                    if (!GeneratedMessageV3.class.isAssignableFrom(p.getType())) {
                        throw new ApphostControllerException("Constructor argument " + p.getName() + " must be of" +
                                " protobuf Message type");
                    }

                    String key = AnnotatedElementUtils.findMergedAnnotation(p, ApphostKey.class).value();
                    boolean nullableField = isNullable(p, p.getType());

                    return new SingleKeyProtobufArgumentGetter(key, nullableField,
                            (Class<GeneratedMessageV3>) p.getType());
                })
                .toList();

    }

    @Nullable
    @Override
    public ApphostKeysContainer get(RequestContext context) {

        var objects = new Object[getters.size()];
        boolean anyNonNull = false;

        for (int i = 0; i < getters.size(); i++) {
            objects[i] = getters.get(i).get(context);
            if (objects[i] != null) {
                anyNonNull = true;
            }
        }

        if (nullable && !anyNonNull) {
            return null;
        } else {

            try {
                return constructor.newInstance(objects);
            } catch (InstantiationException | IllegalAccessException | InvocationTargetException e) {
                throw new RuntimeException("Failed to instantiate ApphostKeysContainer class", e);
            }
        }
    }
}
