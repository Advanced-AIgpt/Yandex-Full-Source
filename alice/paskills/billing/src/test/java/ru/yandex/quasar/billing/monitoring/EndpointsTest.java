package ru.yandex.quasar.billing.monitoring;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.web.client.RestTemplate;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.config.TestConfigProvider;

import static org.junit.jupiter.api.Assertions.assertNotNull;

@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = TestConfigProvider.class,
        properties = {
                "management.endpoints.web.base-path=/",
                "management.endpoints.web.exposure.include=health,unistat,healthcheck"
        },
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
class EndpointsTest {
    @Autowired
    private RestTemplate restTemplate;

    @LocalServerPort
    private int port;

    @Test
    void testHealthCheck() {
        String result = restTemplate.getForObject("http://localhost:" + port + "/healthcheck", String.class);
        assertNotNull(result);
    }

    @Test
    void testUnistat() {
        String result = restTemplate.getForObject("http://localhost:" + port + "/unistat", String.class);
        assertNotNull(result);
    }
}
