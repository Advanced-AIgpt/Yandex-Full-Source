package ru.yandex.quasar.billing.config;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

class TestProdConfig {

    private final ObjectMapper objectMapper = new ObjectMapper();

    @Test
    void testProdConfigDeserialization() throws IOException {
        String blackboxHost = "localhost";
        String socialismHost = "localhost";
        String quasarHostRu = "localhost";
        String quasarHostNet = "localhost";

        String config = Files.readString(Path.of("configs/prod/quasar-billing.cfg"), StandardCharsets.UTF_8);

        config = config.replace("${BLACKBOX_HOST}", blackboxHost)
                .replace("${SOCIALISM_HOST}", socialismHost)
                .replace("${QUASAR_HOST_RU}", quasarHostRu)
                .replace("${QUASAR_HOST_NET}", quasarHostNet);

        BillingConfig billingConfig = objectMapper.readValue(config, BillingConfig.class);

        assertNotNull(billingConfig);
        assertTrue(billingConfig.getUserIpStub().isEmpty(), "userIpStub config is not empty");
    }

    @Test
    void testTestConfigDeserialization() throws IOException {
        String blackboxHost = "localhost";
        String socialismHost = "localhost";
        String quasarHostRu = "localhost";
        String quasarHostNet = "localhost";

        String config = Files.readString(Path.of("configs/test/quasar-billing.cfg"), StandardCharsets.UTF_8);

        config = config.replace("${BLACKBOX_HOST}", blackboxHost)
                .replace("${SOCIALISM_HOST}", socialismHost)
                .replace("${QUASAR_HOST_RU}", quasarHostRu)
                .replace("${QUASAR_HOST_NET}", quasarHostNet);

        BillingConfig billingConfig = objectMapper.readValue(config, BillingConfig.class);

        assertNotNull(billingConfig);
    }
}
