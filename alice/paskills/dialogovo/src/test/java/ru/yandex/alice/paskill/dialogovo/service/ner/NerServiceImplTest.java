package ru.yandex.alice.paskill.dialogovo.service.ner;

import java.io.IOException;
import java.util.Collections;
import java.util.List;

import okhttp3.mockwebserver.MockResponse;
import okhttp3.mockwebserver.MockWebServer;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.http.HttpHeaders;
import org.springframework.util.MimeTypeUtils;
import org.springframework.web.client.RestTemplate;

import ru.yandex.alice.paskill.dialogovo.config.NerApiConfig;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Nlu;
import ru.yandex.alice.paskill.dialogovo.utils.executor.TestExecutorsFactory;

import static org.junit.jupiter.api.Assertions.assertEquals;

class NerServiceImplTest {

    private static final String NLU_RESPONSE = "{\n" +
            "  \"nlu\": {\n" +
            "  \"tokens\": [\n" +
            "    \"давай\",\n" +
            "    \"поиграем\",\n" +
            "    \"в\",\n" +
            "    \"города\"\n" +
            "  ],\n" +
            "  \"entities\": []\n" +
            "  }\n" +
            "}";
    private NerServiceImpl nerService;
    private MockWebServer server;

    @BeforeEach
    void setUp() throws IOException {
        server = new MockWebServer();
        server.start();
        NerApiConfig config = new NerApiConfig(
                server.url("/").url().toString(),
                5000,
                5000,
                86400,
                25000);
        nerService = new NerServiceImpl(config,
                new RestTemplate(),
                TestExecutorsFactory.newSingleThreadExecutor(),
                1
        );
    }

    @AfterEach
    void tearDown() throws IOException {
        server.shutdown();
    }

    @Test
    void testWizardCall() {

        server.enqueue(new MockResponse()
                .setBody(NLU_RESPONSE)
                .setHeader(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE));

        Nlu actual = nerService.getNlu("давай поиграем в города", "1");

        Nlu expected = Nlu.builder()
                .entities(List.of())
                .intents(Collections.emptyMap())
                .tokens(List.of("давай", "поиграем", "в", "города"))
                .build();
        assertEquals(expected, actual);
    }

    @Test
    void testEmptyUtterance() {

        server.enqueue(new MockResponse()
                .setBody(NLU_RESPONSE)
                .setHeader(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE));

        Nlu actual = nerService.getNlu("", "1");

        assertEquals(Nlu.EMPTY, actual);
    }

    @Test
    void testWizard500() {
        server.enqueue(new MockResponse()
                .setResponseCode(500));
        server.enqueue(new MockResponse()
                .setBody(NLU_RESPONSE)
                .setHeader(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE));

        Nlu actual = nerService.getNlu("давай поиграем в города", "1");
        assertEquals(Nlu.EMPTY, actual);

        Nlu actual2 = nerService.getNlu("давай поиграем в города", "1");
        Nlu expected2 = Nlu.builder()
                .entities(List.of())
                .intents(Collections.emptyMap())
                .tokens(List.of("давай", "поиграем", "в", "города"))
                .build();
        assertEquals(expected2, actual2);
    }
}
