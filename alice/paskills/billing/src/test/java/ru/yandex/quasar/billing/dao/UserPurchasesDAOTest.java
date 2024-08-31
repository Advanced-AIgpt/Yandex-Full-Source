package ru.yandex.quasar.billing.dao;

import java.math.BigDecimal;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CountDownLatch;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.transaction.PlatformTransactionManager;
import org.springframework.transaction.support.TransactionTemplate;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.ContentQuality;
import ru.yandex.quasar.billing.beans.LogicalPeriod;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.UnistatService;
import ru.yandex.quasar.billing.services.processing.yapay.YandexPayMerchantTestUtil;

import static java.util.stream.Collectors.toSet;
import static org.assertj.core.api.Assertions.assertThat;
import static org.junit.jupiter.api.Assertions.assertAll;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static ru.yandex.quasar.billing.beans.ContentType.MOVIE;
import static ru.yandex.quasar.billing.beans.ContentType.SEASON;
import static ru.yandex.quasar.billing.beans.ContentType.SUBSCRIPTION;

@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = {TestConfigProvider.class})
class UserPurchasesDAOTest implements YandexPayMerchantTestUtil {

    private static final String PROVIDER = "test provider";
    private static final PricingOption PRICING_OPTION = PricingOption.builder(
            "Тестовая опция оплаты",
            PricingOptionType.SUBSCRIPTION,
            new BigDecimal(399),
            new BigDecimal(500),
            "RUB")
            .providerPayload("{\"test\":\"payload\"}")
            .quality(ContentQuality.HD)
            .provider(PROVIDER)
            .subscriptionPeriod(LogicalPeriod.ofDays(30))
            .specialCommission(false)
            .purchasingItem(ProviderContentItem.create(MOVIE, "test_id"))
            .build();
    private static final PricingOption SKILL_PRICING_OPTION = PricingOption.builder("Тестовая опция оплаты",
            PricingOptionType.BUY, new BigDecimal(399), new BigDecimal(500), "RUB")
            .providerPayload("{\"test\":\"payload\"}")
            .optionId("OPTION_UUID")
            .build();
    @MockBean
    private UnistatService unistatService;
    @Autowired
    private PurchaseOfferDao purchaseOfferDao;
    @Autowired
    private SkillInfoDAO skillInfoDAO;
    @Autowired
    private UserPurchasesDAO userPurchasesDAO;
    @Autowired
    private PlatformTransactionManager transactionManager;


    @Test
    void testSaveReadPurchase() {
        ProviderContentItem contentItem = ProviderContentItem.create(MOVIE, "12345");

        Long purchaseId = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId,
                1L,
                "TOKEN",
                contentItem,
                PRICING_OPTION,
                PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS,
                "sec",
                1L,
                null,
                PRICING_OPTION.getProvider(),
                null,
                PaymentProcessor.TRUST,
                null,
                null)
        );

        PurchaseInfo purchaseInfo = userPurchasesDAO.getPurchaseInfo(purchaseId).get();

        assertAll(
                () -> assertEquals(Long.valueOf(1L), purchaseInfo.getUid()),
                () -> assertEquals("TOKEN", purchaseInfo.getPurchaseToken()),
                () -> assertEquals(purchaseId, purchaseInfo.getPurchaseId()),
                () -> assertEquals(PRICING_OPTION, purchaseInfo.getSelectedOption()),
                () -> assertEquals(PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS, purchaseInfo.getStatus()),
                () -> assertEquals(PROVIDER, purchaseInfo.getProvider()),
                () -> assertEquals(MOVIE, purchaseInfo.getContentItem().getContentType()),
                () -> assertEquals("12345", purchaseInfo.getContentItem().getId()),
                () -> assertEquals("sec", purchaseInfo.getSecurityToken()),
                () -> assertEquals(PRICING_OPTION.getUserPrice(), purchaseInfo.getUserPrice()),
                () -> assertEquals(PRICING_OPTION.getPrice(), purchaseInfo.getOriginalPrice()),
                () -> assertEquals("RUB", purchaseInfo.getCurrencyCode()),
                () -> assertEquals(Long.valueOf(1L), purchaseInfo.getPartnerId()),
                () -> assertNull(purchaseInfo.getPurchaseOfferId()),
                () -> assertNull(purchaseInfo.getMerchantName())
        );
    }

    @Test
    void testSaveReadPurchaseByPurchaseIdAndUid() {
        ProviderContentItem contentItem = ProviderContentItem.create(MOVIE, "12345");

        Long purchaseId = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId,
                1L,
                "TOKEN",
                contentItem,
                PRICING_OPTION,
                PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS,
                "sec",
                1L,
                null,
                PRICING_OPTION.getProvider(),
                null,
                PaymentProcessor.TRUST,
                null,
                null)
        );

        PurchaseInfo purchaseInfo = userPurchasesDAO.getPurchaseInfo(1L, purchaseId).get();

        assertAll(
                () -> assertEquals(Long.valueOf(1L), purchaseInfo.getUid()),
                () -> assertEquals("TOKEN", purchaseInfo.getPurchaseToken()),
                () -> assertEquals(purchaseId, purchaseInfo.getPurchaseId()),
                () -> assertEquals(PRICING_OPTION, purchaseInfo.getSelectedOption()),
                () -> assertEquals(PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS, purchaseInfo.getStatus()),
                () -> assertEquals(PROVIDER, purchaseInfo.getProvider()),
                () -> assertEquals(MOVIE, purchaseInfo.getContentItem().getContentType()),
                () -> assertEquals("12345", purchaseInfo.getContentItem().getId()),
                () -> assertEquals("sec", purchaseInfo.getSecurityToken()),
                () -> assertEquals(PRICING_OPTION.getUserPrice(), purchaseInfo.getUserPrice()),
                () -> assertEquals(PRICING_OPTION.getPrice(), purchaseInfo.getOriginalPrice()),
                () -> assertEquals("RUB", purchaseInfo.getCurrencyCode()),
                () -> assertEquals(Long.valueOf(1L), purchaseInfo.getPartnerId()),
                () -> assertNull(purchaseInfo.getPurchaseOfferId()),
                () -> assertNull(purchaseInfo.getMerchantName())
        );
    }

    @Test
    void testSaveReadPurchaseWithSkillPurchaseOffer() {

        SkillInfo skillInfo = skillInfoDAO.save(testSkillInfo());
        PurchaseOffer purchaseOffer = purchaseOfferDao.save(
                PurchaseOffer.builder("1", skillInfo.getSkillInfoId(), "external_id", List.of(SKILL_PRICING_OPTION),
                        "http://callback/")
                        .title("test title")
                        .uuid(UUID.randomUUID().toString())
                        .skillSessionId("session")
                        .skillUserId("userid")
                        .merchantKey(TEST_MERCHANT_KEY)
                        .build());


        Long purchaseId = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId,
                1L,
                "TOKEN",
                null,
                SKILL_PRICING_OPTION,
                PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS,
                "sec",
                1L,
                purchaseOffer.getId(),
                null,
                null,
                PaymentProcessor.TRUST,
                null,
                null)
        );

        PurchaseInfo purchaseInfo = userPurchasesDAO.getPurchaseInfo(purchaseId).get();

        assertAll(
                () -> assertEquals(Long.valueOf(1L), purchaseInfo.getUid()),
                () -> assertEquals("TOKEN", purchaseInfo.getPurchaseToken()),
                () -> assertEquals(purchaseId, purchaseInfo.getPurchaseId()),
                () -> assertTrue(SKILL_PRICING_OPTION.equalsForBilling(purchaseInfo.getSelectedOption())),
                () -> assertEquals(PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS, purchaseInfo.getStatus()),
                () -> assertNull(purchaseInfo.getProvider()),
                () -> assertNull(purchaseInfo.getContentItem()),
                () -> assertEquals("sec", purchaseInfo.getSecurityToken()),
                () -> assertEquals(SKILL_PRICING_OPTION.getUserPrice(), purchaseInfo.getUserPrice()),
                () -> assertEquals(SKILL_PRICING_OPTION.getPrice(), purchaseInfo.getOriginalPrice()),
                () -> assertEquals("RUB", purchaseInfo.getCurrencyCode()),
                () -> assertEquals(Long.valueOf(1L), purchaseInfo.getPartnerId()),
                () -> assertEquals(purchaseOffer.getId(), purchaseInfo.getPurchaseOfferId())
        );
    }

    @Test
    void testGetLastPurchases() {
        Long purchaseId1 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId1,
                2L,
                "TEST_TOKEN_1",
                ProviderContentItem.create(MOVIE, "54321"),
                PRICING_OPTION,
                PurchaseInfo.Status.CLEARED,
                null, 1L, null, PRICING_OPTION.getProvider(), null, PaymentProcessor.TRUST, null, null)
        );

        Long purchaseId2 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId2,
                2L,
                "TEST_TOKEN_2",
                ProviderContentItem.create(SEASON, "654321"),
                PRICING_OPTION,
                PurchaseInfo.Status.CLEARED,
                null, 1L, null, PRICING_OPTION.getProvider(), null, PaymentProcessor.TRUST, null, null));

        Long purchaseId3 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId3,
                2L,
                "TEST_TOKEN_3",
                ProviderContentItem.create(SUBSCRIPTION, null),
                PRICING_OPTION,
                PurchaseInfo.Status.CLEARED,
                null, 1L, null, PRICING_OPTION.getProvider(), null, null, null, null));

        Long purchaseId4 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId4,
                2L,
                "TEST_TOKEN_4",
                ProviderContentItem.create(MOVIE, "1234567"),
                PRICING_OPTION,
                PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS,
                null, 1L, null, PRICING_OPTION.getProvider(), null, PaymentProcessor.TRUST, null, null));

        Long purchaseId5 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId5,
                2L,
                "TEST_TOKEN_5",
                ProviderContentItem.create(MOVIE, "1234567"),
                PRICING_OPTION,
                PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS,
                null, 1L, null, PRICING_OPTION.getProvider(), null, PaymentProcessor.YANDEX_PAY, null, null));

        List<PurchaseInfo> lastPurchases = userPurchasesDAO.getLastPurchases(2L);

        assertNotNull(lastPurchases);
        assertEquals(3, lastPurchases.size());
        assertEquals(purchaseId3, lastPurchases.get(0).getPurchaseId());
        assertEquals(purchaseId2, lastPurchases.get(1).getPurchaseId());
        assertEquals(purchaseId1, lastPurchases.get(2).getPurchaseId());
    }

    @Test
    void testGetPurchasesWithPagination() {
        SkillInfo skillInfo = skillInfoDAO.save(testSkillInfo());
        // it was created earlier, so it must be a second purchase
        PurchaseInfo secondPurchaseInfo = userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                11L,
                1L,
                "TOKEN",
                ProviderContentItem.create(MOVIE, "12345"),
                PRICING_OPTION,
                PurchaseInfo.Status.STARTED,
                "sec",
                1L,
                null,
                PRICING_OPTION.getProvider(),
                null,
                PaymentProcessor.TRUST,
                null,
                null)
        );

        // it was created later, so it must be a first purchase
        PurchaseInfo firstPurchaseInfo = userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                10L,
                1L,
                "TOKEN",
                null,
                PRICING_OPTION,
                PurchaseInfo.Status.CLEARED,
                "sec",
                1L,
                null,
                "skill",
                skillInfo.getSkillInfoId(),
                PaymentProcessor.YANDEX_PAY,
                34L,
                "OOO КОМ")
        );

        var queriedPurchases = userPurchasesDAO.getPurchases(1L, 1, 0);
        assertEquals(1, queriedPurchases.size());
        assertThat(firstPurchaseInfo).isEqualToComparingFieldByField(queriedPurchases.get(0));


        queriedPurchases = userPurchasesDAO.getPurchases(1L, 2, 1);
        assertEquals(1, queriedPurchases.size());
        assertThat(secondPurchaseInfo).isEqualToComparingFieldByField(queriedPurchases.get(0));


        queriedPurchases = userPurchasesDAO.getPurchases(1L, 3, 0);
        assertEquals(2, queriedPurchases.size());
    }

    @Test
    void testGetLastContentPurchases() {
        Long purchaseId1 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId1,
                3L,
                "TEST_TOKEN_1",
                ProviderContentItem.create(MOVIE, "54321"),
                PRICING_OPTION,
                PurchaseInfo.Status.CLEARED,
                null, 1L, null, PRICING_OPTION.getProvider(), null, PaymentProcessor.TRUST, null, null));

        Long purchaseId2 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId2,
                3L,
                "TEST_TOKEN_2",
                ProviderContentItem.create(SEASON, "654321"),
                PRICING_OPTION,
                PurchaseInfo.Status.CLEARED,
                null, 1L, null, PRICING_OPTION.getProvider(), null,
                null, null, null));

        Long purchaseId3 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId3,
                3L,
                "TEST_TOKEN_3",
                ProviderContentItem.create(SUBSCRIPTION, null),
                PRICING_OPTION,
                PurchaseInfo.Status.CLEARED,
                null, 1L, null, PRICING_OPTION.getProvider(), null, PaymentProcessor.TRUST, null, null));

        Long purchaseId4 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId4,
                3L,
                "TEST_TOKEN_4",
                ProviderContentItem.create(MOVIE, "1234567"),
                PRICING_OPTION,
                PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS,
                null, 1L, null, PRICING_OPTION.getProvider(), null, PaymentProcessor.TRUST, null, null));

        Long purchaseId5 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId5,
                3L,
                "TEST_TOKEN_5",
                ProviderContentItem.create(MOVIE, "1234567"),
                PRICING_OPTION,
                PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS,
                null, 1L, null, PRICING_OPTION.getProvider(), null, PaymentProcessor.YANDEX_PAY, null, null));


        List<PurchaseInfo> lastContentPurchases = userPurchasesDAO.getLastContentPurchases(3L);

        assertNotNull(lastContentPurchases);
        assertEquals(2, lastContentPurchases.size());
        assertEquals(purchaseId2, lastContentPurchases.get(0).getPurchaseId());
        assertEquals(purchaseId1, lastContentPurchases.get(1).getPurchaseId());
    }

    @Test
    void testSaveWithSubscriptionId() {
        ProviderContentItem contentItem = ProviderContentItem.create(MOVIE, "12345");

        Long purchaseId = userPurchasesDAO.getNextPurchaseId();

        PurchaseInfo purchase = PurchaseInfo.createSubscriptionPayment(purchaseId,
                1L,
                "TOKEN",
                contentItem,
                PRICING_OPTION,
                PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS,
                10L,
                false,
                15L,
                PRICING_OPTION.getProvider(), PaymentProcessor.TRUST);

        userPurchasesDAO.savePurchaseInfo(purchase);

        PurchaseInfo purchaseInfo = userPurchasesDAO.getPurchaseInfo(purchaseId).get();

        assertAll(
                () -> assertEquals(Long.valueOf(1L), purchaseInfo.getUid()),
                () -> assertEquals("TOKEN", purchaseInfo.getPurchaseToken()),
                () -> assertEquals(purchaseId, purchaseInfo.getPurchaseId()),
                () -> assertEquals(PRICING_OPTION, purchaseInfo.getSelectedOption()),
                () -> assertEquals(PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS, purchaseInfo.getStatus()),
                () -> assertEquals(PROVIDER, purchaseInfo.getProvider()),
                () -> assertEquals(MOVIE, purchaseInfo.getContentItem().getContentType()),
                () -> assertEquals("12345", purchaseInfo.getContentItem().getId()),
                () -> assertNull(purchaseInfo.getSecurityToken()),
                () -> assertEquals(Long.valueOf(10L), purchaseInfo.getSubscriptionId()),
                () -> assertEquals(BigDecimal.valueOf(399), purchaseInfo.getUserPrice()),
                () -> assertEquals(BigDecimal.valueOf(500), purchaseInfo.getOriginalPrice()),
                () -> assertEquals("RUB", purchaseInfo.getCurrencyCode()),
                () -> assertEquals(Long.valueOf(15L), purchaseInfo.getPartnerId())

        );
    }

    @Test
    void testSaveWithSubscriptionTrial() {
        ProviderContentItem contentItem = ProviderContentItem.create(MOVIE, "12345");

        Long purchaseId = userPurchasesDAO.getNextPurchaseId();

        PurchaseInfo purchase = PurchaseInfo.createSubscriptionPayment(purchaseId,
                1L,
                "TOKEN",
                contentItem,
                PRICING_OPTION,
                PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS,
                10L,
                true,
                15L,
                PRICING_OPTION.getProvider(), PaymentProcessor.TRUST);

        userPurchasesDAO.savePurchaseInfo(purchase);

        PurchaseInfo purchaseInfo = userPurchasesDAO.getPurchaseInfo(purchaseId).get();

        assertAll(
                () -> assertEquals(Long.valueOf(1L), purchaseInfo.getUid()),
                () -> assertEquals("TOKEN", purchaseInfo.getPurchaseToken()),
                () -> assertEquals(purchaseId, purchaseInfo.getPurchaseId()),
                () -> assertEquals(PRICING_OPTION, purchaseInfo.getSelectedOption()),
                () -> assertEquals(PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS, purchaseInfo.getStatus()),
                () -> assertEquals(PROVIDER, purchaseInfo.getProvider()),
                () -> assertEquals(MOVIE, purchaseInfo.getContentItem().getContentType()),
                () -> assertEquals("12345", purchaseInfo.getContentItem().getId()),
                () -> assertNull(purchaseInfo.getSecurityToken()),
                () -> assertEquals(Long.valueOf(10L), purchaseInfo.getSubscriptionId()),
                // as we set null for trial payment
                () -> assertEquals(BigDecimal.ZERO, purchaseInfo.getUserPrice()),
                () -> assertEquals(BigDecimal.ZERO, purchaseInfo.getOriginalPrice()),
                () -> assertEquals("RUB", purchaseInfo.getCurrencyCode()),
                () -> assertEquals(Long.valueOf(15L), purchaseInfo.getPartnerId())
        );
    }

    @Test
    void testSaveWithNullContentItem() {
        Long purchaseId = userPurchasesDAO.getNextPurchaseId();

        PurchaseInfo purchase = PurchaseInfo.createSinglePayment(purchaseId,
                1L,
                "TOKEN",
                null,
                PRICING_OPTION,
                PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS,
                null,
                1L,
                null,
                PRICING_OPTION.getProvider(),
                null,
                PaymentProcessor.TRUST,
                null,
                null);

        userPurchasesDAO.savePurchaseInfo(purchase);

        PurchaseInfo purchaseInfo = userPurchasesDAO.getPurchaseInfo(purchaseId).get();

        assertAll(
                () -> assertEquals(Long.valueOf(1L), purchaseInfo.getUid()),
                () -> assertEquals("TOKEN", purchaseInfo.getPurchaseToken()),
                () -> assertEquals(purchaseId, purchaseInfo.getPurchaseId()),
                () -> assertEquals(PRICING_OPTION, purchaseInfo.getSelectedOption()),
                () -> assertEquals(PurchaseInfo.Status.ERROR_NOT_ENOUGH_FUNDS, purchaseInfo.getStatus()),
                () -> assertEquals(PROVIDER, purchaseInfo.getProvider()),
                () -> assertNull(purchaseInfo.getContentItem())
        );

    }

    @Test
    void testFindAllById() {
        Long purchaseId1 = userPurchasesDAO.getNextPurchaseId();

        PurchaseInfo expected = userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId1,
                3L,
                "TEST_TOKEN_1",
                ProviderContentItem.create(MOVIE, "54321"),
                PRICING_OPTION,
                PurchaseInfo.Status.CLEARED,
                null, 1L, null, PRICING_OPTION.getProvider(), null, PaymentProcessor.TRUST, null, null));

        Long purchaseId2 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId2,
                4L,
                "TEST_TOKEN_2",
                ProviderContentItem.create(MOVIE, "54321"),
                PRICING_OPTION,
                PurchaseInfo.Status.CLEARED,
                null, 1L, null, PRICING_OPTION.getProvider(), null, PaymentProcessor.TRUST, null, null));

        List<PurchaseInfo> actual = userPurchasesDAO.findAllByUid(3L);

        Set<Long> actualIds = actual.stream().map(PurchaseInfo::getPurchaseId).collect(toSet());
        assertEquals(Set.of(purchaseId1), actualIds);
    }

    @Test
    void testFindAllWaitingForClearing() {
        // given
        Long purchaseId1 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId1,
                3L,
                "TEST_TOKEN_1",
                ProviderContentItem.create(MOVIE, "54321"),
                PRICING_OPTION,
                PurchaseInfo.Status.WAITING_FOR_CLEARING,
                null, 1L, null, PRICING_OPTION.getProvider(), null, PaymentProcessor.TRUST, null, null));

        Long purchaseId2 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId2,
                4L,
                "TEST_TOKEN_2",
                ProviderContentItem.create(MOVIE, "54321"),
                PRICING_OPTION,
                PurchaseInfo.Status.WAITING_FOR_CLEARING,
                null, 1L, null, PRICING_OPTION.getProvider(), null, PaymentProcessor.TRUST, null, null));

        Long purchaseId3 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId3,
                4L,
                "TEST_TOKEN_2",
                ProviderContentItem.create(MOVIE, "54321"),
                PRICING_OPTION,
                PurchaseInfo.Status.CLEARED,
                null, 1L, null, PRICING_OPTION.getProvider(), null, PaymentProcessor.TRUST, null, null));

        //when
        TransactionTemplate transactionTemplate = new TransactionTemplate(transactionManager);
        CountDownLatch latch = new CountDownLatch(1);
        CountDownLatch latch2 = new CountDownLatch(1);


        CompletableFuture<List<PurchaseInfo>> future = CompletableFuture.supplyAsync(() -> transactionTemplate.execute(
                status -> {
                    try {
                        List<PurchaseInfo> purchases = userPurchasesDAO.findAllWaitingForClearing(Long.MIN_VALUE, 1);
                        latch2.countDown();
                        latch.await();
                        return purchases;
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    return null;
                }
        ));
        List<PurchaseInfo> actual = transactionTemplate.execute(
                status -> {
                    try {
                        latch2.await();
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    List<PurchaseInfo> purchases = userPurchasesDAO.findAllWaitingForClearing(Long.MIN_VALUE, 1);
                    latch.countDown();
                    return purchases;
                }
        );

        // then
        Set<Long> actualIds = actual.stream().map(PurchaseInfo::getPurchaseId).collect(toSet());
        assertEquals(Set.of(purchaseId2), actualIds);


        Set<Long> actualIds2 = future.join().stream().map(PurchaseInfo::getPurchaseId).collect(toSet());
        assertEquals(Set.of(purchaseId1), actualIds2);
    }

    @Test
    void testSetClearingDate() {
        // given
        Long purchaseId1 = userPurchasesDAO.getNextPurchaseId();

        userPurchasesDAO.savePurchaseInfo(PurchaseInfo.createSinglePayment(
                purchaseId1,
                3L,
                "TEST_TOKEN_1",
                ProviderContentItem.create(MOVIE, "54321"),
                PRICING_OPTION,
                PurchaseInfo.Status.WAITING_FOR_CLEARING,
                null, 1L, null, PRICING_OPTION.getProvider(), null, PaymentProcessor.TRUST, null, null));

        // when

        Instant now = Instant.now().truncatedTo(ChronoUnit.MILLIS);
        userPurchasesDAO.updateClearedStatus(purchaseId1, now);

        //then
        assertEquals(now, userPurchasesDAO.getPurchaseInfo(purchaseId1).get().getClearDate());
    }

    private SkillInfo testSkillInfo() {
        return SkillInfo.builder("id")
                .privateKey("key")
                .publicKey("key")
                .slug("slug")
                .build();
    }
}
