package ru.yandex.alice.paskill.dialogovo.utils;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.web.client.RestTemplate;

import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider;

@SpringBootTest(classes = TestConfigProvider.class, webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@AutoConfigureWebClient(registerRestTemplate = true)
public abstract class IntegrationTestBase {
    @Autowired
    protected RestTemplate restTemplate;

    @LocalServerPort
    protected int port;
}
