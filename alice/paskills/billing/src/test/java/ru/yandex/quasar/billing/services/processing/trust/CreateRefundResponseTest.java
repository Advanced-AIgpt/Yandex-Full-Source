package ru.yandex.quasar.billing.services.processing.trust;

import java.io.IOException;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

@JsonTest
@SpringJUnitConfig
class CreateRefundResponseTest {
    @Autowired
    private JacksonTester<CreateRefundResponse> tester;

    @Test
    void testDeserialize() throws IOException {
        tester.parse("{\"status\": \"success\", \"trust_refund_id\": \"5c31cf15bb044914caec5880\"}")
                .assertThat().isEqualTo(new CreateRefundResponse(CreateRefundStatus.success,
                "5c31cf15bb044914caec5880"));
    }

    @Test
    void testDeserializeError() throws IOException {
        tester.parse("{\"status\": \"some_error\"}")
                .assertThat().isEqualTo(new CreateRefundResponse(CreateRefundStatus.unknown, null));
    }
}
