package ru.yandex.quasar.billing.services.mediabilling;

import java.io.IOException;
import java.math.BigDecimal;
import java.time.Period;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.List;
import java.util.Set;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

@SpringJUnitConfig
@JsonTest
class SubscriptionsTest {

    @Autowired
    private JacksonTester<Subscriptions.Wrapper> tester;

    @Test
    void testDeserialize() throws IOException {
        Subscriptions expected = new Subscriptions(
                Set.of("basic-plus", "kp-amediateka", "basic-music"),
                List.of(
                        new Subscriptions.RenewableInfo(
                                "ru.yandex.plus.kinopoisk.amediateka.1month.autorenewable.native.web.1month.trial",
                                ZonedDateTime.of(2019, 2, 12, 14, 43, 20, 0, ZoneId.of("UTC")).toInstant(),
                                ZonedDateTime.of(2019, 2, 12, 14, 43, 20, 0, ZoneId.of("UTC")).toInstant(),
                                Period.ofMonths(1),
                                new Subscriptions.Price(BigDecimal.valueOf(649), "rub"),
                                false,
                                "yandex",
                                Set.of("basic-plus", "kp-amediateka", "basic-music"),
                                225,
                                false
                        )
                ),
                List.of(
                        new Subscriptions.NonAutoRenewableRemainder(
                                Set.of("basic-plus", "basic-music"),
                                12,
                                225
                        )
                ),
                null,
                null
        );

        tester.parse("{\n" +
                "    \"invocationInfo\": {\n" +
                "        \"hostname\": \"sas1-1569-sas-music-test-back-14646.gencfg-c.yandex.net\",\n" +
                "        \"action\": \"GET_CommonAccountActionContainer.getOrderInfo/billing/order-info\",\n" +
                "        \"app-name\": \"music-web\",\n" +
                "        \"app-version\": \"2019-02-21.stable-98.4483667 (exported; 2019-02-21 10:46)\",\n" +
                "        \"req-id\": \"d9ff582f8ac966913ba091126dc3c8eb\",\n" +
                "        \"exec-duration-millis\": \"10\"\n" +
                "    },\n" +
                "    \"result\": {\n" +
                "        \"availableFeatures\": [\n" +
                "            \"basic-plus\",\n" +
                "            \"kp-amediateka\",\n" +
                "            \"basic-music\"\n" +
                "        ],\n" +
                "        \"autoRenewable\": [\n" +
                "            {\n" +
                "                \"productId\": \"ru.yandex.plus.kinopoisk.amediateka.1month.autorenewable.native.web" +
                ".1month.trial\",\n" +
                "                \"nextCharge\": \"2019-02-12T14:43:20Z\",\n" +
                "                \"expires\": \"2019-02-12T14:43:20Z\",\n" +
                "                \"commonPeriodDuration\": \"P1M\",\n" +
                "                \"price\": {\n" +
                "                    \"amount\": \"649\",\n" +
                "                    \"currency\": \"rub\"\n" +
                "                },\n" +
                "                \"finished\": false,\n" +
                "                \"vendor\": \"yandex\",\n" +
                "                \"features\": [\n" +
                "                    \"kp-amediateka\",\n" +
                "                    \"basic-plus\",\n" +
                "                    \"basic-music\"\n" +
                "                ],\n" +
                "                \"region\": 225,\n" +
                "                \"debug\": false\n" +
                "            }\n" +
                "        ],\n" +
                "        \"info\": {},\n" +
                "        \"nonAutoRenewableRemainders\": [{\n" +
                "            \"features\": [\n" +
                "                \"basic-plus\",\n" +
                "                \"basic-music\"\n" +
                "            ],\n" +
                "            \"remainderDays\": 12,\n" +
                "            \"region\": 225\n" +
                "        }]\n" +
                "    }\n" +
                "}").assertThat().returns(expected, Subscriptions.Wrapper::getResult);
    }

    @Test
    void testDeserialize2() throws IOException {
        Subscriptions expected = new Subscriptions(
                Set.of("basic-plus", "basic-music"),
                null,
                null,
                List.of(
                        new Subscriptions.NonAutoRenewable(
                                "basic-plus",
                                ZonedDateTime.of(2018, 8, 28, 13, 4, 8, 0, ZoneId.of("UTC")).toInstant(),
                                ZonedDateTime.of(2019, 8, 31, 20, 59, 59, 0, ZoneId.of("UTC")).toInstant(),
                                225
                        )
                ),
                List.of(
                        new Subscriptions.TrialAvailability("basic-plus", true)
                )
        );

        tester.parse("{\n" +
                "  \"invocationInfo\": {\n" +
                "    \"hostname\": \"myt1-0178-d43-msk-myt-music-st-e72-18274.gencfg-c.yandex.net\",\n" +
                "    \"action\": \"GET_InternalAccountActionContainer.getSubscriptions/subscriptions\",\n" +
                "    \"app-name\": \"music-web\",\n" +
                "    \"app-version\": \"2019-02-27.stable-101.4507976 (exported; 2019-02-27 17:55)\",\n" +
                "    \"req-id\": \"7be7516f271f40b98eb169653386b9d0\",\n" +
                "    \"exec-duration-millis\": \"5\"\n" +
                "  },\n" +
                "  \"result\": {\n" +
                "    \"availableFeatures\": [\n" +
                "      \"basic-plus\",\n" +
                "      \"basic-music\"\n" +
                "    ],\n" +
                "    \"trialAvailability\": [\n" +
                "      {\n" +
                "        \"feature\": \"basic-plus\",\n" +
                "        \"available\": true\n" +
                "      }\n" +
                "    ],\n" +
                "    \"autoSubscriptionAvailable\": true,\n" +
                "    \"nonAutoRenewable\": [\n" +
                "      {\n" +
                "        \"feature\": \"basic-plus\",\n" +
                "        \"start\": \"2018-08-28T13:04:08+00:00\",\n" +
                "        \"end\": \"2019-08-31T20:59:59+00:00\",\n" +
                "        \"region\": 225\n" +
                "      }\n" +
                "    ]\n" +
                "  }\n" +
                "}").assertThat().returns(expected, Subscriptions.Wrapper::getResult);
    }

}
