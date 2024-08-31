package ru.yandex.alice.paskills.common.apphost.spring;

import java.lang.reflect.AnnotatedElement;

import javax.annotation.Nullable;

public class NullabilityUtil {
    private NullabilityUtil() {

    }

    static boolean isNullable(AnnotatedElement param, Class<?> clazz) {
        return param.isAnnotationPresent(Nullable.class) ||
                param.isAnnotationPresent(org.springframework.lang.Nullable.class) ||
                clazz.getName().endsWith("?");
    }
}
