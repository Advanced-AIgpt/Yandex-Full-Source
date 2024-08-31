package ru.yandex.quasar.billing.controller;

import java.io.IOException;
import java.math.BigDecimal;
import java.net.URI;
import java.time.Instant;
import java.time.ZonedDateTime;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.UUID;

import javax.annotation.Nonnull;
import javax.servlet.http.HttpServletRequest;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.fasterxml.jackson.databind.node.TextNode;
import org.json.JSONObject;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.ContentItem;
import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.LogicalPeriod;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionTestUtil;
import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.PurchaseOffer;
import ru.yandex.quasar.billing.dao.PurchaseOfferDao;
import ru.yandex.quasar.billing.dao.SkillInfo;
import ru.yandex.quasar.billing.dao.SkillProduct;
import ru.yandex.quasar.billing.dao.SkillProductType;
import ru.yandex.quasar.billing.dao.UserPastesDAO;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.dao.UserSkillProduct;
import ru.yandex.quasar.billing.dao.UserSkillProductDao;
import ru.yandex.quasar.billing.exception.BadRequestException;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;
import ru.yandex.quasar.billing.providers.AvailabilityInfo;
import ru.yandex.quasar.billing.providers.IContentProvider;
import ru.yandex.quasar.billing.providers.ProviderActiveSubscriptionInfo;
import ru.yandex.quasar.billing.providers.ProviderPricingOptions;
import ru.yandex.quasar.billing.providers.StreamData;
import ru.yandex.quasar.billing.providers.TestContentProvider;
import ru.yandex.quasar.billing.providers.amediateka.AmediatekaContentProvider;
import ru.yandex.quasar.billing.providers.universal.UniversalProvider;
import ru.yandex.quasar.billing.services.AuthorizationService;
import ru.yandex.quasar.billing.services.BillingService;
import ru.yandex.quasar.billing.services.PaymentInfo;
import ru.yandex.quasar.billing.services.processing.FreeProcessingBillingService;
import ru.yandex.quasar.billing.services.processing.NdsType;
import ru.yandex.quasar.billing.services.processing.ProcessingServiceManager;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;
import ru.yandex.quasar.billing.services.processing.trust.PaymentMethod;
import ru.yandex.quasar.billing.services.processing.trust.TrustBillingClient;
import ru.yandex.quasar.billing.services.processing.trust.TrustBillingService;
import ru.yandex.quasar.billing.services.processing.yapay.YaPayBillingService;
import ru.yandex.quasar.billing.services.processing.yapay.YandexPayClient;
import ru.yandex.quasar.billing.services.processing.yapay.YandexPayMerchantTestUtil;
import ru.yandex.quasar.billing.services.promo.QuasarPromoService;
import ru.yandex.quasar.billing.services.skills.SkillsService;
import ru.yandex.quasar.billing.services.sup.MobilePushService;
import ru.yandex.quasar.billing.services.tvm.TvmClientName;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;
import ru.yandex.quasar.billing.util.JsonUtil;

import static java.util.Collections.emptyMap;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.allOf;
import static org.hamcrest.Matchers.hasEntry;
import static org.junit.jupiter.api.Assertions.assertAll;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.refEq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static ru.yandex.quasar.billing.providers.TestContentProvider.PROVIDER_NAME;
import static ru.yandex.quasar.billing.services.tvm.TestTvmClientImpl.getTestTicket;
import static ru.yandex.quasar.billing.util.AuthHelper.STATION_USER_AGENT;

/**
 * A test to check {@link BillingController}'s HTTP API
 * <p>
 * Employs a mock {@link IContentProvider} -- {@link TestContentProvider} (with name `test`).
 * <p>
 * TODO: implement tests for all methods.
 */
@ExtendWith(EmbeddedPostgresExtension.class)
@AutoConfigureWebClient
@SpringBootTest(
        properties = "spring.main.allow-bean-definition-overriding=true",
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT,
        classes = {SyncExecutorServicesConfig.class, TestConfigProvider.class}
)
class BillingControllerTest implements PricingOptionTestUtil, YandexPayMerchantTestUtil {

    public static final String UID = "999";
    private final StreamData dummyStreamData = StreamData.byUrl("https://url");
    @Autowired
    private TestRestTemplate testRestTemplate;
    @Autowired
    private TestContentProvider delegateContentProvider;
    private IContentProvider testContentProvider = mock(IContentProvider.class);
    @Autowired
    private ObjectMapper objectMapper;
    @Autowired
    private TrustBillingService trustService;
    @Autowired
    private FreeProcessingBillingService freeProcessingBillingService;
    @SpyBean
    private AuthorizationService authorizationService;
    @MockBean
    private UserPastesDAO userPastesDAO;
    @MockBean
    private UserPurchasesDAO userPurchasesDAO;
    @MockBean
    private PurchaseOfferDao purchaseOfferDao;
    @MockBean
    private UserSkillProductDao userSkillProductDao;
    @MockBean
    private MobilePushService mobilePushService;
    @MockBean
    private ProcessingServiceManager processingServiceManager;
    @Mock
    private TrustBillingService trustServiceMock;
    @MockBean
    private QuasarPromoService quasarPromoService;
    @MockBean
    private YaPayBillingService yaPayBillingService;

    @SpyBean
    private AmediatekaContentProvider amediatekaContentProvider;
    @SpyBean
    private UniversalProvider kinopoiskProvider;
    @SpyBean
    private BillingService billingService;
    @MockBean
    private YandexPayClient yandexPayClient;
    @MockBean(name = "trustBillingClient")
    private TrustBillingClient trustBillingClient;
    @Autowired
    private SkillsService skillsService;
    @LocalServerPort
    private int port;

    private UriComponentsBuilder url(String method) {
        return UriComponentsBuilder.fromUriString("http://localhost:" + port).pathSegment("billing", method);
    }

    @BeforeEach
    void resetMock() {
        Mockito.reset(testContentProvider);
        delegateContentProvider.setDelegate(testContentProvider);

        when(yandexPayClient.getMerchantByKey(eq(TEST_MERCHANT_KEY)))
                .thenReturn(MERCHANT);
    }

    @Test
    void testGetMetaInfoRepsSmoke() throws IOException {
        when(testContentProvider.getContentMetaInfo(Mockito.any())).thenReturn(new ContentMetaInfo(
                "testTitle",
                "testImageUrl",
                123,
                234,
                "345",
                "testCountry",
                null,
                null)
        );

        ResponseEntity<String> result = testRestTemplate.getForEntity(
                url("getContentMetaInfo")
                        .queryParam("contentItem", JsonUtil.toJsonQuotes("{'type': 'film', '" + PROVIDER_NAME + "': " +
                                "{'id': '123456'}}"))
                        .build().encode().toUri(),
                String.class);

        assertEquals(result.getStatusCode(), HttpStatus.OK);

        ContentMetaInfo responseContentMetaInfo = objectMapper.readValue(result.getBody(), ContentMetaInfo.class);

        assertEquals(responseContentMetaInfo.getTitle(), "testTitle");
        assertEquals(responseContentMetaInfo.getImageUrl(), "testImageUrl");
        assertEquals(Integer.valueOf(123), responseContentMetaInfo.getYear());
        assertEquals(Integer.valueOf(234), responseContentMetaInfo.getDurationMinutes());
        assertEquals("345", responseContentMetaInfo.getAgeRestriction());
        assertEquals("testCountry", responseContentMetaInfo.getCountry());
    }


    @Test
    void testGetMetaInfoReqSmoke() {
        testRestTemplate.getForEntity(
                url("getContentMetaInfo")
                        .queryParam("contentItem", JsonUtil.toJsonQuotes("{'type': 'film', '" + PROVIDER_NAME + "': " +
                                "{'id': '123456'}}"))
                        .build().encode().toUri(),  // NB: should use toURI not to encode it all twice
                String.class);

        ArgumentCaptor<ProviderContentItem> captor = ArgumentCaptor.forClass(ProviderContentItem.class);

        verify(testContentProvider).getContentMetaInfo(captor.capture());

        assertEquals("123456", captor.getValue().getId());
        assertEquals(ContentType.MOVIE, captor.getValue().getContentType());
    }

    @Test
    void testPricingOptionsNewStyle() {
        URI uri = url("pricingOptions")
                .queryParam("contentItem", JsonUtil.toJsonQuotes("{'type': 'film', '" + PROVIDER_NAME + "': {'id': " +
                        "'123456'}}"))
                .build().toUri();

        checkPricingOptions(uri);
    }

    private void checkPricingOptions(URI uri) {
        mockAuth();

        ProviderContentItem purchasingItem = ProviderContentItem.create(ContentType.SUBSCRIPTION,
                "test_subscription_id");
        PricingOption pricingOption = createSubscriptionPricingOption(PROVIDER_NAME, BigDecimal.ONE, purchasingItem,
                LogicalPeriod.ofDays(1));
        when(testContentProvider.getPricingOptions(any(), any())).thenReturn(ProviderPricingOptions.create(true,
                Collections.singletonList(pricingOption), null));

        ResponseEntity<String> result = testRestTemplate.getForEntity(uri, String.class);

        assertEquals(HttpStatus.OK, result.getStatusCode());

        ArgumentCaptor<ProviderContentItem> captor = ArgumentCaptor.forClass(ProviderContentItem.class);
        verify(testContentProvider).getPricingOptions(captor.capture(), eq("222"));
        ProviderContentItem captorValue = captor.getValue();

        assertEquals("123456", captorValue.getId());
        assertEquals(ContentType.MOVIE, captorValue.getContentType());

    }

    /**
     * setup our authorizationService to return mock-like results.
     * We are using a spy so it normally just works as a regular authorizationService
     */
    private void mockAuth() {
        doReturn(UID).when(authorizationService).getSecretUid(any());
        doReturn("127.0.0.1").when(authorizationService).getUserIp(any());
        doReturn(Optional.of("222")).when(authorizationService).getProviderTokenByUid(any(), any());
    }

    @Test
    @SuppressWarnings("unchecked")
    void checkBadRequest() throws IOException {
        final ZonedDateTime beforeCall = ZonedDateTime.now();

        BadRequestException exception = BadRequestException.unsupportedContentType(ContentType.MOVIE);

        doThrow(exception).when(testContentProvider).getContentMetaInfo(any());

        ResponseEntity<String> responseEntity = testRestTemplate.getForEntity(
                url("getContentMetaInfo")
                        .queryParam("contentItem", JsonUtil.toJsonQuotes("{'type': 'film', '" + PROVIDER_NAME + "': " +
                                "{'id': '123'}}"))
                        .build().toUri(),
                String.class
        );

        assertEquals(HttpStatus.BAD_REQUEST, responseEntity.getStatusCode());

        final ZonedDateTime afterCallDate = ZonedDateTime.now();

        assert afterCallDate.compareTo(beforeCall) > 0;

        assertThat(
                (Map<String, String>) new ObjectMapper().readValue(responseEntity.getBody(), HashMap.class),
                allOf(
                        hasEntry("message", "Unsupported type film"),
                        hasEntry("time", exception.getExceptionTime()),
                        hasEntry("id", exception.getExceptionId())
                )
        );
    }


    @Test
    void testFailOnInvalidSessionException() {
        ProviderUnauthorizedException exception = new ProviderUnauthorizedException("testProviderName", true);

        doReturn("999").when(authorizationService).getSecretUid(any(HttpServletRequest.class));
        doReturn(Optional.of("testSession")).when(authorizationService).getProviderTokenByUid(any(), any());
        doThrow(exception).when(testContentProvider).getPricingOptions(any(), eq("testSession"));

        ResponseEntity<String> responseEntity = testRestTemplate.getForEntity(
                url("pricingOptions")
                        .queryParam("contentItem", JsonUtil.toJsonQuotes("{'type': 'film', '" + PROVIDER_NAME + "': " +
                                "{'id': '123'}}"))
                        .build().toUri(),
                String.class
        );

        assertEquals(HttpStatus.UNAUTHORIZED, responseEntity.getStatusCode());
        assertEquals("testProviderName", new JSONObject(responseEntity.getBody()).getString("providerName"));
    }

    @Test
    @DisplayName("Проверка, что покупки фильмов через Траст правильно попадают в историю покупок")
    void testGetTransactionsHistoryV2Trust() {
        mockAuth();

        ProviderContentItem contentItem = ProviderContentItem.create(ContentType.MOVIE, "film1");
        PricingOption pricingOption = createPricingOption(delegateContentProvider.getProviderName(),
                BigDecimal.valueOf(5), BigDecimal.TEN, PricingOptionType.BUY, contentItem);

        when(userPurchasesDAO.getPurchases(Long.valueOf(UID), 1, 0))
                .thenReturn(List.of(
                        PurchaseInfo.createSinglePayment(1L, Long.valueOf(UID), "purchaseToken_1",
                        contentItem, pricingOption, PurchaseInfo.Status.CLEARED, "secToken", 1L, null,
                        delegateContentProvider.getProviderName(), null, PaymentProcessor.TRUST, null, null)
                ));

        when(processingServiceManager.get(PaymentProcessor.TRUST)).thenReturn(trustServiceMock);

        when(trustServiceMock.getPaymentShortInfo(any(), any(), any())).thenReturn(
                new PaymentInfo("token", "card-xxx", "12123123123123",
                        BigDecimal.TEN, "RUB", Instant.parse("2012-08-30T00:00:00Z"), "MasterCard",
                        PaymentInfo.Status.CLEARED, Instant.now()
        ));

        URI url = url("v2/getTransactionsHistory")
                .queryParam("offset", 0)
                .queryParam("limit", 1)
                .build().toUri();

        var response = testRestTemplate.getForEntity(url, GetTransactionsHistoryV2Response.class).getBody();

        assertNotNull(response);
        assertEquals(1, response.getItems().size());
        var item = response.getItems().get(0);
        assertEquals(1, item.getBasket().size());
        var basket = item.getBasket().get(0);

        assertAll(
                () -> assertEquals(1L, item.getPurchaseId()),
                () -> assertEquals("testProvider", item.getPurchaseType()),
                () -> assertNull(item.getSkillInfo()),
                () -> assertEquals("12123123123123", item.getMaskedCardNumber()),
                () -> assertEquals("MasterCard", item.getPaymentSystem()),
                () -> assertEquals(BigDecimal.TEN, item.getTotalPrice()),
                () -> assertEquals("RUB", item.getCurrency()),
                () -> assertEquals(TransactionItem.Status.SUCCESS, item.getStatus()),
                () -> assertEquals(Instant.parse("2012-08-30T00:00:00Z"), item.getStatusChangeDate()),
                () -> assertEquals(TransactionItem.PricingType.BUY, item.getPricingType()),

                () -> assertEquals(BigDecimal.valueOf(5), basket.getUserPrice()),
                () -> assertEquals(BigDecimal.TEN, basket.getPrice()),
                () -> assertEquals("title", basket.getTitle()),
                () -> assertEquals(BigDecimal.ONE, basket.getQuantity()),
                () -> assertEquals(NdsType.nds_20, basket.getNdsType())
        );
    }

    @Test
    @DisplayName("Проверка, что покупки внутри навыков через Яднес.Оплату правильно попадают в историю покупок")
    void testGetTransactionsHistoryV2Skill() {
        mockAuth();
        PricingOption pricingOption = createPricingOption("skill", BigDecimal.valueOf(5),
                BigDecimal.TEN, PricingOptionType.BUY, null);

        when(userPurchasesDAO.getPurchases(Long.valueOf(UID), 1, 0))
                .thenReturn(List.of(
                        PurchaseInfo.createSinglePayment(1L, Long.valueOf(UID), "purchaseToken_1",
                                null, pricingOption, PurchaseInfo.Status.AUTHORIZED, "secToken", 1L, 12L,
                                "skill", 1L, PaymentProcessor.YANDEX_PAY, null, "ООО Ком")
                ));

        when(purchaseOfferDao.findBySkillIdAndPurchaseOfferId(1L, 12L))
                .thenReturn(Optional.of(PurchaseOffer.builder("999", 1L, "purchase_id",
                            List.of(createSkillPricingOption("PRODUCT_UID", BigDecimal.ONE, BigDecimal.TEN)),
                            "https://callbackUrl")
                        .title("title")
                        .skillSessionId("session")
                        .skillUserId("3343")
                        .skillName("Друзья")
                        .skillImageUrl("https://temp.ru")
                        .merchantKey("m_key")
                        .build()));

        when(processingServiceManager.get(PaymentProcessor.YANDEX_PAY)).thenReturn(yaPayBillingService);

        when(yaPayBillingService.getPaymentShortInfo(any(), any(), any())).thenReturn(
                new PaymentInfo("token", "card-xxx", "12123123123123",
                        BigDecimal.TEN, "RUB", Instant.parse("2012-08-30T00:00:00Z"), "MasterCard",
                        PaymentInfo.Status.AUTHORIZED, Instant.now()
                ));

        URI url = url("v2/getTransactionsHistory")
                .queryParam("offset", 0)
                .queryParam("limit", 1)
                .build().toUri();

        var response = testRestTemplate.getForEntity(url, GetTransactionsHistoryV2Response.class).getBody();

        assertNotNull(response);
        var item = response.getItems().get(0);
        assertEquals(1, response.getItems().size());
        var skillInfo = item.getSkillInfo();
        assertEquals(1, item.getBasket().size());
        var basket = item.getBasket().get(0);

        assertAll(
                () -> assertEquals(1L, item.getPurchaseId()),
                () -> assertEquals("skill", item.getPurchaseType()),

                () -> assertNotNull(skillInfo),
                () -> assertEquals("Друзья", skillInfo.getName()),
                () -> assertEquals("ООО Ком", skillInfo.getMerchantName()),
                () -> assertEquals("https://temp.ru", skillInfo.getLogoUrl()),

                () -> assertEquals("12123123123123", item.getMaskedCardNumber()),
                () -> assertEquals("MasterCard", item.getPaymentSystem()),
                () -> assertEquals(BigDecimal.TEN, item.getTotalPrice()),
                () -> assertEquals("RUB", item.getCurrency()),
                () -> assertEquals(TransactionItem.Status.WAITING_FOR_PURCHASE, item.getStatus()),
                () -> assertEquals(Instant.parse("2012-08-30T00:00:00Z"), item.getStatusChangeDate()),
                () -> assertEquals(TransactionItem.PricingType.BUY, item.getPricingType()),

                () -> assertEquals(BigDecimal.valueOf(5), basket.getUserPrice()),
                () -> assertEquals(BigDecimal.TEN, basket.getPrice()),
                () -> assertEquals("title", basket.getTitle()),
                () -> assertEquals(BigDecimal.ONE, basket.getQuantity()),
                () -> assertEquals(NdsType.nds_20, basket.getNdsType())
        );
    }

    @Test
    @DisplayName("Проверка, что бесплатные активации продуктов навыков правильно попадают в историю покупок")
    void testGetTransactionsHistoryV2FreeProductActivation() {
        mockAuth();

        PricingOption.PricingOptionLine pricingOptionLine = new PricingOption.PricingOptionLine("id",
                BigDecimal.ZERO, BigDecimal.ZERO, "Анна", BigDecimal.ONE, NdsType.nds_none);
        var pricingOption = PricingOption.builder("Анна", PricingOptionType.BUY, BigDecimal.ZERO,
                BigDecimal.ZERO, "RUB").items(List.of(pricingOptionLine)).build();

        PurchaseInfo payment = PurchaseInfo.createFreeSkillProductPayment(1L, Long.valueOf(UID),
                "purchaseToken_1", pricingOption, PurchaseInfo.Status.CLEARED, 5L, "skill", 1L);

        when(userPurchasesDAO.getPurchases(Long.valueOf(UID), 1, 0))
                .thenReturn(List.of(payment));
        when(userPurchasesDAO.getPurchaseInfo("purchaseToken_1"))
                .thenReturn(Optional.of(payment))
                .thenReturn(Optional.of(payment));

        when(userSkillProductDao.getUserSkillProductById(5L))
                .thenReturn(Optional.of(new UserSkillProduct(5L, UID, null, getSkillProduct(), "Друзья", "temp.ru")));

        when(processingServiceManager.get(PaymentProcessor.FREE)).thenReturn(freeProcessingBillingService);

        URI url = url("v2/getTransactionsHistory")
                .queryParam("offset", 0)
                .queryParam("limit", 1)
                .build().toUri();

        var response = testRestTemplate.getForEntity(url, GetTransactionsHistoryV2Response.class).getBody();

        assertNotNull(response);
        var item = response.getItems().get(0);
        assertEquals(1, response.getItems().size());
        var skillInfo = item.getSkillInfo();
        assertEquals(1, item.getBasket().size());
        var basket = item.getBasket().get(0);

        assertAll(
                () -> assertEquals(1L, item.getPurchaseId()),
                () -> assertEquals("skill", item.getPurchaseType()),

                () -> assertNotNull(skillInfo),
                () -> assertEquals("Друзья", skillInfo.getName()),
                () -> assertEquals("temp.ru", skillInfo.getLogoUrl()),
                () -> assertNull(skillInfo.getMerchantName()),

                () -> assertEquals(BigDecimal.ZERO, item.getTotalPrice()),
                () -> assertEquals("RUB", item.getCurrency()),
                () -> assertEquals(TransactionItem.Status.SUCCESS, item.getStatus()),
                () -> assertEquals(TransactionItem.PricingType.BUY, item.getPricingType()),

                () -> assertEquals(BigDecimal.ZERO, basket.getUserPrice()),
                () -> assertEquals(BigDecimal.ZERO, basket.getPrice()),
                () -> assertEquals("Анна", basket.getTitle()),
                () -> assertEquals(BigDecimal.ONE, basket.getQuantity()),
                () -> assertEquals(NdsType.nds_none, basket.getNdsType())
        );
    }

    @Nonnull
    private SkillProduct getSkillProduct() {
        return new SkillProduct(UUID.randomUUID(), "skill", "Анна",
                SkillProductType.NON_CONSUMABLE, BigDecimal.ZERO, false);
    }

    @Test
    @DisplayName("Проверка, что покупка наёдена по purchase_id")
    void testGetTransactionsItem() {
        mockAuth();
        PricingOption pricingOption = createPricingOption("skill", BigDecimal.valueOf(5),
                BigDecimal.TEN, PricingOptionType.BUY, null);

        when(userPurchasesDAO.getPurchaseInfo(Long.valueOf(UID), 1L))
                .thenReturn(Optional.of(
                        PurchaseInfo.createSinglePayment(1L, Long.valueOf(UID), "purchaseToken_1",
                                null, pricingOption, PurchaseInfo.Status.AUTHORIZED, "secToken", 1L, 12L,
                                "skill", 1L, PaymentProcessor.YANDEX_PAY, null, "ООО Ком")
                ));

        when(purchaseOfferDao.findBySkillIdAndPurchaseOfferId(1L, 12L))
                .thenReturn(Optional.of(PurchaseOffer.builder("999", 1L, "purchase_id",
                        List.of(createSkillPricingOption("PRODUCT_UID", BigDecimal.ONE, BigDecimal.TEN)),
                        "https://callbackUrl")
                        .title("title")
                        .skillSessionId("session")
                        .skillUserId("3343")
                        .skillName("Друзья")
                        .skillImageUrl("https://temp.ru")
                        .merchantKey("m_key")
                        .build()));

        when(processingServiceManager.get(PaymentProcessor.YANDEX_PAY)).thenReturn(yaPayBillingService);

        when(yaPayBillingService.getPaymentShortInfo(any(), any(), any())).thenReturn(
                new PaymentInfo("token", "card-xxx", "12123123123123",
                        BigDecimal.TEN, "RUB", Instant.parse("2012-08-30T00:00:00Z"), "MasterCard",
                        PaymentInfo.Status.AUTHORIZED, Instant.now()
                ));

        URI url = url("v2/transaction/1").build().toUri();

        var item = testRestTemplate.getForEntity(url, TransactionItem.class).getBody();

        assertNotNull(item);
        var skillInfo = item.getSkillInfo();
        assertEquals(1, item.getBasket().size());
        var basket = item.getBasket().get(0);

        assertAll(
                () -> assertEquals(1L, item.getPurchaseId()),
                () -> assertEquals("skill", item.getPurchaseType()),

                () -> assertNotNull(skillInfo),
                () -> assertEquals("Друзья", skillInfo.getName()),
                () -> assertEquals("ООО Ком", skillInfo.getMerchantName()),
                () -> assertEquals("https://temp.ru", skillInfo.getLogoUrl()),

                () -> assertEquals("12123123123123", item.getMaskedCardNumber()),
                () -> assertEquals("MasterCard", item.getPaymentSystem()),
                () -> assertEquals(BigDecimal.TEN, item.getTotalPrice()),
                () -> assertEquals("RUB", item.getCurrency()),
                () -> assertEquals(TransactionItem.Status.WAITING_FOR_PURCHASE, item.getStatus()),
                () -> assertEquals(Instant.parse("2012-08-30T00:00:00Z"), item.getStatusChangeDate()),
                () -> assertEquals(TransactionItem.PricingType.BUY, item.getPricingType()),

                () -> assertEquals(BigDecimal.valueOf(5), basket.getUserPrice()),
                () -> assertEquals(BigDecimal.TEN, basket.getPrice()),
                () -> assertEquals("title", basket.getTitle()),
                () -> assertEquals(BigDecimal.ONE, basket.getQuantity()),
                () -> assertEquals(NdsType.nds_20, basket.getNdsType())
        );
    }

    @Test
    void testGetSubscriptionsInfoOnActiveSubs() {
        mockAuth();
        doReturn(emptyMap()).when(amediatekaContentProvider).getActiveSubscriptions(any());
        Instant activeTill = Instant.now().plusSeconds(60 * 60);

        ProviderContentItem contentItem = ProviderContentItem.create(ContentType.SUBSCRIPTION, "1d");
        ProviderContentItem contentItem2 = ProviderContentItem.create(ContentType.SUBSCRIPTION, "2d");


        when(testContentProvider.getActiveSubscriptions(anyString()))
                .thenReturn(Map.of(contentItem, ProviderActiveSubscriptionInfo.builder(contentItem)
                        .title(contentItem.toString())
                        .activeTill(activeTill)
                        .build()));

        PricingOption pricingOption = createSubscriptionPricingOption(PROVIDER_NAME, BigDecimal.TEN, contentItem2,
                LogicalPeriod.ofDays(60));
        PricingOption pricingOption2 = createSubscriptionPricingOption(PROVIDER_NAME, BigDecimal.TEN, contentItem2,
                LogicalPeriod.ofDays(30));

        // when
        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        headers.add("Authorization", "wrongToken");

        ResponseEntity<GetSubscriptionsInfoResponse> responseEntity = testRestTemplate.exchange(url(
                "getSubscriptionsInfo").queryParam("provider", PROVIDER_NAME).build().toUri(),
                HttpMethod.GET,
                new HttpEntity<>(headers),
                GetSubscriptionsInfoResponse.class);

        assertEquals(HttpStatus.OK, responseEntity.getStatusCode());
        GetSubscriptionsInfoResponse.Item activeSubItem =
                GetSubscriptionsInfoResponse.Item.builder(delegateContentProvider.getProviderName(), contentItem,
                        contentItem.toString())
                        .activeTill(activeTill)
                        .subscriptionId(null)
                        .nextPaymentDate(null)
                        // .cancellable(false)
                        .pricingOption(null)
                        .providerLoginRequired(false)
                        .build();

        assertEquals(new GetSubscriptionsInfoResponse(List.of(activeSubItem)),
                responseEntity.getBody());
    }

    @Test
    void testGetCardsList() {
        // given
        mockAuth();
        PaymentMethod paymentMethod = PaymentMethod.builder("card-x8508", "card")
                .system("MasterCard")
                .account("510000****1573")
                .cardBank("RBS BANK (ROMANIA), S.A.")
                .expirationMonth(4)
                .expirationYear(2019)
                .holder("TEST TEST")
                .paymentSystem("MasterCard")
                .regionId(225)
                .expired(false)
                .build();

        when(trustBillingClient.getCardsList(eq(UID), anyString()))
                .thenReturn(List.of(paymentMethod));

        // when
        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        headers.add("Authorization", "some_token");

        when(processingServiceManager.get(PaymentProcessor.TRUST)).thenReturn(trustService);

        ResponseEntity<GetCardsListResponse> responseEntity = testRestTemplate.exchange(url("getCardsList")
                        .queryParam("processor", "TRUST")
                        .build()
                        .toUri(),
                HttpMethod.GET,
                new HttpEntity<>(headers),
                GetCardsListResponse.class);

        // then
        assertEquals(HttpStatus.OK, responseEntity.getStatusCode());
        GetCardsListResponse.CardInfo expectedCard = GetCardsListResponse.CardInfo.builder()
                .id("card-x8508")
                .paymentMethod("card")
                .system("MasterCard")
                .account("510000****1573")
                .paymentSystem("MasterCard")
                .expired(false)
                .build();
        assertEquals(List.of(expectedCard), responseEntity.getBody().getCardList());
    }

    @Test
    void testGetPurchasedContent() {
        mockAuth();
        ProviderContentItem contentItem = ProviderContentItem.create(ContentType.MOVIE, "film1");
        PricingOption pricingOption = createPricingOption(delegateContentProvider.getProviderName(),
                BigDecimal.TEN,
                PricingOptionType.BUY,
                contentItem);
        ContentMetaInfo contentMetaInfo = new ContentMetaInfo(
                "testTitle",
                "testImageUrl",
                123,
                234,
                "345",
                "testCountry",
                null,
                null);
        when(userPurchasesDAO.getLastContentPurchases(Long.valueOf(UID)))
                .thenReturn(List.of(PurchaseInfo.createSinglePayment(1L, Long.valueOf(UID), "purchaseToken",
                        contentItem, pricingOption, PurchaseInfo.Status.CLEARED, "secToken", 1L, null,
                        delegateContentProvider.getProviderName(), null, PaymentProcessor.TRUST, null, null)));

        when(testContentProvider.getContentMetaInfo(contentItem))
                .thenReturn(contentMetaInfo);

        // when
        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        headers.add("Authorization", "OAuth some_token");

        ResponseEntity<GetContentPurchasesResponse> responseEntity = testRestTemplate.exchange(url(
                "getPurchasedContent")
                        .build().toUri(),
                HttpMethod.GET,
                new HttpEntity<>(headers),
                GetContentPurchasesResponse.class);

        // then
        GetContentPurchasesResponse expected = new GetContentPurchasesResponse(
                List.of(new GetContentPurchasesResponse.Item(contentMetaInfo,
                        delegateContentProvider.getProviderName(), contentItem))
        );
        assertEquals(expected, responseEntity.getBody());

    }

    @Test
    void testGetPurchasedContentOnDuplicateItems() {
        mockAuth();
        ProviderContentItem contentItem = ProviderContentItem.create(ContentType.MOVIE, "film1");
        PricingOption pricingOption = createPricingOption(delegateContentProvider.getProviderName(),
                BigDecimal.TEN,
                PricingOptionType.BUY,
                contentItem);
        ContentMetaInfo contentMetaInfo = new ContentMetaInfo(
                "testTitle",
                "testImageUrl",
                123,
                234,
                "345",
                "testCountry",
                null,
                null);
        when(userPurchasesDAO.getLastContentPurchases(Long.valueOf(UID)))
                .thenReturn(List.of(PurchaseInfo.createSinglePayment(1L, Long.valueOf(UID), "purchaseToken",
                        contentItem, pricingOption, PurchaseInfo.Status.CLEARED, "secToken", 1L, null,
                        delegateContentProvider.getProviderName(), null, PaymentProcessor.TRUST, null, null),
                        PurchaseInfo.createSinglePayment(2L, Long.valueOf(UID), "purchaseToken2", contentItem,
                                pricingOption, PurchaseInfo.Status.CLEARED, "secToken2", 1L, null,
                                delegateContentProvider.getProviderName(), null, PaymentProcessor.TRUST, null, null)));

        when(testContentProvider.getContentMetaInfo(contentItem))
                .thenReturn(contentMetaInfo);

        // when
        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        headers.add("Authorization", "OAuth some_token");

        ResponseEntity<GetContentPurchasesResponse> responseEntity = testRestTemplate.exchange(url(
                "getPurchasedContent")
                        .build().toUri(),
                HttpMethod.GET,
                new HttpEntity<>(headers),
                GetContentPurchasesResponse.class);

        // then
        // duplicated records should be eliminated
        GetContentPurchasesResponse expected = new GetContentPurchasesResponse(
                List.of(new GetContentPurchasesResponse.Item(contentMetaInfo,
                        delegateContentProvider.getProviderName(), contentItem))
        );
        assertEquals(expected, responseEntity.getBody());

    }

    @Test
    void testContentAvailable() throws JsonProcessingException {
        // given
        mockAuth();
        ProviderContentItem providerContentItem = ProviderContentItem.create(ContentType.MOVIE, "1");
        ContentItem contentItem = new ContentItem(PROVIDER_NAME, providerContentItem);
        when(testContentProvider.getAvailability(eq(providerContentItem), anyString(), any(), eq(STATION_USER_AGENT),
                anyBoolean()))
                .thenReturn(AvailabilityInfo.available(false, null, dummyStreamData));

        // when
        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        headers.add("User-Agent", STATION_USER_AGENT);
        ResponseEntity<AvailabilityResult> actual = testRestTemplate.exchange(url("content_available")
                        .queryParam("content_item", objectMapper.writeValueAsString(contentItem))
                        .build()
                        .encode()
                        .toUri(),
                HttpMethod.GET,
                new HttpEntity<>(headers),
                AvailabilityResult.class);

        // then
        assertEquals(HttpStatus.OK, actual.getStatusCode());
        AvailabilityResult expected = new AvailabilityResult(Map.of(PROVIDER_NAME,
                new AvailabilityResult.ProviderAvailabilityState(AvailabilityResult.Availability.AVAILABLE, null,
                        null)));
        assertEquals(expected, actual.getBody());
    }

    @Test
    void testInitSkillPurchaseOk() throws SkillsService.SkillAccessViolationException, IOException {
        // given
        mockAuth();
        String skillId = "3ad36498-f5rd-4079-a14b-788652932056";
        SkillInfo skill = skillsService.registerSkill(skillId, Long.valueOf(UID), "skillslug");

        var request = new OfferSkillPurchaseRequest(
                new OfferSkillPurchaseRequest.SkillInfo(skillId, "skill name", "img url", "https://url_to_skill/"),
                "session",
                "userid",
                null,
                new OfferSkillPurchaseRequest.PurchaseRequest(
                        "1674b49eed8f4de28c8cebfe411930d8",
                        "http://url_to_image",
                        "Заказ 111",
                        "Описание букета",
                        TrustCurrency.RUB,
                        PricingOptionType.BUY,
                        (ObjectNode) objectMapper.createObjectNode().set("a", new TextNode("val")),
                        TEST_MERCHANT_KEY,
                        List.of(new OfferSkillPurchaseRequest.PurchaseRequestProduct(
                                "8674b49eed8f4de28c8cebfe411930d7", "Букет из 15 Роз", BigDecimal.valueOf(199),
                                BigDecimal.valueOf(299), BigDecimal.ONE, NdsType.nds_20)),
                        false,
                        null
                ),
                objectMapper.readValue("{ }", ObjectNode.class)
        );

        doReturn(new BillingService.CreatedOffer("UUID", "http://url"))
                .when(billingService).createSkillPurchaseOffer(anyString(), refEq(skill, "createdAt"), any(), any(),
                any(), anyString(), anyString(), anyString());

        // when
        var headers = new LinkedMultiValueMap<String, String>();
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER, getTestTicket(TvmClientName.bass));
        headers.add("Authorization", "OAuth some_token");
        ResponseEntity<InitSkillPurchaseResponse> responseEntity = testRestTemplate.postForEntity(url(
                "createSkillPurchaseOffer")
                        .queryParam("deviceId", "deviceId")
                        .build()
                        .toUri(),
                new HttpEntity<>(request, headers),
                InitSkillPurchaseResponse.class);

        // then
        assertEquals(HttpStatus.OK, responseEntity.getStatusCode());
        assertEquals("UUID", responseEntity.getBody().getOrderId());
    }

    @Test
    void testInitSkillPurchaseWrongRequestId() throws SkillsService.SkillAccessViolationException {
        // given
        mockAuth();
        String skillId = "3ad36498-f5rd-4079-a14b-788652932056";
        SkillInfo skill = skillsService.registerSkill(skillId, Long.valueOf(UID), "skillslug");

        var request = new OfferSkillPurchaseRequest(
                new OfferSkillPurchaseRequest.SkillInfo(skillId, "skill name", "img url", "https://url_to_skill/"),
                "session",
                "userid",
                null,
                // invalid purchaseRequestId
                new OfferSkillPurchaseRequest.PurchaseRequest(
                        "wrong value",
                        "http://url_to_image",
                        "Заказ 111",
                        "Описание букета",
                        TrustCurrency.RUB,
                        PricingOptionType.BUY,
                        (ObjectNode) objectMapper.createObjectNode().set("a", new TextNode("val")),
                        TEST_MERCHANT_KEY,
                        List.of(new OfferSkillPurchaseRequest.PurchaseRequestProduct(
                                "8674b49eed8f4de28c8cebfe411930d7", "Букет из 15 Роз", BigDecimal.valueOf(199),
                                BigDecimal.valueOf(299), BigDecimal.ONE, NdsType.nds_20)),
                        false,
                        null
                ),
                null
        );

        doReturn(new BillingService.CreatedOffer("UUID", "http://url"))
                .when(billingService).createSkillPurchaseOffer(
                        anyString(),
                        refEq(skill, "createdAt"),
                        any(BillingService.SkillOfferParams.class),
                        any(),
                        any(),
                        anyString(),
                        anyString(),
                        anyString()
                );

        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER, getTestTicket(TvmClientName.bass));
        headers.add("Authorization", "OAuth some_token");

        // when
        ResponseEntity<String> responseEntity = testRestTemplate.postForEntity(url("createSkillPurchaseOffer")
                        .queryParam("deviceId", "deviceId")
                        .build()
                        .toUri(),
                new HttpEntity<>(request, headers),
                String.class);

        // then
        System.out.println(responseEntity.getBody());
        assertEquals(HttpStatus.BAD_REQUEST, responseEntity.getStatusCode());

    }

    @Test
    void testInitSkillPurchaseWrongProductId() throws SkillsService.SkillAccessViolationException {
        // given
        mockAuth();
        String skillId = "3ad36498-f5rd-4079-a14b-788652932056";
        SkillInfo skill = skillsService.registerSkill(skillId, Long.valueOf(UID), "skillslug");

        var request = new OfferSkillPurchaseRequest(
                new OfferSkillPurchaseRequest.SkillInfo(skillId, "skill name", "img url", "https://url_to_skill/"),
                "session",
                "userid",
                null,
                new OfferSkillPurchaseRequest.PurchaseRequest(
                        "1674b49eed8f4de28c8cebfe411930d8",
                        "http://url_to_image",
                        "Заказ 111",
                        "Описание букета",
                        TrustCurrency.RUB,
                        PricingOptionType.BUY,
                        (ObjectNode) objectMapper.createObjectNode().set("a", new TextNode("val")),
                        TEST_MERCHANT_KEY,
                        // empty productId
                        List.of(new OfferSkillPurchaseRequest.PurchaseRequestProduct("", "Букет из 15 Роз",
                                BigDecimal.valueOf(199), BigDecimal.valueOf(299), BigDecimal.ONE, NdsType.nds_20)),
                        false,
                        null
                ),
                null
        );


        doReturn(new BillingService.CreatedOffer("UUID", "http://url"))
                .when(billingService).createSkillPurchaseOffer(
                        anyString(),
                        refEq(skill, "createdAt"),
                        any(BillingService.SkillOfferParams.class),
                        any(),
                        any(),
                        anyString(),
                        anyString(),
                        anyString()
                );

        // when
        MultiValueMap<String, String> headers = new LinkedMultiValueMap<>();
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER, getTestTicket(TvmClientName.bass));
        headers.add("Authorization", "OAuth some_token");

        ResponseEntity<String> responseEntity = testRestTemplate.postForEntity(url("createSkillPurchaseOffer")
                        .queryParam("deviceId", "deviceId")
                        .build()
                        .toUri(),
                new HttpEntity<>(request, headers),
                String.class);

        // then
        System.out.println(responseEntity.getBody());
        assertEquals(HttpStatus.BAD_REQUEST, responseEntity.getStatusCode());
    }

}
