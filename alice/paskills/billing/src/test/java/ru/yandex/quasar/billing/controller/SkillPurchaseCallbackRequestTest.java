package ru.yandex.quasar.billing.controller;

import java.io.IOException;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;

@JsonTest
class SkillPurchaseCallbackRequestTest {

    @Autowired
    private JacksonTester<SkillPurchaseCallbackRequest> tester;

    @Test
    void testDeserialize() throws IOException {
        SkillPurchaseCallbackRequest expected = SkillPurchaseCallbackRequest.builder()
                .type(SkillPurchaseCallbackRequest.CallbackType.ORDER_STATUS_UPDATED)
                .data(SkillPurchaseCallbackRequest.Payload.builder()
                        //.updated(Instant.parse("2019-07-02T16:08:26.722788Z"))
                        .orderId(125L)
                        .newStatus("held")
                        .serviceMerchantId(54L)
                        .build())
                .build();
        tester.parse("{\"type\": \"order_status_updated\", \"data\": {\"updated\": \"2019-07-02T16:08:26" +
                ".722788+03:00\", \"order_id\": 125, \"new_status\": \"held\", \"service_merchant_id\": 54}}")
                .assertThat()
                .isEqualTo(expected);

    }
}
