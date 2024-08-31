package ru.yandex.quasar.billing.services.promo;

import java.io.IOException;
import java.time.Instant;
import java.time.format.DateTimeFormatter;
import java.util.List;
import java.util.Set;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import static ru.yandex.quasar.billing.services.promo.BackendDeviceInfo.Region.RU;

@ExtendWith(SpringExtension.class)
@JsonTest
class DeviceListWrapperTest {

    @Autowired
    private JacksonTester<DeviceListWrapper> tester;

    @Test
    void testDeserialize() throws IOException {
        Instant now = Instant.now();
        DeviceListWrapper wrapper = new DeviceListWrapper(List.of(new BackendDeviceInfo("74005034440c0423078e",
                Platform.YANDEXSTATION, Set.of(), "Понтий", null,
                Instant.from(DateTimeFormatter.ISO_INSTANT.parse("2021-08-12T15:12:17.123412Z")))), "ok");
        tester.parse(
                "{\n" +
                        "  \"devices\": [\n" +
                        "    {\n" +
                        "      \"activation_region\": null,\n" +
                        "      \"first_activation_date\": \"2021-08-12T15:12:17.123412Z\",\n" +
                        "      \"id\": \"74005034440c0423078e\",\n" +
                        "      \"name\": \"Понтий\",\n" +
                        "      \"platform\": \"yandexstation\",\n" +
                        "      \"tags\": []\n" +
                        "    }\n" +
                        "  ],\n" +
                        "  \"status\": \"ok\"\n" +
                        "}").assertThat().isEqualTo(wrapper);
    }

    @Test
    void testDeserializeWk7y() throws IOException {
        Instant now = Instant.now();
        DeviceListWrapper wrapper = new DeviceListWrapper(List.of(new BackendDeviceInfo("74005034440c0423078e",
                Platform.WK7Y, Set.of(), "Понтий", null,
                Instant.from(DateTimeFormatter.ISO_INSTANT.parse("2021-08-12T15:12:17.123412Z")))), "ok");
        tester.parse(
                "{\n" +
                        "  \"devices\": [\n" +
                        "    {\n" +
                        "      \"activation_region\": null,\n" +
                        "      \"first_activation_date\": \"2021-08-12T15:12:17.123412Z\",\n" +
                        "      \"id\": \"74005034440c0423078e\",\n" +
                        "      \"name\": \"Понтий\",\n" +
                        "      \"platform\": \"wk7y\",\n" +
                        "      \"tags\": []\n" +
                        "    }\n" +
                        "  ],\n" +
                        "  \"status\": \"ok\"\n" +
                        "}").assertThat().isEqualTo(wrapper);
    }

    @Test
    void testDeserializeWithRegion() throws IOException {
        DeviceListWrapper wrapper = new DeviceListWrapper(List.of(new BackendDeviceInfo("74005034440c0423078e",
                Platform.YANDEXSTATION, Set.of(), "Понтий", RU,
                Instant.from(DateTimeFormatter.ISO_INSTANT.parse("2021-08-12T15:12:17.123412Z")))), "ok");
        tester.parse(
                "{\n" +
                        "  \"devices\": [\n" +
                        "    {\n" +
                        "      \"activation_region\": \"RU\",\n" +
                        "      \"first_activation_date\": \"2021-08-12T15:12:17.123412Z\",\n" +
                        "      \"id\": \"74005034440c0423078e\",\n" +
                        "      \"name\": \"Понтий\",\n" +
                        "      \"platform\": \"yandexstation\",\n" +
                        "      \"tags\": []\n" +
                        "    }\n" +
                        "  ],\n" +
                        "  \"status\": \"ok\"\n" +
                        "}").assertThat().isEqualTo(wrapper);
    }
}
