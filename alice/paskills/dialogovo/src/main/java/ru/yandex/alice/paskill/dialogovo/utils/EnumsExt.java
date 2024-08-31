package ru.yandex.alice.paskill.dialogovo.utils;

import java.util.Optional;
import java.util.function.Function;

public class EnumsExt {
    private EnumsExt() {
        throw new UnsupportedOperationException();
    }

    public static <E extends Enum<E>> boolean isInEnum(Class<E> enumClass, Function<E, Boolean> predicate) {
        for (E e : enumClass.getEnumConstants()) {
            if (predicate.apply(e)) {
                return true;
            }
        }

        return false;
    }

    public static <E extends Enum<E>> Optional<E> findEnum(Class<E> enumClass, Function<E, Boolean> predicate) {
        for (E e : enumClass.getEnumConstants()) {
            if (predicate.apply(e)) {
                return Optional.of(e);
            }
        }

        return Optional.empty();
    }
}
