package ru.yandex.quasar.billing.dao;

import java.math.BigDecimal;
import java.sql.Timestamp;
import java.time.Instant;
import java.util.List;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.LogicalPeriod;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionTestUtil;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.UnistatService;

import static org.junit.jupiter.api.Assertions.assertAll;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNotSame;
import static org.junit.jupiter.api.Assertions.assertNull;
import static ru.yandex.quasar.billing.beans.ContentType.MOVIE;
import static ru.yandex.quasar.billing.beans.ContentType.SEASON;

@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = {TestConfigProvider.class})
class UserSubscriptionsDAOTest implements PricingOptionTestUtil {

    private static final String PROVIDER = "test provider";
    private final PricingOption pricingOption = createSubscriptionPricingOption(PROVIDER,
            new BigDecimal(399),
            ProviderContentItem.create(MOVIE, "test_id"),
            LogicalPeriod.ofDays(30));

    @MockBean
    private UnistatService unistatService;

    @Autowired
    private UserSubscriptionsDAO userSubscriptionsDAO;

    @Test
    void testUpdateSubscriptionStatusAndActiveTill() {
        Long subscriptionId = userSubscriptionsDAO.getNextSubscriptionId();

        userSubscriptionsDAO.save(SubscriptionInfo.create(
                subscriptionId,
                2L,
                new Timestamp(System.currentTimeMillis()),
                ProviderContentItem.create(SEASON, "55555"),
                pricingOption,
                SubscriptionInfo.Status.CREATED,
                new Timestamp(System.currentTimeMillis()),
                null,
                0,
                "1",
                1L,
                PROVIDER, pricingOption.getPurchasingItem(),
                PaymentProcessor.TRUST));

        SubscriptionInfo subscriptionInfoBeforeUpdate = userSubscriptionsDAO.getSubscriptionInfo(subscriptionId);

        assertNotNull(subscriptionInfoBeforeUpdate);
        assertAll(
                () -> assertEquals(subscriptionId, subscriptionInfoBeforeUpdate.getSubscriptionId()),
                () -> assertEquals(SubscriptionInfo.Status.CREATED, subscriptionInfoBeforeUpdate.getStatus()),
                () -> assertEquals(PROVIDER, subscriptionInfoBeforeUpdate.getProvider()),
                () -> assertNull(subscriptionInfoBeforeUpdate.getTrialPeriod()),
                () -> assertEquals("1", subscriptionInfoBeforeUpdate.getProductCode()),
                () -> assertEquals(pricingOption.getUserPrice(), subscriptionInfoBeforeUpdate.getUserPrice()),
                () -> assertEquals(pricingOption.getPrice(), subscriptionInfoBeforeUpdate.getOriginalPrice()),
                () -> assertEquals(pricingOption.getSubscriptionPeriod().toString().substring(1),
                        subscriptionInfoBeforeUpdate.getSubscriptionPeriod()),
                () -> assertEquals(pricingOption.getCurrency(), subscriptionInfoBeforeUpdate.getCurrencyCode())
        );

        userSubscriptionsDAO.updateSubscriptionStatusAndActiveTill(subscriptionId, SubscriptionInfo.Status.ACTIVE,
                new Timestamp(12345L));

        SubscriptionInfo subscriptionInfoAfterUpdate = userSubscriptionsDAO.getSubscriptionInfo(subscriptionId);

        assertNotSame(subscriptionInfoBeforeUpdate, subscriptionInfoAfterUpdate);
        assertNotNull(subscriptionInfoAfterUpdate);

        assertAll(
                () -> assertEquals(subscriptionId, subscriptionInfoAfterUpdate.getSubscriptionId()),
                () -> assertEquals(SubscriptionInfo.Status.ACTIVE, subscriptionInfoAfterUpdate.getStatus()),
                () -> assertEquals(PROVIDER, subscriptionInfoAfterUpdate.getProvider()),
                () -> assertEquals(12345L, subscriptionInfoAfterUpdate.getActiveTill().getTime()),
                () -> assertNotEquals(subscriptionInfoBeforeUpdate.getActiveTill().getTime(),
                        subscriptionInfoAfterUpdate.getActiveTill().getTime()),
                () -> assertNull(subscriptionInfoAfterUpdate.getTrialPeriod()),
                () -> assertEquals("1", subscriptionInfoAfterUpdate.getProductCode())
        );
    }

    @Test
    void testGetActiveSubscriptions() {
        Long subscriptionId1 = userSubscriptionsDAO.getNextSubscriptionId();

        userSubscriptionsDAO.save(SubscriptionInfo.create(
                subscriptionId1,
                3L,
                new Timestamp(System.currentTimeMillis()),
                ProviderContentItem.create(SEASON, "55555"),
                pricingOption,
                SubscriptionInfo.Status.ACTIVE,
                new Timestamp(System.currentTimeMillis()),
                null,
                0,
                "1",
                1L,
                PROVIDER,
                pricingOption.getPurchasingItem(),
                PaymentProcessor.TRUST));

        userSubscriptionsDAO.updateSubscriptionStatusAndActiveTill(subscriptionId1, SubscriptionInfo.Status.ACTIVE,
                new Timestamp(System.currentTimeMillis() + TimeUnit.DAYS.toMillis(5)));

        Long subscriptionId2 = userSubscriptionsDAO.getNextSubscriptionId();

        userSubscriptionsDAO.save(SubscriptionInfo.create(
                subscriptionId2,
                3L,
                new Timestamp(System.currentTimeMillis()),
                ProviderContentItem.create(MOVIE, "44444"),
                pricingOption,
                SubscriptionInfo.Status.ACTIVE,
                new Timestamp(System.currentTimeMillis()),
                null,
                0,
                "1",
                1L,
                PROVIDER,
                pricingOption.getPurchasingItem(),
                PaymentProcessor.TRUST));

        userSubscriptionsDAO.updateSubscriptionStatusAndActiveTill(subscriptionId2, SubscriptionInfo.Status.ACTIVE,
                new Timestamp(System.currentTimeMillis() - TimeUnit.DAYS.toMillis(5)));

        Long subscriptionId3 = userSubscriptionsDAO.getNextSubscriptionId();

        userSubscriptionsDAO.save(SubscriptionInfo.create(
                subscriptionId3,
                3L,
                new Timestamp(System.currentTimeMillis()),
                ProviderContentItem.create(MOVIE, "44444"),
                pricingOption,
                SubscriptionInfo.Status.ACTIVE,
                new Timestamp(System.currentTimeMillis()),
                null,
                0,
                "1",
                1L,
                PROVIDER,
                pricingOption.getPurchasingItem(),
                PaymentProcessor.TRUST));

        userSubscriptionsDAO.updateSubscriptionStatusAndActiveTill(subscriptionId3, SubscriptionInfo.Status.DISMISSED,
                new Timestamp(System.currentTimeMillis() + TimeUnit.DAYS.toMillis(15)));

        Long subscriptionId4 = userSubscriptionsDAO.getNextSubscriptionId();

        userSubscriptionsDAO.save(SubscriptionInfo.create(
                subscriptionId4,
                3L,
                new Timestamp(System.currentTimeMillis()),
                ProviderContentItem.create(MOVIE, "44444"),
                pricingOption,
                SubscriptionInfo.Status.ACTIVE,
                new Timestamp(System.currentTimeMillis()),
                null,
                0,
                "1",
                1L,
                PROVIDER,
                pricingOption.getPurchasingItem(),
                PaymentProcessor.TRUST));

        userSubscriptionsDAO.updateSubscriptionStatusAndActiveTill(subscriptionId4, SubscriptionInfo.Status.DISMISSED,
                new Timestamp(System.currentTimeMillis() - TimeUnit.DAYS.toMillis(15)));

        Long subscriptionId5 = userSubscriptionsDAO.getNextSubscriptionId();
        userSubscriptionsDAO.save(SubscriptionInfo.create(
                subscriptionId5,
                3L,
                Timestamp.from(Instant.now()),
                ProviderContentItem.create(MOVIE, "44444"),
                pricingOption,
                SubscriptionInfo.Status.EXPIRED,
                Timestamp.from(Instant.now()),
                null,
                0,
                "1",
                1L,
                PROVIDER,
                pricingOption.getPurchasingItem(),
                PaymentProcessor.TRUST));

        Long subscriptionId6 = userSubscriptionsDAO.getNextSubscriptionId();
        userSubscriptionsDAO.save(SubscriptionInfo.create(
                subscriptionId6,
                3L,
                Timestamp.from(Instant.now()),
                ProviderContentItem.create(MOVIE, "44444"),
                pricingOption,
                SubscriptionInfo.Status.EXPIRED,
                Timestamp.from(Instant.now().minusSeconds(60 * 60)),
                null,
                0,
                "1",
                1L,
                PROVIDER,
                pricingOption.getPurchasingItem(),
                PaymentProcessor.TRUST));

        List<SubscriptionInfo> activeSubscriptions = userSubscriptionsDAO.getActiveSubscriptions(3L);
        Set<Long> subscriptionIds = activeSubscriptions.stream()
                .map(SubscriptionInfo::getSubscriptionId)
                .collect(Collectors.toSet());

        assertEquals(Set.of(subscriptionId1, subscriptionId3), subscriptionIds);
    }

    @Test
    void testFindByUid() {
        Long subscriptionId1 = userSubscriptionsDAO.getNextSubscriptionId();

        userSubscriptionsDAO.save(SubscriptionInfo.create(
                subscriptionId1,
                3L,
                new Timestamp(System.currentTimeMillis()),
                ProviderContentItem.create(SEASON, "55555"),
                pricingOption,
                SubscriptionInfo.Status.ACTIVE,
                new Timestamp(System.currentTimeMillis()),
                null,
                0,
                "1",
                1L,
                PROVIDER,
                pricingOption.getPurchasingItem(),
                PaymentProcessor.TRUST));

        Long subscriptionId2 = userSubscriptionsDAO.getNextSubscriptionId();
        userSubscriptionsDAO.save(SubscriptionInfo.create(
                subscriptionId2,
                4L,
                new Timestamp(System.currentTimeMillis()),
                ProviderContentItem.create(SEASON, "55555"),
                pricingOption,
                SubscriptionInfo.Status.ACTIVE,
                new Timestamp(System.currentTimeMillis()),
                null,
                0,
                "1",
                1L,
                PROVIDER,
                pricingOption.getPurchasingItem(),
                PaymentProcessor.TRUST));

        List<SubscriptionInfo> activeSubscriptions = userSubscriptionsDAO.findAllByUid(3L);
        Set<Long> subscriptionIds = activeSubscriptions.stream()
                .map(SubscriptionInfo::getSubscriptionId)
                .collect(Collectors.toSet());

        assertEquals(Set.of(subscriptionId1), subscriptionIds);
    }

}
