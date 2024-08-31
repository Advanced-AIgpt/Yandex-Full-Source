package ru.yandex.quasar.billing.dao;

import java.math.BigDecimal;
import java.util.List;

import javax.annotation.Nonnull;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.LogicalPeriod;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionTestUtil;
import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.config.TestConfigProvider;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;

@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = {TestConfigProvider.class})
class UserPromoCodeDaoTest implements PricingOptionTestUtil {

    private static final String PROVIDER = "test";
    @Autowired
    private UserPromoCodeDao userPromoCodeDao;

    @Test
    void testSaveAndQuery() {
        PricingOption pricingOption = getPricingOption();
        UserPromoCode userPromoCode =
                new UserPromoCode(1L, PROVIDER, "code", PricingOptionType.BUY, 1, pricingOption, 1L);

        assertFalse(userPromoCodeDao.findByUidAndProviderAndCode(1L, PROVIDER, "code").isPresent());

        userPromoCodeDao.save(userPromoCode);

        assertEquals(userPromoCode, userPromoCodeDao.findByUidAndProviderAndCode(1L, PROVIDER, "code").get());
    }

    @Test
    void testFindByUid() {
        PricingOption pricingOption = getPricingOption();
        UserPromoCode userPromoCode =
                new UserPromoCode(1L, PROVIDER, "code", PricingOptionType.BUY, 1, pricingOption, 1L);
        UserPromoCode userPromoCode2 =
                new UserPromoCode(2L, PROVIDER, "code1", PricingOptionType.BUY, 1, pricingOption, 1L);
        userPromoCodeDao.save(userPromoCode);
        userPromoCodeDao.save(userPromoCode2);

        assertEquals(List.of(userPromoCode), userPromoCodeDao.findAllByUid(1L));
    }

    @Nonnull
    private PricingOption getPricingOption() {
        ProviderContentItem purchasingItem = ProviderContentItem.create(ContentType.SUBSCRIPTION,
                "test_subscription_id");
        return createSubscriptionPricingOption(PROVIDER, BigDecimal.ONE, purchasingItem, LogicalPeriod.ofDays(1));
    }
}
