package ru.yandex.quasar.billing.dao;

import java.math.BigDecimal;
import java.util.List;
import java.util.UUID;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.DeliveryInfo;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionTestUtil;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.processing.yapay.YandexPayMerchantTestUtil;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = {TestConfigProvider.class})
class PurchaseOfferDaoTest implements PricingOptionTestUtil, YandexPayMerchantTestUtil {
    private static final String EXTERNAL_ID = UUID.randomUUID().toString();
    private static final String ANOTHER_EXTERNAL_ID = UUID.randomUUID().toString();
    private final PricingOption skillPricingOption = createSkillPricingOption("PRODUCT_UID", BigDecimal.ONE,
            BigDecimal.TEN);
    @Autowired
    private PurchaseOfferDao purchaseOfferDao;
    @Autowired
    private SkillInfoDAO skillInfoDAO;
    private SkillInfo skill1;
    private SkillInfo skill2;

    @BeforeEach
    void setUp() {
        //Map<String, SkillMerchant> merchantInfo = Map.of("298aea40-d5a1-4fc4-437c-7a27650c9c5f", SkillMerchant
        // .create("298aea40-d5a1-4fc4-437c-7a27650c9c5f", 1L));
        skill1 = skillInfoDAO.save(SkillInfo.builder("skill1").publicKey("1").privateKey("1").build());

        //Map<String, SkillMerchant> merchantInfo2 = Map.of("298aea40-d5a1-4f27-437c-7a27650c9c5f", SkillMerchant
        // .create("298aea40-d5a1-4f27-437c-7a27650c9c5f", 2L));
        skill2 = skillInfoDAO.save(SkillInfo.builder("skill2").publicKey("1").privateKey("1").build());
    }

    @Test
    void testSave() {
        PurchaseOffer purchaseOffer = PurchaseOffer.builder("999", skill1.getSkillInfoId(), EXTERNAL_ID,
                List.of(skillPricingOption), "https://callbackUrl")
                .title("title")
                .imageUrl("http://image")
                .description("description")
                .skillSessionId("session")
                .skillUserId("userid")
                .skillName("Skill name")
                .skillImageUrl("https://asdasdasd.asdasdasd.ru")
                .merchantKey("31c77d40-f5a1-4fc4-437c-7a27650c9c5f")
                .deviceId("qweqweqweqwe")
                .deliveryInfo(DeliveryInfo.builder()
                        .productId("id")
                        .city("Москва")
                        .settlement("г. Москва")
                        .index("125677")
                        .street("лл. Льва Толстого")
                        .house("5")
                        .housing("5")
                        .building("2")
                        .porch("78")
                        .floor("14")
                        .flat("67")
                        .build())
                .build();

        PurchaseOffer saved = purchaseOfferDao.save(purchaseOffer);

        assertNotNull(saved.getId());

        PurchaseOffer queried = purchaseOfferDao.findById(saved.getId()).get();

        Assertions.assertThat(queried).isEqualToIgnoringGivenFields(saved, "createdAt");
    }

    @Test
    void testFindByPartnerIdAndPurchaseRequestId() {

        purchaseOfferDao.save(createDummyOffer("999", skill1, EXTERNAL_ID));

        PurchaseOffer purchaseOffer2 = purchaseOfferDao.save(createDummyOffer("999", skill2, EXTERNAL_ID));

        PurchaseOffer queried = purchaseOfferDao.findBySkillIdAndPurchaseRequestId(skill2.getSkillInfoId(),
                EXTERNAL_ID).get();

        Assertions.assertThat(queried).isEqualToIgnoringGivenFields(purchaseOffer2, "createdAt");
    }

    @Test
    void testFindBySkillUuidAndPurchaseOfferId() {
        PurchaseOffer targetOffer = purchaseOfferDao.save(createDummyOffer("999", skill1, EXTERNAL_ID));
        purchaseOfferDao.save(createDummyOffer("999", skill2, EXTERNAL_ID));
        purchaseOfferDao.save(createDummyOffer("999", skill1, ANOTHER_EXTERNAL_ID));

        var queried = purchaseOfferDao.findBySkillIdAndPurchaseOfferId(skill1.getSkillInfoId(), targetOffer.getId());

        assertTrue(queried.isPresent());

        Assertions.assertThat(queried.get()).isEqualToIgnoringGivenFields(targetOffer, "createdAt");
    }

    @Test
    void testFindByUidAndUuid() {

        purchaseOfferDao.save(createDummyOffer("999", skill1, EXTERNAL_ID));

        PurchaseOffer purchaseOffer2 = purchaseOfferDao.save(createDummyOffer("999", skill2, EXTERNAL_ID));

        PurchaseOffer queried = purchaseOfferDao.findByUidAndUuid("999", purchaseOffer2.getUuid()).get();

        Assertions.assertThat(queried).isEqualToIgnoringGivenFields(purchaseOffer2, "createdAt");
    }

    @Test
    void testBindToUid() {
        PurchaseOffer purchaseOffer = purchaseOfferDao.save(createDummyOffer(null, skill1, EXTERNAL_ID));

        boolean changed = purchaseOfferDao.bindToUser(purchaseOffer.getUuid(), "999");
        assertTrue(changed, "Purchase not changed");
    }

    @Test
    void testBindToUidForAlreadyBound() {
        PurchaseOffer purchaseOffer = purchaseOfferDao.save(createDummyOffer("999", skill2, EXTERNAL_ID));


        boolean changed2 = purchaseOfferDao.bindToUser(purchaseOffer.getUuid(), "111");
        assertFalse(changed2, "Purchase changed but it already has uid defined");
    }

    @Test
    void testBindToUidWrongStatus() {

        PurchaseOffer offer = createDummyOffer(null, skill2, UUID.randomUUID().toString());
        offer.setStatus(PurchaseOfferStatus.SUCCESS);

        PurchaseOffer purchaseOffer3 = purchaseOfferDao.save(offer);


        boolean changed3 = purchaseOfferDao.bindToUser(purchaseOffer3.getUuid(), "888");
        assertFalse(changed3, "Purchase changed but it has wrong status");
    }

    @Test
    void testBindToUidWrongAlreadyBoundToSelf() {

        PurchaseOffer purchaseOffer = purchaseOfferDao.save(createDummyOffer("999", skill2,
                UUID.randomUUID().toString()));


        boolean changed = purchaseOfferDao.bindToUser(purchaseOffer.getUuid(), "999");
        assertTrue(changed, "Purchase changed but it has wrong status");
    }

    private PurchaseOffer createDummyOffer(String uid, SkillInfo skill, String offerUuid) {
        return PurchaseOffer.builder(uid, skill.getSkillInfoId(), offerUuid, List.of(skillPricingOption),
                "https://callbackUrl")
                .title("title")
                .imageUrl("http://image")
                .description("description")
                .skillSessionId("session")
                .skillUserId("userid")
                .skillName("Skill name")
                .skillImageUrl("https://asdasdasd.asdasdasd.ru")
                .merchantKey(TEST_MERCHANT_KEY)
                .build();
    }
}
