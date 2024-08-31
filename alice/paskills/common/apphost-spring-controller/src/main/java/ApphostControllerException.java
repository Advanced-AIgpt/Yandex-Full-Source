package ru.yandex.alice.paskills.common.apphost.spring;

public class ApphostControllerException extends RuntimeException {
    public ApphostControllerException() {
        super();
    }

    public ApphostControllerException(String message) {
        super(message);
    }

    public ApphostControllerException(String message, Throwable cause) {
        super(message, cause);
    }

    public ApphostControllerException(Throwable cause) {
        super(cause);
    }
}
