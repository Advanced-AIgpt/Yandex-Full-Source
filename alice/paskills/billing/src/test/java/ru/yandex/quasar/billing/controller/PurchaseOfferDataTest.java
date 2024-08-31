package ru.yandex.quasar.billing.controller;

import java.io.IOException;
import java.math.BigDecimal;
import java.util.List;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import ru.yandex.quasar.billing.dao.PurchaseOfferStatus;

import static org.assertj.core.api.Assertions.assertThat;

@JsonTest
@SpringJUnitConfig
class PurchaseOfferDataTest {
    private final String expected = "{" +
            "  \"purchaseOfferUuid\":\"7ddbf471-e6d6-4bd6-b18a-961d4d8ba8ad\"," +
            "  \"purchaseRequestId\":\"33512cf9-46c4-460d-b1e1-25aadcaa3c26\"," +
            "  \"skillUuid\":\"2ca8abdc-0628-4569-bb27-9158a2f31981\"," +
            "  \"name\":\"Квазар биллинг тест\"," +
            "  \"imageUrl\":\"https://avatars.mdst.yandex.net/get-dialogs/3757/2e73aae0be8ff038e43e/mobile-logo-x1\"," +
            "  \"selectedOptionUuid\":\"33512cf9-46c4-460d-b1e1-25aadcaa3c26\"," +
            "  \"merchantInfo\":{" +
            "    \"name\":\"ООО \\\"ЭДМОД\\\"\"," +
            "    \"address\":\"620144, г Екатеринбург, ул 8 Марта, д 70, оф 233\"," +
            "    \"ogrn\":\"1116671019398\"," +
            "    \"inn\":\"6671383440\"," +
            "    \"workingHours\":\"9:00-18:00\"," +
            "    \"description\":\"Розовый слон в мишуре с герляндой\"" +
            "  }," +
            "  \"currency\":\"RUB\"," +
            "  \"totalPrice\":301700.0," +
            "  \"totalUserPrice\":997," +
            "  \"status\":\"NEW\"," +
            "  \"products\":[" +
            "    {" +
            "      \"productId\":\"690fe12b-805c-4698-bda9-c217896b3089\"," +
            "      \"quantity\":1.0," +
            "      \"title\":\"Большой слон\"," +
            "      \"userPrice\":199.0," +
            "      \"price\":100500.0" +
            "    },{" +
            "      \"productId\":\"0d1cd477-37d3-49d4-a131-7ebb575ff298\"," +
            "      \"quantity\":2.0," +
            "      \"title\":\"Громадный слон\"," +
            "      \"userPrice\":299.0," +
            "      \"price\":100500.0" +
            "    }" +
            "  ]," +
            "  \"deliveryInfo\":{" +
            "    \"productId\":\"4d4d71c7-fc11-4fcd-9eb7-6fc454bbb312\"," +
            "    \"index\":\"123123\"," +
            "    \"city\":\"Москва\"," +
            "    \"settlement\":\"Зелиноград\"," +
            "    \"street\": \"Ул. Льва Толстого\"," +
            "    \"house\": \"16\"," +
            "    \"housing\": \"5\"," +
            "    \"building\": \"7\"," +
            "    \"porch\": \"3\"," +
            "    \"floor\": \"5\"," +
            "    \"flat\": \"71\"," +
            "    \"price\": 200.0" +
            "  }" +
            "}";
    @Autowired

    private JacksonTester<PurchaseOfferData> tester;

    @Test
    void testSerialize() throws IOException {
        PurchaseOfferData purchaseOfferData = PurchaseOfferData.builder("7ddbf471-e6d6-4bd6-b18a-961d4d8ba8ad",
                "33512cf9-46c4-460d-b1e1-25aadcaa3c26", "33512cf9-46c4-460d-b1e1-25aadcaa3c26")
                .skillUuid("2ca8abdc-0628-4569-bb27-9158a2f31981")
                .name("Квазар биллинг тест")
                .imageUrl("https://avatars.mdst.yandex.net/get-dialogs/3757/2e73aae0be8ff038e43e/mobile-logo-x1")
                .merchantInfo(PurchaseOfferData.MerchantInfo.builder()
                        .name("ООО \"ЭДМОД\"")
                        .address("620144, г Екатеринбург, ул 8 Марта, д 70, оф 233")
                        .ogrn("1116671019398")
                        .inn("6671383440")
                        .workingHours("9:00-18:00")
                        .description("Розовый слон в мишуре с герляндой")
                        .build())
                .currency("RUB")
                .status(PurchaseOfferStatus.NEW)
                .products(List.of(
                        PurchaseOfferData.PurchaseOfferProductData.builder()
                                .productId("690fe12b-805c-4698-bda9-c217896b3089")
                                .quantity(new BigDecimal("1.0"))
                                .title("Большой слон")
                                .userPrice(new BigDecimal("199.0"))
                                .price(new BigDecimal("100500.0"))
                                .build(),
                        PurchaseOfferData.PurchaseOfferProductData.builder()
                                .productId("0d1cd477-37d3-49d4-a131-7ebb575ff298")
                                .quantity(new BigDecimal("2.0"))
                                .title("Громадный слон")
                                .userPrice(new BigDecimal("299.0"))
                                .price(new BigDecimal("100500.0"))
                                .build()
                ))
                .deliveryInfo(PurchaseOfferData.PurchaseDeliveryInfo.builder()
                        .productId("4d4d71c7-fc11-4fcd-9eb7-6fc454bbb312")
                        .index("123123")
                        .city("Москва")
                        .settlement("Зелиноград")
                        .street("Ул. Льва Толстого")
                        .house("16")
                        .housing("5")
                        .building("7")
                        .porch("3")
                        .floor("5")
                        .flat("71")
                        .price(new BigDecimal("200.0"))
                        .build())
                .build();

        assertThat(tester.write(purchaseOfferData)).isEqualToJson(expected);
    }
}
