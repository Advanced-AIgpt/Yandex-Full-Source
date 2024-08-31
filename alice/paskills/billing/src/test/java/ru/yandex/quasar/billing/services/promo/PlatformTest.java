package ru.yandex.quasar.billing.services.promo;

import java.io.IOException;

import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.Data;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import static org.junit.jupiter.api.Assertions.assertEquals;

@SpringJUnitConfig(classes = {ObjectMapper.class})
class PlatformTest {

    @Autowired
    private ObjectMapper mapper;

    @Test
    void testDeserializeUnknownPlatform() throws IOException {

        DeserializeSample backendDeviceInfo = mapper.readValue("{\n" +
                "  \"platform\": \"yandexkofemolka\"\n" +
                "}", DeserializeSample.class);

        assertEquals("yandexkofemolka", backendDeviceInfo.getPlatform().toString());

    }

    @Data
    static class DeserializeSample {
        private Platform platform;
    }
}
