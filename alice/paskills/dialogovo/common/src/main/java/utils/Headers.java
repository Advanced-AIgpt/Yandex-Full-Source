package ru.yandex.alice.paskill.dialogovo.utils;

public class Headers {
    public static final String X_REQUEST_ID = "X-Request-Id";
    public static final String X_REQ_ID = "X-Req-Id";
    public static final String X_REAL_IP = "X-Real-IP";
    public static final String X_FORWARDED_FOR = "X-Forwarded-For";
    public static final String REFERER = "Referer";
    public static final String X_DEVELOPER_TRUSTED_TOKEN = "X-Developer-Trusted-Token";
    public static final String X_TRUSTED_SERVICE_TVM_CLIENT_ID = "X-Trusted-Service-Tvm-Client-Id";
    public static final String X_UID = "X-uid";

    private Headers() {
        throw new UnsupportedOperationException();
    }

}
