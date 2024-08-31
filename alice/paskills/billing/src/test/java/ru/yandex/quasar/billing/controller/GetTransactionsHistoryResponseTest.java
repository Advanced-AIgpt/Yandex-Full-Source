package ru.yandex.quasar.billing.controller;

import java.io.IOException;
import java.math.BigDecimal;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.List;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import ru.yandex.quasar.billing.services.processing.TrustCurrency;

import static org.assertj.core.api.Assertions.assertThat;


@ExtendWith(SpringExtension.class)
@JsonTest
public class GetTransactionsHistoryResponseTest {

    @Autowired
    private JacksonTester<GetTransactionsHistoryResponse> tester;

    @Test
    public void testSerialize() throws IOException {
        GetTransactionsHistoryResponse response = new GetTransactionsHistoryResponse(
                List.of(new GetTransactionsHistoryResponse.Item("provider_name",
                                "itemtitle",
                                "510000****1573",
                                "MasterCard",
                                ZonedDateTime.of(2018, 9, 24, 15, 32, 50, 674000000, ZoneId.of("UTC")).toInstant(),
                                BigDecimal.TEN,
                                TrustCurrency.RUB.getCurrencyCode()
                        )
                )
        );

        System.out.println(tester.write(response));
        assertThat(tester.write(response))
                .isEqualToJson("{ \"items\": [{\n" +
                        "\"provider\":\"provider_name\",\n" +
                        "\"title\":\"itemtitle\",\n" +
                        "\"maskedCardNumber\":\"510000****1573\",\n" +
                        "\"paymentSystem\":\"MasterCard\",\n" +
                        "\"purchaseDate\":\"2018-09-24T15:32:50.674Z\",\n" +
                        "\"amount\":10\n," +
                        "\"currency\":\"RUB\"\n" +
                        "}\n" +
                        "]}");
    }
}
