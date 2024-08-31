package ru.yandex.quasar.billing.util;

import java.util.Objects;
import java.util.Optional;

import javax.annotation.Nullable;

public interface HasCode<C> {
    C getCode();

    static <C, E extends Enum<E> & HasCode<C>> Optional<E> findByCode(Class<E> enumClass, @Nullable C code) {
        if (code != null) {
            for (final E item : enumClass.getEnumConstants()) {
                if (Objects.equals(item.getCode(), code)) {
                    return Optional.of(item);
                }
            }
        }
        return Optional.empty();
    }

    static <C, E extends Enum<E> & HasCode<C>> E getByCode(Class<E> enumClass, C code) {
        return findByCode(enumClass, code).orElseThrow(IllegalArgumentException::new);
    }
}
