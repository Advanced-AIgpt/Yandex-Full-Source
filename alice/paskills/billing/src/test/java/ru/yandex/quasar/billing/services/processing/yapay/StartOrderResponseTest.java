package ru.yandex.quasar.billing.services.processing.yapay;

import java.io.IOException;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;

@JsonTest
class StartOrderResponseTest {
    @Autowired
    private JacksonTester<ResponseWrapper<StartOrderResponse>> tester;

    @Test
    void testDeserialize() throws IOException {
        String response = "{\n" +
                "  \"data\": {\n" +
                "    \"trust_url\": \"https://trust.yandex.ru/...\"\n" +
                "  },\n" +
                "  \"status\": \"success\",\n" +
                "  \"code\": 200\n" +
                "}";

        var expected = new StartOrderResponse("https://trust.yandex.ru/...");


        tester.parse(response).assertThat()
                .extracting(ResponseWrapper::getData)
                .isEqualTo(expected);
    }
}
