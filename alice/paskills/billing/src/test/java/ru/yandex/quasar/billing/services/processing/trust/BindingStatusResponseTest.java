package ru.yandex.quasar.billing.services.processing.trust;

import java.io.IOException;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

@JsonTest
@SpringJUnitConfig
class BindingStatusResponseTest {

    @Autowired
    private JacksonTester<BindingStatusResponse> tester;

    @Test
    void testDeserializeInitialStatus() throws IOException {
        tester.parse("{\n" +
                "    \"status\": \"success\",\n" +
                "    \"payment_resp_desc\": \"payment_not_found\",\n" +
                "    \"payment_resp_code\": \"payment_not_found\",\n" +
                "    \"binding_result\": \"error\",\n" +
                "    \"method\": \"Trust.CheckBinding\"\n" +
                "}").assertThat().isEqualTo(
                new BindingStatusResponse(
                        "success",
                        "error", "payment_not_found", "payment_not_found",
                        null, null, null, null, "Trust.CheckBinding"
                )
        );
    }

    @Test
    void testDeserializeStarted() throws IOException {
        tester.parse("{\n" +
                "    \"status\": \"success\",\n" +
                "    \"payment_resp_desc\": \"in progress\",\n" +
                "    \"payment_resp_code\": \"wait_for_notification\",\n" +
                "    \"binding_result\": \"in_progress\",\n" +
                "    \"timeout\": \"1200\",\n" +
                "    \"purchase_token\": \"8bd02ee263cbdb6f26c1519e2922bbd2\"\n" +
                "}").assertThat().isEqualTo(
                new BindingStatusResponse(
                        "success",
                        "in_progress", "wait_for_notification", "in progress",
                        "8bd02ee263cbdb6f26c1519e2922bbd2", 1200, null, null, null
                )
        );
    }

    @Test
    void testDeserializeSuccess() throws IOException {
        tester.parse("{\n" +
                "    \"status\": \"success\",\n" +
                "    \"rrn\": \"428404\",\n" +
                "    \"payment_resp_code\": \"success\",\n" +
                "    \"binding_result\": \"success\",\n" +
                "    \"payment_resp_desc\": \"paid ok\",\n" +
                "    \"timeout\": \"1200\",\n" +
                "    \"purchase_token\": \"8bd02ee263cbdb6f26c1519e2922bbd2\",\n" +
                "    \"payment_method_id\": \"card-x7062\"\n" +
                "}")
                .assertThat().isEqualTo(
                new BindingStatusResponse(
                        "success",
                        "success", "success", "paid ok",
                        "8bd02ee263cbdb6f26c1519e2922bbd2", 1200, "card-x7062", "428404", null
                )
        );
    }

    @Test
    void testDeserializeNotEnoughFunds() throws IOException {
        tester.parse("{\n" +
                "    \"status\": \"success\",\n" +
                "    \"rrn\": \"318647\",\n" +
                "    \"payment_resp_code\": \"not_enough_funds\",\n" +
                "    \"binding_result\": \"error\",\n" +
                "    \"payment_resp_desc\": \"RC=51, reason=Not enough funds\",\n" +
                "    \"timeout\": \"1200\",\n" +
                "    \"purchase_token\": \"fb84ba258248bbc1356a3d11cfc38362\"\n" +
                "}")
                .assertThat().isEqualTo(
                new BindingStatusResponse(
                        "success",
                        "error", "not_enough_funds", "RC=51, reason=Not enough funds",
                        "fb84ba258248bbc1356a3d11cfc38362", 1200, null, "318647", null
                )
        );
    }

}
