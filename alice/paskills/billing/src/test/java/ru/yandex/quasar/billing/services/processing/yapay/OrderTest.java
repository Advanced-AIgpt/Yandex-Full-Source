package ru.yandex.quasar.billing.services.processing.yapay;

import java.io.IOException;
import java.math.BigDecimal;
import java.time.ZonedDateTime;
import java.util.List;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;

import ru.yandex.quasar.billing.services.processing.NdsType;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;

@JsonTest
class OrderTest {

    private static final String EXPECTED = "{\n" +
            "  \"data\": {\n" +
            "    \"active\": true,\n" +
            "    \"items\": [\n" +
            "      {\n" +
            "        \"nds\": \"nds_20\",\n" +
            "        \"name\": \"slon\",\n" +
            "        \"price\": 1234.0,\n" +
            "        \"amount\": 3.0,\n" +
            "        \"currency\": \"RUB\",\n" +
            "        \"product_id\": 1\n" +
            "      }\n" +
            "    ],\n" +
            "    \"order_id\": 1,\n" +
            "    \"verified\": false,\n" +
            "    \"user_email\": null,\n" +
            "    \"price\": 3702.0,\n" +
            "    \"description\": \"some description\",\n" +
            "    \"receipt_url\": \"https://pay.yandex.ru/transaction/gAAAAABcijfnexcyaqOKsjKLC30Km-MUVsj8nM8OrOXu.." +
            ".\",\n" +
            "    \"kind\": \"pay\",\n" +
            "    \"currency\": \"RUB\",\n" +
            "    \"pay_status\": \"new\",\n" +
            "    \"closed\": null,\n" +
            "    \"created\": \"2019-04-02T11:34:18.576905+00:00\",\n" +
            "    \"caption\": \"some caption\",\n" +
            "    \"revision\": 2,\n" +
            "    \"uid\": 756727506,\n" +
            "    \"user_description\": null,\n" +
            "    \"updated\": \"2019-04-02T11:34:18.576905+00:00\"\n" +
            "  },\n" +
            "  \"status\": \"success\",\n" +
            "  \"code\": 200\n" +
            "}";
    @Autowired
    private JacksonTester<ResponseWrapper<Order>> tester;

    @Test
    void testDeserialize() throws IOException {
        Order result = Order.builder()
                .active(true)
                .items(List.of(new OrderItem(
                        "slon",
                        TrustCurrency.RUB,
                        NdsType.nds_20,
                        new BigDecimal("1234.0"),
                        new BigDecimal("3.0"),
                        1L))
                )
                .orderId(1L)
                .verified(false)
                .userEmail(null)
                .price(new BigDecimal("3702.0"))
                .description("some description")
                .receiptUrl("https://pay.yandex.ru/transaction/gAAAAABcijfnexcyaqOKsjKLC30Km-MUVsj8nM8OrOXu...")
                .kind("pay")
                .currency(TrustCurrency.RUB)
                .payStatus(Order.Status.created)
                .closed(null)
                .created(ZonedDateTime.parse("2019-04-02T11:34:18.576905+00:00").toInstant())
                .caption("some caption")
                .revision(2)
                .uid(756727506L)
                .userDescription(null)
                .updated(ZonedDateTime.parse("2019-04-02T11:34:18.576905+00:00").toInstant())
                .build();

        tester.parse(EXPECTED).assertThat().isEqualTo(ResponseWrapper.success(result));
    }
}
