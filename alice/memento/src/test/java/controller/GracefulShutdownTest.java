package ru.yandex.alice.memento.controller;

import java.net.ConnectException;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.availability.AvailabilityChangeEvent;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.context.ApplicationListener;
import org.springframework.context.ConfigurableApplicationContext;
import org.springframework.context.event.ContextClosedEvent;
import org.springframework.core.env.ConfigurableEnvironment;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.client.ResourceAccessException;

import ru.yandex.alice.memento.Application;
import ru.yandex.alice.memento.storage.TestStorageConfiguration;
import ru.yandex.alice.memento.tvm.TestTvmConfiguration;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Тест подсмотрен в тестах самого Srping'а про graceful shutdown
 */
public class GracefulShutdownTest {
    private TestRestTemplate restTemplate = new TestRestTemplate();

    @LocalServerPort
    private int port;
    private static final Logger logger = LogManager.getLogger();

    private final CountDownLatch latch = new CountDownLatch(1);
    private final CountDownLatch staredLatch = new CountDownLatch(1);

    private volatile ClassLoader threadContextClassLoader;
    private ConfigurableApplicationContext app;


    @BeforeEach
    void setUp() {
        ApplicationListener<ContextClosedEvent> listener = (event) -> {
            threadContextClassLoader = Thread.currentThread().getContextClassLoader();
            latch.countDown();
        };
        ApplicationListener<AvailabilityChangeEvent> startedListener = (event) -> {
            staredLatch.countDown();
        };
        SpringApplication app1 = new SpringApplication(Application.class, TestTvmConfiguration.class,
                TestStorageConfiguration.class);
        app1.setAdditionalProfiles("ut");
        app1.addListeners(listener);
        app1.addListeners(startedListener);

        app = app1.run("server.port=0");
        ConfigurableEnvironment environment = app.getEnvironment();
        port = Integer.parseInt(environment.getProperty("local.server.port"));
    }

    @AfterEach
    void tearDown() {
        if (app != null && app.isActive()) {
            app.close();
        }
    }

    @Test
    void testShutdown() throws InterruptedException {
        assertTrue(staredLatch.await(10, TimeUnit.SECONDS));
        ResponseEntity<String> response =
                restTemplate.getForEntity("http://localhost:" + port + "/actuator/health/readiness", String.class);
        logger.info(response.getBody());
        assertEquals("{\"status\":\"UP\"}", response.getBody());

        HttpHeaders headers = new HttpHeaders();
        headers.setContentType(MediaType.APPLICATION_JSON);
        ResponseEntity<String> s = restTemplate.exchange("http://localhost:" + port + "/actuator/shutdown",
                HttpMethod.POST,
                new HttpEntity<>(headers), String.class);
        assertEquals(HttpStatus.OK, s.getStatusCode());


        try {
            // first request is send while shutdown process hasn't started yet
            response = restTemplate.getForEntity("http://localhost:" + port + "/actuator/health/readiness",
                    String.class);
            logger.info(response.getBody());
            assertEquals("{\"status\":\"UP\"}", response.getBody());

            Thread.sleep(1000);

            response = restTemplate.getForEntity("http://localhost:" + port + "/actuator/health/readiness",
                    String.class);
            assertEquals(HttpStatus.SERVICE_UNAVAILABLE, response.getStatusCode());
        } catch (ResourceAccessException e) {
            if (e.getCause() instanceof ConnectException) {
                logger.warn("Connection refused as expected", e);
                // may catch connect exception as server doesn't accept any new connections
            } else {
                throw e;
            }
        }

        assertTrue(latch.await(15, TimeUnit.SECONDS));
    }
}
