package ru.yandex.quasar.billing.services.processing.trust;

import java.io.IOException;
import java.math.BigDecimal;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import static org.assertj.core.api.Assertions.assertThat;

@SpringJUnitConfig
@JsonTest
class SubscriptionPaymentRefundParamsTest {

    @Autowired
    private JacksonTester<SubscriptionPaymentRefundParams> tester;

    @Test
    void testSerialization() throws IOException {
        SubscriptionPaymentRefundParams value = SubscriptionPaymentRefundParams.subscriptionPayment(
                "4767e33182feb7ed97a9fd157f2358bf",
                "Cancel payment BILLINGSUP-31",
                "2833",
                new BigDecimal("399.00"));

        assertThat(tester.write(value))
                .isEqualToJson("{\n" +
                        "    \"purchase_token\": \"4767e33182feb7ed97a9fd157f2358bf\",\n" +
                        "    \"reason_desc\": \"Cancel payment BILLINGSUP-31\",\n" +
                        "    \"orders\": [\n" +
                        "        {\n" +
                        "            \"delta_amount\": \"399.00\",\n" +
                        "            \"order_id\": \"2833\"\n" +
                        "        }\n" +
                        "    ]\n" +
                        "}");
    }
}
