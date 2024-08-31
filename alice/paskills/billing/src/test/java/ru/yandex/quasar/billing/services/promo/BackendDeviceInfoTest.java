package ru.yandex.quasar.billing.services.promo;

import java.io.IOException;
import java.time.Instant;
import java.time.format.DateTimeFormatter;
import java.util.Set;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static ru.yandex.quasar.billing.beans.PromoType.plus180;
import static ru.yandex.quasar.billing.beans.PromoType.plus90;

@SpringJUnitConfig(classes = {ObjectMapper.class})
class BackendDeviceInfoTest {

    @Autowired
    private ObjectMapper mapper;

    @Test
    void testDeserialize() throws IOException {
        System.out.println(Instant.now().toString());
        BackendDeviceInfo backendDeviceInfo = mapper.readValue("{\n" +
                "  \"activation_code\": 2208439203,\n" +
                "  \"name\": \"Сердитая выдра\",\n" +
                "  \"id\": \"74005034440c081d050e\",\n" +
                "  \"platform\": \"yandexstation\",\n" +
                "  \"first_activation_date\": \"2021-08-12T15:12:17Z\",\n" +
                "  \"tags\": [\"plus90\", \"plus180\"]\n" +
                "}", BackendDeviceInfo.class);
        assertEquals(new BackendDeviceInfo("74005034440c081d050e", Platform.YANDEXSTATION,
                Set.of(plus90.name(), plus180.name()), "Сердитая выдра", null,
                Instant.from(DateTimeFormatter.ISO_INSTANT.parse("2021-08-12T15:12:17Z"))), backendDeviceInfo);
    }

    @Test
    void testDeserializeBadActivationDate() throws IOException {
        System.out.println(Instant.now().toString());
        BackendDeviceInfo backendDeviceInfo = mapper.readValue("{\n" +
                "  \"activation_code\": 2208439203,\n" +
                "  \"name\": \"Сердитая выдра\",\n" +
                "  \"id\": \"74005034440c081d050e\",\n" +
                "  \"platform\": \"yandexstation\",\n" +
                "  \"first_activation_date\": \"\",\n" +
                "  \"tags\": [\"plus90\", \"plus180\"]\n" +
                "}", BackendDeviceInfo.class);
        assertEquals(new BackendDeviceInfo("74005034440c081d050e", Platform.YANDEXSTATION,
                Set.of(plus90.name(), plus180.name()), "Сердитая выдра", null,
                null), backendDeviceInfo);
    }
}
