package ru.yandex.quasar.billing.providers.amediateka;

import java.util.List;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.RestClientTest;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.context.annotation.Import;
import org.springframework.http.MediaType;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;
import org.springframework.test.web.client.MockRestServiceServer;

import ru.yandex.quasar.billing.config.SecretsConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.providers.amediateka.model.Subscription;
import ru.yandex.quasar.billing.services.AuthorizationContext;

import static com.google.common.collect.Lists.newArrayList;
import static org.hamcrest.Matchers.containsString;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.requestTo;
import static org.springframework.test.web.client.response.MockRestResponseCreators.withSuccess;

@RestClientTest(AmediatekaAPI.class)
@SpringJUnitConfig(classes = {TestConfigProvider.class, AmediatekaAPITest.TestConfig.class})
@Disabled
class AmediatekaAPITest {


    @Autowired
    private MockRestServiceServer server;

    @MockBean
    private SecretsConfig secretsConfig;

    @Autowired
    private AmediatekaAPI amediatekaAPI;

    @BeforeEach
    void setUp() {
        when(secretsConfig.getAmediatekaClientSecret()).thenReturn("42c988ca6d004d31b9f4c73207402bdb");
    }

    @Test
    void getActiveSubscriptionsState() {
        server.expect(requestTo(containsString("/user/subscriptions")))
                .andRespond(withSuccess("{\n" +
                        "    \"subscription\": {\n" +
                        "        \"status\": true,\n" +
                        "        \"subscription_id\": {\n" +
                        "            \"id\": 99341344,\n" +
                        "            \"customer_id\": 16771082,\n" +
                        "            \"bundle_id\": 2,\n" +
                        "            \"expires_at\": \"2019-06-08T19:09:36.000+03:00\",\n" +
                        "            \"created_at\": \"2018-05-14T19:09:35.000+03:00\",\n" +
                        "            \"updated_at\": \"2018-07-13T14:34:17.000+03:00\",\n" +
                        "            \"uid\": \"user_subscription_835a8c83-1ec2-4e0c-a1b1-e436c5ad14ae\",\n" +
                        "            \"migrated\": false\n" +
                        "        },\n" +
                        "        \"bundle_uid\": \"bundle_3e488794-f3e9-4c76-a502-bd3930e69bdf\",\n" +
                        "        \"period\": 310,\n" +
                        "        \"subscription_period\": 310,\n" +
                        "        \"demo_videos_remained\": 5\n" +
                        "    },\n" +
                        "    \"transactions\": [\n" +
                        "        {\n" +
                        "            \"type\": \"payment\",\n" +
                        "            \"date\": \"2018-05-14\",\n" +
                        "            \"price\": \"599.00 руб.\",\n" +
                        "            \"period\": 30\n" +
                        "        }\n" +
                        "    ]\n" +
                        "}", MediaType.APPLICATION_JSON));

        List<Subscription> subscriptions = amediatekaAPI.getActiveSubscriptionsState("session");
        List<Subscription> expected = newArrayList(new Subscription(true, "bundle_3e488794-f3e9-4c76-a502" +
                "-bd3930e69bdf", 310));
        assertEquals(expected, subscriptions);

    }

    @Test
    public void getActiveSubscriptionsStateMultiSubs() {
        server.expect(requestTo(containsString("/user/subscriptions")))
                .andRespond(withSuccess("{\n" +
                        "  \"subscription\": {\n" +
                        "    \"status\": true,\n" +
                        "    \"subscription_id\": {\n" +
                        "      \"id\": 99334082,\n" +
                        "      \"customer_id\": 17285914,\n" +
                        "      \"bundle_id\": 2,\n" +
                        "      \"expires_at\": \"2020-08-01T16:00:58.000+03:00\",\n" +
                        "      \"created_at\": \"2018-05-14T16:00:58.000+03:00\",\n" +
                        "      \"updated_at\": \"2018-07-25T11:13:57.000+03:00\",\n" +
                        "      \"uid\": \"user_subscription_a4f4c842-5c68-4b90-807b-a764561210cd\",\n" +
                        "      \"migrated\": false\n" +
                        "    },\n" +
                        "    \"bundle_uid\": \"bundle_3e488794-f3e9-4c76-a502-bd3930e69bdf\",\n" +
                        "    \"period\": 730,\n" +
                        "    \"subscription_period\": 730,\n" +
                        "    \"demo_videos_remained\": 5\n" +
                        "  },\n" +
                        "  \"subscriptions\": [\n" +
                        "    {\n" +
                        "      \"status\": true,\n" +
                        "      \"subscription_id\": 99334082,\n" +
                        "      \"bundle_uid\": \"bundle_3e488794-f3e9-4c76-a502-bd3930e69bdf\",\n" +
                        "      \"period\": 730,\n" +
                        "      \"subscription_period\": 730,\n" +
                        "      \"demo_videos_remained\": 5\n" +
                        "    },\n" +
                        "    {\n" +
                        "      \"status\": true,\n" +
                        "      \"subscription_id\": 99873056,\n" +
                        "      \"bundle_uid\": \"bundle_c557d5cf-b47e-4755-a970-e5b73ae70982\",\n" +
                        "      \"period\": 81,\n" +
                        "      \"subscription_period\": 81,\n" +
                        "      \"demo_videos_remained\": 5\n" +
                        "    }\n" +
                        "  ],\n" +
                        "  \"transactions\": [\n" +
                        "    {\n" +
                        "      \"type\": \"payment\",\n" +
                        "      \"date\": \"2018-05-14\",\n" +
                        "      \"price\": \"599.00 руб.\",\n" +
                        "      \"period\": 30\n" +
                        "    }\n" +
                        "  ]\n" +
                        "}", MediaType.APPLICATION_JSON));

        List<Subscription> subscriptions = amediatekaAPI.getActiveSubscriptionsState("session");
        List<Subscription> expected = newArrayList(
                new Subscription(true, "bundle_3e488794-f3e9-4c76-a502-bd3930e69bdf", 730),
                new Subscription(true, "bundle_c557d5cf-b47e-4755-a970-e5b73ae70982", 81)
        );
        assertEquals(expected, subscriptions);

    }

    @Test
    void getActiveSubscriptionsStateNoSubscriptions() {
        server.expect(requestTo(containsString("/user/subscriptions")))
                .andRespond(withSuccess("{\"transactions\":[]}", MediaType.APPLICATION_JSON));

        List<Subscription> subscriptions = amediatekaAPI.getActiveSubscriptionsState("session");
        List<Subscription> expected = newArrayList();
        assertEquals(expected, subscriptions);

    }

    @TestConfiguration
    @Import({
            AmediatekaContentProvider.class,
            AmediatekaBundlesCache.class,
            AmediatekaAPI.class,
            AuthorizationContext.class
    })
    public static class TestConfig {

    }


}
