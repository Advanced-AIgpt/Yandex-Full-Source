package ru.yandex.alice.paskill.dialogovo.controller;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.web.client.RestTemplate;

import ru.yandex.alice.paskill.dialogovo.config.E2EConfigProvider;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

@SpringBootTest(
        classes = {
                E2EConfigProvider.class
        },
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT
)
@AutoConfigureWebClient(registerRestTemplate = true)
class RecipeStoreControllerTest {

    @Autowired
    RestTemplate restTemplate;

    @Autowired
    ObjectMapper objectMapper;

    @LocalServerPort
    protected int port;

    @Test
    public void getAllRecipes() throws JsonProcessingException {
        var response = restTemplate.exchange(
                "http://localhost:" + port + "/store/recipes/all",
                HttpMethod.GET,
                null,
                String.class);
        assertEquals(response.getStatusCode(), HttpStatus.OK);
        JsonNode node = objectMapper.readTree(response.getBody());
        assertTrue(node.isArray());
        assertTrue(node.size() > 0);
    }

    @Test
    public void getOne() throws JsonProcessingException {
        var response = restTemplate.exchange(
                "http://localhost:" + port + "/store/recipes/blini",
                HttpMethod.GET,
                null,
                String.class);
        assertEquals(response.getStatusCode(), HttpStatus.OK);
        JsonNode node = objectMapper.readTree(response.getBody());
        assertTrue(node.isObject());
    }

    @Test
    public void getSimilar() throws JsonProcessingException {
        var response = restTemplate.exchange(
                "http://localhost:" + port + "/store/recipes/blini/similar",
                HttpMethod.GET,
                null,
                String.class);
        assertEquals(response.getStatusCode(), HttpStatus.OK);
        JsonNode node = objectMapper.readTree(response.getBody());
        assertTrue(node.isArray());
        assertTrue(node.size() > 0);
    }

}
