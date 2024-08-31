package ru.yandex.quasar.billing.services.skills;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.jdbc.DataSourceTransactionManagerAutoConfiguration;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;
import org.springframework.web.client.RestTemplate;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.controller.SyncExecutorServicesConfig;
import ru.yandex.quasar.billing.dao.SkillInfo;
import ru.yandex.quasar.billing.dao.SkillInfoDAO;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.UnistatService;
import ru.yandex.quasar.billing.services.processing.yapay.ServiceMerchantInfo;
import ru.yandex.quasar.billing.services.processing.yapay.YaPayClientException;
import ru.yandex.quasar.billing.services.processing.yapay.YandexPayMerchantRegistry;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;

@SpringJUnitConfig(classes = {
        TestConfigProvider.class,
        SkillsServiceImpl.class,
        RestTemplate.class,
        SkillsServiceImplTest.MockMerchantRegistry.class,
        AuthorizationContext.class,
        SyncExecutorServicesConfig.class,
        DataSourceTransactionManagerAutoConfiguration.class
})
@ExtendWith(EmbeddedPostgresExtension.class)
class SkillsServiceImplTest {

    private static final String SKILL_UUID = "skill1";
    private static final long UID = 999L;
    private static final String SLUG = "skillSlug";
    @Autowired
    private SkillsServiceImpl skillsService;
    @Autowired
    private SkillInfoDAO skillInfoDAO;
    @Autowired
    private MockMerchantRegistry merchantRegistry;
    @MockBean
    private UnistatService unistatService;

    @BeforeEach
    void setUp() {
        skillInfoDAO.deleteAll();
    }

    @Test
    void refreshSkillKeys() {
        // given
        SkillInfo skillInfo = skillInfoDAO.save(dummySkill());

        // when
        var skillInfoNew = skillsService.refreshSkillKeys(skillInfo.getSkillInfoId());

        // then
        assertNotNull(skillInfoNew.getPrivateKey());
        assertNotNull(skillInfoNew.getPublicKey());
        assertNotEquals("oldpubKey", skillInfoNew.getPublicKey());
        assertNotEquals("oldKey", skillInfoNew.getPrivateKey());


        SkillInfo fetchedSkill = skillInfoDAO.findBySkillId(SKILL_UUID)
                .orElseThrow(RuntimeException::new);

        assertNotNull(fetchedSkill.getPrivateKey());
        assertNotNull(fetchedSkill.getPublicKey());
        assertNotEquals("oldpubKey", fetchedSkill.getPublicKey());
        assertNotEquals("oldKey", fetchedSkill.getPrivateKey());
    }

    @Test
    void testRegisterSkill() throws SkillsService.SkillAccessViolationException {
        SkillInfo skillInfo = skillsService.registerSkill(SKILL_UUID, UID, SLUG);

        assertNotNull(skillInfo.getPublicKey());
        assertNotNull(skillInfo.getPrivateKey());
        assertEquals(SLUG, skillInfo.getSlug());
    }

    @Test
    void testRegisterSkillDifferentOwner() {

        skillInfoDAO.save(SkillInfo.builder(SKILL_UUID)
                .ownerUid(UID + 1)
                .publicKey("oldpubKey")
                .privateKey("oldkey")
                .build());

        assertThrows(
                SkillsService.SkillAccessViolationException.class,
                () -> skillsService.registerSkill(SKILL_UUID, UID, SLUG)
        );
    }

    @Test
    void testRegisterSkillidenpotent() throws SkillsService.SkillAccessViolationException {
        // given
        SkillInfo existingSkill = skillInfoDAO.save(dummySkill());

        // when
        SkillInfo skillInfo = skillsService.registerSkill(SKILL_UUID, UID, "dummySlug");

        // then
        assertEquals(existingSkill.getPublicKey(), skillInfo.getPublicKey());
        assertEquals(existingSkill.getPrivateKey(), skillInfo.getPrivateKey());
        assertEquals(existingSkill.getSlug(), skillInfo.getSlug());
    }

    @Test
    void testRequestMerchantAccess() throws SkillsService.BadSkillAccessRequestTokenException {
        //given
        var dummySkill = skillInfoDAO.save(dummySkill());

        //when
        MerchantInfo serviceMerchantInfo = skillsService.requestMerchantAccess(SKILL_UUID, "token", "description");

        //then
        ServiceMerchantInfo next = merchantRegistry.getFirst();
        assertNotNull(next);
        Assertions.assertThat(serviceMerchantInfo).isEqualToIgnoringGivenFields(next, "token");
        SkillInfo skill = skillsService.getSkillById(dummySkill.getSkillInfoId());

        assertFalse(skill.getMerchants().isEmpty(), "Accessible merchant list is empty");
    }

    @Test
    void testSecondRequestMerchantAccess() throws SkillsService.BadSkillAccessRequestTokenException {
        //given
        var dummySkill = skillInfoDAO.save(dummySkill());
        skillsService.requestMerchantAccess(SKILL_UUID, "token", "description");

        //when
        MerchantInfo serviceMerchantInfo = skillsService.requestMerchantAccess(SKILL_UUID, "token", "other " +
                "description");

        //then
        ServiceMerchantInfo next = merchantRegistry.getFirst();
        assertNotNull(next);
        Assertions.assertThat(serviceMerchantInfo).isEqualToIgnoringGivenFields(next, "token");
        assertEquals("description", serviceMerchantInfo.getDescription());
        SkillInfo skill = skillsService.getSkillById(dummySkill.getSkillInfoId());

        assertFalse(skill.getMerchants().isEmpty(), "Accessible merchant list is empty");
    }

    @Test
    void testMerchantAccessInfo() throws SkillsService.BadSkillAccessRequestTokenException {
        //given
        var dummySkill = skillInfoDAO.save(dummySkill());
        skillsService.requestMerchantAccess(SKILL_UUID, "token", "description");

        //when
        MerchantInfo serviceMerchantInfo = skillsService.merchantInfo(SKILL_UUID, "token");

        //then
        ServiceMerchantInfo next = merchantRegistry.getFirst();
        assertNotNull(next);
        Assertions.assertThat(serviceMerchantInfo).isEqualToIgnoringGivenFields(next, "token");
        SkillInfo skill = skillsService.getSkillById(dummySkill.getSkillInfoId());

        assertFalse(skill.getMerchants().isEmpty(), "Accessible merchant list is empty");
    }

    @Test
    void testGetMerchants() throws SkillsService.BadSkillAccessRequestTokenException {
        //given
        var dummySkill = skillInfoDAO.save(dummySkill());
        skillsService.requestMerchantAccess(SKILL_UUID, "token", "description");

        List<MerchantInfo> merchants = skillsService.getMerchants(dummySkill.getSkillUuid());

        ServiceMerchantInfo next = merchantRegistry.getFirst();
        Assertions.assertThat(merchants.get(0)).isEqualToIgnoringGivenFields(next, "token");
    }

    SkillInfo dummySkill() {
        return SkillInfo.builder(SKILL_UUID)
                .ownerUid(UID)
                .slug("dummySlug")
                .publicKey("oldpubKey")
                .privateKey("oldkey")
                .build();
    }

    static class MockMerchantRegistry implements YandexPayMerchantRegistry {
        private final Map<String, ServiceMerchantInfo> registry = new HashMap<>();

        @Override
        public ServiceMerchantInfo requestMerchantAccess(String token, String entityId, String description) {
            if ("token".equals(token)) {
                return registry.computeIfAbsent(token, it -> ServiceMerchantInfo.builder()
                        .enabled(true)
                        .deleted(false)
                        .entityId(entityId)
                        .description(description)
                        .build());

            } else {
                throw new YaPayClientException("Wrong token");
            }
        }

        @Override
        public ServiceMerchantInfo merchantInfo(long serviceMerchantId) {
            return registry.values().stream()
                    .filter(it -> it.getServiceMerchantId() == serviceMerchantId)
                    .findFirst()
                    .orElseThrow(RuntimeException::new);
        }

        public ServiceMerchantInfo getFirst() {
            return registry.values().iterator().next();
        }
    }


}
