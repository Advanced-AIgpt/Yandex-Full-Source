package ru.yandex.quasar.billing.services.processing.trust;

import java.io.IOException;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

@JsonTest
@SpringJUnitConfig
class CreateBindingResponseTest {

    @Autowired
    private JacksonTester<CreateBindingResponse> tester;

    @Test
    void testDeserialize() throws IOException {
        tester.parse("{\n" +
                "    \"status\": \"success\",\n" +
                "    \"purchase_token\": \"077b4720623e008746725a12f514df03\"\n" +
                "}").assertThat().isEqualTo(
                new CreateBindingResponse(
                        CreateBindingResponse.Status.success,
                        "077b4720623e008746725a12f514df03"
                )
        );
    }
}
