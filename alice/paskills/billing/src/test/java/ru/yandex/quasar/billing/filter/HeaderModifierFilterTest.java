package ru.yandex.quasar.billing.filter;

import java.io.IOException;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;

import static org.junit.jupiter.api.Assertions.assertTrue;

class HeaderModifierFilterTest {

    private HeaderModifierFilter filter;

    private BillingConfig billingConfig;

    @BeforeEach
    void setUp() throws IOException {
        billingConfig = new TestConfigProvider().billingConfig();
        filter = new HeaderModifierFilter(billingConfig);
    }

    @Test
    void checkHamsterHost() {
        assertTrue(filter.allowedHost("pazus.hamster.yandex.ru"));
    }

    @Test
    void checkTunnelingHost() {
        assertTrue(filter.allowedHost("pazus.tunneler-si.yandex.ru"));
    }

    @Test
    void checkDevHost() {
        assertTrue(filter.allowedHost("local.yandex.ru"));
    }

    @Test
    void checkProdHost() {
        assertTrue(filter.allowedHost("yandex.ru"));
    }
}
