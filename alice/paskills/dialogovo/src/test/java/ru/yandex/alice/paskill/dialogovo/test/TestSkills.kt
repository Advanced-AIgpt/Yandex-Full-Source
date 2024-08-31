package ru.yandex.alice.paskill.dialogovo.test

import org.junit.platform.commons.util.StringUtils
import ru.yandex.alice.kronstadt.core.domain.Voice
import ru.yandex.alice.paskill.dialogovo.domain.Channel
import ru.yandex.alice.paskill.dialogovo.domain.DeveloperType
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo.Companion.builder

class TestSkills private constructor() {
    companion object {
        const val CITY_GAME_SKILL_ID = "c70fd60d-9fc7-4514-a19a-5f1a598a7868" // Города в тестинге
        const val SECOND_MEMORY_SKILL_ID = "059fe34d-f446-4fc2-b7e2-6504fb89c27b"

        @JvmStatic
        @JvmOverloads
        fun cityGameSkill(
            port: Int = 0,
            featureFlags: Set<String> = emptySet(),
            userFeatureFlags: Map<String, Any> = emptyMap()
        ): SkillInfo {
            return builder() // Города в тестинге
                .id(CITY_GAME_SKILL_ID)
                .userId("564629782")
                .name("Test skill")
                .slug("test-skill-slug")
                .category("category")
                .channel(Channel.ALICE_SKILL)
                .developerName("dev name")
                .developerType(DeveloperType.External)
                .description("description")
                .logoUrl("https://avatars.mds.yandex.net/get-dialogs/758954/1a309e8e7d6781214dc5/orig")
                .storeUrl("https://dialogs.yandex.ru/store/skills/skill1")
                .look(SkillInfo.Look.EXTERNAL)
                .voice(Voice.SHITOVA_US)
                .salt("salt")
                .persistentUserIdSalt("persistentUserIdSalt")
                .webhookUrl("http://localhost:$port/testSkill")
                .onAir(true)
                .isRecommended(true)
                .automaticIsRecommended(true)
                .useZora(false)
                .surfaces(java.util.Set.of())
                .featureFlags(featureFlags)
                .userFeatureFlags(userFeatureFlags)
                .useNlu(true)
                .useStateStorage(false)
                .adBlockId(null)
                .encryptedAppMetricaApiKey(null)
                .monitoringType(SkillInfo.MonitoringType.NONMONITORED)
                .build()
        }

        @JvmStatic
        @JvmOverloads
        fun secondMemorySkill(
            port: Int = 0,
            recommended: Boolean = true,
            useZora: Boolean = false,
            featureFlags: Set<String> = emptySet(),
            userFeatureFlags: Map<String, Any> = emptyMap(),
            webhookUrl: String? = null,
        ): SkillInfo {
            val skillInfoBuilder = builder()
                .id(SECOND_MEMORY_SKILL_ID)
                .name("Вторая память")
                .userId("202060379")
                .channel(Channel.ALICE_SKILL)
                .category("category")
                .developerName("developer name")
                .developerType(DeveloperType.External)
                .description("description")
                .logoUrl("https://avatars.mds.yandex.net/get-dialogs/758954/1a309e8e7d6781214dc5/orig")
                .storeUrl("https://dialogs.yandex.ru/store/skills/skill1")
                .look(SkillInfo.Look.EXTERNAL)
                .voice(Voice.SHITOVA_US)
                .onAir(true)
                .isRecommended(recommended)
                .automaticIsRecommended(true)
                .salt("salt")
                .persistentUserIdSalt("persistentUserIdSalt")
                .webhookUrl("http://localhost:$port/testSkill")
                .surfaces(java.util.Set.of())
                .featureFlags(featureFlags)
                .useZora(useZora)
                .userFeatureFlags(userFeatureFlags)
                .useNlu(true)
                .useStateStorage(false)
                .adBlockId(null)
                .encryptedAppMetricaApiKey(null)
                .monitoringType(SkillInfo.MonitoringType.NONMONITORED)
                .slug("second-memory")

            if (StringUtils.isNotBlank(webhookUrl)) {
                skillInfoBuilder.webhookUrl(webhookUrl)
            }
            return skillInfoBuilder.build()
        }
    }

    init {
        throw UnsupportedOperationException()
    }
}
