package ru.yandex.quasar.billing.providers.universal;

import java.time.Instant;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.hamcrest.Matchers;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.junit.jupiter.api.extension.RegisterExtension;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.http.MediaType;
import org.springframework.test.web.client.MockRestServiceServer;
import org.springframework.web.client.RestTemplate;

import ru.yandex.quasar.billing.RemoteServiceProxyExtension;
import ru.yandex.quasar.billing.RemoteServiceProxyMode;
import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.providers.ProviderActiveSubscriptionInfo;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.util.DisableSSLExtension;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.requestTo;
import static org.springframework.test.web.client.response.MockRestResponseCreators.withSuccess;

@ExtendWith(DisableSSLExtension.class)
@SpringBootTest(classes = TestConfigProvider.class)
@Disabled
class KinopoiskViaUniversalProviderTest {

    private static final String OTT_URL = "https://station.ott-testing.common.yandex.net/";
    private static final Logger logger = LogManager.getLogger();
    private static final String DUPLICATE_ANSWER = "{\n" +
            "  \"purchased_products\": [\n" +
            "    {\n" +
            "      \"product_id\": \"sub:YA_PLUS\",\n" +
            "      \"title\": \"Яндекс.Плюс\",\n" +
            "      \"product_type\": \"subscription\",\n" +
            "      \"is_content_item\": false,\n" +
            "      \"price\": {\n" +
            "        \"price_option_id\": \"sub:YA_PLUS\",\n" +
            "        \"user_price\": 1690,\n" +
            "        \"price\": 1690,\n" +
            "        \"currency\": \"RUB\",\n" +
            "        \"purchase_payload\": {\n" +
            "          \"billing_product_id\": \"ru.yandex.mobile.music.1year.autorenewable.plus.169\"\n" +
            "        },\n" +
            "        \"title\": \"Яндекс.Плюс\",\n" +
            "        \"purchase_type\": \"SUBSCRIPTION\",\n" +
            "        \"period\": \"P1Y\",\n" +
            "        \"processing\": \"MEDIABILLING\"\n" +
            "      },\n" +
            "      \"active_till\": \"2019-06-29T13:43:29Z\",\n" +
            "      \"renew_disabled\": false\n" +
            "    },\n" +
            "    {\n" +
            "      \"product_id\": \"sub:YA_PLUS\",\n" +
            "      \"title\": \"Яндекс.Плюс\",\n" +
            "      \"product_type\": \"subscription\",\n" +
            "      \"is_content_item\": false,\n" +
            "      \"price\": {\n" +
            "        \"price_option_id\": \"sub:YA_PLUS\",\n" +
            "        \"user_price\": 169,\n" +
            "        \"price\": 169,\n" +
            "        \"currency\": \"RUB\",\n" +
            "        \"purchase_payload\": {\n" +
            "          \"billing_product_id\": \"ru.yandex.mobile.music.1month.autorenewable.3month.trial.plus" +
            ".169\"\n" +
            "        },\n" +
            "        \"title\": \"Яндекс.Плюс\",\n" +
            "        \"purchase_type\": \"SUBSCRIPTION\",\n" +
            "        \"period\": \"P1M\",\n" +
            "        \"processing\": \"MEDIABILLING\"\n" +
            "      },\n" +
            "      \"active_till\": \"2019-08-29T13:43:29Z\",\n" +
            "      \"renew_disabled\": false\n" +
            "    },\n" +
            "    {\n" +
            "      \"product_id\": \"sub:YA_PLUS\",\n" +
            "      \"title\": \"Яндекс.Плюс\",\n" +
            "      \"product_type\": \"subscription\",\n" +
            "      \"is_content_item\": false,\n" +
            "      \"active_till\": \"2019-04-05T15:51:03Z\",\n" +
            "      \"renew_disabled\": false\n" +
            "    }\n" +
            "  ]\n" +
            "}";
    @RegisterExtension
    static RemoteServiceProxyExtension wiremock = new RemoteServiceProxyExtension(
            OTT_URL, RemoteServiceProxyMode.REPLAYING
    );
    private final AtomicInteger counter = new AtomicInteger();
    @Autowired
    private BillingConfig config;
    @Autowired
    private RestTemplate restTemplate;
    @Autowired
    private ObjectMapper objectMapper;
    @Autowired
    private AuthorizationContext authorizationContext;
    @SpyBean
    @Qualifier("kinopoiskContentProvider")
    private UniversalProvider provider;

    @Test
    void testContentMetaInfoFilm() {
        ContentMetaInfo contentMetaInfo = provider.getContentMetaInfo(ProviderContentItem.create(ContentType.MOVIE,
                "4e7bc3bdf4658ffd9916e0c5e0fffbdc"));
        System.out.println(contentMetaInfo);
    }

    @Test
    void testContentMetaInfoShow() {
        ContentMetaInfo contentMetaInfo = provider.getContentMetaInfo(ProviderContentItem.create(ContentType.TV_SHOW,
                "401c596a99a16cb3a382fed1738805ab"));
        System.out.println(contentMetaInfo);
    }

    @Test
    void testContentMetaInfoSeason() {
        ContentMetaInfo contentMetaInfo = provider.getContentMetaInfo(ProviderContentItem.createSeason(
                "4c1300e56f1bd1b5be41d13a6aab735c", "401c596a99a16cb3a382fed1738805ab"));
        System.out.println(contentMetaInfo);
    }

    @Test
    void testContentMetaInfoEpisode() {
        ContentMetaInfo contentMetaInfo = provider.getContentMetaInfo(ProviderContentItem.createEpisode(
                "4ff3beaf91f6099d8c6bacbfc089aebf", "4c1300e56f1bd1b5be41d13a6aab735c",
                "401c596a99a16cb3a382fed1738805ab"));
        System.out.println(contentMetaInfo);
    }

    @Test
    void getActiveSubscriptionsDeduplicate() {
        RestTemplate restTemplateTmp = ((UniversalProvider.Client) provider.getClient()).getRestTemplate();
        MockRestServiceServer server = MockRestServiceServer.bindTo(restTemplateTmp).bufferContent().build();

        server.expect(requestTo(Matchers.endsWith("purchased?product_type=SUBSCRIPTION")))
                .andRespond(withSuccess(DUPLICATE_ANSWER, MediaType.APPLICATION_JSON));

        Map<ProviderContentItem, ProviderActiveSubscriptionInfo> actual = provider.getActiveSubscriptions("session");

        Map<ProviderContentItem, ProviderActiveSubscriptionInfo> expected = Map.of(
                ProviderContentItem.create(ContentType.SUBSCRIPTION, "YA_PLUS"),
                ProviderActiveSubscriptionInfo.builder(ProviderContentItem.create(ContentType.SUBSCRIPTION, "YA_PLUS"))
                        .title("Яндекс.Плюс")
                        .activeTill(Instant.parse("2019-08-29T13:43:29Z"))
                        .build()
        );

        assertEquals(expected, actual);

    }

    @DisplayName("Download all content items and parse all their metaInfo")
    @Test
    void getAllContent() {
        AllContentItems allContentItems = provider.getClient().allContent();

        for (String id : allContentItems.getContentItems()) {
            fetchItemInfo(id);
        }
    }

    private void fetchItemInfo(String id) {
        int i = counter.incrementAndGet();
        ContentItemInfo item = provider.getClient().contentItemInfo(id, null);
        logger.info(i + " - " + id);
        logger.info(item.toString());
        item.getChildren().forEach(this::fetchItemInfo);
    }
}
