package ru.yandex.quasar.billing.services.mediabilling;

import java.io.IOException;
import java.math.BigDecimal;
import java.time.ZoneId;
import java.time.ZonedDateTime;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

@JsonTest
@SpringJUnitConfig
class OrderInfoTest {
    @Autowired
    private JacksonTester<OrderInfo.Wrapper> tester;

    @Test
    void testDeserialization() throws IOException {
        OrderInfo expected = new OrderInfo(5014335L,
                BigDecimal.ZERO,
                BigDecimal.ZERO,
                "RUB",
                ZonedDateTime.of(2019, 2, 8, 8, 18, 39, 0, ZoneId.of("UTC")).toInstant(),
                "native-auto-subscription",
                BigDecimal.ZERO,
                BigDecimal.ONE,
                true,
                OrderInfo.OrderStatus.OK);
        tester.parse("{\n" +
                "    \"invocationInfo\": {\n" +
                "        \"hostname\": \"sas1-1569-sas-music-test-back-14646.gencfg-c.yandex.net\",\n" +
                "        \"action\": \"GET_CommonAccountActionContainer.getOrderInfo/billing/order-info\",\n" +
                "        \"app-name\": \"music-web\",\n" +
                "        \"app-version\": \"2019-02-21.stable-98.4483667 (exported; 2019-02-21 10:46)\",\n" +
                "        \"req-id\": \"d9ff582f8ac966913ba091126dc3c8eb\",\n" +
                "        \"exec-duration-millis\": \"10\"\n" +
                "    },\n" +
                "    \"result\": {\n" +
                "        \"orderId\": 5014335,\n" +
                "        \"paidDays\": 0,\n" +
                "        \"debitAmount\": 0,\n" +
                "        \"currency\": \"RUB\",\n" +
                "        \"created\": \"2019-02-08T11:18:39+03:00\",\n" +
                "        \"type\": \"native-auto-subscription\",\n" +
                "        \"paidAmount\": 0,\n" +
                "        \"paidQuantity\": 1,\n" +
                "        \"trialPayment\": true,\n" +
                "        \"status\": \"ok\"\n" +
                "    }\n" +
                "}").assertThat().returns(expected, OrderInfo.Wrapper::getResult);
    }
}
