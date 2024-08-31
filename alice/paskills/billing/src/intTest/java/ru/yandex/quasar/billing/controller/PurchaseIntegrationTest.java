package ru.yandex.quasar.billing.controller;

import java.io.IOException;
import java.math.BigDecimal;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.fasterxml.jackson.databind.node.TextNode;
import org.hamcrest.Matchers;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.context.annotation.Bean;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.HttpServerErrorException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.ContentItem;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.controller.OfferSkillPurchaseRequest.PurchaseDeliveryInfo;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.PurchaseOffer;
import ru.yandex.quasar.billing.dao.PurchaseOfferDao;
import ru.yandex.quasar.billing.dao.PurchaseOfferStatus;
import ru.yandex.quasar.billing.dao.SkillInfo;
import ru.yandex.quasar.billing.dao.SubscriptionInfo;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.dao.UserSubscriptionsDAO;
import ru.yandex.quasar.billing.providers.IntegrationTestProvider;
import ru.yandex.quasar.billing.providers.ProviderPurchaseException;
import ru.yandex.quasar.billing.services.BillingService;
import ru.yandex.quasar.billing.services.PaymentInfo;
import ru.yandex.quasar.billing.services.RefundInfo;
import ru.yandex.quasar.billing.services.TestAuthorizationService;
import ru.yandex.quasar.billing.services.mediabilling.OrderInfo;
import ru.yandex.quasar.billing.services.mediabilling.TestMediaBillingClient;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;
import ru.yandex.quasar.billing.services.processing.trust.PaymentRespStatus;
import ru.yandex.quasar.billing.services.processing.trust.Purchase;
import ru.yandex.quasar.billing.services.processing.trust.RefundStatus;
import ru.yandex.quasar.billing.services.processing.trust.TestTrustClient;
import ru.yandex.quasar.billing.services.processing.trust.TestTrustClientConfig;
import ru.yandex.quasar.billing.services.processing.yapay.OrderRecord;
import ru.yandex.quasar.billing.services.processing.yapay.TestYaPayClientConfig;
import ru.yandex.quasar.billing.services.processing.yapay.TestYandexPayClient;
import ru.yandex.quasar.billing.services.processing.yapay.YaPayBillingService;
import ru.yandex.quasar.billing.services.processing.yapay.YandexPayMerchantTestUtil;
import ru.yandex.quasar.billing.services.skills.SkillsService;
import ru.yandex.quasar.billing.services.skills.SkillsServiceImpl.PurchaseEventRequest;
import ru.yandex.quasar.billing.services.skills.SkillsServiceImpl.PurchaseEventResponse;
import ru.yandex.quasar.billing.services.tvm.TvmClientName;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.comparesEqualTo;
import static org.junit.jupiter.api.Assertions.assertAll;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doThrow;
import static ru.yandex.quasar.billing.controller.OfferSkillPurchaseRequest.PurchaseRequest;
import static ru.yandex.quasar.billing.controller.OfferSkillPurchaseRequest.PurchaseRequestProduct;
import static ru.yandex.quasar.billing.providers.IntegrationTestProvider.AVAILABLE_SUBSCRIPTION;
import static ru.yandex.quasar.billing.providers.IntegrationTestProvider.MEDIABILLING_SUB_FILM;
import static ru.yandex.quasar.billing.providers.IntegrationTestProvider.OTT_TVOD_FILM;
import static ru.yandex.quasar.billing.providers.IntegrationTestProvider.PAID_FILM;
import static ru.yandex.quasar.billing.providers.IntegrationTestProvider.YA_SUBSCRIPTION_PREMIUM;
import static ru.yandex.quasar.billing.services.TestAuthorizationService.PROVIDER_TOKEN;
import static ru.yandex.quasar.billing.services.TestAuthorizationService.UID;
import static ru.yandex.quasar.billing.services.processing.NdsType.nds_20;
import static ru.yandex.quasar.billing.services.processing.trust.Purchase.PaymentState.started;
import static ru.yandex.quasar.billing.services.processing.trust.Purchase.PaymentStatus.canceled;
import static ru.yandex.quasar.billing.services.processing.trust.Purchase.PaymentStatus.cleared;
import static ru.yandex.quasar.billing.services.processing.yapay.TestYandexPayClient.TEST_MERCHANT_ID;
import static ru.yandex.quasar.billing.services.tvm.TestTvmClientImpl.getTestTicket;

/**
 * Dont check purchaseInfo status directly, check it through getPurchaseStatus
 */
@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT,
        classes = {
                TestConfigProvider.class,
                PurchaseIntegrationTest.IntegrationTestConfig.class,
                TestTrustClientConfig.class,
                TestYaPayClientConfig.class
        }
)
@AutoConfigureWebClient(registerRestTemplate = true)
class PurchaseIntegrationTest implements YandexPayMerchantTestUtil {
    private static final String SKILL_UUID = "3712277b-0bf3-468c-a868-fd82c8eac855";

    private static final ObjectNode PAYLOAD = (ObjectNode) new ObjectMapper().createObjectNode()
            .set("a", new TextNode("val12"));

    @Autowired
    private ObjectMapper objectMapper;
    @SpyBean
    private IntegrationTestProvider contentProvider; // this is spy, see config
    @SpyBean
    private UserPurchasesDAO userPurchasesDAO;
    @SpyBean
    private YaPayBillingService yaPayBillingService;
    @Autowired
    private UserSubscriptionsDAO userSubscriptionsDAO;
    @SpyBean
    private TestTrustClient trustClient;
    @SpyBean
    private SkillsService skillsService;
    @Autowired
    private PurchaseOfferDao purchaseOfferDao;
    @Autowired
    private TestMediaBillingClient mediaBillingClient;
    @Autowired
    private TestYandexPayClient yandexPayClient;

    @LocalServerPort
    private int port;
    private RestTemplate restTemplate = new RestTemplate();
    private SkillInfo skill;

    private UriComponentsBuilder url(String... method) {
        return UriComponentsBuilder.fromUriString("http://localhost:" + port)
                .pathSegment("billing")
                .pathSegment(method);
    }

    @BeforeEach
    void setUp() throws SkillsService.SkillAccessViolationException, SkillsService.BadSkillAccessRequestTokenException {
        skill = skillsService.registerSkill(SKILL_UUID, Long.parseLong(UID), "skillslug");
        skillsService.requestMerchantAccess(SKILL_UUID, TestYandexPayClient.TEST_MERCHANT_KEY, "desc");

        Mockito.reset(contentProvider);
        contentProvider.reset();
        //trustClient.prepare();
        yandexPayClient.init(port);
    }

    @AfterEach
    void tearDown() {
        trustClient.clear();
        mediaBillingClient.clear();
        yandexPayClient.clear();
    }

    @Test
    void testSuccessfulPurchase() {
        String purchaseToken = initPurchaseProcess(
                contentProvider.getPricingOptions(PAID_FILM, TestAuthorizationService.PROVIDER_TOKEN)
                        .getPricingOptions().get(0),
                null
        );

        // check status when purchase is not set yet
        assertEquals(PurchaseInfo.Status.STARTED, getPurchaseStatus(purchaseToken));

        // successful callback
        String callbackResult = trustClient.executeCallback(purchaseToken, port);
        assertEquals("OK", callbackResult);

        // check status after cleaning
        checkPaymentCleared(trustClient.getPurchase(purchaseToken));
    }

    @Test
    void testNotEnoughFundsPurchase() {
        String purchaseToken = initPurchaseProcess(
                contentProvider.getPricingOptions(PAID_FILM, TestAuthorizationService.PROVIDER_TOKEN)
                        .getPricingOptions().get(0),
                null
        );

        // check status when purchase is not set yet
        assertEquals(PurchaseInfo.Status.STARTED, getPurchaseStatus(purchaseToken));

        // make not enough funds
        Purchase purchase = trustClient.getPurchase(purchaseToken);
        purchase.setPaymentStatus(Purchase.PaymentStatus.not_authorized);
        purchase.setPaymentRespStatus(PaymentRespStatus.not_enough_funds);

        // PAYMENT_ERROR callback
        String onBillingHoldResult = trustClient.executeCallback(purchaseToken, port);
        assertEquals("PAYMENT_ERROR", onBillingHoldResult);

        // check purchase status is error
        assertEquals(PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS, getPurchaseStatus(purchaseToken));
        assertEquals(started, trustClient.getPurchase(purchaseToken).getStatus());
    }

    @Test
    void testPurchaseTransactionNotPermitted() {
        String purchaseToken = initPurchaseProcess(
                contentProvider.getPricingOptions(PAID_FILM, TestAuthorizationService.PROVIDER_TOKEN)
                        .getPricingOptions().get(0),
                null
        );

        // check status when purchase is not set yet
        assertEquals(PurchaseInfo.Status.STARTED, getPurchaseStatus(purchaseToken));

        // make "transaction not permitted"
        Purchase purchase = trustClient.getPurchase(purchaseToken);
        purchase.setPaymentStatus(Purchase.PaymentStatus.not_authorized);
        purchase.setPaymentRespStatus(PaymentRespStatus.transaction_not_permitted);

        // PAYMENT_ERROR callback
        String onBillingHoldResult = trustClient.executeCallback(purchaseToken, port);
        assertEquals("PAYMENT_ERROR", onBillingHoldResult);

        // check purchase status is error
        assertEquals(PurchaseInfo.Status.ERROR_DO_NOT_TRY_LATER, getPurchaseStatus(purchaseToken));
        assertEquals(started, trustClient.getPurchase(purchaseToken).getStatus());

    }

    // successful hold but provider fails on purchase processing
    @Test
    void testPurchaseProviderPurchaseFailure() throws ProviderPurchaseException {
        String purchaseToken = initPurchaseProcess(
                contentProvider.getPricingOptions(PAID_FILM, TestAuthorizationService.PROVIDER_TOKEN)
                        .getPricingOptions().get(0),
                null
        );

        // check status when purchase is not set yet
        assertEquals(PurchaseInfo.Status.STARTED, getPurchaseStatus(purchaseToken));

        // make content provider fail on purchase processing
        doThrow(new ProviderPurchaseException(PurchaseInfo.Status.ERROR_UNKNOWN))
                .when(contentProvider).processPurchase(eq(PAID_FILM), any(), eq(purchaseToken),
                        eq(TestAuthorizationService.PROVIDER_TOKEN));

        // successful callback but PAYMENT_ERROR response due to provider's exception
        String onBillingHoldResult = trustClient.executeCallback(purchaseToken, port);
        assertEquals("CANCELLED", onBillingHoldResult);

        // check purchase status is error
        assertEquals(PurchaseInfo.Status.ERROR_UNKNOWN, getPurchaseStatus(purchaseToken));
        // check payment unhold
        assertEquals(canceled, trustClient.getPurchase(purchaseToken).getPaymentStatus());

        // refund callback
        String refundCallbackResult = trustClient.executeRefundCallback(purchaseToken, port);
        assertEquals("PURCHASE_REFUNDED", refundCallbackResult);

        assertEquals(PurchaseInfo.Status.ERROR_UNKNOWN, getPurchaseStatus(purchaseToken));
    }

    @Test
    void testSubscriptionPurchaseWhenItsAlreadyActiveOnProviderSide() {
        String purchaseToken = initPurchaseProcess(
                contentProvider.getPricingOptions(AVAILABLE_SUBSCRIPTION, TestAuthorizationService.PROVIDER_TOKEN)
                        .getPricingOptions().get(0),
                null
        );

        // check status when purchase is not set yet
        assertEquals(PurchaseInfo.Status.ALREADY_AVAILABLE, getPurchaseStatus(purchaseToken));
        assertTrue(trustClient.getUserPurchases(UID).isEmpty(), "Purchase created on already available subscription");
        assertTrue(trustClient.getUserSubscriptions(UID).isEmpty(), "Subscription is created on already available " +
                "subscription onf provider's side");
    }

    @Test
    void testSinglePurchaseAndRefund() throws InterruptedException {
        PricingOption pricingOption =
                contentProvider.getPricingOptions(PAID_FILM, PROVIDER_TOKEN).getPricingOptions().get(0);
        String purchaseToken = initPurchaseProcess(
                pricingOption,
                null
        );

        // check status when purchase is not set yet
        assertEquals(PurchaseInfo.Status.STARTED, getPurchaseStatus(purchaseToken));

        // successful callback
        String callbackResult = trustClient.executeCallback(purchaseToken, port);
        assertEquals("OK", callbackResult);

        // check status after cleaning
        Purchase purchase = trustClient.getPurchase(purchaseToken);
        checkPaymentCleared(purchase);


        //refund payment
        CompletableFuture<RefundInfo> refundInfo = CompletableFuture.supplyAsync(() -> refundPayment(purchaseToken))
                .orTimeout(500, TimeUnit.MILLISECONDS);
        while (purchase.getRefund() == null) {
            Thread.sleep(5);
        }
        trustClient.executeRefundCallback(purchaseToken, port);
        assertNotNull(purchase.getRefund());

        RefundInfo expected = new RefundInfo(purchaseToken,
                purchase.getRefund().getRefundId(),
                null,
                trustClient.fiscalUrlForRefundId(purchase.getRefund().getRefundId()),
                pricingOption.getUserPrice(),
                UID);
        assertEquals(expected, refundInfo.join());
        assertEquals(canceled, purchase.getPaymentStatus());

        assertEquals(PurchaseInfo.Status.REFUNDED,
                userPurchasesDAO.getPurchaseInfo(purchaseToken).map(PurchaseInfo::getStatus).orElse(null));

    }

    @Test
    void testSuccessfulPurchaseThroughASkill() throws IOException {

        String purchaseRequestId = "cb3e805d-5a4d-4fe3-9d4f-fd3ddb42e656";
        String productId = UUID.randomUUID().toString();
        var request = new PurchaseRequest(purchaseRequestId,
                "http://img",
                "Заказ 111",
                "Описание",
                TrustCurrency.RUB, PricingOptionType.BUY, PAYLOAD, TEST_MERCHANT_KEY,
                List.of(getPurchaseRequest(productId, "ДляЧека", 1, 1)),
                false,
                null
        );
        var initSkillPurchaseResponse = offerSkillPurchase(request, false);
        var purchaseOfferUuid = initSkillPurchaseResponse.getOrderId();

        assertNotNull(purchaseOfferUuid);

        var uri = UriComponentsBuilder.fromUriString(initSkillPurchaseResponse.getUrl()).build();
        assertEquals(uri.getQueryParams().get("skillId").get(0), SKILL_UUID);

        PurchaseOfferData offer = purchaseOfferData(purchaseOfferUuid);

        validatePurchaseOffer(offer, purchaseRequestId, purchaseOfferUuid, 1, 10, 1);

        var product = offer.getProducts().get(0);
        assertAll(
                () -> assertEquals(BigDecimal.ONE, product.getUserPrice()),
                () -> assertEquals(BigDecimal.TEN, product.getPrice()),
                () -> assertEquals("ДляЧека", product.getTitle()),
                () -> assertEquals(BigDecimal.ONE, product.getQuantity())
        );

        initSkillPurchaseProcess(purchaseOfferUuid, offer.getSelectedOptionUuid(), null);

        // check status when purchase is not set yet
        assertEquals(PurchaseOfferStatus.STARTED, purchaseOfferData(purchaseOfferUuid).getStatus());

        PurchaseOffer purchaseOffer = purchaseOfferDao.findBySkillIdAndPurchaseRequestId(skill.getSkillInfoId(),
                purchaseRequestId).get();

        PurchaseInfo purchaseInfo = userPurchasesDAO.getAllPurchaseOfferId(purchaseOffer.getId()).get(0);
        assertAll(
                () -> assertEquals(0, BigDecimal.ONE.compareTo(purchaseInfo.getUserPrice()),
                        "Wrong userPrice"),
                () -> assertEquals(0, BigDecimal.TEN.compareTo(purchaseInfo.getOriginalPrice()),
                        "Wrong originalPrice"),
                () -> assertEquals(Long.valueOf(UID), purchaseInfo.getUid()),
                () -> assertEquals("skill", purchaseInfo.getProvider()),
                () -> assertEquals(TEST_MERCHANT_ID, purchaseInfo.getMerchantId())
        );

        assertAll(
                () -> assertEquals(purchaseOffer.getId(), purchaseInfo.getPurchaseOfferId()),
                () -> assertEquals(purchaseRequestId, purchaseOffer.getPurchaseRequestId())
        );

        OrderRecord order = yandexPayClient.getOrderRecord(purchaseInfo.getPurchaseToken());
        assertEquals(nds_20, order.getItems().get(0).getNds());
        assertEquals("ДляЧека", order.getItems().get(0).getName());
        assertEquals(BigDecimal.ONE, order.getPrice());
        assertEquals(TrustCurrency.RUB, order.getCurrency());

        // successful callback
        yandexPayClient.waitCompleted();

        // check status after cleaning
        assertEquals("paid", order.getPayStatus());
    }

    @Test
    void testSkillPurchaseCallbackFailure() throws ProviderPurchaseException, IOException {

        doThrow(new ProviderPurchaseException(PurchaseInfo.Status.ERROR_UNKNOWN))
                .when(skillsService)
                .executeSkillCallback(any(), anyString(), anyString(), anyString(), any());

        String purchaseRequestId = UUID.randomUUID().toString();
        String productId = UUID.randomUUID().toString();
        var request = new PurchaseRequest(purchaseRequestId,
                "http://img",
                "Заказ 111",
                "Описание",
                TrustCurrency.RUB, PricingOptionType.BUY, PAYLOAD, TEST_MERCHANT_KEY,
                List.of(getPurchaseRequest(productId, "ДляЧека", 1, 1)),
                false,
                null
        );
        String purchaseOfferUuid = offerSkillPurchase(request, false).getOrderId();

        assertNotNull(purchaseOfferUuid);

        PurchaseOfferData offer = purchaseOfferData(purchaseOfferUuid);

        validatePurchaseOffer(offer, purchaseRequestId, purchaseOfferUuid, 1, 10, 1);

        var product = offer.getProducts().get(0);
        assertAll(
                () -> assertEquals(BigDecimal.ONE, product.getUserPrice()),
                () -> assertEquals(BigDecimal.TEN, product.getPrice()),
                () -> assertEquals("ДляЧека", product.getTitle()),
                () -> assertEquals(BigDecimal.ONE, product.getQuantity())
        );

        initSkillPurchaseProcess(purchaseOfferUuid, offer.getSelectedOptionUuid());

        // check status when purchase is not set yet
        assertEquals(PurchaseOfferStatus.STARTED, purchaseOfferData(purchaseOfferUuid).getStatus());

        PurchaseOffer purchaseOffer = purchaseOfferDao.findBySkillIdAndPurchaseRequestId(skill.getSkillInfoId(),
                purchaseRequestId).get();

        PurchaseInfo purchaseInfo = userPurchasesDAO.getAllPurchaseOfferId(purchaseOffer.getId()).get(0);
        assertAll(
                () -> assertEquals(0, BigDecimal.ONE.compareTo(purchaseInfo.getUserPrice()),
                        "Wrong userPrice"),
                () -> assertEquals(0, BigDecimal.TEN.compareTo(purchaseInfo.getOriginalPrice()),
                        "Wrong originalPrice"),
                () -> assertEquals(Long.valueOf(UID), purchaseInfo.getUid()),
                () -> assertEquals("skill", purchaseInfo.getProvider()),
                () -> assertEquals(TEST_MERCHANT_ID, purchaseInfo.getMerchantId())
        );

        assertAll(
                () -> assertEquals(purchaseOffer.getId(), purchaseInfo.getPurchaseOfferId()),
                () -> assertEquals(purchaseRequestId, purchaseOffer.getPurchaseRequestId())
        );

        OrderRecord order = yandexPayClient.getOrderRecord(purchaseInfo.getPurchaseToken());
        assertEquals(nds_20, order.getItems().get(0).getNds());
        assertEquals("ДляЧека", order.getItems().get(0).getName());
        assertEquals(BigDecimal.ONE, order.getPrice());
        assertEquals(TrustCurrency.RUB, order.getCurrency());

        // successful callback
        yandexPayClient.waitCompleted();

        // check status after cleaning
        assertEquals("canceled", order.getPayStatus());
    }

    @Test
    void testFailedPurchaseThroughASkill() throws IOException {

        String purchaseRequestId = UUID.randomUUID().toString();
        String productId = UUID.randomUUID().toString();
        var request = new PurchaseRequest(purchaseRequestId,
                "http://img",
                "Заказ 111",
                "Описание",
                TrustCurrency.RUB, PricingOptionType.BUY, PAYLOAD, TEST_MERCHANT_KEY,
                List.of(getPurchaseRequest(productId, "ДляЧека", 1, 1)),
                false,
                null
        );
        String purchaseOfferUuid = offerSkillPurchase(request, false).getOrderId();

        assertNotNull(purchaseOfferUuid);

        PurchaseOfferData offer = purchaseOfferData(purchaseOfferUuid);

        validatePurchaseOffer(offer, purchaseRequestId, purchaseOfferUuid, 1, 10, 1);

        // emulate Yandex Payment Crash
        doThrow(new RuntimeException("Emulated exception when starting payment"))
                .when(yaPayBillingService).startPayment(anyString(), any());

        try {
            initSkillPurchaseProcess(purchaseOfferUuid, offer.getSelectedOptionUuid());
        } catch (HttpServerErrorException.InternalServerError ignored) {
            // we raise a correct 5xx exception here
        }

        // check status when purchase is not set yet
        assertEquals(PurchaseOfferStatus.FAILURE, purchaseOfferData(purchaseOfferUuid).getStatus());
    }

    @Test
    void testSuccessfulMultiItemPurchaseThroughASkill() throws IOException {

        String purchaseRequestId = UUID.randomUUID().toString();
        String productId = UUID.randomUUID().toString();
        String productId2 = UUID.randomUUID().toString();
        var request = new PurchaseRequest(
                purchaseRequestId,
                "http://img",
                "Заказ 111",
                "Описание",
                TrustCurrency.RUB, PricingOptionType.BUY, PAYLOAD, TEST_MERCHANT_KEY,
                List.of(
                        getPurchaseRequest(productId, "ДляЧека", 1, 1),
                        getPurchaseRequest(productId2, "ДляЧека2", 10, 2)
                ),
                false,
                null
        );
        String purchaseOfferUuid = offerSkillPurchase(request, false).getOrderId();

        assertNotNull(purchaseOfferUuid);

        PurchaseOfferData offer = purchaseOfferData(purchaseOfferUuid);

        validatePurchaseOffer(offer, purchaseRequestId, purchaseOfferUuid, 2, 30,
                21);

        var product = offer.getProducts().get(0);
        assertAll(
                () -> assertEquals(BigDecimal.ONE, product.getUserPrice()),
                () -> assertEquals(BigDecimal.TEN, product.getPrice()),
                () -> assertEquals("ДляЧека", product.getTitle()),
                () -> assertEquals(BigDecimal.ONE, product.getQuantity())
        );

        var product2 = offer.getProducts().get(1);
        assertAll(
                () -> assertEquals(BigDecimal.TEN, product2.getUserPrice()),
                () -> assertEquals(BigDecimal.TEN, product2.getPrice()),
                () -> assertEquals("ДляЧека2", product2.getTitle()),
                () -> assertEquals(BigDecimal.valueOf(2), product2.getQuantity())
        );

        initSkillPurchaseProcess(purchaseOfferUuid, offer.getSelectedOptionUuid());

        // check status when purchase is not set yet
        assertEquals(PurchaseOfferStatus.STARTED, purchaseOfferData(purchaseOfferUuid).getStatus());

        PurchaseOffer purchaseOffer = purchaseOfferDao.findBySkillIdAndPurchaseRequestId(
                skill.getSkillInfoId(),
                purchaseRequestId
        ).get();

        PurchaseInfo purchaseInfo = userPurchasesDAO.getAllPurchaseOfferId(purchaseOffer.getId()).get(0);
        assertAll(
                () -> assertEquals(0, BigDecimal.valueOf(21).compareTo(purchaseInfo.getUserPrice()),
                        "Wrong userPrice"),
                () -> assertEquals(0, BigDecimal.valueOf(30).compareTo(purchaseInfo.getOriginalPrice()),
                        "Wrong originalPrice"),
                () -> assertEquals(Long.valueOf(UID), purchaseInfo.getUid()),
                () -> assertEquals("skill", purchaseInfo.getProvider()),
                () -> assertEquals(TEST_MERCHANT_ID, purchaseInfo.getMerchantId())
        );

        assertAll(
                () -> assertEquals(purchaseOffer.getId(), purchaseInfo.getPurchaseOfferId()),
                () -> assertEquals(purchaseRequestId, purchaseOffer.getPurchaseRequestId())
        );

        var order = yandexPayClient.getOrderRecord(purchaseInfo.getPurchaseToken());


        assertEquals(nds_20, order.getItems().get(0).getNds());
        assertEquals("ДляЧека", order.getItems().get(0).getName());
        assertEquals(nds_20, order.getItems().get(1).getNds());
        assertEquals("ДляЧека2", order.getItems().get(1).getName());
        assertEquals(BigDecimal.valueOf(21), order.getPrice());
        assertEquals(TrustCurrency.RUB, order.getCurrency());
        assertEquals(TEST_MERCHANT_ID, order.getServiceMerchantId());

        // successful callback

        yandexPayClient.waitCompleted();

        // check status after cleaning
        assertEquals("paid", order.getPayStatus());
    }

    @Test
    void testSuccessfulMultiItemPurchaseWithDeliveryThroughASkill() throws IOException {

        String purchaseRequestId = UUID.randomUUID().toString();
        String productId = UUID.randomUUID().toString();
        String productId2 = UUID.randomUUID().toString();
        PurchaseDeliveryInfo purchaseDeliveryInfo = getDeliveryInfo(250);
        var request = new PurchaseRequest(purchaseRequestId,
                "http://img",
                "Заказ 111",
                "Описание",
                TrustCurrency.RUB, PricingOptionType.BUY, PAYLOAD, TEST_MERCHANT_KEY,
                List.of(
                        getPurchaseRequest(productId, "ДляЧека", 1, 1),
                        getPurchaseRequest(productId2, "ДляЧека2", 10, 2)
                ),
                false,
                purchaseDeliveryInfo
        );
        String purchaseOfferUuid = offerSkillPurchase(request, false).getOrderId();

        assertNotNull(purchaseOfferUuid);

        PurchaseOfferData offer = purchaseOfferData(purchaseOfferUuid);

        validatePurchaseOffer(offer, purchaseRequestId, purchaseOfferUuid, 2, 280,
                271);

        assertAll(
                () -> assertEquals(purchaseDeliveryInfo.getPrice(), offer.getDeliveryInfo().getPrice()),
                () -> assertEquals(purchaseDeliveryInfo.getCity(), offer.getDeliveryInfo().getCity()),
                () -> assertEquals(purchaseDeliveryInfo.getStreet(), offer.getDeliveryInfo().getStreet()),
                () -> assertEquals(purchaseDeliveryInfo.getHouse(), offer.getDeliveryInfo().getHouse())
        );

        var product = offer.getProducts().get(0);

        assertAll(
                () -> assertEquals(BigDecimal.ONE, product.getUserPrice()),
                () -> assertEquals(BigDecimal.TEN, product.getPrice()),
                () -> assertEquals("ДляЧека", product.getTitle()),
                () -> assertEquals(nds_20, product.getNdsType()),
                () -> assertEquals(BigDecimal.ONE, product.getQuantity())
        );

        var product2 = offer.getProducts().get(1);
        assertAll(
                () -> assertEquals(BigDecimal.TEN, product2.getUserPrice()),
                () -> assertEquals(BigDecimal.TEN, product2.getPrice()),
                () -> assertEquals("ДляЧека2", product2.getTitle()),
                () -> assertEquals(nds_20, product2.getNdsType()),
                () -> assertEquals(BigDecimal.valueOf(2), product2.getQuantity())
        );

        initSkillPurchaseProcess(purchaseOfferUuid, offer.getSelectedOptionUuid());

        // check status when purchase is not set yet
        assertEquals(PurchaseOfferStatus.STARTED, purchaseOfferData(purchaseOfferUuid).getStatus());

        PurchaseOffer purchaseOffer = purchaseOfferDao.findBySkillIdAndPurchaseRequestId(
                skill.getSkillInfoId(),
                purchaseRequestId
        ).get();

        PurchaseInfo purchaseInfo = userPurchasesDAO.getAllPurchaseOfferId(purchaseOffer.getId()).get(0);
        assertAll(
                () -> assertThat(purchaseInfo.getUserPrice(), comparesEqualTo(BigDecimal.valueOf(271))),
                () -> assertThat(purchaseInfo.getOriginalPrice(), comparesEqualTo(BigDecimal.valueOf(280))),
                () -> assertEquals(Long.valueOf(UID), purchaseInfo.getUid()),
                () -> assertEquals("skill", purchaseInfo.getProvider()),
                () -> assertEquals(TEST_MERCHANT_ID, purchaseInfo.getMerchantId())
        );

        assertAll(
                () -> assertEquals(purchaseOffer.getId(), purchaseInfo.getPurchaseOfferId()),
                () -> assertEquals(purchaseRequestId, purchaseOffer.getPurchaseRequestId())
        );

        var order = yandexPayClient.getOrderRecord(purchaseInfo.getPurchaseToken());


        assertEquals(nds_20, order.getItems().get(0).getNds());
        assertEquals("ДляЧека", order.getItems().get(0).getName());
        assertEquals(nds_20, order.getItems().get(1).getNds());
        assertEquals("ДляЧека2", order.getItems().get(1).getName());
        assertEquals(nds_20, order.getItems().get(1).getNds());
        assertEquals(BillingService.DELIVERY_PRODUCT_NAME, order.getItems().get(2).getName());
        assertEquals(BigDecimal.valueOf(271), order.getPrice());
        assertEquals(TrustCurrency.RUB, order.getCurrency());
        assertEquals(TEST_MERCHANT_ID, order.getServiceMerchantId());

        // successful callback
        yandexPayClient.waitCompleted();

        // check status after cleaning
        assertEquals("paid", order.getPayStatus());

        PurchaseOfferData offerAfterClear = purchaseOfferData(purchaseOfferUuid);
        assertEquals(PurchaseOfferStatus.SUCCESS, offerAfterClear.getStatus());
        //TODO: revert
        //assertEquals(TestTrustClient.PAYMETHOD_ID, offerAfterClear.getPaymentMethodInfo().getPaymentMethodId());
    }

    @Test
    // TODO: вернуть после решения PAYBACK-178
    @Disabled("Яндекс.Оплата пкоа не присылает ошибку о нехватке средств. Воостановить после PAYBACK-178")
    void testNotEnoughFundsPurchaseThroughASkill() throws IOException {

        String purchaseRequestId = UUID.randomUUID().toString();
        String productId = UUID.randomUUID().toString();

        var request = new PurchaseRequest(purchaseRequestId,
                "http://img",
                "Заказ 111",
                "Описание",
                TrustCurrency.RUB, PricingOptionType.BUY, PAYLOAD, TEST_MERCHANT_KEY,
                List.of(getPurchaseRequest(productId, "ДляЧека", 1, 1)),
                false,
                null
        );
        String purchaseOfferUuid = offerSkillPurchase(request, false).getOrderId();

        assertNotNull(purchaseOfferUuid);

        PurchaseOfferData offer = purchaseOfferData(purchaseOfferUuid);

        validatePurchaseOffer(offer, purchaseRequestId, purchaseOfferUuid, 1, 10, 1);

        var product = offer.getProducts().get(0);
        assertAll(
                () -> assertEquals(BigDecimal.ONE, product.getUserPrice()),
                () -> assertEquals(BigDecimal.TEN, product.getPrice()),
                () -> assertEquals("ДляЧека", product.getTitle()),
                () -> assertEquals(BigDecimal.ONE, product.getQuantity())
                //() -> assertEquals(productId, product.getProductId())
        );

        String purchaseToken = initSkillPurchaseProcess(purchaseOfferUuid, offer.getSelectedOptionUuid());

        // check status when purchase is not set yet
        assertEquals(PurchaseInfo.Status.STARTED, getPurchaseStatus(purchaseToken));

        PurchaseInfo purchaseInfo = userPurchasesDAO.getPurchaseInfo(purchaseToken).get();
        assertAll(
                () -> assertEquals(0, BigDecimal.ONE.compareTo(purchaseInfo.getUserPrice()),
                        "Wrong userPrice"),
                () -> assertEquals(0, BigDecimal.TEN.compareTo(purchaseInfo.getOriginalPrice()),
                        "Wrong originalPrice"),
                () -> assertEquals(Long.valueOf(UID), purchaseInfo.getUid()),
                () -> assertNull(purchaseInfo.getProvider()),
                () -> assertEquals(TEST_MERCHANT_KEY, purchaseInfo.getMerchantId())
        );

        PurchaseOffer purchaseOffer = purchaseOfferDao.findBySkillIdAndPurchaseRequestId(skill.getSkillInfoId(),
                purchaseRequestId).get();
        assertAll(
                () -> assertEquals(purchaseOffer.getId(), purchaseInfo.getPurchaseOfferId()),
                () -> assertEquals(purchaseRequestId, purchaseOffer.getPurchaseRequestId())
        );

        var order = yandexPayClient.getOrderRecord(purchaseToken);
        assertEquals(nds_20, order.getItems().get(0).getNds());
        assertEquals("ДляЧека", order.getItems().get(0).getName());
        assertEquals(BigDecimal.ONE, order.getPrice());
        assertEquals(TrustCurrency.RUB, order.getCurrency());
        assertEquals(TEST_MERCHANT_KEY, order.getServiceMerchantId());

        // make not enough funds
        order.setPayStatus("rejected");

        // successful callback
        String callbackResult = yandexPayClient.executeCallback(order);
        assertEquals("PAYMENT_ERROR", callbackResult);

        // check status after cleaning
        assertEquals(PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS, getPurchaseStatus(purchaseToken));
        assertEquals("rejected", yandexPayClient.getOrderRecord(purchaseToken).getPayStatus());

        PurchaseOfferData offerAfterClear = purchaseOfferData(purchaseOfferUuid);
        assertEquals(PurchaseOfferStatus.FAILURE, offerAfterClear.getStatus());
    }

    @Test
    void testSuccessfulPurchaseThroughASkillUnauthorized() throws IOException {

        String purchaseRequestId = UUID.randomUUID().toString();
        String productId = UUID.randomUUID().toString();
        var request = new PurchaseRequest(purchaseRequestId,
                "http://img",
                "Заказ 111",
                "Описание",
                TrustCurrency.RUB, PricingOptionType.BUY, PAYLOAD, TEST_MERCHANT_KEY,
                List.of(getPurchaseRequest(productId, "ДляЧека", 1, 1)),
                false,
                null
        );
        String purchaseOfferUuid = offerSkillPurchase(request, true).getOrderId();

        assertNotNull(purchaseOfferUuid);

        PurchaseOfferData offer = bindPurchaseOfferToUser(purchaseOfferUuid);

        validatePurchaseOffer(offer, purchaseRequestId, purchaseOfferUuid, 1, 10, 1);

        var product = offer.getProducts().get(0);
        assertAll(
                () -> assertEquals(BigDecimal.ONE, product.getUserPrice()),
                () -> assertEquals(BigDecimal.TEN, product.getPrice()),
                () -> assertEquals("ДляЧека", product.getTitle()),
                () -> assertEquals(BigDecimal.ONE, product.getQuantity())
        );

        initSkillPurchaseProcess(purchaseOfferUuid, offer.getSelectedOptionUuid());

        // check status when purchase is not set yet
        assertEquals(PurchaseOfferStatus.STARTED, purchaseOfferData(purchaseOfferUuid).getStatus());

        PurchaseOffer purchaseOffer = purchaseOfferDao.findBySkillIdAndPurchaseRequestId(skill.getSkillInfoId(),
                purchaseRequestId).get();

        PurchaseInfo purchaseInfo = userPurchasesDAO.getAllPurchaseOfferId(purchaseOffer.getId()).get(0);
        assertAll(
                () -> assertEquals(0, BigDecimal.ONE.compareTo(purchaseInfo.getUserPrice()),
                        "Wrong userPrice"),
                () -> assertEquals(0, BigDecimal.TEN.compareTo(purchaseInfo.getOriginalPrice()),
                        "Wrong originalPrice"),
                () -> assertEquals(Long.valueOf(UID), purchaseInfo.getUid()),
                () -> assertEquals("skill", purchaseInfo.getProvider()),
                () -> assertEquals(Long.valueOf(TEST_MERCHANT_ID), purchaseInfo.getMerchantId())
        );

        assertAll(
                () -> assertEquals(purchaseOffer.getId(), purchaseInfo.getPurchaseOfferId()),
                () -> assertEquals(purchaseRequestId, purchaseOffer.getPurchaseRequestId())
        );

        var order = yandexPayClient.getOrderRecord(purchaseInfo.getPurchaseToken());
        assertEquals(nds_20, order.getItems().get(0).getNds());
        assertEquals("ДляЧека", order.getItems().get(0).getName());
        assertEquals(BigDecimal.ONE, order.getPrice());
        assertEquals(TrustCurrency.RUB, order.getCurrency());
        assertEquals(TEST_MERCHANT_ID, order.getServiceMerchantId());

        // successful callback
        yandexPayClient.waitCompleted();

        // check status after cleaning
        assertEquals("paid", order.getPayStatus());
    }

    private void checkPaymentCleared(Purchase purchase) {
        assertEquals(cleared, purchase.getPaymentStatus());
        assertEquals(PurchaseInfo.Status.WAITING_FOR_CLEARING, getPurchaseStatus(purchase.getPurchaseToken()));
        trustClient.runClearingJob();
        assertEquals(PurchaseInfo.Status.CLEARED, getPurchaseStatus(purchase.getPurchaseToken()));
    }

    @Test
    void testFailurePurchaseThroughASkillUnauthorized() throws IOException {

        String purchaseRequestId = UUID.randomUUID().toString();
        String productId = UUID.randomUUID().toString();
        var request = new PurchaseRequest(purchaseRequestId,
                "http://img",
                "Заказ 111",
                "Описание",
                TrustCurrency.RUB, PricingOptionType.BUY, PAYLOAD, TEST_MERCHANT_KEY,
                List.of(getPurchaseRequest(productId, "ДляЧека", 1, 1)),
                false,
                null
        );
        // initially anonymous purchase
        String purchaseOfferUuid = offerSkillPurchase(request, true).getOrderId();

        assertNotNull(purchaseOfferUuid);

        // offer is not bind to the user
        assertThrows(HttpClientErrorException.NotFound.class, () -> purchaseOfferData(purchaseOfferUuid));
    }

    private void validatePurchaseOffer(PurchaseOfferData offer, String purchaseRequestId, String purchaseOfferUuid,
                                       int productsCount, int totalPrice,
                                       int totalUserPrice
    ) {
        assertAll(
                () -> assertEquals(purchaseRequestId, offer.getPurchaseRequestId()),
                () -> assertEquals(purchaseOfferUuid, offer.getPurchaseOfferUuid()),
                () -> assertEquals("skill name", offer.getName()),
                () -> assertEquals(productsCount, offer.getProducts().size()),
                () -> assertEquals("RUB", offer.getCurrency()),
                () -> assertEquals(PurchaseOfferStatus.NEW, offer.getStatus()),
                () -> assertEquals(BigDecimal.valueOf(totalPrice), offer.getTotalPrice()),
                () -> assertEquals(BigDecimal.valueOf(totalUserPrice), offer.getTotalUserPrice()),
                () -> assertNotNull(offer.getMerchantInfo()),
                () -> assertNotNull(offer.getMerchantInfo().getName()),
                () -> assertNotNull(offer.getMerchantInfo().getInn()),
                () -> assertNotNull(offer.getMerchantInfo().getWorkingHours()),
                () -> assertNull(offer.getPaymentMethodInfo())
        );
    }

    private PurchaseDeliveryInfo getDeliveryInfo(int price) {
        return PurchaseDeliveryInfo.builder()
                .city("Москва")
                .street("Ул. Льва Толстого")
                .house("16")
                .price(new BigDecimal(price))
                .ndsType(nds_20)
                .build();
    }


    @Test
    void testMediabillingSubscriptionPurchase() {
        String purchaseToken = initPurchaseProcess(
                contentProvider.getPricingOptions(MEDIABILLING_SUB_FILM, TestAuthorizationService.PROVIDER_TOKEN)
                        .getPricingOptions().get(0),
                new ContentItem(contentProvider.getProviderName(), MEDIABILLING_SUB_FILM)
        );

        // check subscription is added to the list of available on provider's side
        assertEquals(Set.of(AVAILABLE_SUBSCRIPTION, YA_SUBSCRIPTION_PREMIUM),
                contentProvider.getActiveSubscriptions(TestAuthorizationService.PROVIDER_TOKEN).keySet());

        List<SubscriptionInfo> activeSubscriptions = userSubscriptionsDAO.getActiveSubscriptions(Long.valueOf(UID));
        // empty as Mediabilling processes the subscriptions
        assertThat(activeSubscriptions, Matchers.emptyIterable());

        GetContentPurchasesResponse purchasedContent = getPurchasedContent();
        assertTrue(purchasedContent.getItems().stream()
                        .anyMatch(item -> item.getContentItem().getProviderInfo(contentProvider.getProviderName())
                                .equals(MEDIABILLING_SUB_FILM)),
                "Фильм купленный по подписке не отобразился в списке купленного контента");

        assertThat(mediaBillingClient.getOrders().values(), Matchers.not(Matchers.emptyIterable()));
        assertEquals(OrderInfo.OrderStatus.OK, mediaBillingClient.getOrderInfo(UID, purchaseToken).getStatus());

        // make subscription expired
        contentProvider.expireSubscription(YA_SUBSCRIPTION_PREMIUM);

        // check subscription is active
        assertFalse(contentProvider.getActiveSubscriptions(PROVIDER_TOKEN).containsKey(YA_SUBSCRIPTION_PREMIUM),
                "Subscriptions is not active on provider's side");
    }

    @Test
    void testOttFilmPurchase() {
        PricingOption pricingOption =
                contentProvider.getPricingOptions(OTT_TVOD_FILM, PROVIDER_TOKEN).getPricingOptions().get(0);
        String purchaseToken = initPurchaseProcess(
                contentProvider.getPricingOptions(OTT_TVOD_FILM, TestAuthorizationService.PROVIDER_TOKEN)
                        .getPricingOptions().get(0),
                new ContentItem(contentProvider.getProviderName(), OTT_TVOD_FILM)
        );

        // check status when purchase is not set yet
        assertEquals(PurchaseInfo.Status.STARTED, getPurchaseStatus(purchaseToken));

        // successful callback
        String callbackResult = trustClient.executeCallback(purchaseToken, port);
        assertEquals("OK", callbackResult);

        // check status after cleaning
        checkPaymentCleared(trustClient.getPurchase(purchaseToken));

        var purchase = trustClient.getPurchase(purchaseToken);

        //refund payment
        PaymentInfo ottPaymentInfo = getOttPaymentInfo(purchaseToken);
        assertEquals(pricingOption.getUserPrice(), ottPaymentInfo.getAmount());
        assertEquals(PaymentInfo.Status.CLEARED, ottPaymentInfo.getStatus());

        StartRefundResponse startRefundResponse = ottStartRefund(purchaseToken);
        assertNotNull(startRefundResponse.getRefundId());

        // complete refund
        trustClient.executeRefundCallback(purchaseToken, port);


        RefundInfoResponse ottRefundInfo = getOttRefundInfo(startRefundResponse.getRefundId());
        assertEquals(RefundStatus.success, ottRefundInfo.getStatus());
        assertEquals(trustClient.fiscalUrlForRefundId(purchase.getRefund().getRefundId()),
                ottRefundInfo.getFiscalReceiptUrl());

        assertEquals(canceled, purchase.getPaymentStatus());
        assertEquals(PurchaseInfo.Status.REFUNDED,
                userPurchasesDAO.getPurchaseInfo(purchaseToken).map(PurchaseInfo::getStatus).orElse(null));
    }

    private InitSkillPurchaseResponse offerSkillPurchase(PurchaseRequest purchaseRequest, boolean anonymous)
            throws IOException {

        MultiValueMap<String, String> headers = anonymous ? new HttpHeaders() : getOAuthHeader();
        headers.add("Content-Type", "application/json");
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER, getTestTicket(TvmClientName.bass));

        OfferSkillPurchaseRequest.SkillInfo skillInfo = new OfferSkillPurchaseRequest.SkillInfo(
                skill.getSkillUuid(),
                "skill name",
                "img url",
                "http://localhost:" + port + "/skill"
        );

        var body = new OfferSkillPurchaseRequest(
                skillInfo,
                "2eac4854-fce721f3-b845abba-20d60",
                "AC9WC3DF6FCE052E45A4566A48E6B7193774B84814CE49A922E163B8B29881DC",
                "testdeviceid",
                purchaseRequest,
                objectMapper.readValue("{ }", ObjectNode.class)
        );

        return restTemplate.postForObject(
                url("createSkillPurchaseOffer").build().encode().toUri(),
                new HttpEntity<>(body, headers),
                InitSkillPurchaseResponse.class
        );
    }

    private PurchaseOfferData bindPurchaseOfferToUser(String orderId) {
        MultiValueMap<String, String> authHeader = getOAuthHeader();

        return restTemplate.exchange(url("purchase_offer", orderId, "bind")
                        .build().encode().toUri(),
                HttpMethod.POST,
                new HttpEntity<>(authHeader),
                PurchaseOfferData.class
        ).getBody();
    }

    private PurchaseOfferData purchaseOfferData(String orderId) {
        MultiValueMap<String, String> authHeader = getOAuthHeader();

        return restTemplate.exchange(
                url("purchase_offer", orderId).build().encode().toUri(),
                HttpMethod.GET,
                new HttpEntity<>(authHeader),
                PurchaseOfferData.class
        ).getBody();
    }

    private String initSkillPurchaseProcess(String purchaseOfferUuid, String selectedProductUuid) {
        return initSkillPurchaseProcess(purchaseOfferUuid, selectedProductUuid, TestTrustClient.PAYMETHOD_ID);
    }

    private String initSkillPurchaseProcess(
            String purchaseOfferUuid,
            String selectedProductUuid,
            String paymentCardId
    ) {
        MultiValueMap<String, String> authHeader = getOAuthHeader();
        authHeader.add("Content-Type", "application/x-www-form-urlencoded");

        MultiValueMap<String, String> params = new LinkedMultiValueMap<>();

        //params.add("purchaseOfferUuid", purchaseOfferUuid);
        params.add("selectedOptionUuid", selectedProductUuid);
        params.add("paymentCardId", paymentCardId);
        params.add("userEmail", "user@yandex.ru");
        return restTemplate.postForObject(
                url("purchase_offer", purchaseOfferUuid, "start").build().encode().toUri(),
                new HttpEntity<>(params, authHeader),
                InitPurchaseProcessResponse.class
        ).getPurchaseToken();
    }

    private String initPurchaseProcess(PricingOption pricingOption, @Nullable ContentItem contentItem) {
        MultiValueMap<String, String> authHeader = getOAuthHeader();
        authHeader.add("Content-Type", "application/x-www-form-urlencoded");

        MultiValueMap<String, String> params = new LinkedMultiValueMap<>();
        try {
            if (contentItem != null) {
                params.add("contentItem", objectMapper.writeValueAsString(contentItem));
            }
            params.add("selectedOption", objectMapper.writeValueAsString(pricingOption));
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
        params.add("paymentCardId", TestTrustClient.PAYMETHOD_ID);
        params.add("userEmail", "user@yandex.ru");
        return restTemplate.postForObject(url("initPurchaseProcess")
                        .build().encode().toUri(),
                new HttpEntity<>(params, authHeader),
                InitPurchaseProcessResponse.class
        ).getPurchaseToken();
    }

    private PurchaseInfo.Status getPurchaseStatus(String purchaseToken) {
        ResponseEntity<GetPurchaseStatusResponse> responseEntity = restTemplate.exchange(url("getPurchaseStatus")
                        .queryParam("purchaseToken", purchaseToken)
                        .build().toUri(),
                HttpMethod.GET,
                new HttpEntity<>(getOAuthHeader()),
                GetPurchaseStatusResponse.class);
        if (responseEntity.getStatusCode() != HttpStatus.OK) {
            throw new RuntimeException("RequestException: " + responseEntity.getStatusCodeValue());
        }

        return responseEntity.getBody().getStatus();
    }

    private RefundInfo refundPayment(String purchaseToken) {

        MultiValueMap<String, String> header = new LinkedMultiValueMap<>();
        header.add(TvmHeaders.SERVICE_TICKET_HEADER, getTestTicket(TvmClientName.self));

        return restTemplate.postForObject(url("internal/refundPayment")
                        .queryParam("purchaseToken", purchaseToken)
                        .queryParam("description", "some reason")
                        .build()
                        .toUri(),
                new HttpEntity<>(header),
                RefundInfo.class);
    }

    private PaymentInfo getOttPaymentInfo(String purchaseToken) {

        MultiValueMap<String, String> header = new LinkedMultiValueMap<>();
        header.add(TvmHeaders.SERVICE_TICKET_HEADER, getTestTicket(TvmClientName.self));

        return restTemplate.exchange(url("internal/payment/" + purchaseToken)
                                .build()
                                .toUri(),
                        HttpMethod.GET,
                        new HttpEntity<>(header),
                        PaymentInfo.class)
                .getBody();
    }

    private RefundInfoResponse getOttRefundInfo(String refundId) {

        MultiValueMap<String, String> header = new LinkedMultiValueMap<>();
        header.add(TvmHeaders.SERVICE_TICKET_HEADER, getTestTicket(TvmClientName.self));

        return restTemplate.exchange(url("internal/refund/" + refundId)
                                .build()
                                .toUri(),
                        HttpMethod.GET,
                        new HttpEntity<>(header),
                        RefundInfoResponse.class)
                .getBody();
    }


    private StartRefundResponse ottStartRefund(String purchaseToken) {

        MultiValueMap<String, String> header = new LinkedMultiValueMap<>();
        header.add(TvmHeaders.SERVICE_TICKET_HEADER, getTestTicket(TvmClientName.self));

        return restTemplate.postForObject(url("internal/payment/" + purchaseToken + "/refund")
                        .build()
                        .toUri(),
                new HttpEntity<>(header),
                StartRefundResponse.class);
    }


    private GetContentPurchasesResponse getPurchasedContent() {
        MultiValueMap<String, String> header = getOAuthHeader();

        return restTemplate.exchange(url("getPurchasedContent")
                                .build()
                                .toUri(),
                        HttpMethod.GET,
                        new HttpEntity<>(header),
                        GetContentPurchasesResponse.class)
                .getBody();
    }

    private HttpHeaders getOAuthHeader() {
        HttpHeaders headers = new HttpHeaders();
        headers.add("Authorization", "OAuth " + TestAuthorizationService.OAUTH_TOKEN);
        return headers;
    }

    private PurchaseRequestProduct getPurchaseRequest(String productId, String title, int userPrice, int quantity) {
        return new PurchaseRequestProduct(
                productId,
                title,
                BigDecimal.valueOf(userPrice),
                BigDecimal.TEN,
                BigDecimal.valueOf(quantity),
                nds_20
        );
    }

    @TestConfiguration
    static class IntegrationTestConfig {

        @Autowired
        private BillingConfig billingConfig;

        @Bean
        DummySkillController dummySkillController() {
            return new DummySkillController();
        }

    }

    @RestController()
    private static class DummySkillController {
        @PostMapping("/skill")
        public PurchaseEventResponse purchaseCallback(@RequestBody PurchaseEventRequest body) {
            return new PurchaseEventResponse(PurchaseEventResponse.ResultStatus.OK, "done");
        }
    }
}
