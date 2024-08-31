package ru.yandex.quasar.billing.controller;

import java.io.IOException;
import java.math.BigDecimal;
import java.util.List;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.fasterxml.jackson.databind.node.TextNode;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.quasar.billing.services.processing.NdsType.nds_20;

@ExtendWith(SpringExtension.class)
@JsonTest
class OfferSkillPurchaseRequestTest {

    private static final String SAMPLE = "{\n" +
            "  \"skill\": {\n" +
            "    \"id\": \"3ad36498-f5rd-4079-a14b-788652932056\",\n" +
            "    \"callback_url\": \"https://url_to_skill/\",\n" +
            "    \"name\": \"skill name\",\n" +
            "    \"image_url\": \"img url\"\n" +
            "  },\n" +
            "  \"user_id\": \"userid\",\n" +
            "  \"session_id\": \"session\",\n" +
            "  \"device_id\": \"testdevice\",\n" +
            "  \"purchase_request\": {\n" +
            "    \"purchase_request_id\": \"d432de19be8347d09f656d9fe966e0f1\",\n" +
            "    \"image_url\": \"http://url_to_image\",\n" +
            "    \"caption\": \"Заказ 111\",\n" +
            "    \"description\": \"Описание букета\",\n" +
            "    \"type\": \"BUY\",\n" +
            "    \"currency\": \"RUB\",\n" +
            "    \"payload\": {\"a\":\"val\"},\n" +
            "    \"merchant_key\": \"298aea40-d5a1-4fc4-437c-7a27650c9c5f\",\n" +
            "    \"delivery\": {\n" +
            "      \"city\": \"Москва\",\n" +
            "      \"settlement\": \"г. Москва\",\n" +
            "      \"index\": \"125677\",\n" +
            "      \"street\": \"ул. Льва толстого\",\n" +
            "      \"house\": \"16\",\n" +
            "      \"building\": \"2\",\n" +
            "      \"housing\": \"5\",\n" +
            "      \"porch\": \"78\",\n" +
            "      \"floor\": \"14\",\n" +
            "      \"flat\": \"67\",\n" +
            "      \"price\": 250,\n" +
            "      \"nds_type\": \"nds_20\"\n" +
            "    },\n" +
            "    \"products\": [\n" +
            "      {\n" +
            "        \"product_id\": \"8674b49eed8f4de28c8cebfe411930d7\",\n" +
            "        \"title\": \"Букет из 15 Роз\",\n" +
            "        \"user_price\": \"199.00\",\n" +
            "        \"price\": \"299.00\",\n" +
            "        \"nds_type\": \"nds_20\",\n" +
            "        \"quantity\": \"1\"\n" +
            "      }\n" +
            "    ]\n" +
            "  }\n" +
            // "  \"version\": \"1.1\"\n" +
            "}";

    private final OfferSkillPurchaseRequest expected;

    private final ObjectMapper objectMapper = new ObjectMapper();

    @Autowired
    private JacksonTester<OfferSkillPurchaseRequest> tester;

    OfferSkillPurchaseRequestTest() {
        expected = new OfferSkillPurchaseRequest(
                new OfferSkillPurchaseRequest.SkillInfo(
                        "3ad36498-f5rd-4079-a14b-788652932056",
                        "skill name",
                        "img url",
                        "https://url_to_skill/"
                ),
                "session",
                "userid",
                "testdevice",
                new OfferSkillPurchaseRequest.PurchaseRequest(
                        "d432de19be8347d09f656d9fe966e0f1",
                        "http://url_to_image",
                        "Заказ 111",
                        "Описание букета",
                        TrustCurrency.RUB,
                        PricingOptionType.BUY,
                        (ObjectNode) objectMapper.createObjectNode().set("a", new TextNode("val")),
                        "298aea40-d5a1-4fc4-437c-7a27650c9c5f",
                        List.of(new OfferSkillPurchaseRequest.PurchaseRequestProduct(
                                        "8674b49eed8f4de28c8cebfe411930d7",
                                        "Букет из 15 Роз",
                                        new BigDecimal("199.00"),
                                        new BigDecimal("299.00"),
                                        BigDecimal.ONE,
                                        nds_20
                        )),
                        false,
                        OfferSkillPurchaseRequest.PurchaseDeliveryInfo.builder()
                                .city("Москва")
                                .settlement("г. Москва")
                                .index("125677")
                                .street("ул. Льва толстого")
                                .house("16")
                                .housing("5")
                                .building("2")
                                .porch("78")
                                .floor("14")
                                .flat("67")
                                .price(new BigDecimal("250"))
                                .ndsType(nds_20)
                                .build()
                ),
                null
        );
    }

    @Test
    void testDeserialization() throws IOException {
        tester.parse(SAMPLE)
                .assertThat()
                .isEqualTo(expected);
    }

    @Test
    void testSerialization() throws IOException {
        assertThat(tester.write(expected)).isEqualToJson(SAMPLE);
    }
}
