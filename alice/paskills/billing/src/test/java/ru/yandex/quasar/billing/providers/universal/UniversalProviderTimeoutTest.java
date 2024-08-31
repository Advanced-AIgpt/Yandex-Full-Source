package ru.yandex.quasar.billing.providers.universal;

import java.net.SocketTimeoutException;
import java.time.Duration;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.ReentrantLock;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.TestInstance;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.context.annotation.Bean;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.client.ResourceAccessException;

import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.providers.StreamData;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.UnistatService;
import ru.yandex.quasar.billing.util.DisableSSLExtension;

import static org.hamcrest.CoreMatchers.instanceOf;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.junit.jupiter.api.Assertions.assertThrows;

@ExtendWith(DisableSSLExtension.class)
@SpringBootTest(classes = {TestConfigProvider.class, UniversalProviderTimeoutTest.Config.class},
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@TestInstance(TestInstance.Lifecycle.PER_METHOD)
public class UniversalProviderTimeoutTest {

    private UniversalProvider provider;

    @Autowired
    private BillingConfig config;
    private ReentrantLock lock = new ReentrantLock();

    @Autowired
    private RestTemplateBuilder restTemplateBuilder;
    @Autowired
    private ObjectMapper objectMapper;
    @Autowired
    private AuthorizationContext authorizationContext;
    @MockBean
    private UnistatService unistatService;

    @LocalServerPort
    private int port;

    @BeforeEach
    void setUp() {
        config.getUniversalProviders().get("kinopoisk").setBaseUrl("");


    }

    @Test
    void testTimeout() {

        provider = new UniversalProvider("amediatekaUP",
                "test",
                "test",
                "test",
                "23b0df5a-0f35-4955-9b8b-104e6d1a7b90",
                "http://localhost:" + port + "/timeout_test/",
                restTemplateBuilder
                        .setConnectTimeout(Duration.ofMillis(500))
                        .setReadTimeout(Duration.ofMillis(500)),
                authorizationContext,
                Executors.newSingleThreadExecutor(),
                unistatService);

        var ex = assertThrows(ResourceAccessException.class,
                () -> provider.getStream(ProviderContentItem.createSeason("qweqwe"), "session", "user_id", "agent"));
        assertThat(ex.getCause(), instanceOf(SocketTimeoutException.class));

    }

    @Test
    void testPerf() {

        provider = new UniversalProvider("amediatekaUP",
                "test",
                "test",
                "test",
                "23b0df5a-0f35-4955-9b8b-104e6d1a7b90",
                "http://localhost:" + port + "/timeout_test/",
                restTemplateBuilder
                        .setConnectTimeout(Duration.ofMillis(100))
                        .setReadTimeout(Duration.ofMillis(50)),
                authorizationContext,
                Executors.newSingleThreadExecutor(),
                unistatService);

        int nThreads = 1000;

        ExecutorService service = Executors.newFixedThreadPool(nThreads);

        try {

            for (int i = 0; i < nThreads; i++) {
                service.submit(() -> provider.getStream(ProviderContentItem.createSeason("qweqwe"), "session",
                        "user_id", "agent"));
            }
        } finally {
            service.shutdownNow();
        }

    }

    @TestConfiguration
    static class Config {
        @Bean
        TestController controller() {
            return new TestController();
        }
    }

    @RestController
    private static class TestController {
        private static final Logger logger = LogManager.getLogger();
        private volatile AtomicInteger ind = new AtomicInteger(0);

        @RequestMapping(path = "/timeout_test/**")
        public StreamData getAny() throws InterruptedException {
            //logger.warn("req: " + ind.getAndIncrement() + " time " + System.currentTimeMillis());
            Thread.sleep(1000L);
            return StreamData.byUrl("http://ya.ru");
        }
    }
}
