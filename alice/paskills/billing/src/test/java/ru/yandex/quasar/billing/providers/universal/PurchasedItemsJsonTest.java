package ru.yandex.quasar.billing.providers.universal;

import java.io.IOException;
import java.util.Currency;
import java.util.stream.Stream;

import com.fasterxml.jackson.databind.exc.InvalidFormatException;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;

import static org.junit.jupiter.api.Assertions.assertAll;
import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

@JsonTest
class PurchasedItemsJsonTest {

    private static final String MDL_CURRENCY = "{\n" +
            "  \"purchased_products\": [\n" +
            "    {\n" +
            "      \"product_id\": \"sub:YA_PLUS\",\n" +
            "      \"title\": \"Яндекс.Плюс\",\n" +
            "      \"product_type\": \"subscription\",\n" +
            "      \"is_content_item\": false,\n" +
            "      \"price\": {\n" +
            "        \"price_option_id\": \"sub:YA_PLUS\",\n" +
            "        \"user_price\": 1690,\n" +
            "        \"price\": 1690,\n" +
            "        \"currency\": \"MDL\",\n" +
            "        \"purchase_payload\": {\n" +
            "          \"billing_product_id\": \"ru.yandex.mobile.music.1year.autorenewable.plus.169\"\n" +
            "        },\n" +
            "        \"title\": \"Яндекс.Плюс\",\n" +
            "        \"purchase_type\": \"SUBSCRIPTION\",\n" +
            "        \"period\": \"P1Y\",\n" +
            "        \"processing\": \"MEDIABILLING\"\n" +
            "      },\n" +
            "      \"active_till\": \"2019-06-29T13:43:29Z\",\n" +
            "      \"renew_disabled\": false\n" +
            "    }\n" +
            "  ]\n" +
            "}";

    private static final String UNKNOWN_CURRENCY = "{\n" +
            "  \"purchased_products\": [\n" +
            "    {\n" +
            "      \"product_id\": \"sub:YA_PLUS\",\n" +
            "      \"title\": \"Яндекс.Плюс\",\n" +
            "      \"product_type\": \"subscription\",\n" +
            "      \"is_content_item\": false,\n" +
            "      \"price\": {\n" +
            "        \"price_option_id\": \"sub:YA_PLUS\",\n" +
            "        \"user_price\": 1690,\n" +
            "        \"price\": 1690,\n" +
            "        \"currency\": \"MDL1\",\n" +
            "        \"purchase_payload\": {\n" +
            "          \"billing_product_id\": \"ru.yandex.mobile.music.1year.autorenewable.plus.169\"\n" +
            "        },\n" +
            "        \"title\": \"Яндекс.Плюс\",\n" +
            "        \"purchase_type\": \"SUBSCRIPTION\",\n" +
            "        \"period\": \"P1Y\",\n" +
            "        \"processing\": \"MEDIABILLING\"\n" +
            "      },\n" +
            "      \"active_till\": \"2019-06-29T13:43:29Z\",\n" +
            "      \"renew_disabled\": false\n" +
            "    }\n" +
            "  ]\n" +
            "}";

    @Autowired
    private JacksonTester<PurchasedItems> tester;

    @Test
    void testDeserializeCurrency() throws IOException {
        var actual = tester.parse(MDL_CURRENCY).getObject();
        assertEquals(Currency.getInstance("MDL"), actual.getPurchasedProducts().get(0).getPrice().getCurrency());
    }

    @Test
    void testDeserializeUnknownCurrency() {
        assertThrows(InvalidFormatException.class, () -> tester.parse(UNKNOWN_CURRENCY).getObject());
    }

    @Test
    void testCurrencyCodeDeserialization() {
        assertAll(
                Stream.of(
                        "RUB",
                        "BYN",
                        "KZT",
                        "USD",
                        "EUR",
                        "CHF",
                        "UAH",
                        "AMD",
                        "GEL",
                        "TRY",
                        "UZS",
                        "MDL"
                ).map(c -> (() -> assertDoesNotThrow(() -> Currency.getInstance(c))))
        );

    }

}
