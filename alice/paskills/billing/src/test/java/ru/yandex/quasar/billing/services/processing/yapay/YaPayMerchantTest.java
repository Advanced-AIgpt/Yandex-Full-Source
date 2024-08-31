package ru.yandex.quasar.billing.services.processing.yapay;

import java.io.IOException;
import java.math.BigInteger;
import java.time.LocalDate;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.Map;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import static ru.yandex.quasar.billing.services.processing.yapay.YaPayMerchant.AddressType.legal;
import static ru.yandex.quasar.billing.services.processing.yapay.YaPayMerchant.AddressType.post;
import static ru.yandex.quasar.billing.services.processing.yapay.YaPayMerchant.PersonType.ceo;

@JsonTest
@SpringJUnitConfig
class YaPayMerchantTest {
    @Autowired
    private JacksonTester<YaPayMerchant.Wrapper> tester;

    @Test
    void testDeserialize() throws IOException {
        tester.parse("{\n" +
                "  \"status\": \"success\",\n" +
                "  \"data\": {\n" +
                "    \"created\": \"2019-03-21T17:58:34.813010+00:00\",\n" +
                "    \"status\": \"new\",\n" +
                "    \"persons\": {\n" +
                "      \"ceo\": {\n" +
                "        \"email\": \"email@ya.ru\",\n" +
                "        \"phone\": \"+711_phone\",\n" +
                "        \"patronymic\": \"Patronymic\",\n" +
                "        \"surname\": \"Surname\",\n" +
                "        \"name\": \"Name\",\n" +
                "        \"birthDate\": \"1900-01-02\"\n" +
                "      }\n" +
                "    },\n" +
                "    \"revision\": 1,\n" +
                "    \"addresses\": {\n" +
                "      \"legal\": {\n" +
                "        \"country\": \"RU\",\n" +
                "        \"home\": \"16\",\n" +
                "        \"city\": \"Moscow\",\n" +
                "        \"street\": \"Lva Tolstogo\",\n" +
                "        \"zip\": \"123456\"\n" +
                "      },\n" +
                "      \"post\": {\n" +
                "        \"country\": \"RU\",\n" +
                "        \"home\": \"16\",\n" +
                "        \"city\": \"Moscow\",\n" +
                "        \"street\": \"Lva Tolstogo\",\n" +
                "        \"zip\": \"22222\"\n" +
                "      }\n" +
                "    },\n" +
                "    \"parent_uid\": null,\n" +
                "    \"moderation\": null,\n" +
                "    \"updated\": \"2019-03-21T22:49:23.660585+00:00\",\n" +
                "    \"uid\": 854426053,\n" +
                "    \"bank\": {\n" +
                "      \"account\": \"40702810700190000201\",\n" +
                "      \"bik\": \"044583503\",\n" +
                "      \"name\": \"Tinkoff\",\n" +
                "      \"correspondentAccount\": \"3333333\"\n" +
                "    },\n" +
                "    \"name\": \"Test merchant\",\n" +
                "    \"billing\": {\n" +
                "      \"trust_partner_id\": \"106606227\",\n" +
                "      \"contract_id\": \"1533541\",\n" +
                "      \"trust_submerchant_id\": null,\n" +
                "      \"person_id\": \"9863411\",\n" +
                "      \"client_id\": \"106606227\"\n" +
                "    },\n" +
                "    \"organization\": {\n" +
                "      \"inn\": \"5043041353\",\n" +
                "      \"kpp\": \"504301001\",\n" +
                "      \"fullName\": \"Hoofs & Horns\",\n" +
                "      \"type\": \"OOO\",\n" +
                "      \"ogrn\": \"1234567890\",\n" +
                "      \"englishName\": \"HH\",\n" +
                "      \"name\": \"Yandex\",\n" +
                "      \"siteUrl\": \"pay.yandex.ru\"\n" +
                "    }\n" +
                "  },\n" +
                "  \"code\": 200\n" +
                "}")
                .assertThat()
                .isEqualTo(new YaPayMerchant.Wrapper(
                        YaPayMerchant.builder()
                                .created(ZonedDateTime.of(2019, 3, 21,
                                        17, 58, 34, 813010000,
                                        ZoneId.of("UTC")).toInstant())
                                .status("new")
                                .persons(Map.of(ceo, YaPayMerchant.Person.builder()
                                        .email("email@ya.ru")
                                        .phone("+711_phone")
                                        .patronymic("Patronymic")
                                        .surname("Surname")
                                        .name("Name")
                                        .birthDate(LocalDate.of(1900, 1, 2))
                                        .build()))
                                .revision(BigInteger.ONE)
                                .addresses(Map.of(legal, YaPayMerchant.Address.builder()
                                                .country("RU")
                                                .city("Moscow")
                                                .home("16")
                                                .street("Lva Tolstogo")
                                                .zip("123456")
                                                .build(),
                                        post, YaPayMerchant.Address.builder()
                                                .country("RU")
                                                .city("Moscow")
                                                .home("16")
                                                .street("Lva Tolstogo")
                                                .zip("22222")
                                                .build()
                                ))
                                .parentUid(null)
                                .moderation(null)
                                .updated(ZonedDateTime.of(2019, 3, 21,
                                        22, 49, 23, 660585000,
                                        ZoneId.of("UTC")).toInstant())
                                .uid(BigInteger.valueOf(854426053))
                                .bank(YaPayMerchant.Bank.builder()
                                        .account("40702810700190000201")
                                        .bik("044583503")
                                        .name("Tinkoff")
                                        .correspondentAccount("3333333")
                                        .build())
                                .name("Test merchant")
                                .billing(YaPayMerchant.Billing.builder()
                                        .trustPartnerId("106606227")
                                        .contractId("1533541")
                                        .trustSubmerchantId(null)
                                        .personId("9863411")
                                        .clientId("106606227")
                                        .build())
                                .organization(Organization.builder()
                                        .inn("5043041353")
                                        .kpp("504301001")
                                        .fullName("Hoofs & Horns")
                                        .type("OOO")
                                        .ogrn("1234567890")
                                        .englishName("HH")
                                        .name("Yandex")
                                        .siteUrl("pay.yandex.ru")
                                        .build())
                                .build(),
                        200, "success")
                );
    }
}
