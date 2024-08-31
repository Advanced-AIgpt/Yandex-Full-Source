package ru.yandex.alice.paskill.dialogovo.service.penguinary;

import java.util.List;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.autoconfigure.web.client.RestClientTest;
import org.springframework.http.MediaType;
import org.springframework.test.web.client.MockRestServiceServer;
import org.springframework.test.web.client.match.MockRestRequestMatchers;
import org.springframework.web.client.RestTemplate;

import ru.yandex.alice.paskill.dialogovo.config.PenguinaryConfig;
import ru.yandex.alice.paskill.dialogovo.utils.executor.TestExecutorsFactory;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.springframework.test.web.client.match.MockRestRequestMatchers.requestTo;
import static org.springframework.test.web.client.response.MockRestResponseCreators.withSuccess;

@RestClientTest()
@AutoConfigureWebClient(registerRestTemplate = true)
class PenguinaryServiceImplTest {

    @Autowired
    private RestTemplate restTemplate;
    @Autowired
    private MockRestServiceServer server;
    private PenguinaryServiceImpl client;
    @Autowired
    private ObjectMapper objectMapper;

    @BeforeEach
    void setUp() {
        var config = new PenguinaryConfig("http://localhost/", 300, 10000);
        client = new PenguinaryServiceImpl(config, restTemplate, TestExecutorsFactory.newSingleThreadExecutor());
        server.reset();
    }

    @Test
    void searchUtterance() throws JsonProcessingException {
        String response = objectMapper.writeValueAsString(new PenguinaryResponse(new double[]{0.1d, 0.2d}, List.of(
                "skillid-1", "skillid-2")));
        server.expect(requestTo("http://localhost/get_intents"))
                .andExpect(MockRestRequestMatchers.content().json("{\"node_id\":\"skill_activation\", \"utterance\": " +
                        "\"привет\"}"))
                .andRespond(withSuccess(response, MediaType.APPLICATION_JSON));
        PenguinaryResult actual = client.findSkillsByUtterance("привет");

        server.verify();

        PenguinaryResult expected = new PenguinaryResult(List.of(
                new PenguinaryResult.Candidate(0.1d, "skillid-1"),
                new PenguinaryResult.Candidate(0.2d, "skillid-2")
        ));
        assertEquals(expected, actual);
    }

    @Test
    void searchUtteranceSort() throws JsonProcessingException {
        String response = objectMapper.writeValueAsString(new PenguinaryResponse(new double[]{0.2d, 0.1d}, List.of(
                "skillid-2", "skillid-1")));
        server.expect(requestTo("http://localhost/get_intents"))
                .andExpect(MockRestRequestMatchers.content().json("{\"node_id\":\"skill_activation\", \"utterance\": " +
                        "\"привет\"}"))
                .andRespond(withSuccess(response, MediaType.APPLICATION_JSON));
        PenguinaryResult actual = client.findSkillsByUtterance("привет");

        server.verify();

        PenguinaryResult expected = new PenguinaryResult(List.of(
                new PenguinaryResult.Candidate(0.1d, "skillid-1"),
                new PenguinaryResult.Candidate(0.2d, "skillid-2")
        ));
        assertEquals(expected, actual);
    }

    @Test
    void searchUtteranceEmpty() throws JsonProcessingException {
        String response = objectMapper.writeValueAsString(new PenguinaryResponse(new double[0], List.of()));
        server.expect(requestTo("http://localhost/get_intents"))
                .andExpect(MockRestRequestMatchers.content().json("{\"node_id\":\"skill_activation\", \"utterance\": " +
                        "\"привет\"}"))
                .andRespond(withSuccess(response, MediaType.APPLICATION_JSON));
        PenguinaryResult actual = client.findSkillsByUtterance("привет");

        server.verify();

        assertEquals(PenguinaryResult.EMPTY, actual);
    }
}
