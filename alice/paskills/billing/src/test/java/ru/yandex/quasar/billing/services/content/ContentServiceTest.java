package ru.yandex.quasar.billing.services.content;

import java.math.BigDecimal;
import java.sql.Timestamp;
import java.time.Instant;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.function.Supplier;
import java.util.stream.Stream;

import org.hamcrest.Matchers;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.stubbing.Answer;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.boot.autoconfigure.web.client.RestTemplateAutoConfiguration;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Component;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;
import org.springframework.web.client.HttpClientErrorException;

import ru.yandex.quasar.billing.beans.ContentItem;
import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.LogicalPeriod;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionTestUtil;
import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.beans.PricingOptions;
import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.controller.SyncExecutorServicesConfig;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.SubscriptionInfo;
import ru.yandex.quasar.billing.dao.UserPastesDAO;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.dao.UserSkillProductDao;
import ru.yandex.quasar.billing.dao.UserSubscriptionsDAO;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;
import ru.yandex.quasar.billing.providers.AvailabilityInfo;
import ru.yandex.quasar.billing.providers.DelegatingContentProvider;
import ru.yandex.quasar.billing.providers.IContentProvider;
import ru.yandex.quasar.billing.providers.ProviderActiveSubscriptionInfo;
import ru.yandex.quasar.billing.providers.ProviderManager;
import ru.yandex.quasar.billing.providers.ProviderPricingOptions;
import ru.yandex.quasar.billing.providers.StreamData;
import ru.yandex.quasar.billing.providers.TestContentProvider;
import ru.yandex.quasar.billing.providers.universal.UniversalProviderConfigurator;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.AuthorizationService;
import ru.yandex.quasar.billing.services.BillingServiceImpl;
import ru.yandex.quasar.billing.services.UnistatService;
import ru.yandex.quasar.billing.services.UserPurchaseLockService;
import ru.yandex.quasar.billing.services.processing.ProcessingService;
import ru.yandex.quasar.billing.services.processing.ProcessingServiceManager;
import ru.yandex.quasar.billing.services.processing.trust.TrustBillingService;
import ru.yandex.quasar.billing.services.processing.yapay.YaPayBillingService;
import ru.yandex.quasar.billing.services.processing.yapay.YandexPayClient;
import ru.yandex.quasar.billing.services.promo.DeviceInfo;
import ru.yandex.quasar.billing.services.promo.Platform;
import ru.yandex.quasar.billing.services.promo.PromoProvider;
import ru.yandex.quasar.billing.services.promo.QuasarBackendClient;
import ru.yandex.quasar.billing.services.promo.QuasarPromoService;
import ru.yandex.quasar.billing.services.skills.SkillsService;
import ru.yandex.quasar.billing.services.sup.MobilePushService;
import ru.yandex.quasar.billing.util.AuthHelper;

import static java.util.Collections.emptyList;
import static java.util.Collections.emptySet;
import static java.util.Collections.singletonList;
import static java.util.stream.Collectors.toMap;
import static java.util.stream.Collectors.toSet;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import static ru.yandex.quasar.billing.providers.RejectionReason.PURCHASE_NOT_FOUND;
import static ru.yandex.quasar.billing.providers.TestContentProvider.PROVIDER_NAME;
import static ru.yandex.quasar.billing.providers.TestContentProvider.PROVIDER_SOCIAL_NAME;

@SpringJUnitConfig(classes = {
        AuthorizationContext.class,
        ContentService.class,
        TestContentProvider.class,
        ContentServiceTest.TestContentProvider2.class,
        SyncExecutorServicesConfig.class,
        ProviderManager.class,
        BillingServiceImpl.class,
        UnistatService.class,
        TestConfigProvider.class,
        UniversalProviderConfigurator.class,
        RestTemplateAutoConfiguration.class
})
@MockBean(classes = {
        TrustBillingService.class,
        YaPayBillingService.class,
        SkillsService.class,
        YandexPayClient.class,
        UserSkillProductDao.class,
        QuasarBackendClient.class,
})
class ContentServiceTest implements PricingOptionTestUtil {

    private static final String SECOND_PROVIDER_NAME = "testProvider2";
    private static final String SECOND_PROVIDER_SOCIAL_NAME = "testProviderSocial2";
    private final String testUid = "123456";
    private final Long pastId = -1L;
    private final String productCode = "1";
    private final String userIp = "127.0.0.1";
    private final String userAgent = AuthHelper.STATION_USER_AGENT;
    private final StreamData dummyStreamData = StreamData.byUrl("https://url");
    @Autowired
    private ContentService contentService;
    @Autowired
    private TestContentProvider contentProvider;
    @Autowired
    private TestContentProvider2 contentProvider2;
    @SpyBean
    @Qualifier("kinopoiskContentProvider")
    private IContentProvider kinopoiskProvider;
    @MockBean
    private UserPurchasesDAO userPurchasesDao;
    private final IContentProvider contentProviderMock = mock(IContentProvider.class);
    private final IContentProvider secondContentProviderMock = mock(IContentProvider.class);
    @MockBean
    private AuthorizationService authorizationService;
    @MockBean
    private UserPastesDAO userPastesDAO;
    @MockBean
    private UserSubscriptionsDAO userSubscriptionsDAO;
    @MockBean
    private MobilePushService mobilePushService;
    @MockBean
    private ProcessingServiceManager processingServiceManager;
    @MockBean
    private QuasarPromoService quasarPromoService;
    @MockBean
    private UserPurchaseLockService userPurchaseLockService;

    private final ProcessingService trustBillingService = mock(ProcessingService.class);
    private ProviderContentItem providerContentItem;
    private ProviderContentItem providerSubscriptionContentItem;
    private ContentItem contentItem;
    private ContentItem subscriptionContentItem;
    private SubscriptionInfo firstProviderSubscription;
    private final Instant activeTill = Instant.now().plusSeconds(60 * 60 * 24);
    private PricingOption subscriptionPricingOption;
    private PricingOption subscriptionPricingOptionWithIntroPeriod;
    private Map<IContentProvider, String> mockProviderNames;

    @SuppressWarnings("unchecked")
    @BeforeEach
    void setUp() {
        when(processingServiceManager.get(PaymentProcessor.TRUST)).thenReturn(trustBillingService);
        providerContentItem = ProviderContentItem.create(ContentType.MOVIE, "id1");
        providerSubscriptionContentItem = PromoType.test_promo.getPromoItem();
        // ContentItem for only one provider
        contentItem = new ContentItem(ContentType.MOVIE, Map.of(PROVIDER_NAME, providerContentItem,
                SECOND_PROVIDER_NAME, providerContentItem));
        subscriptionContentItem = new ContentItem(ContentType.SUBSCRIPTION, Map.of(PROVIDER_NAME,
                providerSubscriptionContentItem, SECOND_PROVIDER_NAME, providerSubscriptionContentItem));
        when(authorizationService.userHasPlus()).thenReturn(true);

        mockProviderNames = Map.of(
                contentProviderMock, contentProvider.getProviderName(),
                secondContentProviderMock, contentProvider2.getProviderName()
        );

        when(userPurchaseLockService.processWithLock(any(), any(), any(Supplier.class)))
                .then(invocation -> ((Supplier) invocation.getArgument(2)).get());

        subscriptionPricingOption = createSubscriptionPricingOption(PROVIDER_NAME, BigDecimal.TEN,
                providerSubscriptionContentItem, LogicalPeriod.ofDays(30));
        subscriptionPricingOptionWithIntroPeriod = createSubscriptionPricingOptionWithIntroPeriod(PROVIDER_NAME,
                BigDecimal.TEN, BigDecimal.ONE, providerSubscriptionContentItem, LogicalPeriod.ofDays(30),
                LogicalPeriod.ofMonths(3));

        firstProviderSubscription = SubscriptionInfo.create(1L, Long.valueOf(testUid), Timestamp.from(Instant.now()),
                providerSubscriptionContentItem, subscriptionPricingOption, SubscriptionInfo.Status.ACTIVE,
                Timestamp.from(activeTill), "sec",
                0, productCode, 100L, subscriptionPricingOption.getProvider(),
                subscriptionPricingOption.getPurchasingItem(), PaymentProcessor.TRUST);

        contentProvider.setDelegate(contentProviderMock);
        contentProvider2.setDelegate(secondContentProviderMock);

        when(contentProviderMock.getContentMetaInfo(providerContentItem))
                .thenReturn(createTestMovieContentMetaInfo());
        when(contentProviderMock.getProviderName()).thenReturn(contentProvider.getProviderName());
        when(contentProviderMock.getSocialAPIServiceName()).thenReturn(contentProvider.getSocialAPIServiceName());

        when(secondContentProviderMock.getContentMetaInfo(providerContentItem))
                .thenReturn(createTestMovieContentMetaInfo());
        when(secondContentProviderMock.getProviderName()).thenReturn(contentProvider2.getProviderName());
        when(secondContentProviderMock.getSocialAPIServiceName())
                .thenReturn(contentProvider2.getSocialAPIServiceName());

        doReturn(createTestMovieContentMetaInfo())
                .when(kinopoiskProvider).getContentMetaInfo(providerContentItem);

        when(authorizationService.getProviderTokenByUid(testUid, PROVIDER_SOCIAL_NAME)).thenReturn(Optional.of(
                "session"));
        when(authorizationService.getProviderTokenByUid(testUid, SECOND_PROVIDER_SOCIAL_NAME))
                .thenReturn(Optional.of("session"));
        when(userPastesDAO.savePaste(any(), any())).thenReturn(pastId);
        when(userSubscriptionsDAO.getActiveSubscriptions(any())).thenReturn(List.of());

        when(quasarPromoService.getUserDevices(testUid)).thenReturn(emptyList());
    }

    @Test
    void getContentMetaInfoTest() {
        // given
        ProviderContentItem providerContentItemTmp = ProviderContentItem.create(ContentType.MOVIE, "id1");
        ContentItem contentItem2 = new ContentItem(PROVIDER_NAME, providerContentItemTmp);

        ContentMetaInfo contentMetaInfo = createTestMovieContentMetaInfo();
        when(contentProvider.getContentMetaInfo(providerContentItemTmp)).thenReturn(contentMetaInfo);

        // when
        ContentMetaInfo actual = contentService.getContentMetaInfo(contentItem2);

        // then
        assertEquals(contentMetaInfo, actual);
    }

    @Test
    void testGetProviderActiveSubscriptionsSummariesWithoutSocialTokens() {
        when(authorizationService.getProviderTokenByUid(testUid, PROVIDER_SOCIAL_NAME)).thenReturn(Optional.empty());

        Instant till = Instant.now().plusSeconds(60 * 60 * 24);
        when(contentProviderMock.getActiveSubscriptions(any()))
                .thenReturn(Map.of(providerSubscriptionContentItem, createSubscriptionInfo(till)));

        Collection<ActiveSubscriptionSummary> activeSubscriptionsSummaries =
                contentService.getProviderActiveSubscriptionsSummaries(testUid, PROVIDER_NAME);

        assertEquals(emptyList(), activeSubscriptionsSummaries);
    }

    @Test
    void testGetProviderActiveSubscriptionsSummariesBadSocialTokens() {
        // given
        when(contentProviderMock.getActiveSubscriptions(any()))
                .thenThrow(new ProviderUnauthorizedException(PROVIDER_NAME, true));


        try {
            contentService.getProviderActiveSubscriptionsSummaries(testUid, PROVIDER_NAME);
            fail();
        } catch (ProviderUnauthorizedException e) {
            assertEquals(PROVIDER_NAME, e.getProviderName());
        }

    }

    @Test
    void testGetProviderActiveSubscriptionsSummariesOnProviderOnly() {
        when(contentProviderMock.getActiveSubscriptions(any()))
                .thenReturn(Map.of(providerSubscriptionContentItem, createSubscriptionInfo(activeTill)));

        Collection<ActiveSubscriptionSummary> activeSubscriptionsSummaries =
                contentService.getProviderActiveSubscriptionsSummaries(testUid, PROVIDER_NAME);

        ContentMetaInfo metaInfo = contentProvider.getContentMetaInfo(providerSubscriptionContentItem);
        ActiveSubscriptionSummary summary = ActiveSubscriptionSummary.builder()
                .title(metaInfo.getTitle())
                .providerContentItem(providerSubscriptionContentItem)
                .activeTill(activeTill)
                .subscriptionId(null)
                .build();

        assertEquals(singletonList(summary), activeSubscriptionsSummaries);
    }

    @Test
    void testGetProviderActiveSubscriptionsSummariesOnOurSubscription() {
        var till = Instant.now().plusSeconds(60 * 60 * 24);
        when(contentProviderMock.getActiveSubscriptions(any()))
                .thenReturn(Map.of(providerSubscriptionContentItem, createSubscriptionInfo(till)));
        when(userSubscriptionsDAO.getActiveSubscriptions(Long.valueOf(testUid)))
                .thenReturn(List.of(firstProviderSubscription));

        Collection<ActiveSubscriptionSummary> activeSubscriptionsSummaries =
                contentService.getProviderActiveSubscriptionsSummaries(testUid, PROVIDER_NAME);

        ContentMetaInfo metaInfo = contentProvider.getContentMetaInfo(providerSubscriptionContentItem);
        ActiveSubscriptionSummary summary = ActiveSubscriptionSummary.builder()
                .title(metaInfo.getTitle())
                .providerContentItem(providerSubscriptionContentItem)
                .activeTill(till)
                .subscriptionId(firstProviderSubscription.getSubscriptionId())
                .nextPaymentDate(firstProviderSubscription.getActiveTill().toInstant())
                .renewEnabled(true)
                .build();

        assertEquals(singletonList(summary), activeSubscriptionsSummaries);
    }

    @Test
    void testFilmRented() {
        // given
        Instant until = ZonedDateTime.of(2030, 1, 1, 0, 0, 0, 0, ZoneId.of("UTC")).toInstant();

        contentAvailableRented(contentProviderMock, until);
        contentPurchaseNeeded(secondContentProviderMock, true, 150);


        // when
        Map<String, ContentAvailabilityInfo> actual = contentService.checkContentAvailable(testUid, contentItem,
                userIp, userAgent);

        // then
        Map<String, ContentAvailabilityInfo> expected =
                Map.of(PROVIDER_NAME, ContentAvailabilityInfo.available(until),
                        SECOND_PROVIDER_NAME, ContentAvailabilityInfo.purchasable(BigDecimal.valueOf(150), "RUB"));

        assertEquals(expected, actual);
    }

    @Test
    void testPricingOptions() {
        // given
        mockDeviceWithPromo(Set.of());
        when(contentProviderMock.getPricingOptions(eq(providerSubscriptionContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(subscriptionPricingOption), null));
        when(secondContentProviderMock.getPricingOptions(eq(providerSubscriptionContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(), null));

        // when
        PricingOptions pricingOptions = contentService.getPricingOptions(testUid, subscriptionContentItem);

        //then
        assertFalse(pricingOptions.isAvailable());
        assertThat(pricingOptions.getProvidersWhereAvailable(), Matchers.empty());
        assertEquals(List.of(subscriptionPricingOption), pricingOptions.getPricingOptions());
        assertEquals(Set.of(), pricingOptions.getProvidersWithPromo());
    }

    @Test
    void testPricingOptionsWithFirstPaymentDiscount() {
        // given
        mockDeviceWithPromo(Set.of());
        when(contentProviderMock.getPricingOptions(eq(providerSubscriptionContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(subscriptionPricingOptionWithIntroPeriod),
                        null));
        when(secondContentProviderMock.getPricingOptions(eq(providerSubscriptionContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(), null));

        // when
        PricingOptions pricingOptions = contentService.getPricingOptions(testUid, subscriptionContentItem);

        //then
        assertFalse(pricingOptions.isAvailable());
        assertThat(pricingOptions.getProvidersWhereAvailable(), Matchers.empty());
        assertEquals(List.of(subscriptionPricingOptionWithIntroPeriod), pricingOptions.getPricingOptions());
        assertEquals(Set.of(), pricingOptions.getProvidersWithPromo());
    }


    @Test
    void testPricingOptionsWithPromo() {
        // given
        mockDeviceWithPromo(Set.of(PromoProvider.testProvider));
        when(contentProviderMock.getPricingOptions(eq(providerSubscriptionContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(subscriptionPricingOption), null));
        when(secondContentProviderMock.getPricingOptions(eq(providerSubscriptionContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(), null));

        // when
        PricingOptions pricingOptions = contentService.getPricingOptions(testUid, subscriptionContentItem);

        //then

        assertFalse(pricingOptions.isAvailable());
        assertThat(pricingOptions.getProvidersWhereAvailable(), Matchers.empty());
        assertEquals(List.of(subscriptionPricingOption), pricingOptions.getPricingOptions());
        assertEquals(Set.of(PROVIDER_NAME), pricingOptions.getProvidersWithPromo());
    }

    @Test
    void testPricingOptionsWithPromoBy() {
        // given
        mockDeviceWithPromo(Set.of(PromoProvider.testProvider));
        when(contentProviderMock.getPricingOptions(eq(providerSubscriptionContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(subscriptionPricingOption), null));
        when(secondContentProviderMock.getPricingOptions(eq(providerSubscriptionContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(), null));

        // when
        PricingOptions pricingOptions = contentService.getPricingOptions(testUid, subscriptionContentItem);

        //then

        assertFalse(pricingOptions.isAvailable());
        assertThat(pricingOptions.getProvidersWhereAvailable(), Matchers.empty());
        assertEquals(List.of(subscriptionPricingOption), pricingOptions.getPricingOptions());
        assertEquals(Set.of(PROVIDER_NAME), pricingOptions.getProvidersWithPromo());
    }

    @Test
    void testPricingOptionsWithMultiplePromo() {
        // given
        when(quasarPromoService.getUserDevices(testUid))
                .thenReturn(List.of(
                        new DeviceInfo("deviceId", Platform.YANDEXSTATION, "name",
                                Map.of(PromoProvider.testProvider, PromoType.test_promo), Instant.EPOCH),
                        new DeviceInfo("deviceId2", Platform.YANDEXSTATION, "name",
                                Map.of(PromoProvider.testProvider, PromoType.test_promo2), Instant.EPOCH)
                ));
        when(contentProviderMock.getPricingOptions(eq(providerSubscriptionContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(subscriptionPricingOption), null));
        when(secondContentProviderMock.getPricingOptions(eq(providerSubscriptionContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(), null));

        // when
        PricingOptions pricingOptions = contentService.getPricingOptions(testUid, subscriptionContentItem);

        //then

        assertFalse(pricingOptions.isAvailable());
        assertThat(pricingOptions.getProvidersWhereAvailable(), Matchers.empty());
        assertEquals(List.of(subscriptionPricingOption), pricingOptions.getPricingOptions());
        assertEquals(Set.of(PROVIDER_NAME), pricingOptions.getProvidersWithPromo());
        // for a given provider we should take promotype with the largest duration
        assertEquals(Map.of(PROVIDER_NAME, PromoType.test_promo2), pricingOptions.getProvidersWithPromoMap());
    }

    @Test
    void testPricingOptionsMoviePurchase() {
        // given
        var itemPrice = createPricingOption(contentProvider.getProviderName(), BigDecimal.valueOf(150),
                PricingOptionType.BUY, providerContentItem);
        mockDeviceWithPromo2(PromoType.test_promo);
        when(contentProviderMock.getPricingOptions(eq(providerContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(itemPrice), null));
        when(secondContentProviderMock.getPricingOptions(eq(providerContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(), null));

        // when
        PricingOptions pricingOptions = contentService.getPricingOptions(testUid, contentItem);

        //then

        assertFalse(pricingOptions.isAvailable());
        assertThat(pricingOptions.getProvidersWhereAvailable(), Matchers.empty());
        assertEquals(List.of(itemPrice), pricingOptions.getPricingOptions());
        assertEquals(emptySet(), pricingOptions.getAccountLinkingRequired());
        assertEquals(Set.of(), pricingOptions.getProvidersWithPromo());
    }

    @Test
    void testPricingOptionsMoviePurchaseOrSubscriptionOrPromo() {
        // given
        var purchasePrice = createPricingOption(mockProviderNames.get(secondContentProviderMock),
                BigDecimal.valueOf(150), PricingOptionType.BUY, providerContentItem);
        mockDeviceWithPromo(Set.of(PromoProvider.testProvider));
        when(contentProviderMock.getPricingOptions(eq(providerContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(subscriptionPricingOption), null));
        when(secondContentProviderMock.getPricingOptions(eq(providerContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(purchasePrice), null));

        // when
        PricingOptions pricingOptions = contentService.getPricingOptions(testUid, contentItem);

        //then
        assertFalse(pricingOptions.isAvailable());
        assertThat(pricingOptions.getProvidersWhereAvailable(), Matchers.empty());
        assertEquals(Set.of(purchasePrice, subscriptionPricingOption), Set.copyOf(pricingOptions.getPricingOptions()));
        assertEquals(emptySet(), pricingOptions.getAccountLinkingRequired());
        assertEquals(Set.of(PROVIDER_NAME), pricingOptions.getProvidersWithPromo());
    }

    @Test
    void testPricingOptionsMovieAvailableButSecondProviderHasPromo() {
        // given
        mockDeviceWithPromo(Set.of(PromoProvider.testProvider));
        when(contentProviderMock.getPricingOptions(eq(providerContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(subscriptionPricingOption), null));
        when(secondContentProviderMock.getPricingOptions(eq(providerContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(true, List.of(), null));

        // when
        PricingOptions pricingOptions = contentService.getPricingOptions(testUid, contentItem);

        //then
        assertTrue(pricingOptions.isAvailable());
        assertEquals(Set.of(contentProvider2.getProviderName()), pricingOptions.getProvidersWhereAvailable());
        assertEquals(emptySet(), pricingOptions.getAccountLinkingRequired());
        //assertEquals(emptyList(), pricingOptions.getPricingOptions());
        assertEquals(emptySet(), pricingOptions.getProvidersWithPromo());
    }

    @Test
    void testPricingOptionsMoviePurchaseOrSubscriptionOrPromoButAccountLinkingRequired() {
        // given
        var purchasePrice = createPricingOption(mockProviderNames.get(secondContentProviderMock),
                BigDecimal.valueOf(150), PricingOptionType.BUY, providerContentItem);

        mockDeviceWithPromo(Set.of(PromoProvider.testProvider));
        when(authorizationService.getProviderTokenByUid(testUid, SECOND_PROVIDER_SOCIAL_NAME))
                .thenReturn(Optional.empty());

        when(contentProviderMock.getPricingOptions(eq(providerContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(subscriptionPricingOption), null));
        when(secondContentProviderMock.getPricingOptions(eq(providerContentItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, List.of(purchasePrice), null));

        // when
        PricingOptions pricingOptions = contentService.getPricingOptions(testUid, contentItem);

        //then
        assertFalse(pricingOptions.isAvailable());
        assertEquals(emptySet(), pricingOptions.getProvidersWhereAvailable());
        assertEquals(Set.of(SECOND_PROVIDER_NAME), pricingOptions.getAccountLinkingRequired());
        //assertEquals(emptyList(), pricingOptions.getPricingOptions());
        assertEquals(Set.of(PROVIDER_NAME), pricingOptions.getProvidersWithPromo());
    }

    @Test
    void testSkipFailuresOnGetPurchasedMetaData() {
        // given
        ProviderContentItem okItem = ProviderContentItem.create(ContentType.MOVIE, "1");
        ProviderContentItem failingItem = ProviderContentItem.create(ContentType.MOVIE, "2");
        when(userPurchasesDao.getLastContentPurchases(Long.valueOf(testUid)))
                .thenReturn(List.of(createDummyOption(okItem), createDummyOption(failingItem)));

        when(contentProviderMock.getContentMetaInfo(eq(okItem)))
                .thenReturn(createTestMovieContentMetaInfo());
        when(contentProviderMock.getContentMetaInfo(eq(failingItem)))
                .thenThrow(new HttpClientErrorException(HttpStatus.NOT_FOUND));

        // when
        List<PurchasedContentInfo> actual = contentService.getLastPurchasedContentInfo(testUid);

        // then
        assertEquals(Set.of(okItem), actual.stream().map(PurchasedContentInfo::getContentItem).collect(toSet()));
    }

    @Test
    void testMultipleItemsGetPurchasedMetaData() {
        // given
        ProviderContentItem okItem = ProviderContentItem.create(ContentType.MOVIE, "1");
        ProviderContentItem okItem2 = ProviderContentItem.create(ContentType.MOVIE, "2");
        when(userPurchasesDao.getLastContentPurchases(Long.valueOf(testUid)))
                .thenReturn(List.of(createDummyOption(okItem), createDummyOption(okItem2)));

        when(contentProviderMock.getContentMetaInfo(eq(okItem)))
                .thenReturn(createTestMovieContentMetaInfo());
        when(contentProviderMock.getContentMetaInfo(eq(okItem2)))
                .thenReturn(createTestMovieContentMetaInfo());

        // when
        List<PurchasedContentInfo> actual = contentService.getLastPurchasedContentInfo(testUid);

        // then
        assertEquals(Set.of(okItem, okItem2),
                actual.stream().map(PurchasedContentInfo::getContentItem).collect(toSet()));
    }

    @Test
    void testSingleItemGetPurchasedMetaData() {
        // given
        ProviderContentItem okItem = ProviderContentItem.create(ContentType.MOVIE, "1");
        when(userPurchasesDao.getLastContentPurchases(Long.valueOf(testUid)))
                .thenReturn(List.of(createDummyOption(okItem)));

        when(contentProviderMock.getContentMetaInfo(eq(okItem)))
                .thenReturn(createTestMovieContentMetaInfo());

        // when
        List<PurchasedContentInfo> actual = contentService.getLastPurchasedContentInfo(testUid);

        // then
        assertEquals(Set.of(okItem), actual.stream().map(PurchasedContentInfo::getContentItem).collect(toSet()));
    }


    @Test
    void testNoneItemGetPurchasedMetaData() {
        // given
        when(userPurchasesDao.getLastContentPurchases(Long.valueOf(testUid)))
                .thenReturn(List.of());

        // when
        List<PurchasedContentInfo> actual = contentService.getLastPurchasedContentInfo(testUid);

        // then
        assertEquals(Set.of(), actual.stream().map(PurchasedContentInfo::getContentItem).collect(toSet()));
    }

    private PurchaseInfo createDummyOption(ProviderContentItem item) {
        PricingOption option = createPricingOption(PROVIDER_NAME, BigDecimal.TEN, PricingOptionType.BUY, item);
        return PurchaseInfo.createSinglePayment(1L, Long.valueOf(testUid), "qwe", item, option,
                PurchaseInfo.Status.CLEARED, null, 1L, null, PROVIDER_NAME, null, PaymentProcessor.TRUST, null, null);
    }

    private void contentAvailableRented(IContentProvider providerMock, Instant until) {
        Answer<AvailabilityInfo> answer = inv -> AvailabilityInfo.available(false, until,
                inv.<Boolean>getArgument(4) ? dummyStreamData : StreamData.EMPTY);
        when(providerMock.getAvailability(eq(providerContentItem), any(), eq(userIp), eq(userAgent), anyBoolean()))
                .then(answer);
    }

    private void contentPurchaseNeeded(IContentProvider providerMock, boolean requestAccountBinding, int price) {
        when(providerMock.getAvailability(eq(providerContentItem), any(), eq(userIp), eq(userAgent), anyBoolean()))
                .thenReturn(AvailabilityInfo.unavailable(requestAccountBinding,
                        List.of(createPricingOption(mockProviderNames.get(providerMock), BigDecimal.valueOf(price),
                                PricingOptionType.BUY, providerContentItem)), PURCHASE_NOT_FOUND));
    }

    private void mockDeviceWithPromo(Set<PromoProvider> providers) {
        when(quasarPromoService.getUserDevices(testUid))
                .thenReturn(List.of(new DeviceInfo("deviceId", Platform.YANDEXSTATION, "name",
                        providers.stream().collect(toMap(x -> x, x -> PromoType.test_promo)), Instant.EPOCH)));
    }

    private void mockDeviceWithPromo2(PromoType... promos) {
        when(quasarPromoService.getUserDevices(testUid))
                .thenReturn(List.of(new DeviceInfo("deviceId", Platform.YANDEXSTATION, "name",
                        Stream.of(promos).collect(toMap(promo -> promo.getProvider(), x -> x)), Instant.EPOCH)));
    }

    private ContentMetaInfo createTestMovieContentMetaInfo() {
        return new ContentMetaInfo("testTitle", "testImageUrl", 123, 234, "345", "testCountry", null, null);
    }

    private ProviderActiveSubscriptionInfo createSubscriptionInfo(Instant till) {
        return ProviderActiveSubscriptionInfo.builder(providerSubscriptionContentItem)
                .title(providerSubscriptionContentItem.toString())
                .activeTill(till)
                .build();
    }

    @Component
    static class TestContentProvider2 extends DelegatingContentProvider {
        TestContentProvider2() {
            super(SECOND_PROVIDER_NAME, SECOND_PROVIDER_SOCIAL_NAME, null);
        }

    }
}
