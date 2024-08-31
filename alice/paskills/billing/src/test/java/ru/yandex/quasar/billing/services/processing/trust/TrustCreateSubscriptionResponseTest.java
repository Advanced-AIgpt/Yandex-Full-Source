package ru.yandex.quasar.billing.services.processing.trust;

import java.io.IOException;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import static org.junit.jupiter.api.Assertions.assertEquals;

@ExtendWith(SpringExtension.class)
@JsonTest
public class TrustCreateSubscriptionResponseTest {

    @Autowired
    private JacksonTester<TrustCreateSubscriptionResponse> json;

    @Test
    public void testAvailableSerialization() throws IOException {
        var request = TrustCreateSubscriptionResponse.created();

        assertEquals(request, json.parse("{\"status\":\"success\",\"status_code\":\"created\"}").getObject());
    }

}
