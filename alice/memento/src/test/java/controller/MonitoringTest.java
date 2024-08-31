package ru.yandex.alice.memento.controller;

import com.google.protobuf.Message;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.context.annotation.Bean;
import org.springframework.http.HttpStatus;
import org.springframework.http.converter.HttpMessageConverter;
import org.springframework.test.context.ActiveProfiles;

import ru.yandex.alice.memento.storage.InMemoryStorageDao;
import ru.yandex.alice.memento.storage.StorageDao;
import ru.yandex.alice.memento.tvm.TestTvmConfiguration;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

@SpringBootTest(classes = {TestTvmConfiguration.class, MonitoringTest.Configuration.class},
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@ActiveProfiles("ut")
public class MonitoringTest {
    private TestRestTemplate restTemplate;
    @Autowired
    private HttpMessageConverter<Message> messageConverter;
    @Autowired
    private HttpMessageConverter<String> stringMessageConverter;
    @LocalServerPort
    private int port;

    @LocalServerPort
    private int actuatorPort/* = 8081*/;

    @BeforeEach
    void setUp() {
        restTemplate = new TestRestTemplate(new RestTemplateBuilder());
    }

    @Test
    void testPing() {
        var actual = restTemplate.getForEntity("http://localhost:" + port + "/healthcheck", String.class);

        assertEquals(HttpStatus.OK, actual.getStatusCode());
        assertEquals("OK", actual.getBody());
    }

    @Test
    void testReadiness() {
        var actual = restTemplate.getForEntity("http://localhost:" + actuatorPort + "/actuator/health/readiness",
                String.class);

        assertEquals(HttpStatus.OK, actual.getStatusCode());
        assertEquals("{\"status\":\"UP\"}", actual.getBody());
    }

    @Test
    void testLiveness() {
        var actual = restTemplate.getForEntity("http://localhost:" + actuatorPort + "/actuator/health/liveness",
                String.class);

        assertEquals(HttpStatus.OK, actual.getStatusCode());
        assertEquals("{\"status\":\"UP\"}", actual.getBody());
    }

    @Test
    void testHealth() {
        var actual = restTemplate.getForEntity("http://localhost:" + actuatorPort + "/actuator/health", String.class);

        assertEquals(HttpStatus.OK, actual.getStatusCode());
        assertEquals("{\"status\":\"UP\",\"groups\":[\"liveness\",\"readiness\"]}", actual.getBody());
    }

    @Test
    @Disabled
    void testActuator() {
        var actual = restTemplate.getForEntity("http://localhost:" + actuatorPort + "/actuator", String.class);

        assertEquals(HttpStatus.OK, actual.getStatusCode());
        assertEquals("OK", actual.getBody());
    }

    @Test
    void testSolomon() {
        var actual = restTemplate.getForEntity("http://localhost:" + port + "/solomon", String.class);

        assertEquals(HttpStatus.OK, actual.getStatusCode());
        assertTrue(actual.getBody().contains("jetty.connections"), "\"jetty.connections\" not found in the sensors " +
                "map");
    }

    @TestConfiguration
    static class Configuration {
        @Bean
        StorageDao settingsStorageDao() {
            return new InMemoryStorageDao();
        }

    }
}
