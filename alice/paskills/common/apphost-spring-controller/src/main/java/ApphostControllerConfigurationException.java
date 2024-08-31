package ru.yandex.alice.paskills.common.apphost.spring;

public class ApphostControllerConfigurationException extends RuntimeException {
    public ApphostControllerConfigurationException() {
    }

    public ApphostControllerConfigurationException(String message) {
        super(message);
    }

    public ApphostControllerConfigurationException(String message, Throwable cause) {
        super(message, cause);
    }

    public ApphostControllerConfigurationException(Throwable cause) {
        super(cause);
    }
}
