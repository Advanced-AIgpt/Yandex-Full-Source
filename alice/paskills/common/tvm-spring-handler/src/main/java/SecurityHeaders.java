package ru.yandex.alice.paskills.common.tvm.spring.handler;

public class SecurityHeaders {
    public static final String X_DEVELOPER_TRUSTED_TOKEN = "X-Developer-Trusted-Token";
    public static final String X_TRUSTED_SERVICE_TVM_CLIENT_ID = "X-Trusted-Service-Tvm-Client-Id";
    public static final String X_UID = "X-Uid";
    public static final String USER_TICKET_HEADER = "X-Ya-User-Ticket";
    public static final String SERVICE_TICKET_HEADER = "X-Ya-Service-Ticket";

    private SecurityHeaders() {
        throw new UnsupportedOperationException();
    }
}
