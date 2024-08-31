package ru.yandex.quasar.billing.services.processing.yapay;

import java.io.IOException;
import java.math.BigDecimal;
import java.util.List;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;

import ru.yandex.quasar.billing.services.processing.NdsType;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;

@JsonTest
class CreateOrderRequestTest {

    @Autowired
    private JacksonTester<CreateOrderRequest> tester;

    @Test
    void testSerialize() throws IOException {
        String val = "{\n" +
                "  \"caption\": \"some caption\",\n" +
                "  \"description\": \"some description\",\n" +
                "  \"autoclear\": true,\n" +
                "  \"user_email\": \"user@yandex.ru\",\n" +
                "  \"return_url\": \"http://ya.ru\",\n" +
                "  \"paymethod_id\": \"card-xxx\",\n" +
                "  \"mode\": \"prod\",\n" +
                "  \"items\": [\n" +
                "    {\n" +
                "      \"name\": \"slon\",\n" +
                "      \"price\": \"1234.23\",\n" +
                "      \"nds\": \"nds_20\",\n" +
                "      \"currency\": \"RUB\",\n" +
                "      \"amount\": 1\n" +
                "    }\n" +
                "  ]\n" +
                "}";
        CreateOrderRequest request = new CreateOrderRequest("some caption",
                "some description",
                true,
                "user@yandex.ru",
                "9999",
                null,
                "http://ya.ru",
                "card-xxx",
                List.of(new OrderItem(
                        "slon",
                        TrustCurrency.RUB,
                        NdsType.nds_20,
                        new BigDecimal("1234.23"),
                        BigDecimal.ONE,
                        null
                )),
                Mode.PROD);
        Assertions.assertThat(tester.write(request))
                .isEqualToJson(val);
    }
}
