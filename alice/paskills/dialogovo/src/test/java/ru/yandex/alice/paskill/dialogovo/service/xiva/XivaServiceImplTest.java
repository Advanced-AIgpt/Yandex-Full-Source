package ru.yandex.alice.paskill.dialogovo.service.xiva;

import java.util.List;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.collect.Lists;
import org.json.JSONArray;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.test.web.client.MockRestServiceServer;
import org.springframework.web.client.RestTemplate;

import ru.yandex.alice.paskill.dialogovo.config.SecretsConfig;
import ru.yandex.alice.paskill.dialogovo.config.XivaConfig;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;
import ru.yandex.alice.paskill.dialogovo.processor.DirectiveToDialogUriConverter;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.AccountLinkingCompleteDirective;
import ru.yandex.alice.paskill.dialogovo.utils.executor.TestExecutorsFactory;

import static org.hamcrest.Matchers.containsString;
import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.startsWith;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.content;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.jsonPath;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.method;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.queryParam;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.requestTo;
import static org.springframework.test.web.client.response.MockRestResponseCreators.withSuccess;

class XivaServiceImplTest {

    private static final String TEST_XIVA_URL = "https://push-sandbox.yandex.ru";
    // example from https://push.yandex-team.ru/doc/guide.html#api-reference-list
    private static final String LIST_SUBSCRIPTIONS_RESPONSE = "[\n" +
            "   {\n" +
            "      \"client\" : \"test_client\",\n" +
            "      \"filter\" : \"\",\n" +
            "      \"id\" : \"870a4d3070ff6319119946ef3e25049953fc2c87\",\n" +
            "      \"session\" : \"session_abcde\",\n" +
            "      \"ttl\" : 8,\n" +
            "      \"url\" : \"http://sample/callback\"\n" +
            "   },\n" +
            "   {\n" +
            "      \"client\" : \"mobile\",\n" +
            "      \"extra\" : \"\",\n" +
            "      \"filter\" : \"\",\n" +
            "      \"id\" : \"mob:870a4d3070ff6319119946ef3e25049953fc2c88\",\n" +
            "      \"platform\" : \"gcm\",\n" +
            "      \"session\" : \"test_session\",\n" +
            "      \"ttl\" : 31536000,\n" +
            "      \"uuid\" : \"test_uuid\"\n" +
            "   }\n" +
            "]";
    private static final String DATA = "{\"item\":{\"actors\":\"\",\"available\":1,\"cover_url_16x9\":\"http://thumbs" +
            ".dfs.ivi.ru/storage36/contents/b/6/b4d6e5b40ea61ce4f04bef3e94bbae.jpg/1920x1080/\"," +
            "\"cover_url_2x3\":\"http://thumbs.dfs.ivi.ru/storage17/contents/6/4/16e1649e00167805728020bfd3353c" +
            ".jpg?ivi_poster=1/329x506/\",\"description\":\"\",\"directors\":\"\",\"duration\":5452," +
            "\"genre\":\"Приключение\",\"human_readable_id\":\"\",\"misc_ids\":{\"kinopoisk\":\"968043\"}," +
            "\"name\":\"Кролик Питер\",\"price_from\":99,\"provider_info\":[{\"available\":1," +
            "\"provider_item_id\":\"172366\",\"provider_name\":\"ivi\",\"type\":\"movie\"}]," +
            "\"provider_item_id\":\"172366\",\"provider_name\":\"ivi\",\"rating\":6.85,\"release_year\":2018," +
            "\"relevance\":105392568,\"relevance_prediction\":0.1272397106,\"seasons_count\":0," +
            "\"thumbnail_url_2x3_small\":\"http://thumbs.dfs.ivi" +
            ".ru/storage17/contents/6/4/16e1649e00167805728020bfd3353c.jpg?ivi_poster=1/88x135/\",\"type\":\"movie\"," +
            "\"unauthorized\":0}}";
    private XivaServiceImpl xivaClient;
    private MockRestServiceServer mockServer;
    private DirectiveToDialogUriConverter converter;

    @BeforeEach
    void setUp() {
        RestTemplate restTemplate = new RestTemplate();
        mockServer = MockRestServiceServer.createServer(restTemplate);
        converter = new DirectiveToDialogUriConverter(new ObjectMapper(), new DialogovoRequestContext());
        SecretsConfig secretsConfig = mock(SecretsConfig.class);
        when(secretsConfig.getXivaToken()).thenReturn("token");
        xivaClient = new XivaServiceImpl(
                new XivaConfig(TEST_XIVA_URL, 5000, 5000),
                secretsConfig,
                restTemplate,
                TestExecutorsFactory.newSingleThreadExecutor(), converter);
    }

    @AfterEach
    void tearDown() {
        mockServer.reset();
    }

    @Test
    void testGetSubscriptionList() {
        // given
        mockServer.expect(requestTo(startsWith(TEST_XIVA_URL)))
                .andExpect(requestTo(containsString("/list")))
                .andRespond(withSuccess(LIST_SUBSCRIPTIONS_RESPONSE, MediaType.APPLICATION_JSON));

        // when
        List<XivaSubscriptionInfo> actual = xivaClient.listSubscriptions("testuid", "serviceName");


        // then
        List<XivaSubscriptionInfo> expected = Lists.newArrayList(
                new XivaSubscriptionInfo("870a4d3070ff6319119946ef3e25049953fc2c87", "test_client", "session_abcde",
                        null, 8),
                new XivaSubscriptionInfo("mob:870a4d3070ff6319119946ef3e25049953fc2c88", "mobile", "test_session",
                        "test_uuid", 31536000)
        );
        assertEquals(expected, actual);

    }

    @Test
    void testGetSubscriptionListonEmptyResponseBody() {
        // given
        mockServer.expect(requestTo(startsWith(TEST_XIVA_URL)))
                .andExpect(requestTo(containsString("/list")))
                .andRespond(withSuccess());

        // when
        List<XivaSubscriptionInfo> actual = xivaClient.listSubscriptions("testuid", "wrongServiceName");

        // then
        List<XivaSubscriptionInfo> expected = Lists.newArrayList();
        assertEquals(expected, actual);

    }

    @Test
    void testGetSubscriptionListOnEmptyJsonArray() {
        mockServer.expect(requestTo(startsWith(TEST_XIVA_URL)))
                .andExpect(requestTo(containsString("/list")))
                .andRespond(withSuccess(new JSONArray().toString(), MediaType.APPLICATION_JSON));

        List<XivaSubscriptionInfo> actual = xivaClient.listSubscriptions("testuid", "serviceName");


        List<XivaSubscriptionInfo> expected = Lists.newArrayList();
        assertEquals(expected, actual);

    }

    @Test
    void testSendPush() {
        mockServer.expect(requestTo(startsWith(TEST_XIVA_URL)))
                .andExpect(requestTo(containsString("/send")))
                .andExpect(method(HttpMethod.POST))
                .andExpect(queryParam("user", "uid"))
                .andExpect(queryParam("event", "server_action"))
                .andExpect(content().contentType(MediaType.APPLICATION_JSON))
                .andExpect(jsonPath("$.payload").exists())
                .andExpect(jsonPath("$.payload.name").value(equalTo("external_skill__account_linking_complete")))
                .andExpect(jsonPath("$.payload.type").value(equalTo("server_action")))
                .andExpect(jsonPath("$.payload.payload.skill_id").value(equalTo("skill-id")))
                .andExpect(jsonPath("$.subscriptions[0].subscription_id").value(
                        "870a4d3070ff6319119946ef3e25049953fc2c87"))
                .andRespond(withSuccess());

        var directive = converter.wrapCallbackDirective(
                new AccountLinkingCompleteDirective("skill-id", null)
        );
        xivaClient.sendPush("uid", "server_action", "870a4d3070ff6319119946ef3e25049953fc2c87", null, directive);
        mockServer.verify();
    }

}
