package ru.yandex.quasar.billing.services.promo;

import java.net.SocketTimeoutException;
import java.time.Duration;

import org.hamcrest.Matchers;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.web.client.RestTemplateAutoConfiguration;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;
import org.springframework.web.client.ResourceAccessException;
import org.springframework.web.client.RestTemplate;

import ru.yandex.quasar.billing.config.TestConfigProvider;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.junit.jupiter.api.Assertions.assertThrows;

/**
 * Test illustrating how timeout exceptions look like
 */
@SpringJUnitConfig(classes = {TestConfigProvider.class, RestTemplateAutoConfiguration.class})
class QuasarBackendClientImplTest {

    @Autowired
    private RestTemplateBuilder builder;

    @Test
    @Disabled
    void connectionTimeout() {
        RestTemplate restTemplate = builder.setConnectTimeout(Duration.ofMillis(1L)).build();

        var e = assertThrows(ResourceAccessException.class, () -> restTemplate.headForHeaders("http://yandex.ru"));

        System.out.println(e.getMessage());
        assertThat(e.getMostSpecificCause(), Matchers.instanceOf(SocketTimeoutException.class));
    }

    @Test
    @Disabled
    void readTimeout() {
        RestTemplate restTemplate = builder.setReadTimeout(Duration.ofMillis(1L)).build();

        var e = assertThrows(ResourceAccessException.class, () -> restTemplate.headForHeaders("https://ya.ru"));

        System.out.println(e.getMessage());
        assertThat(e.getMostSpecificCause(), Matchers.instanceOf(SocketTimeoutException.class));
    }
}
