package ru.yandex.quasar.billing;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.boot.test.context.SpringBootTest;

import ru.yandex.quasar.billing.config.TestConfigProvider;

import static org.junit.jupiter.api.Assertions.assertTrue;

@SpringBootTest(properties = {
        "BLACKBOX_HOST=blackbox.yandex.net",
        "SOCIALISM_HOST=api.social.yandex.ru",
        "QUASAR_HOST_RU=localhost",
        "QUASAR_HOST_NET=localhost",
        "CALLBACK_HOST=https://localhost/billing",
        "QLOUD_TVM_TOKEN=123123",
        "CSRF_TOKEN_KEY=1",
        "quasar.config.path=configs/dev/quasar-billing.cfg"
}, classes = TestConfigProvider.class)
@ExtendWith(EmbeddedPostgresExtension.class)
class ApplicationTest {

    @Test
    void verifyCorrectApplicationContext() {
        assertTrue(true);
    }
}
