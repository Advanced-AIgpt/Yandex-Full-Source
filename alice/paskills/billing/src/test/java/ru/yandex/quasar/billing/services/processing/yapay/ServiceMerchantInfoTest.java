package ru.yandex.quasar.billing.services.processing.yapay;

import java.io.IOException;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;

@JsonTest
class ServiceMerchantInfoTest {
    @Autowired
    private JacksonTester<ResponseWrapper<ServiceMerchantInfo>> tester;

    @Test
    void testDeserialization() throws IOException {
        var expected = ServiceMerchantInfo.builder()
                .serviceMerchantId(289L)
                .deleted(false)
                .enabled(false)
                .entityId("1234")
                .description("Test")
                .organization(Organization.builder()
                        .inn("5043041353")
                        .name("Yandex")
                        .englishName("HH")
                        .ogrn("1234567890")
                        .type("OOO")
                        .kpp("504301001")
                        .fullName("Hoofs & Horns")
                        .siteUrl("pay.yandex.ru")
                        .scheduleText("с 9 до 6")
                        .build())
                .build();

        tester.parse("{\n" +
                "  \"code\": 200,\n" +
                "  \"data\": {\n" +
                "    \"revision\": 1,\n" +
                "    \"uid\": 756727506,\n" +
                "    \"updated\": \"2019-06-06T07:15:15.904254+00:00\",\n" +
                "    \"entity_id\": \"1234\",\n" +
                "    \"deleted\": false,\n" +
                "    \"organization\": {\n" +
                "      \"inn\": \"5043041353\",\n" +
                "      \"name\": \"Yandex\",\n" +
                "      \"englishName\": \"HH\",\n" +
                "      \"ogrn\": \"1234567890\",\n" +
                "      \"type\": \"OOO\",\n" +
                "      \"kpp\": \"504301001\",\n" +
                "      \"fullName\": \"Hoofs & Horns\",\n" +
                "      \"siteUrl\": \"pay.yandex.ru\",\n" +
                "      \"description\": null,\n" +
                "      \"scheduleText\": \"с 9 до 6\"\n" +
                "    },\n" +
                "    \"enabled\": false,\n" +
                "    \"service_merchant_id\": 289,\n" +
                "    \"service_id\": 10,\n" +
                "    \"created\": \"2019-06-06T07:15:15.904254+00:00\",\n" +
                "    \"description\": \"Test\"\n" +
                "  },\n" +
                "  \"status\": \"success\"\n" +
                "}").assertThat()
                .extracting(ResponseWrapper::getData)
                .isEqualTo(expected);
    }
}
