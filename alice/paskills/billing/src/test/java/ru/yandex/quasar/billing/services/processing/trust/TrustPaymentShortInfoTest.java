package ru.yandex.quasar.billing.services.processing.trust;

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
class TrustPaymentShortInfoTest {

    private static final String EXAMPLE = "{\n" +
            "    \"uid\": \"730144893\",\n" +
            "    \"payment_method\": \"trial_payment\",\n" +
            "    \"payment_status\": \"authorized\",\n" +
            "    \"currency\": \"RUB\",\n" +
            "    \"orders\": [\n" +
            "        {\n" +
            "            \"subs_until_ts\": \"1539013266.655\",\n" +
            "            \"subs_period\": \"30D\",\n" +
            "            \"uid\": \"730144893\",\n" +
            "            \"subs_period_count\": 1,\n" +
            "            \"current_amount\": [\n" +
            "                [\n" +
            "                    \"RUB\",\n" +
            "                    \"0.00\"\n" +
            "                ]\n" +
            "            ],\n" +
            "            \"current_qty\": \"1.00\",\n" +
            "            \"order_id\": \"235\",\n" +
            "            \"next_charge_payment_method\": \"card-x5ab25f78910d393c8b05f5df\",\n" +
            "            \"has_trial_period\": 1,\n" +
            "            \"paid_amount\": \"0.00\",\n" +
            "            \"order_ts\": \"1537803661.504\",\n" +
            "            \"begin_ts\": \"1537803666.655\",\n" +
            "            \"commission_category\": \"1500\",\n" +
            "            \"payments\": [\n" +
            "                \"7e787e4ae3a3cf3ba6bb3e001434b9f9\"\n" +
            "            ],\n" +
            "            \"orig_amount\": \"0.00\",\n" +
            "            \"subs_state\": 1,\n" +
            "            \"product_type\": \"subs\",\n" +
            "            \"product_name\": \"amediateka subscription 14_30\",\n" +
            "            \"ready_to_pay\": 1,\n" +
            "            \"subs_trial_period\": \"14D\",\n" +
            "            \"product_id\": \"1018\"\n" +
            "        }\n" +
            "    ],\n" +
            "    \"user_account\": \"510000****1573\",\n" +
            "    \"paysys_sent_ts\": \"1537803666.253\",\n" +
            "    \"basket_rows\": [\n" +
            "        {\n" +
            "            \"order_id\": \"235\",\n" +
            "            \"amount\": \"0.00\",\n" +
            "            \"quantity\": \"1.00\"\n" +
            "        }\n" +
            "    ],\n" +
            "    \"current_amount\": \"0.00\",\n" +
            "    \"status\": \"success\",\n" +
            "    \"orig_amount\": \"0.00\",\n" +
            "    \"start_ts\": \"1537803664.757\",\n" +
            "    \"update_ts\": \"1537803661.648\",\n" +
            "    \"purchase_token\": \"7e787e4ae3a3cf3ba6bb3e001434b9f9\",\n" +
            "    \"paymethod_id\": \"trial_payment\",\n" +
            "    \"final_status_ts\": \"1537803666.000\",\n" +
            "    \"payment_ts\": \"1537803666.145\",\n" + //2018-09-24T15:41:06.145Z
            "    \"binding_result\": \"success\",\n" +
            "    \"card_type\": \"MasterCard\",\n" +
            "    \"amount\": \"0.00\",\n" +
            "    \"payment_timeout\": \"1200.000\",\n" +
            "    \"user_email\": \"yndx.quasar.test.12@yandex.ru\"\n" +
            "}";
    @Autowired
    private JacksonTester<TrustPaymentShortInfo> tester;

    @Test
    void testDeserialize() throws IOException {

        BigDecimal amount = new BigDecimal("0.00");
        tester.parse(EXAMPLE)
                .assertThat().isEqualTo(new TrustPaymentShortInfo(
                null,
                null,
                "authorized",
                "510000****1573",
                "MasterCard",

                amount,
                TrustCurrency.RUB,
                ZonedDateTime.of(2018, 9, 24, 15, 41, 6, 145_000_000, ZoneId.of("UTC")).toInstant(),
                "trial_payment",
                null,
                List.of(new TrustPaymentShortInfo.TrustOrderInfo("235", amount)))

        );
    }

    @Test
    void testSerialize() throws IOException {
        BigDecimal amount = new BigDecimal("0.00");
        TrustPaymentShortInfo obj = new TrustPaymentShortInfo(
                null,
                null,
                "authorized",
                "510000****1573",
                "MasterCard",
                amount,
                TrustCurrency.RUB,
                ZonedDateTime.of(2018, 9, 24, 15, 41, 6, 145_000_000, ZoneId.of("UTC")).toInstant(),
                "card-x5ab25f78910d393c8b05f5df",
                null,
                List.of(new TrustPaymentShortInfo.TrustOrderInfo("235", amount)));
        assertThat(tester.write(obj))
                .isEqualToJson("{\n" +
                        "    \"payment_status\": \"authorized\",\n" +
                        "    \"user_account\": \"510000****1573\",\n" +
                        "    \"payment_ts\": \"1537803666.145\",\n" + //2018-09-24T15:41:06.145Z
                        "    \"card_type\": \"MasterCard\",\n" +
                        "    \"currency\": \"RUB\",\n" +
                        "    \"paymethod_id\": \"card-x5ab25f78910d393c8b05f5df\",\n" +
                        "    \"amount\": \"0.00\",\n" +
                        "    \"orders\": [\n" +
                        "        {\n" +
                        "            \"order_id\": \"235\",\n" +
                        "            \"orig_amount\": \"0.00\"\n" +
                        "        }\n" +
                        "    ]\n" +
                        "}");

    }
}
