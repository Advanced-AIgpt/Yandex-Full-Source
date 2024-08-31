package ru.yandex.quasar.billing.services.sup;

import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ThreadLocalRandom;

import org.springframework.stereotype.Component;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.SupPushServiceClientConfig;

@Component
public class MobilePushService {

    public static final String STATION_BILLING_CARD_ID_PREFIX = "station_billing_";
    public static final int DEFAULT_DATE_TO_OFFSET = 7200;

    private final SupPushServiceClientConfig config;

    public MobilePushService(BillingConfig config) {
        this.config = config.getSupPushServiceClientConfig();
    }

    public String generateCardId() {
        return STATION_BILLING_CARD_ID_PREFIX + ThreadLocalRandom.current().nextLong(0, Long.MAX_VALUE);
    }

    public String getActivatePromoPeriodUrl(String providerName) {
        return UriComponentsBuilder.fromUriString(config.getLandingBaseUrl()).pathSegment(
                        "id", providerName, "promoperiod")
                .build()
                .toUri()
                .toASCIIString();
    }

    public String getWrappedLinkUrl(String innerUrl) {
        return "yellowskin://?url=" + URLEncoder.encode(innerUrl, StandardCharsets.UTF_8);
    }

}
