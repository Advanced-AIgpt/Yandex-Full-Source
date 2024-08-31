package ru.yandex.alice.paskills.common.tvm.spring.handler;

public class SecurityRequestAttributes {

    public static final String TVM_CLIENT_ID_REQUEST_ATTR = "ru.yandex.alice.paskills.tvm_client_id";
    public static final String USER_TICKET_REQUEST_ATTR = "ru.yandex.alice.paskills.tvm.user_ticket";
    public static final String UID_REQUEST_ATTR = "ru.yandex.alice.paskills.uid";

    private SecurityRequestAttributes() {
        throw new UnsupportedOperationException();
    }
}
