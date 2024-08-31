package ru.yandex.alice.paskill.dialogovo.controller;

import java.io.File;
import java.io.IOException;
import java.time.Instant;
import java.util.List;
import java.util.UUID;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
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
                ShowControllerE2ETest.SyncExecutorsConfiguration.class
        },
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT
)
@AutoConfigureWebClient(registerRestTemplate = true)
class ShowControllerE2ETest extends BaseControllerE2ETest {

    private static final Logger logger = LogManager.getLogger();

    static List<Arguments> getApplyTestData() throws IOException {
        return getTestDataFromDir("show/*");
    }

    @ParameterizedTest(name = "apply {0}")
    @MethodSource("getApplyTestData")
    void applyEndpointTest(String name, String path, String baseUrl)
            throws JSONException, IOException, InterruptedException {
        logger.debug("start test case");
        var dir = new File(path);

        String uuidReplacement = UUID.randomUUID().toString();

        mockServices(dir, uuidReplacement, Instant.now());

        var headers = getHttpHeadersWithJson();

        var runResponse = restTemplate.exchange(
                "http://localhost:" + port + "/show/MORNING/update",
                HttpMethod.POST,
                new HttpEntity<>(headers),
                Void.class
        );
        assertEquals(runResponse.getStatusCode(), HttpStatus.OK);
        validateRequests(dir);
        logger.debug("finish test case");
    }
}
