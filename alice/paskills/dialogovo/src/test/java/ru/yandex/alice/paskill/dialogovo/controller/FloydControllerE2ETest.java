package ru.yandex.alice.paskill.dialogovo.controller;

import java.io.File;
import java.io.IOException;
import java.time.Instant;
import java.util.List;
import java.util.UUID;

import com.fasterxml.jackson.databind.node.ObjectNode;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.json.JSONException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.skyscreamer.jsonassert.JSONAssert;
import org.skyscreamer.jsonassert.JSONCompareMode;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;

import ru.yandex.alice.kronstadt.test.DynamicValueTokenComparator;
import ru.yandex.alice.paskill.dialogovo.config.E2EConfigProvider;

import static org.junit.jupiter.api.Assertions.assertEquals;

@SpringBootTest(classes = {E2EConfigProvider.class, FloydControllerE2ETest.SyncExecutorsConfiguration.class},
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@AutoConfigureWebClient(registerRestTemplate = true)
public class FloydControllerE2ETest extends BaseControllerE2ETest {

    private static final Logger logger = LogManager.getLogger();

    static List<Arguments> getFloydRequestTestData() throws IOException {
        return getTestDataFromDir("integration_floyd/*");
    }

    @BeforeEach
    void setUp() throws IOException {
        super.setUp();
    }

    @ParameterizedTest(name = "floyd {0}")
    @MethodSource("getFloydRequestTestData")
    void floydEndpointTest(String name, String path) throws JSONException, IOException, InterruptedException {
        var dir = new File(path);

        String uuidReplacement = UUID.randomUUID().toString();
        String webHookUrlReplacement = urlForStub("webhookServer", "/");

        Instant now = Instant.now();
        String timestampReplacement = String.valueOf(now.toEpochMilli());

        mockServices(dir, uuidReplacement, now);

        String requestJson = readContent("floyd_request.json")
                .replaceAll("\"<TIMESTAMP>\"", timestampReplacement)
                .replaceAll("<UUID>", uuidReplacement)
                .replaceAll("<WEBHOOK_SERVER_URL>", webHookUrlReplacement);

        var headers = getHttpHeadersWithJson();
        headers.add(FloydController.RANDOM_SEED_HEADER, "42");

        var node = objectMapper.readValue(requestJson, ObjectNode.class);

        String request = objectMapper.writeValueAsString(node);
        var floydResponse = restTemplate.exchange(
                "http://localhost:" + port + "/floyd/skill/672f7477-d3f0-443d-9bd5-2487ab0b6a4c",
                HttpMethod.POST,
                new HttpEntity<>(request, headers),
                String.class);
        assertEquals(floydResponse.getStatusCode(), HttpStatus.OK);

        validateRequests(dir);

        var expectedFloydResponse = readContent("floyd_response.json");

        if (expectedFloydResponse != null) {
            try {
                JSONAssert.assertEquals(expectedFloydResponse, floydResponse.getBody(),
                        new DynamicValueTokenComparator(JSONCompareMode.STRICT));
            } catch (AssertionError e) {
                logger.warn("Assertion failure:" + floydResponse.getBody(), e);
                throw e;
            }

        }

    }
}
