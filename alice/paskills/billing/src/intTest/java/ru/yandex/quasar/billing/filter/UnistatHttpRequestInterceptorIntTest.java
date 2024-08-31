package ru.yandex.quasar.billing.filter;

import java.time.Duration;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Primary;
import org.springframework.http.ResponseEntity;
import org.springframework.test.context.junit.jupiter.SpringExtension;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.client.ResourceAccessException;
import org.springframework.web.client.RestTemplate;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.UnistatService;

import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;

@ExtendWith(SpringExtension.class)
@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT,
        classes = {
                TestConfigProvider.class,
                UnistatHttpRequestInterceptorIntTest.TestConfig.class,
        }
)
@AutoConfigureWebClient(registerRestTemplate = true)
class UnistatHttpRequestInterceptorIntTest {
    @Autowired
    private RestTemplate restTemplateSmallTimeout;

    @MockBean
    public UnistatService unistatService;

    @LocalServerPort
    private int port;

    @Test
    void testTimeoutSignalExist() {
        assertThrows(
                ResourceAccessException.class,
                () -> restTemplateSmallTimeout.getForEntity("http://localhost:" + port + "/long_operation",
                        String.class));

        String expectedSignal = "localhost_long_operation";
        verify(unistatService).logOperationDurationHist(
                eq("quasar_billing_remote_method_" + expectedSignal + "_duration_dhhh"),
                anyLong());
        verify(unistatService).incrementStatValue("quasar_billing_remote_method_" + expectedSignal + "_timeout_dmmm");
        verify(unistatService).incrementStatValue("quasar_billing_remote_method_" + expectedSignal + "_calls_dmmm");
    }

    @TestConfiguration
    static class TestConfig {

        @Bean
        TestController testController() {
            return new TestController();
        }

        @Bean
        @Primary
        public RestTemplate restTemplateSmallTimeout(
                RestTemplateBuilder restTemplateBuilder,
                UnistatHttpRequestInterceptor unistatHttpRequestInterceptor
        ) {
            return restTemplateBuilder.setConnectTimeout(Duration.ofMillis(100))
                    .setReadTimeout(Duration.ofMillis(100))
                    .additionalInterceptors(unistatHttpRequestInterceptor)
                    .build();
        }
    }

    @RestController
    static class TestController {
        @GetMapping(path = "/long_operation")
        private ResponseEntity<String> dummy() throws InterruptedException {
            Thread.sleep(1000);
            return ResponseEntity.ok("{}");
        }
    }
}
