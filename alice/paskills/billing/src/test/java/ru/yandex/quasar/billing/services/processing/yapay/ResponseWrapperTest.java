package ru.yandex.quasar.billing.services.processing.yapay;

import java.io.IOException;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;

@JsonTest
class ResponseWrapperTest {

    @Autowired
    private JacksonTester<ResponseWrapper<ResponseWrapper.Empty>> tester;

    @Test
    void testDeserializeEmpty() throws IOException {
        var response = "{\n" +
                "  \"data\": {},\n" +
                "  \"status\": \"success\",\n" +
                "  \"code\": 200\n" +
                "}";
        tester.parse(response).assertThat().isEqualTo(ResponseWrapper.EMPTY);
    }
}
