package ru.yandex.alice.paskill.dialogovo.controller;

import java.io.File;
import java.io.IOException;
import java.time.Instant;
import java.util.List;
import java.util.UUID;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.json.JSONException;
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

@SpringBootTest(classes = {E2EConfigProvider.class, VinsControllerE2ETest.SyncExecutorsConfiguration.class},
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@AutoConfigureWebClient(registerRestTemplate = true)
public class VinsControllerE2ETest extends BaseControllerE2ETest {

    private static final Logger logger = LogManager.getLogger();

    static List<Arguments> getVinsRequestTestData() throws IOException {
        return getTestDataFromDir("integration_vins/*");
    }

    //private final static String PROJECT_PATH =
    //        "/Users/pazus/arc/arcadia/alice/paskills/dialogovo/src/test/resources/integration_vins/";

    @ParameterizedTest(name = "vins {0}")
    @MethodSource("getVinsRequestTestData")
    void vinsEndpointTest(String name, String path) throws JSONException, IOException, InterruptedException {
        var dir = new File(path);

        String uuidReplacement = UUID.randomUUID().toString();
        String webHookUrlReplacement = urlForStub("webhookServer", "/");

        Instant now = Instant.now();
        String timestampReplacement = String.valueOf(now.toEpochMilli());

        mockServices(dir, uuidReplacement, now);

        String requestJson = readContent("vins_request.json")
                .replaceAll("\"<TIMESTAMP>\"", timestampReplacement)
                .replaceAll("<UUID>", uuidReplacement)
                .replaceAll("<WEBHOOK_SERVER_URL>", webHookUrlReplacement);

        var headers = getHttpHeadersWithJson();

        var node = objectMapper.readValue(requestJson, ObjectNode.class);

        var prettyMapper = objectMapper.writerWithDefaultPrettyPrinter();

        String request = objectMapper.writeValueAsString(node);
        var vinsResponse = restTemplate.exchange(
                "http://localhost:" + port + "/vins",
                HttpMethod.POST,
                new HttpEntity<>(request, headers),
                String.class);
        assertEquals(vinsResponse.getStatusCode(), HttpStatus.OK);

        validateRequests(dir);

        var expectedVinsResponse = readContent("vins_response.json")
                .replaceAll("\"<TIMESTAMP>\"", timestampReplacement)
                .replaceAll("<UUID>", uuidReplacement)
                .replaceAll("<WEBHOOK_SERVER_URL>", webHookUrlReplacement);

        if (expectedVinsResponse != null) {
            try {
                JSONAssert.assertEquals("vins_response assertion:\n",
                        expectedVinsResponse,
                        vinsResponse.getBody(),
                        new DynamicValueTokenComparator(JSONCompareMode.STRICT));
            } catch (AssertionError e) {
                String prettyBody = prettyMapper.writeValueAsString(
                        objectMapper.readValue(vinsResponse.getBody(), JsonNode.class)
                );
                //canonize(new File(PROJECT_PATH + name), "vins_response.json", prettyBody);
                logger.warn("Assertion failure:\n" + prettyBody, e);
                throw e;
            }

        }

    }
}
