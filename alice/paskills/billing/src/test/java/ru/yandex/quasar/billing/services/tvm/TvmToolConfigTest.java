package ru.yandex.quasar.billing.services.tvm;

import java.io.File;
import java.io.IOException;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;

import static org.junit.jupiter.api.Assertions.assertTrue;

@JsonTest
class TvmToolConfigTest {
    @Autowired
    private JacksonTester<TvmToolConfig> tester;

    @Test
    void testDeserialize() throws IOException {
        TvmToolConfig tvmToolConfig = tester.readObject(new File("misc/tvm.prod.json"));
        assertTrue(tvmToolConfig.getClients().get("quasar-billing").getDsts().size() > 5);
    }
}
