package ru.yandex.quasar.billing.services.sup;

import java.net.URI;
import java.net.URISyntaxException;
import java.nio.charset.StandardCharsets;
import java.util.Optional;

import org.apache.http.client.utils.URLEncodedUtils;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import ru.yandex.quasar.billing.beans.PricingOptionTestUtil;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.processing.yapay.YandexPayMerchantTestUtil;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static ru.yandex.quasar.billing.util.JsonUtil.toJsonQuotes;

@SpringJUnitConfig(classes = {MobilePushService.class, TestConfigProvider.class})
@AutoConfigureWebClient(registerRestTemplate = true)
public class MobilePushServiceTest implements PricingOptionTestUtil, YandexPayMerchantTestUtil {

    private static final String UID = "999";
    @Autowired
    private MobilePushService pushService;

    private static Optional<String> maybeQueryParam(URI uri, String name) {
        return URLEncodedUtils.parse(uri.getRawQuery(), StandardCharsets.UTF_8).stream()
                .filter(item -> item.getName().equals(name))
                .map(item -> Optional.ofNullable(item.getValue()))
                .findFirst().orElse(Optional.empty());
    }

    private static String queryParam(URI uri, String name) {
        return maybeQueryParam(uri, name).get();
    }

    @Test
    void getWrappedLinkUrlFull() {

        String url = pushService.getWrappedLinkUrl("https://yandex.ru/quasar/push?srcrwr=QUASAR_HOST:testing.quasar" +
                ".common.yandex.ru/&contentItem=%7B%22type%22:%22season%22,%22test%22:%7B%22id%22:%22123%22," +
                "%22tv_show_id%22:%22456%22,%22contentType%22:%22season%22%7D%7D&expectedUid=999&deviceId=test%20%25" +
                "%20device%20id&contentPlayPayload=test%20%25/?$%20payload");

        assertNotNull(url);

        URI pushUrl = URI.create(url);
        assertEquals("yellowskin", pushUrl.getScheme());

        URI landingUri = URI.create(queryParam(pushUrl, "url"));

        assertEquals("yandex.ru", landingUri.getHost());
        assertEquals("https", landingUri.getScheme());

        assertEquals(UID, queryParam(landingUri, "expectedUid"));
        assertEquals("test % device id", queryParam(landingUri, "deviceId"));
        assertEquals("test %/?$ payload", queryParam(landingUri, "contentPlayPayload"));
        assertEquals(toJsonQuotes("{'type':'season','test':{'id':'123','tv_show_id':'456','contentType':'season'}}"),
                queryParam(landingUri, "contentItem"));
    }

    @Test
    void getWrappedLinkUrl() throws URISyntaxException {
        String url = pushService.getWrappedLinkUrl("https://yandex.ru/quasar/push?srcrwr=QUASAR_HOST:testing.quasar" +
                ".common.yandex.ru/&contentItem=%7B%22type%22:%22season%22,%22test%22:%7B%22id%22:%22123%22," +
                "%22tv_show_id%22:%22456%22,%22contentType%22:%22season%22%7D%7D&expectedUid=test%20uid&deviceId=test" +
                "%20%25%20device%20id&contentPlayPayload=test%20%25/?$%20payload");

        URI landingUri = new URI(queryParam(new URI(url), "url"));

        assertEquals("yandex.ru", landingUri.getHost());
        assertEquals("https", landingUri.getScheme());

        assertEquals("test uid", queryParam(landingUri, "expectedUid"));
        assertEquals("test % device id", queryParam(landingUri, "deviceId"));
        assertEquals("test %/?$ payload", queryParam(landingUri, "contentPlayPayload"));
        assertEquals(toJsonQuotes("{'type':'season','test':{'id':'123','tv_show_id':'456','contentType':'season'}}"),
                queryParam(landingUri, "contentItem"));
    }

}
