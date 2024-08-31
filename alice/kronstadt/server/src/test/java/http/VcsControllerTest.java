package ru.yandex.alice.kronstadt.server.http;

import java.util.Objects;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.web.client.RestTemplate;

import ru.yandex.alice.kronstadt.core.VcsUtils;
import ru.yandex.passport.tvmauth.TvmClient;

import static org.junit.jupiter.api.Assertions.assertEquals;

/**
 * Test for {@link VcsController}
 */
@SpringBootTest(
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@AutoConfigureWebClient(registerRestTemplate = true)
class VcsControllerTest {
    @Autowired
    private VcsUtils vcsUtils;

    @Autowired
    private RestTemplate restTemplate;

    @LocalServerPort
    private int port;

    @MockBean
    private TvmClient tvmClient;

    @Test
    void test_getVcsVersionJson() {
        var response = restTemplate.getForObject(
                "http://localhost:" + port + "/version_json",
                VcsController.VcsVersionJsonResponse.class);

        Objects.requireNonNull(response);
        assertEquals(vcsUtils.getBranch(), response.getBranch());
        assertEquals(vcsUtils.getTag(), response.getTag());
    }
}
