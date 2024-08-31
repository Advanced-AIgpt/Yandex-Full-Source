package ru.yandex.alice.paskills.common.tvm.spring.handler;

import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.ResponseStatus;

@ResponseStatus(HttpStatus.FORBIDDEN)
public class TvmAuthorizationException extends RuntimeException {

    public TvmAuthorizationException() {
    }

    public TvmAuthorizationException(String message) {
        super(message);
    }

    public TvmAuthorizationException(Throwable cause) {
        super(cause);
    }
}
