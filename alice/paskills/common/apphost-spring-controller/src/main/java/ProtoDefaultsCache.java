package ru.yandex.alice.paskills.common.apphost.spring;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import com.google.protobuf.GeneratedMessageV3;
import org.springframework.util.ReflectionUtils;

import static java.util.Objects.requireNonNull;

class ProtoDefaultsCache {
    private static final Map<Class<?>, GeneratedMessageV3> DEFAULT_INSTANCES = new ConcurrentHashMap<>();

    private ProtoDefaultsCache() {
        throw new UnsupportedOperationException();
    }

    static GeneratedMessageV3 getDefaultProtoInstance(Class<?> paramType) {
        return DEFAULT_INSTANCES.computeIfAbsent(paramType,
                clazz -> (GeneratedMessageV3) requireNonNull(
                        ReflectionUtils.invokeMethod(
                                requireNonNull(ReflectionUtils.findMethod(clazz, "getDefaultInstance")), null)
                )
        );

    }
}
