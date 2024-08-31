package ru.yandex.alice.kronstadt.core.utils;

import java.util.Arrays;
import java.util.Map;
import java.util.Optional;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

public class StringEnumResolver<T extends Enum<T> & StringEnum> {

    private final Map<String, T> valuesByValue;
    private final Class<T> enumClass;

    public StringEnumResolver(Class<T> enumClass) {
        this.enumClass = enumClass;
        valuesByValue = Arrays
                .stream(enumClass.getEnumConstants())
                .collect(Collectors.toUnmodifiableMap(StringEnum::value, Function.identity()));
    }

    public Optional<T> fromValueO(@Nullable String value) {
        return Optional.ofNullable(value).map(valuesByValue::get);
    }

    @Nullable
    public T fromValueOrNull(@Nullable String value) {
        if (value == null) {
            return null;
        } else {
            return valuesByValue.get(value);
        }
    }

    public T fromValueOrDefault(@Nullable String value, T defaultValue) {
        return fromValueO(value).orElse(defaultValue);
    }

    public T fromValueOrThrow(@Nullable String value) throws InvalidEnumStringException {
        T enumValue = valuesByValue.get(value);
        if (enumValue == null) {
            throw new InvalidEnumStringException(enumClass.getName(), value);
        }
        return enumValue;
    }

    public static <T extends Enum<T> & StringEnum> StringEnumResolver<T> resolver(Class<T> enumClass) {
        return new StringEnumResolver<>(enumClass);
    }

    public static class InvalidEnumStringException extends RuntimeException {

        public InvalidEnumStringException(String enumClassName, @Nullable String value) {
            super(String.format("Failed to parse %s value from \"%s\"", enumClassName, value));
        }

    }
}
