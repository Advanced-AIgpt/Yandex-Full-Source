package ru.yandex.alice.kronstadt.server.http.middleware;

import java.util.Map;

import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.context.annotation.Bean;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.alice.paskills.common.tvm.spring.handler.TvmAuthorizationInterceptor;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.passport.tvmauth.TvmClient;

import static org.junit.jupiter.api.Assertions.assertEquals;

@SpringBootTest(classes = {
        SolomonInterceptorTest.TestConfig.class
},
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
class SolomonInterceptorTest {

    @Autowired
    private MetricRegistry metricRegistry;

    @Autowired
    private TestRestTemplate restTemplate;

    @MockBean
    private TvmClient tvmClient;

    @LocalServerPort
    private int port;

    private String call(String method) {
        return restTemplate.getForObject("http://localhost:" + port + "/solomon_test/" + method, String.class);
    }

    @Test
    void testRequestOk() {
        // given
        assertEquals(0L, metricRegistry.rate("http.in.requests_rate", Labels.of("path", "solomon_test_ok")).get(),
                "wrong result request rate");
        assertEquals(0L,
                metricRegistry.rate("http.in.requests_failure_rate", Labels.of("path", "solomon_test_ok")).get(),
                "wrong result failure rate");

        // when
        String result = call("ok");

        // then
        assertEquals(1L, metricRegistry.rate("http.in.requests_rate", Labels.of("path", "solomon_test_ok")).get(),
                "wrong result request rate");
        assertEquals(0L,
                metricRegistry.rate("http.in.requests_failure_rate", Labels.of("path", "solomon_test_ok")).get(),
                "wrong result failure rate");
    }

    @Test
    @Disabled("flaky test")
    void testRequestSolomonHandle() {
        // given
        assertEquals(0L, metricRegistry.rate("http.in.requests_rate", Labels.of("path", "solomon")).get(), "wrong " +
                "result request rate");
        assertEquals(0L, metricRegistry.rate("http.in.requests_failure_rate", Labels.of("path", "solomon")).get(),
                "wrong result failure rate");

        // when
        var result = restTemplate.getForObject("http://localhost:" + port + "/solomon", String.class);

        // then
        assertEquals(1L, metricRegistry.rate("http.in.requests_rate", Labels.of("path", "solomon")).get(), "wrong " +
                "result request rate");
        assertEquals(0L, metricRegistry.rate("http.in.requests_failure_rate", Labels.of("path", "solomon")).get(),
                "wrong result failure rate");
    }

    @Test
    void testRequest400() {
        // given
        assertEquals(0L, metricRegistry.rate("http.in.requests_rate", Labels.of("path", "solomon_test_400")).get(),
                "wrong result request rate");
        assertEquals(0L,
                metricRegistry.rate("http.in.requests_failure_rate", Labels.of("path", "solomon_test_400")).get(),
                "wrong result failure rate");

        // when
        String result = call("400");

        // then
        assertEquals(1L, metricRegistry.rate("http.in.requests_rate", Labels.of("path", "solomon_test_400")).get(),
                "wrong result request rate");
        assertEquals(1L,
                metricRegistry.rate("http.in.requests_failure_rate", Labels.of("path", "solomon_test_400")).get(),
                "wrong result failure rate");
    }

    @Test
    void testRequest500() {
        // given
        assertEquals(0L, metricRegistry.rate("http.in.requests_rate", Labels.of("path", "solomon_test_500")).get(),
                "wrong result request rate");
        assertEquals(0L,
                metricRegistry.rate("http.in.requests_failure_rate", Labels.of("path", "solomon_test_500")).get(),
                "wrong result failure rate");

        // when
        String result = call("500");

        // then
        assertEquals(1L, metricRegistry.rate("http.in.requests_rate", Labels.of("path", "solomon_test_500")).get(),
                "wrong result request rate");
        assertEquals(1L,
                metricRegistry.rate("http.in.requests_failure_rate", Labels.of("path", "solomon_test_500")).get(),
                "wrong result failure rate");
    }

    @Test
    void testRequestEx() {
        // given
        assertEquals(0L, metricRegistry.rate("http.in.requests_rate", Labels.of("path", "solomon_test_ex")).get(),
                "wrong result request rate");
        assertEquals(0L,
                metricRegistry.rate("http.in.requests_failure_rate", Labels.of("path", "solomon_test_ex")).get(),
                "wrong result failure rate");

        // when
        String result = call("ex");

        // then
        assertEquals(1L, metricRegistry.rate("http.in.requests_rate", Labels.of("path", "solomon_test_ex")).get(),
                "wrong result request rate");
        assertEquals(1L,
                metricRegistry.rate("http.in.requests_failure_rate", Labels.of("path", "solomon_test_ex")).get(),
                "wrong result failure rate");
    }

    @TestConfiguration
    static class TestConfig {

        @Bean
        public TestController testController() {
            return new TestController();
        }

        @Bean
        public TvmAuthorizationInterceptor testTvmAuthorizationInterceptor(TvmClient tvmClient) {
            return new TvmAuthorizationInterceptor(tvmClient,
                    Map.of(),
                    null,
                    false,
                    false
            );
        }
    }

    @RestController
    private static class TestController {
        @GetMapping(path = "/solomon_test/ok")
        private ResponseEntity<String> ok() throws InterruptedException {
            Thread.sleep(100);
            return ResponseEntity.ok("{}");
        }

        @GetMapping(path = "/solomon_test/400")
        private ResponseEntity<String> err400() {
            return ResponseEntity.badRequest().build();
        }

        @GetMapping(path = "/solomon_test/500")
        private ResponseEntity<String> err500() {
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }

        @GetMapping(path = "/solomon_test/ex")
        private ResponseEntity<String> ex() {
            throw new RuntimeException("some error");
        }
    }
}
