package ru.yandex.quasar.billing.services.processing.yapay;

import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;
import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.Data;

@Data
class ResponseWrapper<T> {
    public static final ResponseWrapper<Empty> EMPTY = success(new Empty(null));
    private final T data;
    private final Status status;
    private final int code;

    public static <T> ResponseWrapper<T> success(T data) {
        return new ResponseWrapper<>(data, Status.success, 200);
    }

    enum Status {
        success,
        fail,
        @JsonEnumDefaultValue
        other
    }

    @Data
    @JsonInclude(JsonInclude.Include.NON_NULL)
    static class Empty {
        private final Object dummy;
    }
}
