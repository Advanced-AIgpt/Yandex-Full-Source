package ru.yandex.quasar.billing.services.processing.yapay;

import java.math.BigInteger;
import java.time.Instant;
import java.util.Map;

import static ru.yandex.quasar.billing.services.processing.yapay.YaPayMerchant.AddressType.legal;
import static ru.yandex.quasar.billing.services.processing.yapay.YaPayMerchant.AddressType.post;
import static ru.yandex.quasar.billing.services.processing.yapay.YaPayMerchant.PersonType.ceo;

public interface YandexPayMerchantTestUtil {
    YaPayMerchant MERCHANT = YaPayMerchant.builder()
            .persons(Map.of(ceo, YaPayMerchant.Person.builder()
                    .name("Иван")
                    .surname("Иванов")
                    .build()))
            .status("new")
            .bank(YaPayMerchant.Bank.builder()
                    .account("123123123")
                    .bik("123123123")
                    .correspondentAccount("323232323232323232")
                    .name("ОАО Банк")
                    .build())
            .organization(Organization.builder()
                    .name("ООО Ромашка")
                    .inn("123123123123123")
                    .kpp("3232323232")
                    .ogrn("33322232323")
                    .fullName("ООО Ромашка полное наименование")
                    .scheduleText("09:00 - 18:00")
                    .build())
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
            .uid(BigInteger.ONE)
            .billing(YaPayMerchant.Billing.builder()
                    .clientId("123123")
                    .personId("11111111")
                    .trustSubmerchantId("999999")
                    .trustPartnerId("777777")
                    .contractId("321113")
                    .build())
            .revision(BigInteger.ONE)
            .created(Instant.now())
            .updated(Instant.now())
            .build();
    Long MERCHANT_ID = Long.valueOf(MERCHANT.getBilling().getClientId());
    String SUBMERCHANT_ID = MERCHANT.getBilling().getTrustSubmerchantId();
    String TEST_MERCHANT_KEY = "c7d740f1-eb8e-4fbd-9276-c21e0c559898";
}
