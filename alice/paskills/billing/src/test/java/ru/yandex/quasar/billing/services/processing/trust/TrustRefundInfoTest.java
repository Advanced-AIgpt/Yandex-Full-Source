package ru.yandex.quasar.billing.services.processing.trust;

import java.io.IOException;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

@SpringJUnitConfig
@JsonTest
class TrustRefundInfoTest {
    @Autowired
    private JacksonTester<TrustRefundInfo> tester;

    @Test
    void testDeserializeRefundOnStart() throws IOException {
        tester.parse("{\"status\": \"wait_for_notification\", \"status_desc\": \"refund is in queue\"}")
                .assertThat().isEqualTo(new TrustRefundInfo(RefundStatus.wait_for_notification, "refund is in queue",
                null));
    }

    @Test
    void testDeserializeRefundSuccess() throws IOException {
        tester.parse("{\"status\": \"success\", \"fiscal_receipt_url\": \"https://trust.yandex" +
                ".ru/checks/675d1a1973aae32905f9a198aceabcfb/receipts/5c31cf15bb044914caec5880?mode=mobile\", " +
                "\"status_desc\": \"refund sent to payment system\"}")
                .assertThat().isEqualTo(new TrustRefundInfo(RefundStatus.success, "refund sent to payment system",
                "https://trust.yandex.ru/checks/675d1a1973aae32905f9a198aceabcfb/receipts/5c31cf15bb044914caec5880" +
                        "?mode=mobile"));
    }

    @Test
    void testDeserializeRefundFailure() throws IOException {
        tester.parse("{\"status\": \"failed\", \"status_desc\": \"some failure description\"}")
                .assertThat().isEqualTo(new TrustRefundInfo(RefundStatus.failed, "some failure description", null));
    }
}
