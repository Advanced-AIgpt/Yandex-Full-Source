package ru.yandex.alice.paskill.dialogovo.controller;

import java.io.File;
import java.io.IOException;
import java.time.Instant;
import java.util.List;
import java.util.UUID;

import com.fasterxml.jackson.databind.node.ObjectNode;
import org.json.JSONException;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;

import ru.yandex.alice.paskill.dialogovo.config.E2EConfigProvider;

import static org.junit.jupiter.api.Assertions.assertEquals;

@SpringBootTest(
        classes = {
                E2EConfigProvider.class,
                BillingControllerE2ETest.SyncExecutorsConfiguration.class
        },
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT
)
@AutoConfigureWebClient(registerRestTemplate = true)
public class BillingControllerE2ETest extends BaseControllerE2ETest {
    static List<Arguments> getApplyTestData() throws IOException {
        return getTestDataFromDir("integration_billing/*");
    }

    @ParameterizedTest(name = "{0}")
    @MethodSource("getApplyTestData")
    void applyEndpointTest(String name, String path, String baseUrl) throws JSONException, IOException {
        var dir = new File(path);

        String uuidReplacement = UUID.randomUUID().toString();

        mockServices(dir, uuidReplacement, Instant.now());

        ObjectNode node = objectMapper.readValue(readContent("billing_controller_request.json"), ObjectNode.class);

        var request = objectMapper.writeValueAsString(node)
                .replaceAll("\"<TIMESTAMP>\"", String.valueOf(System.currentTimeMillis()))
                .replaceAll("<UUID>", UUID.randomUUID().toString());

        var response = restTemplate.exchange(
                "http://localhost:" + port + "/billing/skill-purchase/callback",
                HttpMethod.POST,
                new HttpEntity<>(request, getHttpHeadersWithJson()),
                String.class);

        assertEquals(response.getStatusCode(), HttpStatus.OK);
        validateResponse("billing_controller_response.json", response);

        validateRequests(dir);
    }
}
