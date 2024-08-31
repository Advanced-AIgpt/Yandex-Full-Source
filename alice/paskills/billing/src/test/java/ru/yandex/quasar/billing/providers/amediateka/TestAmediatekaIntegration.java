package ru.yandex.quasar.billing.providers.amediateka;

import java.io.IOException;
import java.util.Map;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.hamcrest.Matchers;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.junit.jupiter.api.extension.RegisterExtension;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.util.StringUtils;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.RemoteServiceProxyExtension;
import ru.yandex.quasar.billing.RemoteServiceProxyMode;
import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.EpisodeProviderContentItem;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.beans.SeasonProviderContentItem;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.SecretsConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;
import ru.yandex.quasar.billing.providers.AvailabilityInfo;
import ru.yandex.quasar.billing.providers.IContentProvider;
import ru.yandex.quasar.billing.providers.ProviderActiveSubscriptionInfo;
import ru.yandex.quasar.billing.providers.ProviderIntegrationTestUtil;
import ru.yandex.quasar.billing.providers.ProviderPricingOptions;
import ru.yandex.quasar.billing.providers.RejectionReason;
import ru.yandex.quasar.billing.providers.StreamData;
import ru.yandex.quasar.billing.providers.amediateka.model.ProviderPayload;
import ru.yandex.quasar.billing.services.processing.trust.PaymentMethodsClient;
import ru.yandex.quasar.billing.services.processing.trust.TrustBillingClient;
import ru.yandex.quasar.billing.util.AuthHelper;

import static java.util.stream.Collectors.toList;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsString;
import static org.hamcrest.Matchers.hasItem;
import static org.hamcrest.Matchers.is;
import static org.hamcrest.Matchers.isEmptyOrNullString;
import static org.hamcrest.Matchers.not;
import static org.junit.jupiter.api.Assertions.assertAll;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.when;
import static ru.yandex.quasar.billing.beans.ContentType.MOVIE;
import static ru.yandex.quasar.billing.beans.ContentType.SEASON;
import static ru.yandex.quasar.billing.beans.ContentType.SUBSCRIPTION;
import static ru.yandex.quasar.billing.beans.ContentType.TV_SHOW;

/**
 * This test calls production API of Amediateka with some test-ish ids
 */
@SpringBootTest(classes = {TestConfigProvider.class})
@MockBean(name = "yandexPayTrustClient", classes = TrustBillingClient.class)
@MockBean(name = "mediaBillingTrustClient", classes = PaymentMethodsClient.class)
@ExtendWith(EmbeddedPostgresExtension.class)
@Disabled
public class TestAmediatekaIntegration {


    private static final String SOME_FILM_ID = "5553f744-fb67-4304-b018-300fb4339158";
    private static final String SOME_EPISODE_ID = "a73e7a17add3351a67a5fff95ad9128e_RU";
    private static final String SOME_SEASON_ID = "siezon_1_7264641a-4e02-4d9a-b4ad-3111900fc0f4";
    private static final String SOME_SERIES_ID = "banshi_ec3c3678-ea01-4142-a6a4-a955e258aa45";
    private static final String SOME_BUNDLE_ID = "bundle_3e488794-f3e9-4c76-a502-bd3930e69bdf";
    // this has no subscription
    private static final String SESSION_ID = "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9" +
            ".eyJ1aWQiOiJpVnkyT1FpaEFkVjdSTzI0aEVKa3JwZFFrbG5vcEZPQkx6VHNlQXBOWkV3IiwiY3VzdG9tZXJfaWQiOjE4MzA4MjQ2L" +
            "CJkZXZpY2VfaWQiOm51bGx9.GtQI0LyjgCf8SnQfko3Qth7MNihq2HduD80qM1wCBq8";
    // this has subscription
    private static final String SESSION_WITH_SUBSCRIPTIONS_ID = "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9" +
            ".eyJ1aWQiOiJJUGVPcU9tVVNQSmtmU1d5Wm4zN3JXbVBMaVotSTJ6TFhlTERfeXFmZkpnIiwiY3VzdG9tZXJfaWQiOjE2NzcxMDgy" +
            "LCJkZXZpY2VfaWQiOm51bGx9.qyQT-wIoZJGJetrYln80JECqTJWLqX6N0OKMnsu93QQ";
    // it has first episode free, so we can test availability
    private static final SeasonProviderContentItem WESTWORLD_S_01 =
            ProviderContentItem.createSeason("sezon-1_af5c08", "mir-dikogo-zapada");
    private static final String USER_IP = "127.0.0.1";
    private static final String USER_AGENT = AuthHelper.STATION_USER_AGENT;
    @Autowired
    @Qualifier("amediatekaContentProvider")
    protected IContentProvider amediatekaContentProvider;
    @RegisterExtension
    protected RemoteServiceProxyExtension wiremock = new RemoteServiceProxyExtension(
            "https://api.amediateka.ru/",
            RemoteServiceProxyMode.REPLAYING
    );
    @Autowired
    private ObjectMapper objectMapper;
    @Autowired
    private BillingConfig billingConfig;
    @SpyBean
    private SecretsConfig secretsConfig;
    @Autowired
    private AmediatekaBundlesCache bundlesCache;
    private String originalUrl;

    @BeforeEach
    public void setUp() {
        originalUrl = billingConfig.getAmediatekaConfig().getApiUrl();
        billingConfig.getAmediatekaConfig().setApiUrl(wiremock.getUrl());
        when(secretsConfig.getAmediatekaClientSecret()).thenReturn("42c988ca6d004d31b9f4c73207402bdb");
    }

    @AfterEach
    void tearDown() {
        billingConfig.getAmediatekaConfig().setApiUrl(originalUrl);
    }

    @Test
    void testGetStreamFilm() {
        StreamData streamData = amediatekaContentProvider.getStream(ProviderContentItem.create(MOVIE, SOME_FILM_ID),
                SESSION_WITH_SUBSCRIPTIONS_ID, "5.255.255.5", AuthHelper.STATION_USER_AGENT);

        assertNotNull(streamData);
        assertThat(streamData.getUrl(), not(isEmptyOrNullString()));
    }


    @Test
    void testPricingOptionsExistingFilm() throws IOException {
        ProviderPricingOptions pricingOptions =
                amediatekaContentProvider.getPricingOptions(ProviderContentItem.create(MOVIE, SOME_FILM_ID),
                        SESSION_ID);

        assertNotNull(pricingOptions);
        assertFalse(pricingOptions.isAvailable());
        assertTrue(pricingOptions.getPricingOptions().size() > 0);

        ProviderPayload providerPayload = objectMapper.readValue(
                pricingOptions.getPricingOptions().stream()
                        .filter(it -> it.getPurchasingItem().equals(ProviderContentItem.create(SUBSCRIPTION,
                                SOME_BUNDLE_ID)))
                        .findFirst()
                        .orElseThrow(RuntimeException::new)
                        .getProviderPayload(),
                ProviderPayload.class);

        assertEquals(SOME_BUNDLE_ID, providerPayload.getBundleId());
        assertEquals("yandexprice", providerPayload.getPriceUid());
    }

    @Test
    void testPricingOptionsExistingSeason() {
        ProviderPricingOptions pricingOptions =
                amediatekaContentProvider.getPricingOptions(ProviderContentItem.create(SEASON, SOME_SEASON_ID),
                        SESSION_ID);

        assertNotNull(pricingOptions);
        assertFalse(pricingOptions.isAvailable());
        assertTrue(pricingOptions.getPricingOptions().size() > 0);
    }

    @Test
    void testPricingOptionsExistingSeries() {
        ProviderPricingOptions pricingOptions =
                amediatekaContentProvider.getPricingOptions(ProviderContentItem.create(TV_SHOW, SOME_SERIES_ID),
                        SESSION_ID);

        assertNotNull(pricingOptions);
        assertFalse(pricingOptions.isAvailable());
        assertTrue(pricingOptions.getPricingOptions().size() > 0);
    }

    @Test
    void testPricingOptionsExistingFilmAsNotFilm() {
        ProviderPricingOptions pricingOptions =
                amediatekaContentProvider.getPricingOptions(ProviderContentItem.create(SEASON, SOME_FILM_ID),
                        SESSION_ID);

        assertNotNull(pricingOptions);
        assertFalse(pricingOptions.isAvailable());
        assertFalse(pricingOptions.getPricingOptions().size() > 0);
    }

    @Test
    void testPricingOptionsMissingFilm() {
        String someNonsenseId = "never_ever_will_there_be_a_film_like_this";

        ProviderPricingOptions pricingOptions =
                amediatekaContentProvider.getPricingOptions(ProviderContentItem.create(MOVIE, someNonsenseId),
                        SESSION_ID);

        assertNotNull(pricingOptions);
        assertFalse(pricingOptions.isAvailable());
        assertEquals(0, pricingOptions.getPricingOptions().size());
    }

    @Test
    void testPricingOptionsForExistingSubscription() {
        String subscriptionBundleId = "bundle_3e488794-f3e9-4c76-a502-bd3930e69bdf";

        ProviderPricingOptions pricingOptions =
                amediatekaContentProvider.getPricingOptions(ProviderContentItem.create(SUBSCRIPTION,
                        subscriptionBundleId), SESSION_WITH_SUBSCRIPTIONS_ID);

        assertNotNull(pricingOptions);
        assertTrue(pricingOptions.isAvailable()); // as we already have active subscriptions
        assertTrue(pricingOptions.getPricingOptions().isEmpty());
    }

    @Test
    void testPricingOptionsForAnotherExistingSubscription() {
        //String subscriptionBundleId = "bundle_3e488794-f3e9-4c76-a502-bd3930e69bdf"; - this one is active
        String subscriptionBundleId = "bundle_086cbcf3-f8f7-483b-b28d-15135b8bb8a4"; // propovednik subscription


        ProviderPricingOptions pricingOptions =
                amediatekaContentProvider.getPricingOptions(ProviderContentItem.create(SUBSCRIPTION,
                        subscriptionBundleId), SESSION_WITH_SUBSCRIPTIONS_ID);

        assertNotNull(pricingOptions);
        assertFalse(pricingOptions.isAvailable()); // as we already have active subscriptions but not the one we look
        // for
        assertFalse(pricingOptions.getPricingOptions().isEmpty());
    }

    @Test
    void testPricingOptionsForNotExistingSubscription() {
        String subscriptionBundleId = "bundle_3e488794-f3e9-4c76-a502-bd3930e69bdf";

        ProviderPricingOptions pricingOptions =
                amediatekaContentProvider.getPricingOptions(ProviderContentItem.create(SUBSCRIPTION,
                        subscriptionBundleId), SESSION_ID);

        assertNotNull(pricingOptions);
        assertFalse(pricingOptions.isAvailable()); // noa active subscription
        assertFalse(pricingOptions.getPricingOptions().isEmpty());
    }

    @Test
    void testAvailabilityBatchOneItem() {
        ProviderContentItem contentItem = ProviderContentItem.create(MOVIE, SOME_FILM_ID);

        AvailabilityInfo availabilityBatch = amediatekaContentProvider.getAvailability(contentItem,
                SESSION_WITH_SUBSCRIPTIONS_ID, USER_IP, USER_AGENT, true);

        assertNull(availabilityBatch.getMinPrice());
        assertTrue(availabilityBatch.isAvailable());
    }

    @Test
    void testAvailabilityAnonymous() {
        ProviderContentItem contentItem = ProviderContentItem.create(MOVIE, SOME_FILM_ID);

        AvailabilityInfo availabilityInfo = amediatekaContentProvider.getAvailability(contentItem, null, USER_IP,
                USER_AGENT, true);

        assertTrue(availabilityInfo.isRequiresAccountBinding());
        assertFalse(availabilityInfo.isAvailable());
        assertEquals(RejectionReason.PURCHASE_NOT_FOUND, availabilityInfo.getRejectionReason());
        assertThat(availabilityInfo.getPricingOptions(), Matchers.not(Matchers.emptyIterable()));
    }

    @Test
    void testAvailabilityBatchTwoItems() {
        ProviderContentItem contentItem = ProviderContentItem.create(MOVIE, SOME_FILM_ID);
        ProviderContentItem anotherContentItem = ProviderContentItem.createSeason(SOME_SEASON_ID, SOME_SERIES_ID);

        AvailabilityInfo availabilityBatch = amediatekaContentProvider.getAvailability(contentItem,
                SESSION_WITH_SUBSCRIPTIONS_ID, USER_IP, USER_AGENT, true);
        AvailabilityInfo availabilityBatch2 = amediatekaContentProvider.getAvailability(anotherContentItem,
                SESSION_WITH_SUBSCRIPTIONS_ID, USER_IP, USER_AGENT, true);

        assertNull(availabilityBatch.getMinPrice());
        assertTrue(availabilityBatch.isAvailable());

        assertNull(availabilityBatch2.getMinPrice());
        assertTrue(availabilityBatch2.isAvailable());

    }

    @Test
    void testPricingOptionsSeason() {
        SeasonProviderContentItem seasonItem = ProviderContentItem.createSeason(SOME_SEASON_ID, SOME_SERIES_ID);

        ProviderPricingOptions seasonPricingOptions = amediatekaContentProvider.getPricingOptions(seasonItem,
                SESSION_ID);

        assertNotNull(seasonPricingOptions);
        assertFalse(seasonPricingOptions.isAvailable());
        assertFalse(seasonPricingOptions.getPricingOptions().isEmpty());
    }

    @Test
    void testPricingOptionsEpisode() {
        EpisodeProviderContentItem episodeItem = ProviderContentItem.createEpisode(SOME_EPISODE_ID, SOME_SEASON_ID,
                SOME_SERIES_ID);

        ProviderPricingOptions episodePricingOptions = amediatekaContentProvider.getPricingOptions(episodeItem,
                SESSION_ID);

        assertNotNull(episodePricingOptions);
        assertFalse(episodePricingOptions.isAvailable());
        assertFalse(episodePricingOptions.getPricingOptions().isEmpty());

        SeasonProviderContentItem seasonItem = ProviderContentItem.createSeason(SOME_SEASON_ID, SOME_SERIES_ID);

        ProviderPricingOptions seasonPricingOptions = amediatekaContentProvider.getPricingOptions(seasonItem,
                SESSION_ID);

        assertEquals(episodePricingOptions, seasonPricingOptions);
    }

    @Test
    void testAvailabilitySeason() {
        SeasonProviderContentItem seasonItem = ProviderContentItem.createSeason(SOME_SEASON_ID, SOME_SERIES_ID);

        AvailabilityInfo availabilityInfo = amediatekaContentProvider.getAvailability(seasonItem, SESSION_ID, USER_IP,
                USER_AGENT, true);

        assertNotNull(availabilityInfo);
        assertFalse(availabilityInfo.isAvailable());
        assertNotNull(availabilityInfo.getMinPrice());
        assertFalse(availabilityInfo.isRequiresAccountBinding());
    }

    @Test
    void testAvailabilityEpisode() {
        EpisodeProviderContentItem episodeItem = ProviderContentItem.createEpisode(SOME_EPISODE_ID, SOME_SEASON_ID,
                SOME_SERIES_ID);

        AvailabilityInfo episodeAvailabilityInfo = amediatekaContentProvider.getAvailability(episodeItem, SESSION_ID,
                USER_IP, USER_AGENT, true);

        assertNotNull(episodeAvailabilityInfo);
        assertFalse(episodeAvailabilityInfo.isAvailable());
        assertNotNull(episodeAvailabilityInfo.getMinPrice());
        assertFalse(episodeAvailabilityInfo.isRequiresAccountBinding());

        SeasonProviderContentItem seasonItem = ProviderContentItem.createSeason(SOME_SEASON_ID, SOME_SERIES_ID);

        AvailabilityInfo seasonAvailabilityInfo = amediatekaContentProvider
                .getAvailability(seasonItem, SESSION_ID, USER_IP, USER_AGENT, true);

        assertEquals(episodeAvailabilityInfo, seasonAvailabilityInfo);
    }

    @Test
    void testContentMetaInfoSeason() {
        SeasonProviderContentItem seasonItem = ProviderContentItem.createSeason(SOME_SEASON_ID, SOME_SERIES_ID);

        ContentMetaInfo contentMetaInfo = amediatekaContentProvider.getContentMetaInfo(seasonItem);

        assertNotNull(contentMetaInfo);
        assertEquals("Банши", contentMetaInfo.getTitle());
        assertNotNull(contentMetaInfo.getImageUrl());
        assertEquals(Integer.valueOf(2013), contentMetaInfo.getYear());
        assertNull(contentMetaInfo.getDurationMinutes());
        assertEquals("18+", contentMetaInfo.getAgeRestriction());
        assertEquals("США", contentMetaInfo.getCountry());
        assertEquals(Integer.valueOf(1), contentMetaInfo.getSeasonNumber());
        assertTrue(StringUtils.hasText(contentMetaInfo.getDescription()));
    }

    @Test
    void testContentMetaInfoTvShow() {
        ProviderContentItem tvShowItem = ProviderContentItem.create(TV_SHOW, SOME_SERIES_ID);

        ContentMetaInfo contentMetaInfo = amediatekaContentProvider.getContentMetaInfo(tvShowItem);

        assertNotNull(contentMetaInfo);
        assertEquals("Банши", contentMetaInfo.getTitle());
        assertNotNull(contentMetaInfo.getImageUrl());
        assertEquals(Integer.valueOf(2013), contentMetaInfo.getYear());
        assertNull(contentMetaInfo.getDurationMinutes());
        assertEquals("18+", contentMetaInfo.getAgeRestriction());
        assertEquals("США", contentMetaInfo.getCountry());
        assertNull(contentMetaInfo.getSeasonNumber());
        assertTrue(StringUtils.hasText(contentMetaInfo.getDescription()));
    }

    @Test
    void testContentMetaInfoMovie() {
        ProviderContentItem movieItem = ProviderContentItem.create(MOVIE, SOME_FILM_ID);

        ContentMetaInfo contentMetaInfo = amediatekaContentProvider.getContentMetaInfo(movieItem);

        assertAll(
                () -> assertNotNull(contentMetaInfo),
                () -> assertEquals("Омерзительная восьмерка", contentMetaInfo.getTitle()),
                () -> assertNotNull(contentMetaInfo.getImageUrl()),
                () -> assertEquals(Integer.valueOf(2015), contentMetaInfo.getYear()),
                () -> assertEquals(Integer.valueOf(160), contentMetaInfo.getDurationMinutes()),
                () -> assertEquals("18+", contentMetaInfo.getAgeRestriction()),
                () -> assertEquals("США", contentMetaInfo.getCountry()),
                () -> assertNull(contentMetaInfo.getSeasonNumber()),
                () -> assertTrue(StringUtils.hasText(contentMetaInfo.getDescription()))
        );
    }

    @Test
    void testContentMetaInfoBundle() {
        ProviderContentItem bundleItem = ProviderContentItem.create(SUBSCRIPTION, SOME_BUNDLE_ID);

        ContentMetaInfo contentMetaInfo = amediatekaContentProvider.getContentMetaInfo(bundleItem);

        assertNotNull(contentMetaInfo);
        assertAll(
                () -> assertEquals("Amediateka (Основная подписка)", contentMetaInfo.getTitle()),
                () -> assertNull(contentMetaInfo.getImageUrl()),
                () -> assertNull(contentMetaInfo.getYear()),
                () -> assertNull(contentMetaInfo.getDurationMinutes()),
                () -> assertNull(contentMetaInfo.getAgeRestriction()),
                () -> assertNull(contentMetaInfo.getCountry()),
                () -> assertNull(contentMetaInfo.getSeasonNumber()),
                () -> assertTrue(StringUtils.hasText(contentMetaInfo.getDescription()))
        );
    }

    @Test
    void testSubscriptionPricingOptionsForDefaultAndAbcAvailable() {

        String sessionWithDefault = SESSION_WITH_SUBSCRIPTIONS_ID;
        Map<ProviderContentItem, ProviderActiveSubscriptionInfo> activeSubscriptions =
                amediatekaContentProvider.getActiveSubscriptions(sessionWithDefault);


        assertThat(activeSubscriptions.keySet().stream().map(ProviderContentItem::getId).collect(toList()),
                hasItem(bundlesCache.getBundleId(AmediatekaContentProvider.DEFAULT_BUNDLE_SLUG).get())
        );

    }

    @Test
    void testSubscriptionAsContentType() {
        ProviderPricingOptions pricingOptions =
                amediatekaContentProvider.getPricingOptions(ProviderContentItem.create(SUBSCRIPTION, SOME_BUNDLE_ID),
                        SESSION_ID);

        assertThat(pricingOptions, is(ProviderIntegrationTestUtil.reasonablePricingOptions()));
    }

    @Test
    void testFreeFirstEpisode() {
        // westworld's first episode is free
        // check that it is unavailable anyway
        assertThat(
                amediatekaContentProvider.getAvailability(WESTWORLD_S_01, SESSION_ID, USER_IP, USER_AGENT, false)
                        .isAvailable(),
                is(false)
        );
    }

    @Test
    @Disabled
        // hidden under UniversalProvider
    void testAvailabilityBatchClientNotMatch() {
        ProviderContentItem contentItem = ProviderContentItem.create(MOVIE, SOME_FILM_ID);

        String oldClientId = billingConfig.getAmediatekaConfig().getClientId();
        doReturn("").when(secretsConfig).getAmediatekaClientSecret();
        try {
            //set prod clientId but leave all the rest for test
            // get wrong_secret exception
            billingConfig.getAmediatekaConfig().setClientId("amediateka");
            try {
                amediatekaContentProvider.getAvailability(contentItem, SESSION_ID, USER_IP, USER_AGENT, true);
                fail("ProviderUnauthorizedException not raised");
            } catch (ProviderUnauthorizedException e) {
                assertEquals(amediatekaContentProvider.getProviderName(), e.getProviderName());
                assertThat(e.getMessage(), containsString("client_not_match"));
                assertTrue(e.isSocialApiSessionFound());
            }
        } finally {
            billingConfig.getAmediatekaConfig().setClientId(oldClientId);
        }

    }


    @Test
    void testActiveSubscriptionsSmoke() {
        var activeSubscriptions = amediatekaContentProvider.getActiveSubscriptions(SESSION_ID);

        assertNotNull(activeSubscriptions);
        assertTrue(activeSubscriptions.isEmpty());
    }

    @Test
    void testActiveSubscriptionsInvalidToken() {
        try {
            var activeSubscriptions = amediatekaContentProvider.getActiveSubscriptions("WRONG_TOKEN");
            fail("no exception raised");
        } catch (ProviderUnauthorizedException e) {
            assertEquals(amediatekaContentProvider.getProviderName(), e.getProviderName());
        }

    }

    @Test
    void getContentMetaInfoEpisodeWithoutYear() {
        ContentMetaInfo contentMetaInfo =
                amediatekaContentProvider.getContentMetaInfo(ProviderContentItem.createEpisode("e56c8a88-ebca-4619" +
                        "-b0c4-e7370617a2b8", "sezon-1_456cbc", "devushka-po-vyzovu"));

    }
}
