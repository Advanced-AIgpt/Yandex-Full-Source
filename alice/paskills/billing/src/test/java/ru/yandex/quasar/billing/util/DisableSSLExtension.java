package ru.yandex.quasar.billing.util;

import org.junit.jupiter.api.extension.AfterAllCallback;
import org.junit.jupiter.api.extension.BeforeEachCallback;
import org.junit.jupiter.api.extension.ExtensionContext;

public class DisableSSLExtension implements BeforeEachCallback, AfterAllCallback {

    @Override
    public void afterAll(ExtensionContext context) throws Exception {
        SSLUtil.turnOnSslChecking();
    }

    @Override
    public void beforeEach(ExtensionContext context) throws Exception {
        SSLUtil.turnOffSslChecking();
    }
}
