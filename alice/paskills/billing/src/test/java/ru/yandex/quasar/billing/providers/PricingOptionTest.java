package ru.yandex.quasar.billing.providers;

import java.io.IOException;
import java.math.BigDecimal;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import ru.yandex.quasar.billing.beans.ContentQuality;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.LogicalPeriod;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;

import static org.assertj.core.api.Assertions.assertThat;

@ExtendWith(SpringExtension.class)
@JsonTest
class PricingOptionTest {
    private static final String REFERENCE_JSON = "{\n" +
            "                \"userPrice\": 99,\n" +
            "                \"price\": 99,\n" +
            "                \"currency\": \"RUB\",\n" +
            "                \"providerPayload\": \"{\\\"foo\\\":\\\"val\\\"}\",\n" +
            "                \"title\": \"Аренда - 99 руб.\",\n" +
            "                \"type\": \"RENT\",\n" +
            "                \"quality\": \"SD\",\n" +
            "                \"provider\": \"ivi\",\n" +
            "                \"subscriptionPeriodDays\": null,\n" +
            "                \"specialCommission\": false,\n" +
            "                \"purchasingItem\": {\n" +
            "                    \"contentType\": \"film\",\n" +
            "                    \"id\": \"155893\"\n" +
            "                }\n" +
            "            }";
    private static final PricingOption REFERENCE_OBJECT = PricingOption.builder("Аренда - 99 руб.",
            PricingOptionType.RENT, new BigDecimal(99),
            new BigDecimal(99),
            "RUB")
            .providerPayload("{\"foo\":\"val\"}")
            .quality(ContentQuality.SD)
            .provider("ivi")
            .specialCommission(false)
            .purchasingItem(ProviderContentItem.create(ContentType.MOVIE, "155893"))
            .build();

    @Autowired
    private JacksonTester<PricingOption> tester;

    @Test
    void setDeserialize() throws IOException {

        assertThat(tester.parse(REFERENCE_JSON))
                .isEqualTo(REFERENCE_OBJECT);
    }

    @Test
    void testSerialize() throws IOException {
        assertThat(tester.write(REFERENCE_OBJECT))
                .isEqualToJson(REFERENCE_JSON);
    }

    @Test
    void setDeserializeSubscriptionPeriodDays() throws IOException {

        String serialized = "{\n" +
                "    \"userPrice\": 99,\n" +
                "    \"price\": 99,\n" +
                "    \"currency\": \"RUB\",\n" +
                "    \"providerPayload\": \"{\\\"foo\\\":\\\"val\\\"}\",\n" +
                "    \"title\": \"Аренда - 99 руб.\",\n" +
                "    \"type\": \"SUBSCRIPTION\",\n" +
                "    \"quality\": \"SD\",\n" +
                "    \"provider\": \"ivi\",\n" +
                "    \"subscriptionPeriodDays\": 30,\n" +
                "    \"subscriptionPeriod\": \"P30D\",\n" +
                "    \"specialCommission\": false,\n" +
                "    \"purchasingItem\": {\n" +
                "        \"contentType\": \"film\",\n" +
                "        \"id\": \"155893\"\n" +
                "    }\n" +
                "}";
        PricingOption expected = PricingOption.builder("Аренда - 99 руб.",
                PricingOptionType.SUBSCRIPTION,
                new BigDecimal(99),
                new BigDecimal(99),
                "RUB")
                .providerPayload("{\"foo\":\"val\"}")
                .quality(ContentQuality.SD)
                .provider("ivi")
                .specialCommission(false)
                .subscriptionPeriod(LogicalPeriod.ofDays(30))
                .purchasingItem(ProviderContentItem.create(ContentType.MOVIE, "155893"))
                .build();

        assertThat(tester.parse(serialized))
                .isEqualTo(expected);

        assertThat(tester.write(expected))
                .isEqualToJson(serialized);
    }

    @Test
    void setDeserializeSubscriptionPeriodAndTrial() throws IOException {

        String serialized = "{\n" +
                "   \"userPrice\": 99,\n" +
                "   \"price\": 99,\n" +
                "   \"currency\": \"RUB\",\n" +
                "   \"providerPayload\": \"{\\\"foo\\\":\\\"val\\\"}\",\n" +
                "   \"title\": \"Аренда - 99 руб.\",\n" +
                "   \"type\": \"SUBSCRIPTION\",\n" +
                "   \"quality\": \"SD\",\n" +
                "   \"provider\": \"ivi\",\n" +
                "   \"subscriptionPeriodDays\": 30,\n" +
                "   \"subscriptionPeriod\": \"P30D\",\n" +
                "   \"subscriptionTrialPeriod\": \"P1M\",\n" +
                "   \"specialCommission\": false,\n" +
                "   \"purchasingItem\": {\n" +
                "       \"contentType\": \"film\",\n" +
                "       \"id\": \"155893\"\n" +
                "   }\n" +
                "}";
        PricingOption expected = PricingOption.builder("Аренда - 99 руб.",
                PricingOptionType.SUBSCRIPTION,
                new BigDecimal(99),
                new BigDecimal(99),
                "RUB")
                .providerPayload("{\"foo\":\"val\"}")
                .quality(ContentQuality.SD)
                .provider("ivi")
                .specialCommission(false)
                .subscriptionPeriod(LogicalPeriod.ofDays(30))
                .subscriptionTrialPeriod(LogicalPeriod.ofMonths(1))
                .purchasingItem(ProviderContentItem.create(ContentType.MOVIE, "155893"))
                .build();

        assertThat(tester.parse(serialized))
                .isEqualTo(expected);

        assertThat(tester.write(expected))
                .isEqualToJson(serialized);
    }

}
