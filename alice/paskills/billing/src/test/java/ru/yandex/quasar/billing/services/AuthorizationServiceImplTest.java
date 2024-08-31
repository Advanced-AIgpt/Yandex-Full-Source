package ru.yandex.quasar.billing.services;

import java.net.URI;
import java.util.Optional;

import javax.servlet.http.HttpServletRequest;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.context.annotation.Bean;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.test.web.client.MockRestServiceServer;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.passport.tvmauth.TicketStatus;
import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.passport.tvmauth.Unittest;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.exception.HTTPExceptionHandler;
import ru.yandex.quasar.billing.exception.UnauthorizedException;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;
import ru.yandex.quasar.billing.util.CSRFHelper;

import static org.hamcrest.Matchers.emptyOrNullString;
import static org.hamcrest.Matchers.not;
import static org.hamcrest.core.StringStartsWith.startsWith;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.when;
import static org.springframework.http.MediaType.APPLICATION_JSON;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.header;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.method;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.queryParam;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.requestTo;
import static org.springframework.test.web.client.response.MockRestResponseCreators.withSuccess;
import static ru.yandex.quasar.billing.filter.HeaderModifierFilter.HEADER_X_CSRF_TOKEN;

@AutoConfigureWebClient(registerRestTemplate = true)
@SpringBootTest(classes = {TestConfigProvider.class, AuthorizationServiceImplTest.TestConfig.class},
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
class AuthorizationServiceImplTest {

    private static final String UID = "123123";
    private static final String TEST_PROVIDER_SOCIAL_NAME = "testProviderSocialName";
    private static final String TOKEN = "TEST_TOKEN_VALUE";
    private static final String TOKEN_ID = "305115407";
    private static final String SOCIAL_RESPONSE = "{\n" +
            "  \"token\": {\n" +
            "    \"application\": \"" + TEST_PROVIDER_SOCIAL_NAME + "\", \n" +
            "    \"confirmed\": \"2018-07-30 10:36:20\", \n" +
            "    \"confirmed_ts\": 1532936180, \n" +
            "    \"created\": \"2018-07-30 10:36:20\", \n" +
            "    \"created_ts\": 1532936180, \n" +
            "    \"expired\": \"2019-01-29 10:36:13\", \n" +
            "    \"expired_ts\": 1548747373, \n" +
            "    \"profile_id\": null, \n" +
            "    \"scope\": \"\", \n" +
            "    \"secret\": null, \n" +
            "    \"token_id\": " + TOKEN_ID + ", \n" +
            "    \"uid\": 605289217, \n" +
            "    \"value\": \"TEST_TOKEN_VALUE\", \n" +
            "    \"verified\": \"2018-07-30 10:36:20\", \n" +
            "    \"verified_ts\": 1532936180\n" +
            "  }\n" +
            "}";
    private static final String BLACKBOX_RESPONSE = "{\n" +
            "  \"oauth\": {\n" +
            "    \"uid\": \"605289217\",\n" +
            "    \"token_id\": \"794189046\",\n" +
            "    \"device_id\": \"\",\n" +
            "    \"device_name\": \"\",\n" +
            "    \"scope\": \"login:email login:info music:read music:write quasar:all\",\n" +
            "    \"ctime\": \"2018-03-23 12:43:45\",\n" +
            "    \"issue_time\": \"2018-07-26 18:18:35\",\n" +
            "    \"expire_time\": \"2019-07-26 18:18:35\",\n" +
            "    \"is_ttl_refreshable\": true,\n" +
            "    \"client_id\": \"client_id\",\n" +
            "    \"client_name\": \"Quasar\",\n" +
            "    \"client_icon\": \"https:\\/\\/avatars.mds.yandex" +
            ".net\\/get-oauth\\/40350\\/0f7488e7bfdf49be85c64158f2b67c6c-af12db23725d4144b9571c7e59e2141d\\/normal\"," +
            "\n" +
            "    \"client_homepage\": \"\",\n" +
            "    \"client_ctime\": \"2017-02-28 18:00:47\",\n" +
            "    \"client_is_yandex\": true,\n" +
            "    \"xtoken_id\": \"794188879\",\n" +
            "    \"meta\": \"\"\n" +
            "  },\n" +
            "  \"uid\": {\n" +
            "    \"value\": \"605289217\",\n" +
            "    \"lite\": false,\n" +
            "    \"hosted\": false\n" +
            "  },\n" +
            "  \"login\": \"yndx-quasar-test4\",\n" +
            "  \"have_password\": true,\n" +
            "  \"have_hint\": true,\n" +
            "  \"karma\": {\n" +
            "    \"value\": 0\n" +
            "  },\n" +
            "  \"karma_status\": {\n" +
            "    \"value\": 0\n" +
            "  },\n" +
            "  \"user_ticket\": \"user_ticket\",\n" +
            "  \"status\": {\n" +
            "    \"value\": \"VALID\",\n" +
            "    \"id\": 0\n" +
            "  },\n" +
            "  \"error\": \"OK\",\n" +
            "  \"connection_id\": \"t:794189046\"\n" +
            "}";
    @MockBean
    private UnistatService unistatService;
    @Autowired
    private AuthorizationServiceImpl authorizationService;
    private MockRestServiceServer mockServer;
    @Autowired
    private BillingConfig billingConfig;
    @MockBean
    private TvmClient tvmClient;
    @Autowired
    private TestRestTemplate testRestTemplate;
    @Autowired
    private AuthorizationContext context;
    @LocalServerPort
    private int port;

    @BeforeEach
    void setUp() {
        mockServer = MockRestServiceServer.bindTo(authorizationService.restTemplate()).bufferContent().build();
        when(tvmClient.checkServiceTicket(anyString()))
                .thenReturn(Unittest.createServiceTicket(TicketStatus.MALFORMED, 0));
        when(tvmClient.checkServiceTicket(eq("service_ticket")))
                .thenReturn(Unittest.createServiceTicket(TicketStatus.OK, 1));
        when(tvmClient.checkUserTicket(anyString()))
                .thenReturn(Unittest.createUserTicket(TicketStatus.MALFORMED, 1, new String[0], new long[0]));
        when(tvmClient.checkUserTicket(eq("user_token")))
                .thenReturn(Unittest.createUserTicket(TicketStatus.OK, Long.parseLong(UID), new String[0],
                        new long[0]));
        when(tvmClient.getServiceTicketFor("blackbox"))
                .thenReturn("service_ticket_for_blackbox");
        context.clearUserContext();
    }

    @Test
    void getProviderTokenByUid() {
        mockSocial();
        Optional<String> providerTokenByUid =
                authorizationService.getProviderTokenByUid(UID, TEST_PROVIDER_SOCIAL_NAME);

        assertEquals(TOKEN, providerTokenByUid.orElse(null));
    }

    @Test
    void getProviderTokenByUidForKinopoisk() {
        mockSocial();
        Optional<String> providerTokenByUid = authorizationService.getProviderTokenByUid(UID, "yandex-kinopoisk");

        assertEquals("dummy", providerTokenByUid.orElse(null));
    }

    @Test
    void testWrongOAuthToken() {
        mockServer.expect(requestTo(startsWith(billingConfig.getSocialAPIClientConfig().getBlackboxBaseUrl())))
                .andExpect(header("Authorization", startsWith("OAuth ")))
                .andRespond(withSuccess("{\"status\":{\"value\":\"INVALID\",\"id\":5},\"error\":\"expired_token\"}",
                        APPLICATION_JSON));

        assertThrows(UnauthorizedException.class, () -> authorizationService.getUidAndTicket("WORONG_TOKEN", null,
                "192.168.0.1", null, null, null, false));
    }

    @Test
    void testGoodOAuthToken() {
        String response = "{\"oauth\":{\"uid\":\"602190738\",\"token_id\":\"123\",\"device_id\":\"\"," +
                "\"device_name\":\"\",\"scope\":\"login:email login:info music:read music:write quasar:all\"," +
                "\"ctime\":\"2018-02-26 15:34:48\",\"issue_time\":\"2018-05-31 18:09:06\"," +
                "\"expire_time\":\"2019-05-31 18:09:06\",\"is_ttl_refreshable\":true,\"client_id\":\"qweqweqwe\"," +
                "\"client_name\":\"Quasar\",\"client_icon\":\"https:\\/\\/avatars.mds.yandex" +
                ".net\\/get-oauth\\/40350\\/qweqweqwe-asdasdasd\\/normal\",\"client_homepage\":\"\"," +
                "\"client_ctime\":\"2017-02-28 18:00:47\",\"client_is_yandex\":true,\"xtoken_id\":\"772320000\"," +
                "\"meta\":\"\"},\"uid\":{\"value\":\"602190738\",\"lite\":false,\"hosted\":false}," +
                "\"login\":\"yndx-quasar-test2\",\"have_password\":true,\"have_hint\":true,\"karma\":{\"value\":0}," +
                "\"karma_status\":{\"value\":0},\"status\":{\"value\":\"VALID\",\"id\":0},\"error\":\"OK\"," +
                "\"connection_id\":\"t:123\"}";
        String goodToken = "DUMMY_TOKEN";
        mockServer.expect(requestTo(startsWith(billingConfig.getSocialAPIClientConfig().getBlackboxBaseUrl())))
                .andExpect(header("Authorization", "OAuth " + goodToken))
                .andRespond(withSuccess(response, APPLICATION_JSON));

        AuthorizationServiceImpl.UserCredential uid = authorizationService.getUidAndTicket(goodToken, null, "192.168" +
                ".0.1", null, null, null, false);
        assertEquals("602190738", uid.getUid());
    }

    @Test
    void testGoodTokenBlackbox() {
        // given
        String goodToken = "DUMMY_TOKEN";
        mockServer.expect(requestTo(startsWith(billingConfig.getSocialAPIClientConfig().getBlackboxBaseUrl())))
                .andExpect(method(HttpMethod.GET))
                .andExpect(queryParam("method", "oauth"))
                .andExpect(queryParam("userip", not(emptyOrNullString())))
                .andExpect(queryParam("format", "json"))
                .andExpect(queryParam("get_user_ticket", "yes"))
                .andExpect(header("Authorization", "OAuth " + goodToken))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, "service_ticket_for_blackbox"))
                .andRespond(withSuccess(BLACKBOX_RESPONSE, APPLICATION_JSON));

        // then
        var headers = new HttpHeaders();
        headers.add("Authorization", "OAuth " + goodToken);
        ResponseEntity<AuthorizationContext.UserContext> response = testRestTemplate.exchange(URI.create("http" +
                        "://localhost:" + port + "/test_context"), HttpMethod.GET, new HttpEntity<>(headers),
                AuthorizationContext.UserContext.class);

        assertEquals("605289217", response.getBody().getUid());
        assertEquals("user_ticket", response.getBody().getUserTicket());
    }

    @Test
    void testGoodSessionBlackbox() {
        // given
        String goodSession = "good_session";
        mockServer.expect(requestTo(startsWith(billingConfig.getSocialAPIClientConfig().getBlackboxBaseUrl())))
                .andExpect(method(HttpMethod.GET))
                .andExpect(queryParam("method", "sessionid"))
                .andExpect(queryParam("userip", not(emptyOrNullString())))
                .andExpect(queryParam("format", "json"))
                .andExpect(queryParam("sessionid", goodSession))
                .andExpect(queryParam("get_user_ticket", "yes"))
                .andExpect(queryParam("host", "yandex.ru"))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, "service_ticket_for_blackbox"))
                .andRespond(withSuccess(BLACKBOX_RESPONSE, APPLICATION_JSON));

        // then
        var headers = new HttpHeaders();
        headers.add(HEADER_X_CSRF_TOKEN, getValidCsrfToken("605289217"));
        headers.add("Cookie", "Session_id=" + goodSession);
        headers.add(HttpHeaders.HOST, "yandex.ru");

        ResponseEntity<AuthorizationContext.UserContext> response = testRestTemplate.exchange(URI.create("http" +
                        "://localhost:" + port + "/test_context"), HttpMethod.GET, new HttpEntity<>(headers),
                AuthorizationContext.UserContext.class);

        assertEquals("605289217", response.getBody().getUid());
        assertEquals("user_ticket", response.getBody().getUserTicket());
    }

    @Test
    void testGoodSessionBlackboxTld() {
        // given
        String goodSession = "good_session";
        var host = "quasar.yandex.com";
        mockServer.expect(requestTo(startsWith(billingConfig.getSocialAPIClientConfig().getBlackboxBaseUrl())))
                .andExpect(method(HttpMethod.GET))
                .andExpect(queryParam("method", "sessionid"))
                .andExpect(queryParam("userip", not(emptyOrNullString())))
                .andExpect(queryParam("format", "json"))
                .andExpect(queryParam("sessionid", goodSession))
                .andExpect(queryParam("get_user_ticket", "yes"))
                .andExpect(queryParam("host", host))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, "service_ticket_for_blackbox"))
                .andRespond(withSuccess(BLACKBOX_RESPONSE, APPLICATION_JSON));

        // then
        HttpHeaders headers = new HttpHeaders();
        headers.add(HEADER_X_CSRF_TOKEN, getValidCsrfToken("605289217"));
        headers.add(HttpHeaders.HOST, host);
        headers.add("Cookie", "Session_id=" + goodSession);


        ResponseEntity<AuthorizationContext.UserContext> response = testRestTemplate.exchange(URI.create("http" +
                        "://localhost:" + port + "/test_context"), HttpMethod.GET, new HttpEntity<>(headers),
                AuthorizationContext.UserContext.class);

        assertEquals("605289217", response.getBody().getUid());
        assertEquals("user_ticket", response.getBody().getUserTicket());
    }

    @Test
    void tvmAuthentication() {
        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        headers.add("X-Ya-Service-Ticket", "service_ticket");
        headers.add("X-Ya-User-Ticket", "user_token");

        ResponseEntity<AuthorizationContext.UserContext> response = testRestTemplate.exchange(URI.create("http" +
                        "://localhost:" + port + "/test_context"), HttpMethod.GET, new HttpEntity<>(headers),
                AuthorizationContext.UserContext.class);

        assertEquals(UID, response.getBody().getUid());
        assertEquals("user_token", response.getBody().getUserTicket());
    }

    @Test
    void tvmAuthenticationNoServiceTicket() {
        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        // headers.add("X-Ya-Service-Ticket", "user_token"); - request without service ticket
        headers.add("X-Ya-User-Ticket", "user_token");

        ResponseEntity<String> response = testRestTemplate.exchange(URI.create("http://localhost:" + port +
                "/test_context"), HttpMethod.GET, new HttpEntity<>(headers), String.class);

        assertEquals(HttpStatus.UNAUTHORIZED, response.getStatusCode());
    }

    @Test
    void tvmAuthenticationWrongServiceTicket() {
        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        headers.add("X-Ya-Service-Ticket", "wrong_service_ticket");
        headers.add("X-Ya-User-Ticket", "user_token");

        ResponseEntity<String> response = testRestTemplate.exchange(URI.create("http://localhost:" + port +
                "/test_context"), HttpMethod.GET, new HttpEntity<>(headers), String.class);

        assertEquals(HttpStatus.FORBIDDEN, response.getStatusCode());
    }

    @Test
    void tvmAuthenticationWrongUserTicket() {
        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        headers.add("X-Ya-Service-Ticket", "service_ticket");
        headers.add("X-Ya-User-Ticket", "wrong_user_token");

        ResponseEntity<String> response = testRestTemplate.exchange(URI.create("http://localhost:" + port +
                "/test_context"), HttpMethod.GET, new HttpEntity<>(headers), String.class);

        assertEquals(HttpStatus.FORBIDDEN, response.getStatusCode());
    }

    @Test
    void testCorrectCsrf() {
        String csrfToken = getValidCsrfToken("605289217");

        String session = "session";
        mockServer.expect(requestTo(startsWith(billingConfig.getSocialAPIClientConfig().getBlackboxBaseUrl())))
                .andExpect(method(HttpMethod.GET))
                .andExpect(queryParam("method", "sessionid"))
                .andExpect(queryParam("userip", not(emptyOrNullString())))
                .andExpect(queryParam("format", "json"))
                .andExpect(queryParam("sessionid", session))
                .andExpect(queryParam("get_user_ticket", "yes"))
                .andExpect(queryParam("host", "yandex.ru"))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, "service_ticket_for_blackbox"))
                .andRespond(withSuccess(BLACKBOX_RESPONSE, APPLICATION_JSON));

        var headers = new HttpHeaders();
        headers.add(HttpHeaders.HOST, "yandex.ru");
        headers.add("Cookie", "Session_id=" + session + ";yandexuid=605289217");
        headers.add(HEADER_X_CSRF_TOKEN, csrfToken);


        ResponseEntity<String> response = testRestTemplate.exchange(URI.create("http://localhost:" + port +
                "/test_auth"), HttpMethod.POST, new HttpEntity<>(headers), String.class);

        assertEquals("605289217", response.getBody());
    }

    @Test
    void testIncorrectCsrf() {
        // expired token
        long till = System.currentTimeMillis() / 1000 - 24 * 3600 * 2;
        String csrfToken = new CSRFHelper(TestConfigProvider.CSRF_TOKEN.getBytes())
                .generate("605289217", "605289217", till);

        String session = "session";
        mockServer.expect(requestTo(startsWith(billingConfig.getSocialAPIClientConfig().getBlackboxBaseUrl())))
                .andExpect(method(HttpMethod.GET))
                .andExpect(queryParam("method", "sessionid"))
                .andExpect(queryParam("userip", not(emptyOrNullString())))
                .andExpect(queryParam("format", "json"))
                .andExpect(queryParam("sessionid", session))
                .andExpect(queryParam("get_user_ticket", "yes"))
                .andExpect(queryParam("host", "yandex.ru"))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, "service_ticket_for_blackbox"))
                .andRespond(withSuccess(BLACKBOX_RESPONSE, APPLICATION_JSON));

        var headers = new HttpHeaders();
        headers.add(HttpHeaders.HOST, "yandex.ru");
        headers.add("Cookie", "Session_id=" + session + ";yandexuid=605289217");
        headers.add(HEADER_X_CSRF_TOKEN, csrfToken);


        ResponseEntity<HTTPExceptionHandler.ErrorInfo> response = testRestTemplate.exchange(URI.create("http" +
                        "://localhost:" + port + "/test_auth"), HttpMethod.POST, new HttpEntity<>(headers),
                HTTPExceptionHandler.ErrorInfo.class);

        assertEquals(HttpStatus.FORBIDDEN, response.getStatusCode());
        assertEquals("Invalid x-csrf-token", response.getBody().getMessage());
    }

    @Test
    void testCsrfMissing() {

        String session = "session";
        mockServer.expect(requestTo(startsWith(billingConfig.getSocialAPIClientConfig().getBlackboxBaseUrl())))
                .andExpect(method(HttpMethod.GET))
                .andExpect(queryParam("method", "sessionid"))
                .andExpect(queryParam("userip", not(emptyOrNullString())))
                .andExpect(queryParam("format", "json"))
                .andExpect(queryParam("sessionid", session))
                .andExpect(queryParam("get_user_ticket", "yes"))
                .andExpect(queryParam("host", "yandex.ru"))
                .andExpect(header(TvmHeaders.SERVICE_TICKET_HEADER, "service_ticket_for_blackbox"))
                .andRespond(withSuccess(BLACKBOX_RESPONSE, APPLICATION_JSON));

        var headers = new HttpHeaders();
        headers.add(HttpHeaders.HOST, "yandex.ru");
        headers.add("Cookie", "Session_id=" + session);

        ResponseEntity<HTTPExceptionHandler.ErrorInfo> response = testRestTemplate.exchange(URI.create("http" +
                        "://localhost:" + port + "/test_auth"), HttpMethod.POST, new HttpEntity<>(headers),
                HTTPExceptionHandler.ErrorInfo.class);

        assertEquals(HttpStatus.FORBIDDEN, response.getStatusCode());
        assertEquals("Invalid x-csrf-token", response.getBody().getMessage());
    }

    @Test
    void testHasPlusTrue() {
        // given
        mockServer.expect(requestTo(startsWith("https://localhost/blackbox")))
                .andExpect(queryParam("method", "userinfo"))
                .andExpect(queryParam("userip", not(emptyOrNullString())))
                .andExpect(queryParam("format", "json"))
                .andExpect(queryParam("uid", UID))
                .andExpect(queryParam("attributes", "1015"))
                .andRespond(withSuccess("{\"users\":[{\"id\":\"605289217\",\"uid\":{\"value\":\"605289217\"," +
                        "\"lite\":false,\"hosted\":false},\"login\":\"yndx-quasar-test4\",\"have_password\":true," +
                        "\"have_hint\":true,\"karma\":{\"value\":0},\"karma_status\":{\"value\":0}," +
                        "\"attributes\":{\"1015\":\"1\"}}]}", APPLICATION_JSON));
        context.setCurrentUid(UID);
        context.setUserIp("1.1.1.1");

        // then
        assertTrue(authorizationService.userHasPlus());
    }

    @Test
    void testHasPlusBBFailure() {
        // given
        mockServer.expect(requestTo(startsWith("https://localhost/blackbox")))
                .andExpect(queryParam("method", "userinfo"))
                .andExpect(queryParam("userip", not(emptyOrNullString())))
                .andExpect(queryParam("format", "json"))
                .andExpect(queryParam("uid", ""))
                .andExpect(queryParam("attributes", "1015"))
                .andRespond(withSuccess("{\"exception\":{\"value\":\"INVALID_PARAMS\",\"id\":2},\"error\":\"BlackBox " +
                        "error: Missing login argument\",\"request_id\":\"dc82eac19bd121fe\"}", APPLICATION_JSON));
        //context.setCurrentUid(uid);
        //context.setUserIp("1.1.1.1");

        // then
        assertFalse(authorizationService.userHasPlus());
    }

    @Test
    void testHasPlusFalse() {
        // given
        mockServer.expect(requestTo(startsWith("https://localhost/blackbox")))
                .andExpect(queryParam("method", "userinfo"))
                .andExpect(queryParam("userip", not(emptyOrNullString())))
                .andExpect(queryParam("format", "json"))
                .andExpect(queryParam("uid", UID))
                .andExpect(queryParam("attributes", "1015"))
                .andRespond(withSuccess("{\"users\":[{\"id\":\"602520544\",\"uid\":{\"value\":\"602520544\"," +
                        "\"lite\":false,\"hosted\":false},\"login\":\"yndx-quasar-test3\",\"have_password\":true," +
                        "\"have_hint\":true,\"karma\":{\"value\":0},\"karma_status\":{\"value\":0}," +
                        "\"attributes\":{}}]}", APPLICATION_JSON));
        context.setCurrentUid(UID);
        context.setUserIp("1.1.1.1");

        // then
        assertFalse(authorizationService.userHasPlus());
    }

    private String getValidCsrfToken(String uid) {
        long till = System.currentTimeMillis() / 1000;
        return new CSRFHelper(TestConfigProvider.CSRF_TOKEN.getBytes())
                .generate(uid, uid, till);
    }

    private void mockSocial() {
        mockServer.expect(requestTo(startsWith(billingConfig.getSocialAPIClientConfig().getSocialApiBaseUrl() +
                        "/token/newest")))
                .andExpect(method(HttpMethod.GET))
                .andExpect(queryParam("uid", UID))
                .andExpect(queryParam("application_name", TEST_PROVIDER_SOCIAL_NAME))
                .andRespond(withSuccess(SOCIAL_RESPONSE, APPLICATION_JSON));
    }

    @TestConfiguration
    static class TestConfig {
        @Bean
        TestController testController(@Autowired AuthorizationServiceImpl authorizationService,
                                      @Autowired AuthorizationContext context) {
            return new TestController(authorizationService, context);
        }
    }

    @RestController
    static class TestController {

        @Autowired
        private final AuthorizationServiceImpl authorizationService;
        @Autowired
        private final AuthorizationContext context;

        TestController(AuthorizationServiceImpl authorizationService, AuthorizationContext context) {
            this.authorizationService = authorizationService;
            this.context = context;
        }


        @RequestMapping(value = "test_auth", method = {RequestMethod.GET, RequestMethod.POST})
        public String testAuth(HttpServletRequest request) {
            String uid = authorizationService.getSecretUid(request);
            return uid;
        }

        @GetMapping("test_context")
        public AuthorizationContext.UserContext testContext(HttpServletRequest request) {
            context.clearUserContext();
            authorizationService.getSecretUid(request);
            return context.getUserContext();
        }
    }

}
