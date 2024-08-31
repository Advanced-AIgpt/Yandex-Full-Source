package ru.yandex.quasar.billing.beans;

import java.io.IOException;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import static ru.yandex.quasar.billing.beans.PaymentProcessor.TRUST;
import static ru.yandex.quasar.billing.beans.PaymentProcessor.YANDEX_PAY;


@SpringJUnitConfig
@JsonTest
class PaymentProcessorTest {
    @Autowired
    private JacksonTester<TestClass> tester;

    @Test
    void testDeserialize() throws IOException {
        tester.parse("{\"processor\":\"TRUST\"}")
                .assertThat().isEqualTo(new TestClass(TRUST));
    }

    @Test
    void testDeserializeUnknownToTrust() throws IOException {
        tester.parse("{\"processor\":\"UNKNOWN\"}")
                .assertThat().isEqualTo(new TestClass(TRUST));
    }

    @Test
    void testDeserializeUnknownToYandexPay() throws IOException {
        tester.parse("{\"processor\":\"YANDEX_PAY\"}")
                .assertThat().isEqualTo(new TestClass(YANDEX_PAY));
    }

    @Data
    private static class TestClass {
        @JsonProperty("processor")
        private PaymentProcessor processor;

        TestClass() {
        }

        TestClass(PaymentProcessor processor) {
            this.processor = processor;
        }
    }

}
