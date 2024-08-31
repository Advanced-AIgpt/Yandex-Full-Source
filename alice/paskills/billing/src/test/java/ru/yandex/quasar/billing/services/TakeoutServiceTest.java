package ru.yandex.quasar.billing.services;

import java.util.List;
import java.util.Map;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.config.TestConfigProvider;

import static java.util.Collections.emptyList;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static ru.yandex.quasar.billing.services.TakeoutService.PURCHASE_OFFER;
import static ru.yandex.quasar.billing.services.TakeoutService.USED_DEVICE_PROMO;
import static ru.yandex.quasar.billing.services.TakeoutService.USER_PROMO_CODE;
import static ru.yandex.quasar.billing.services.TakeoutService.USER_PURCHASES;
import static ru.yandex.quasar.billing.services.TakeoutService.USER_SUBSCRIPTIONS;

@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = TestConfigProvider.class)
class TakeoutServiceTest {

    @Autowired
    private TakeoutService takeoutService;

    @Test
    void getUserInfo() {
        Map<String, List<?>> actual = takeoutService.getUserData("1");

        Map<String, List<?>> expected = Map.of(PURCHASE_OFFER, emptyList(),
                USED_DEVICE_PROMO, emptyList(),
                USER_PROMO_CODE, emptyList(),
                USER_PURCHASES, emptyList(),
                USER_SUBSCRIPTIONS, emptyList()
        );
        assertEquals(expected, actual);
    }
}
