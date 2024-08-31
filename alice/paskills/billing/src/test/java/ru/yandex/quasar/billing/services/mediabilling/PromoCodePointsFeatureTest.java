package ru.yandex.quasar.billing.services.mediabilling;

import java.io.IOException;
import java.math.BigDecimal;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;

@JsonTest
class PromoCodePointsFeatureTest {

    @Autowired
    private JacksonTester<PromoCodePointsFeature> tester;

    @Test
    void testDeserialization() throws IOException {
        tester.parse("""
                {"amount": "299.00"}
                """).assertThat().isEqualTo(new PromoCodePointsFeature(new BigDecimal("299.00")));

    }
}
