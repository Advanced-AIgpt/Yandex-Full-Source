package ru.yandex.alice.paskills.common.tvm.spring.handler;

import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.EnableAutoConfiguration;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.servlet.config.annotation.InterceptorRegistry;
import org.springframework.web.servlet.config.annotation.WebMvcConfigurer;

import ru.yandex.passport.tvmauth.TicketStatus;
import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.passport.tvmauth.Unittest;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.when;

@SpringBootTest(classes = {AuthorizationInterceptorTest.TestConfig.class},
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
class AuthorizationInterceptorTest {

    private static final String ALLOWED_TICKET = "ALLOWED_TICKET";
    private static final String UNALLOWED_TICKET = "UNALLOWED_TICKET";

    private static final String INVALID_TICKET = "INVALID_TICKET";
    private static final String EMPTY_TICKET = "";
    @Nullable
    private static final String NULL_TICKET = null;
    private static final int ALLOWED_SERVICE_TVM_CLIENT_ID = 2000860;
    private static final int NOT_ALLOWED_SERVICE_TVM_CLIENT_ID = 2000861;
    private static final String TRUSTED_TOKEN = "TRUSTED";
    private static final String ALLOWED_USER_TICKET = "CORRECT_USER_TICKET";
    private static final String INVALID_USER_TICKET = "INVALID_USER_TICKET";

    @Autowired
    private TestRestTemplate restTemplate;
    @MockBean
    private TvmClient tvmClient;

    @LocalServerPort
    private int port;

    @BeforeEach
    void setUp() {
        when(tvmClient.checkServiceTicket(ALLOWED_TICKET))
                .thenReturn(Unittest.createServiceTicket(TicketStatus.OK, ALLOWED_SERVICE_TVM_CLIENT_ID, 0));
        when(tvmClient.checkServiceTicket(UNALLOWED_TICKET))
                .thenReturn(Unittest.createServiceTicket(TicketStatus.OK, 2, 0));
        when(tvmClient.checkServiceTicket(INVALID_TICKET))
                .thenReturn(Unittest.createServiceTicket(TicketStatus.INVALID_DST, 0));
        when(tvmClient.checkUserTicket(eq(ALLOWED_USER_TICKET)))
                .thenReturn(Unittest.createUserTicket(TicketStatus.OK, 1, new String[0], new long[0]));
        when(tvmClient.checkUserTicket(eq(INVALID_USER_TICKET)))
                .thenReturn(Unittest.createUserTicket(TicketStatus.EXPIRED, 1, new String[0], new long[0]));
    }

    @Test
    void testAllowedOnAllowOnly() {
        assertEquals(HttpStatus.OK, request("allowOnly", ALLOWED_TICKET));
    }

    @Test
    void testUnallowedOnAllowOnly() {
        assertEquals(HttpStatus.FORBIDDEN, request("allowOnly", UNALLOWED_TICKET));
    }

    @Test
    void testAllowedOnNoCheck() {
        assertEquals(HttpStatus.OK, request("noCheck", ALLOWED_TICKET));
    }

    @Test
    void testUnallowedOnNoCheck() {
        assertEquals(HttpStatus.OK, request("noCheck", UNALLOWED_TICKET));
    }

    @Test
    void testAllowedOnAllowAll() {
        assertEquals(HttpStatus.OK, request("allowAll", ALLOWED_TICKET));
    }

    @Test
    void testUnallowedOnAllowAll() {
        assertEquals(HttpStatus.OK, request("allowAll", UNALLOWED_TICKET));
    }

    @Test
    void testInvalidOnAllowOnly() {
        assertEquals(HttpStatus.FORBIDDEN, request("allowOnly", INVALID_TICKET));
    }

    @Test
    void testInvalidOnNoCheck() {
        assertEquals(HttpStatus.OK, request("noCheck", INVALID_TICKET));
    }

    @Test
    void testInvalidOnAllowAll() {
        assertEquals(HttpStatus.FORBIDDEN, request("allowAll", INVALID_TICKET));
    }

    @Test
    void testEmptyOnAllowOnly() {
        assertEquals(HttpStatus.FORBIDDEN, request("allowOnly", EMPTY_TICKET));
    }

    @Test
    void testEmptyOnNoCheck() {
        assertEquals(HttpStatus.OK, request("noCheck", EMPTY_TICKET));
    }

    @Test
    void testEmptyOnAllowAll() {
        assertEquals(HttpStatus.FORBIDDEN, request("allowAll", EMPTY_TICKET));
    }

    @Test
    void testNullOnAllowOnly() {
        assertEquals(HttpStatus.FORBIDDEN, request("allowOnly", NULL_TICKET));
    }

    @Test
    void testNullOnNoCheck() {
        assertEquals(HttpStatus.OK, request("noCheck", NULL_TICKET));
    }

    @Test
    void testNullOnAllowAll() {
        assertEquals(HttpStatus.FORBIDDEN, request("allowAll", NULL_TICKET));
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

    @Test
    void testRequestAllowAllWithUserNoTicket() {
        assertEquals(HttpStatus.FORBIDDEN, request("allowAllWithUser", ALLOWED_TICKET, null));
    }

    @Test
    void testRequestAllowAllWithUserInvalidTicket() {
        assertEquals(HttpStatus.FORBIDDEN, request("allowAllWithUser", ALLOWED_TICKET, INVALID_USER_TICKET));
    }

    @Test
    void testRequestAllowAllWithUserCorrectTicket() {
        assertEquals(HttpStatus.OK, request("allowAllWithUser", ALLOWED_TICKET, ALLOWED_USER_TICKET));
    }

    @Test
    void testAllowedOnAllowAllWithUser() {
        assertEquals(HttpStatus.OK, request("allowAll", ALLOWED_TICKET, ALLOWED_USER_TICKET));
    }

    @Test
    void testRequestWithTrustedTokenWithoutTrustedTvmClientIdAllowNoCheck() {
        assertEquals(HttpStatus.OK, requestWithTrustedToken("noCheck", null));
    }

    @Test
    void testRequestWithTrustedTokenWithoutTrustedTvmClientIdAllowAll() {
        assertEquals(HttpStatus.FORBIDDEN, requestWithTrustedToken("allowAll", null));
    }

    @Test
    void testRequestWithTrustedTokenWithoutTrustedTvmClientIdAllowOnly() {
        assertEquals(HttpStatus.FORBIDDEN, requestWithTrustedToken("allowOnly", null));
    }

    @Test
    void testRequestWithTrustedTokenWithTrustedTvmClientIdAllowNoCheck() {
        assertEquals(HttpStatus.OK, requestWithTrustedToken("noCheck", ALLOWED_SERVICE_TVM_CLIENT_ID));
    }

    @Test
    void testRequestWithTrustedTokenWithTrustedTvmClientIdAllowAll() {
        assertEquals(HttpStatus.OK, requestWithTrustedToken("allowAll", ALLOWED_SERVICE_TVM_CLIENT_ID));
    }

    @Test
    void testRequestWithTrustedTokenWithIncorrectTrustedTvmClientIdAllowNoCheck() {
        assertEquals(HttpStatus.OK, requestWithTrustedToken("noCheck", NOT_ALLOWED_SERVICE_TVM_CLIENT_ID));
    }

    @Test
    void testRequestWithTrustedTokenWithIncorrectTrustedTvmClientIdAllowAll() {
        assertEquals(HttpStatus.OK, requestWithTrustedToken("allowAll", NOT_ALLOWED_SERVICE_TVM_CLIENT_ID));
    }

    @Test
    void testRequestWithTrustedTokenWithTrustedTvmClientIdAllowOnly() {
        assertEquals(HttpStatus.OK, requestWithTrustedToken("allowOnly", ALLOWED_SERVICE_TVM_CLIENT_ID));
    }

    @Test
    void testRequestWithTrustedTokenWithIncorrectTrustedTvmClientIdAllowOnly() {
        assertEquals(HttpStatus.FORBIDDEN, requestWithTrustedToken("allowOnly", NOT_ALLOWED_SERVICE_TVM_CLIENT_ID));
    }

    @Test
    void testRequest2AllowAllWithUserNoTicket() {
        assertEquals(HttpStatus.FORBIDDEN, request2("allowAllWithUser", ALLOWED_TICKET, null));
    }

    @Test
    void testRequest2AllowAllWithUserInvalidTicket() {
        assertEquals(HttpStatus.FORBIDDEN, request2("allowAllWithUser", ALLOWED_TICKET, INVALID_USER_TICKET));
    }

    @Test
    void testRequest2AllowAllWithUserCorrectTicket() {
        assertEquals(HttpStatus.OK, request2("allowAllWithUser", ALLOWED_TICKET, ALLOWED_USER_TICKET));
    }

    @Test
    void testRequest2AllowedOnAllowAllWithUser() {
        assertEquals(HttpStatus.OK, request2("allowAll", ALLOWED_TICKET, ALLOWED_USER_TICKET));
    }


    private HttpStatus request(String method, @Nullable String ticket) {
        return request(method, ticket, null);
    }

    private HttpStatus requestWithoutTicket(String method) {
        return request(method, null);
    }

    private HttpStatus request(String method, @Nullable String ticket, @Nullable String userTicket) {
        var headers = new HttpHeaders();
        if (ticket != null) {
            headers.add(SecurityHeaders.SERVICE_TICKET_HEADER, ticket);
        }
        if (userTicket != null) {
            headers.add(SecurityHeaders.USER_TICKET_HEADER, userTicket);
        }
        return restTemplate.exchange("http://localhost:" + port + "/tvmTest/" + method,
                HttpMethod.GET,
                new HttpEntity<>(headers),
                String.class)
                .getStatusCode();
    }

    private HttpStatus request2(String method, @Nullable String ticket, @Nullable String userTicket) {
        var headers = new HttpHeaders();
        if (ticket != null) {
            headers.add(SecurityHeaders.SERVICE_TICKET_HEADER, ticket);
        }
        if (userTicket != null) {
            headers.add(SecurityHeaders.USER_TICKET_HEADER, userTicket);
        }
        return restTemplate.exchange("http://localhost:" + port + "/tvmTest2/" + method,
                HttpMethod.GET,
                new HttpEntity<>(headers),
                String.class)
                .getStatusCode();
    }

    private HttpStatus requestWithTrustedToken(String method, @Nullable Integer trustedTvmClientId) {
        var headers = new HttpHeaders();
        headers.add(SecurityHeaders.X_DEVELOPER_TRUSTED_TOKEN, TRUSTED_TOKEN);
        if (trustedTvmClientId != null) {
            headers.add(SecurityHeaders.X_TRUSTED_SERVICE_TVM_CLIENT_ID, trustedTvmClientId + "");
        }
        return restTemplate.exchange("http://localhost:" + port + "/tvmTest/" + method,
                HttpMethod.GET,
                new HttpEntity<>(headers),
                String.class)
                .getStatusCode();
    }

    @Configuration
    @EnableAutoConfiguration
    static class TestConfig implements WebMvcConfigurer {

        private final TvmClient tvmClient;

        TestConfig(TvmClient tvmClient) {
            this.tvmClient = tvmClient;
        }

        @Override
        public void addInterceptors(InterceptorRegistry registry) {
            var interceptor = new TvmAuthorizationInterceptor(tvmClient,
                    Map.of("megamind", Set.of(ALLOWED_SERVICE_TVM_CLIENT_ID)), TRUSTED_TOKEN,
                    true, true);
            registry.addInterceptor(interceptor);
        }

        @Bean
        TestController testController() {
            return new TestController();
        }

        @Bean
        TestController2 testController2() {
            return new TestController2();
        }
    }

    @RestController
    private static class TestController {
        @GetMapping(path = "/tvmTest/allowOnly")
        @TvmRequired("megamind")
        private ResponseEntity<String> dummy() {
            return ResponseEntity.ok("{}");
        }

        @GetMapping(path = "/tvmTest/noCheck")
        private ResponseEntity<String> noCheck() {
            return ResponseEntity.ok("{}");
        }

        @GetMapping(path = "/tvmTest/allowAll")
        @TvmRequired()
        private ResponseEntity<String> allowAll() {
            return ResponseEntity.ok("{}");
        }

        @GetMapping(path = "/tvmTest/allowAllWithUser")
        @TvmRequired(userRequired = true)
        private ResponseEntity<String> allowAllWithUser() {
            return ResponseEntity.ok("{}");
        }
    }

    @RestController
    @TvmRequired
    private static class TestController2 {
        @GetMapping(path = "/tvmTest2/allowOnly")
        @TvmRequired("megamind")
        private ResponseEntity<String> dummy() {
            return ResponseEntity.ok("{}");
        }

        @GetMapping(path = "/tvmTest2/allowAll")
        private ResponseEntity<String> allowAll() {
            return ResponseEntity.ok("{}");
        }

        @GetMapping(path = "/tvmTest2/allowAllWithUser")
        @TvmRequired(userRequired = true)
        private ResponseEntity<String> allowAllWithUser() {
            return ResponseEntity.ok("{}");
        }
    }

}
