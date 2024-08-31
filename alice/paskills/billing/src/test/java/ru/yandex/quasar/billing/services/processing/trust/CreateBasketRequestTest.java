package ru.yandex.quasar.billing.services.processing.trust;

import java.io.IOException;
import java.math.BigDecimal;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import ru.yandex.quasar.billing.services.processing.TrustCurrency;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.quasar.billing.services.processing.NdsType.nds_20;


@ExtendWith(SpringExtension.class)
@JsonTest
class CreateBasketRequestTest {

    @Autowired
    private JacksonTester<CreateBasketRequest> tester;

    @Test
    void testSimpleBasket() throws IOException {
        CreateBasketRequest simpleBasket = CreateBasketRequest.createSimpleBasket(BigDecimal.valueOf(100L),
                TrustCurrency.RUB,
                "2",
                "card-x8508",
                "test@yandex.ru",
                "https://test.quasar.common.yandex.ru",
                nds_20,
                "fiscalTitle",
                "commission",
                null
        );
        assertThat(tester.write(simpleBasket))
                .isEqualToJson("{\n" +
                        "\t\"amount\": 100,\n" +
                        "\t\"currency\": \"RUB\",\n" +
                        "\t\"product_id\": \"2\",\n" +
                        "\t\"paymethod_id\": \"card-x8508\",\n" +
                        "\t\"user_email\": \"test@yandex.ru\",\n" +
                        "\t\"back_url\": \"https://test.quasar.common.yandex.ru\",\n" +
                        "\t\"fiscal_nds\": \"nds_20\",\n" +
                        "\t\"fiscal_title\": \"fiscalTitle\",\n" +
                        "\t\"commission_category\": \"commission\"\n" +
                        "}");
    }

    @Test
    void testYaPayBasket() throws IOException {
        CreateBasketRequest simpleBasket = CreateBasketRequest.createSimpleBasket(BigDecimal.valueOf(100L),
                TrustCurrency.RUB,
                "2",
                "card-x8508",
                "test@yandex.ru",
                "https://test.quasar.common.yandex.ru",
                nds_20,
                "fiscalTitle",
                "commission",
                null
        );
        assertThat(tester.write(simpleBasket))
                .isEqualToJson("{\n" +
                        "\t\"amount\": 100,\n" +
                        "\t\"currency\": \"RUB\",\n" +
                        "\t\"product_id\": \"2\",\n" +
                        "\t\"paymethod_id\": \"card-x8508\",\n" +
                        "\t\"user_email\": \"test@yandex.ru\",\n" +
                        "\t\"back_url\": \"https://test.quasar.common.yandex.ru\",\n" +
                        "\t\"fiscal_nds\": \"nds_20\",\n" +
                        "\t\"fiscal_title\": \"fiscalTitle\",\n" +
                        "\t\"commission_category\": \"commission\"\n" +
                        "}");
    }

    @Test
    void testSubscriptionBasket() throws IOException {
        CreateBasketRequest simpleBasket = CreateBasketRequest.createSubscriptionBasket("card-x8508", "test@yandex" +
                ".ru", "https://test.quasar.common.yandex.ru", 1, "2");
        assertThat(tester.write(simpleBasket))
                .isEqualToJson("{\n" +
                        "  \"paymethod_id\": \"card-x8508\",\n" +
                        "  \"user_email\": \"test@yandex.ru\",\n" +
                        "  \"back_url\": \"https://test.quasar.common.yandex.ru\",\n" +
                        "  \"orders\": [{  \"order_id\":\"2\",\n" +
                        "    \"qty\":1\n" +
                        "    }]\n" +
                        "}");
    }
}
