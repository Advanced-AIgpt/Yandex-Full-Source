package ru.yandex.quasar.billing.dao;

import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.NoSuchAlgorithmException;
import java.util.Base64;
import java.util.Map;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.config.TestConfigProvider;

import static org.junit.jupiter.api.Assertions.assertAll;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;

@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = {TestConfigProvider.class})
class SkillInfoDAOTest {

    @Autowired
    private SkillInfoDAO skillInfoDAO;

    @Test
    void testSave() throws NoSuchAlgorithmException {
        KeyPairGenerator kpg = KeyPairGenerator.getInstance("RSA");
        kpg.initialize(2048);
        KeyPair kp = kpg.generateKeyPair();

        Base64.Encoder encoder = Base64.getEncoder();

        String privateKey = encoder.encodeToString(kp.getPrivate().getEncoded());
        String publicKey = encoder.encodeToString(kp.getPublic().getEncoded());
        SkillInfo skillInfo = skillInfoDAO.save(
                SkillInfo.builder("skill1")
                        .privateKey(privateKey)
                        .publicKey(publicKey)
                        .build());


        assertNotNull(skillInfo.getSkillInfoId());

        SkillInfo queried = skillInfoDAO.findById(skillInfo.getSkillInfoId()).get();

        assertAll(
                () -> assertEquals("skill1", queried.getSkillUuid()),
                () -> assertEquals(privateKey, queried.getPrivateKey()),
                () -> assertEquals(publicKey, queried.getPublicKey())
        );

    }


    @Test
    void testFindBySkillId() {

        // given
        skillInfoDAO.save(SkillInfo.builder("skill1")
                .privateKey("private")
                .publicKey("public")
                .build());
        SkillMerchant skillMerchant = SkillMerchant.builder()
                .token("298aea40-d5a1-4fc4-437c-7a27650c9c5f")
                .serviceMerchantId(2L)
                .entityId("alice-skill:49eb5204-5e16-4106-a867-7353d94cd2e7")
                .description("description")
                .build();
        skillInfoDAO.save(SkillInfo.builder("skill2")
                .privateKey("private")
                .publicKey("public")
                .merchants(Map.of(skillMerchant.getToken(), skillMerchant))
                .build());

        // when
        SkillInfo found = skillInfoDAO.findBySkillId("skill2").get();

        // then
        assertEquals("skill2", found.getSkillUuid());
        assertEquals(skillMerchant, found.getMerchants().get("298aea40-d5a1-4fc4-437c-7a27650c9c5f"));
    }

    @Test
    void testFindBySkillInfoId() {

        // given
        skillInfoDAO.save(SkillInfo.builder("skill1")
                .privateKey("private")
                .publicKey("public")
                .build());
        SkillMerchant skillMerchant = SkillMerchant.builder()
                .token("298aea40-d5a1-4fc4-437c-7a27650c9c5f")
                .serviceMerchantId(2L)
                .entityId("alice-skill:49eb5204-5e16-4106-a867-7353d94cd2e7")
                .description("description")
                .build();
        SkillInfo target = skillInfoDAO.save(SkillInfo.builder("skill2")
                .privateKey("private")
                .publicKey("public")
                .merchants(Map.of(skillMerchant.getToken(), skillMerchant))
                .build());

        // when
        SkillInfo found = skillInfoDAO.findBySkillId(target.getSkillInfoId()).get();

        // then
        assertEquals("skill2", found.getSkillUuid());
        assertEquals(skillMerchant, found.getMerchants().get("298aea40-d5a1-4fc4-437c-7a27650c9c5f"));
    }
}
