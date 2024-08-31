package ru.yandex.alice.megamind.cowsay;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import ru.yandex.alice.megamind.protos.common.FrameProto.TSemanticFrame;
import ru.yandex.alice.megamind.protos.common.FrameProto.TSemanticFrame.TSlot;
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TInput;
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TScenarioRunRequest;
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioRunResponse;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

@SpringBootTest(webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
class CowSayControllerTest {
    @LocalServerPort
    private int port;

    @Autowired
    private TestRestTemplate restTemplate;

    @Test
    void cowsayRunMoo() {
        // Original request "Как говорит коровка?"
        final byte[] byteRequest = TScenarioRunRequest.newBuilder()
                .setInput(TInput.newBuilder()
                        .addSemanticFrames(TSemanticFrame.newBuilder()
                                .setName("alice.cowsay")
                                .addSlots(TSlot.newBuilder()
                                        .setName("animal")
                                        .setValue("cow")
                                        .build())
                                .build())
                        .build())
                .build()
                .toByteArray();

        HttpHeaders headers = new HttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, "application/protobuf");
        headers.add(HttpHeaders.ACCEPT, "application/protobuf");
        TScenarioRunResponse result = restTemplate.exchange("http://localhost:" + port + "/cowsay/run", HttpMethod.POST,
                new HttpEntity<>(byteRequest, headers), TScenarioRunResponse.class).getBody();

        assertFalse(result.getFeatures().getIsIrrelevant());
        assertEquals("Муу!", result.getResponseBody().getLayout().getCards(0).getText());
    }

    @Test
    void cowsayRunIrrelevant() {
        final byte[] byteRequest = TScenarioRunRequest.newBuilder()
                .setInput(TInput.newBuilder()
                        .addSemanticFrames(TSemanticFrame.newBuilder()
                                .setName("alice.foobar")
                                .build())
                        .build())
                .build()
                .toByteArray();

        HttpHeaders headers = new HttpHeaders();
        headers.add(HttpHeaders.CONTENT_TYPE, "application/protobuf");
        headers.add(HttpHeaders.ACCEPT, "application/protobuf");
        TScenarioRunResponse result = restTemplate.exchange("http://localhost:" + port + "/cowsay/run", HttpMethod.POST,
                new HttpEntity<>(byteRequest, headers), TScenarioRunResponse.class).getBody();

        assertTrue(result.getFeatures().getIsIrrelevant());
    }
}
