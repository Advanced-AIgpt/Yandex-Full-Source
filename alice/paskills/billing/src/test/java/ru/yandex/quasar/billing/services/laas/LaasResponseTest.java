package ru.yandex.quasar.billing.services.laas;

import java.io.IOException;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;

@JsonTest
class LaasResponseTest {

    @Autowired
    private JacksonTester<LaasResponse> tester;

    @Test
    void testDeserialize() throws IOException {
        tester.parse("{\n" +
                "  \"region_id\":2,\n" +
                "  \"precision\":2,\n" +
                "  \"latitude\":59.939095,\n" +
                "  \"longitude\":30.315868,\n" +
                "  \"should_update_cookie\":false,\n" +
                "  \"is_user_choice\":true,\n" +
                "  \"suspected_region_id\":213,\n" +
                "  \"city_id\":213,\n" +
                "  \"region_by_ip\":213,\n" +
                "  \"suspected_region_city\":213,\n" +
                "  \"location_accuracy\":15000,\n" +
                "  \"location_unixtime\":1500477209,\n" +
                "  \"suspected_latitude\":55.753960,\n" +
                "  \"suspected_longitude\":37.620393,\n" +
                "  \"suspected_location_accuracy\":15000,\n" +
                "  \"suspected_location_unixtime\":1500477209,\n" +
                "  \"suspected_precision\":2,\n" +
                "  \"probable_regions_reliability\":1.70,\n" +
                "  \"probable_regions\":\n" +
                "    [\n" +
                "      {\n" +
                "        \"region_id\":213,\n" +
                "        \"weight\":0.160000\n" +
                "      },\n" +
                "      {\n" +
                "        \"region_id\":2,\n" +
                "        \"weight\":0.720000\n" +
                "      }\n" +
                "    ],\n" +
                "  \"country_id_by_ip\":225,\n" +
                "  \"is_anonymous_vpn\":false,\n" +
                "  \"is_public_proxy\":false,\n" +
                "  \"is_serp_trusted_net\":true,\n" +
                "  \"is_tor\":false,\n" +
                "  \"is_hosting\":false,\n" +
                "  \"is_gdpr\":false,\n" +
                "  \"is_mobile\":false,\n" +
                "  \"is_yandex_net\":false,\n" +
                "  \"is_yandex_staff\":false\n" +
                "}").assertThat()
                .isEqualTo(LaasResponse.builder()
                        .countryId(225)
                        .isAnonymousVpn(false)
                        .isUserChoice(true)
                        .regionByIp(213)
                        .regionId(2)
                        .isPublicProxy(false)
                        .build());
    }
}
