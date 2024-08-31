package ru.yandex.quasar.billing.exception;

/**
 * An exception class to be thrown when some internal error occurs and we want it neatly logged.
 * <p>
 * Allows to send some extra info to the log but not to the caller to ease debugging.
 */
//@ResponseStatus(value = HttpStatus.INTERNAL_SERVER_ERROR, reason = "Internal error occurred")
public class InternalErrorException extends RuntimeException {

    public InternalErrorException(String extraInfo, Throwable cause) {
        super("Internal error has occurred: " + extraInfo, cause);
    }

    public InternalErrorException(String extraInfo) {
        this(extraInfo, null);
    }

}
