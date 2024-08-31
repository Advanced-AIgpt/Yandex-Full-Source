package ru.yandex.quasar.billing.filter;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.context.annotation.Bean;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.passport.tvmauth.TicketStatus;
import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.passport.tvmauth.Unittest;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.Mockito.when;
import static ru.yandex.quasar.billing.services.tvm.TvmClientName.blackbox;
import static ru.yandex.quasar.billing.services.tvm.TvmClientName.quasar_backend;

@SpringBootTest(classes = {
        TestConfigProvider.class,
        TvmServiceAuthorizationInterceptorTest.TestConfig.class
}, webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
class TvmServiceAuthorizationInterceptorTest {

    private static final String ALLOWED_TICKET = "ALLOWED_TICKET";
    private static final String UNALLOWED_TICKET = "UNALLOWED_TICKET";

    private static final String INVALID_TICKET = "INVALID_TICKET";
    private static final String EMPTY_TICKET = "";
    private static final String NULL_TICKET = null;
    @Autowired
    private TestRestTemplate restTemplate;
    @MockBean
    private TvmClient tvmClient;

    @LocalServerPort
    private int port;


    @BeforeEach
    void setUp() {
        when(tvmClient.checkServiceTicket(ALLOWED_TICKET))
                .thenReturn(Unittest.createServiceTicket(TicketStatus.OK, quasar_backend.ordinal()));
        when(tvmClient.checkServiceTicket(UNALLOWED_TICKET))
                .thenReturn(Unittest.createServiceTicket(TicketStatus.OK, blackbox.ordinal()));
        when(tvmClient.checkServiceTicket(INVALID_TICKET))
                .thenReturn(Unittest.createServiceTicket(TicketStatus.MALFORMED, quasar_backend.ordinal()));
    }

    @Test
    void testAllowedOnAllowOnly() {
        assertEquals(HttpStatus.OK, request(ALLOWED_TICKET, "allowOnly"));
    }

    @Test
    void testUnallowedOnAllowOnly() {
        assertEquals(HttpStatus.FORBIDDEN, request(UNALLOWED_TICKET, "allowOnly"));
    }

    @Test
    void testAllowedOnNoCheck() {
        assertEquals(HttpStatus.OK, request(ALLOWED_TICKET, "noCheck"));
    }

    @Test
    void testUnallowedOnNoCheck() {
        assertEquals(HttpStatus.OK, request(UNALLOWED_TICKET, "noCheck"));
    }

    @Test
    void testAllowedOnAllowAll() {
        assertEquals(HttpStatus.OK, request(ALLOWED_TICKET, "allowAll"));
    }

    @Test
    void testUnallowedOnAllowAll() {
        assertEquals(HttpStatus.OK, request(UNALLOWED_TICKET, "allowAll"));
    }

    @Test
    void testInvalidOnAllowOnly() {
        assertEquals(HttpStatus.FORBIDDEN, request(INVALID_TICKET, "allowOnly"));
    }

    @Test
    void testInvalidOnNoCheck() {
        assertEquals(HttpStatus.OK, request(INVALID_TICKET, "noCheck"));
    }

    @Test
    void testInvalidOnAllowAll() {
        assertEquals(HttpStatus.FORBIDDEN, request(INVALID_TICKET, "allowAll"));
    }

    @Test
    void testEmptyOnAllowOnly() {
        assertEquals(HttpStatus.FORBIDDEN, request(EMPTY_TICKET, "allowOnly"));
    }

    @Test
    void testEmptyOnNoCheck() {
        assertEquals(HttpStatus.OK, request(EMPTY_TICKET, "noCheck"));
    }

    @Test
    void testEmptyOnAllowAll() {
        assertEquals(HttpStatus.FORBIDDEN, request(EMPTY_TICKET, "allowAll"));
    }

    @Test
    void testNullOnAllowOnly() {
        assertEquals(HttpStatus.FORBIDDEN, request(NULL_TICKET, "allowOnly"));
    }

    @Test
    void testNullOnNoCheck() {
        assertEquals(HttpStatus.OK, request(NULL_TICKET, "noCheck"));
    }

    @Test
    void testNullOnAllowAll() {
        assertEquals(HttpStatus.FORBIDDEN, request(NULL_TICKET, "allowAll"));
    }

    @Test
    void testRequestWithoutTokenAllowOnly() {
        assertEquals(HttpStatus.FORBIDDEN, requestWithoutTicket("allowOnly"));
    }

    @Test
    void testRequestWithoutTokenAllowAll() {
        assertEquals(HttpStatus.FORBIDDEN, requestWithoutTicket("allowAll"));
    }

    @Test
    void testRequestWithoutTokenAllowNoCheck() {
        assertEquals(HttpStatus.OK, requestWithoutTicket("noCheck"));
    }

    private HttpStatus request(String ticket, String method) {
        var headers = new LinkedMultiValueMap<String, String>();
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER, ticket);
        return restTemplate.exchange("http://localhost:" + port + "/tvmTest/" + method,
                HttpMethod.GET,
                new HttpEntity<>(headers),
                String.class)
                .getStatusCode();
    }

    private HttpStatus requestWithoutTicket(String method) {
        return restTemplate.getForEntity("http://localhost:" + port + "/tvmTest/" + method,
                String.class)
                .getStatusCode();
    }

    @TestConfiguration
    static class TestConfig {
        @Bean
        TestController testController() {
            return new TestController();
        }
    }

    @RestController
    static class TestController {
        @GetMapping(path = "/tvmTest/allowOnly")
        @TvmRequired(quasar_backend)
        private ResponseEntity<String> dummy() {
            return ResponseEntity.ok("{}");
        }

        @GetMapping(path = "/tvmTest/noCheck")
        private ResponseEntity<String> noCheck() {
            return ResponseEntity.ok("{}");
        }

        @GetMapping(path = "/tvmTest/allowAll")
        @TvmRequired
        private ResponseEntity<String> allowAll() {
            return ResponseEntity.ok("{}");
        }
    }

}
