package ru.yandex.alice.paskill.dialogovo.providers.recommendation

import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Test
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.boot.test.context.SpringBootTest
import org.springframework.boot.test.context.TestConfiguration
import org.springframework.context.annotation.Bean
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.domain.Surface
import ru.yandex.alice.kronstadt.core.input.Input
import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.providers.recommendation.StaticSkillListRecommendationProviderTest.TestSkillProviderConfiguration
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillCategoryKey
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillTagsKey
import ru.yandex.alice.paskill.dialogovo.scenarios.Scenarios
import ru.yandex.alice.paskill.dialogovo.service.SurfaceChecker
import ru.yandex.alice.paskill.dialogovo.test.TestSkills
import java.time.Instant
import java.util.Optional

@SpringBootTest(
    classes = [TestConfigProvider::class, TestSkillProviderConfiguration::class],
    webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT
)
internal class StaticSkillListRecommendationProviderTest {
    @Autowired
    @Qualifier("providerWithExperimentalSkillList")
    private lateinit var provider: StaticSkillListRecommendationProvider

    @Test
    fun testTakeDefaultSkillIdListWithoutExperiments() {
        val request = MegaMindRequest<Any>(
            clientInfo = ClientInfo(),
            scenarioMeta = Scenarios.DIALOGOVO,
            randomSeed = 42,
            experiments = mutableSetOf(),
            serverTime = Instant.now(),
            input = Input.Unknown(""),
            requestId = "req-id",
            voiceSession = true
        )
        val skills = provider.getSkills(request.random, 1, request)
        Assertions.assertEquals(1, skills.size)
        Assertions.assertEquals("672f7477-d3f0-443d-9bd5-2487ab0b6a4c", skills[0].id)
    }

    @Test
    fun testTakeDefaultSkillIdListWithInvalidExperimentFlag() {
        val request = MegaMindRequest<Any>(
            clientInfo = ClientInfo(),
            scenarioMeta = Scenarios.DIALOGOVO,
            randomSeed = 42,
            experiments = mutableSetOf("dialogovo_test_override_skill_list_invalid"),
            serverTime = Instant.now(),
            input = Input.Unknown(""),
            requestId = "req-id",
            voiceSession = true
        )
        val skills = provider.getSkills(request.random, 1, request)
        Assertions.assertEquals(1, skills.size)
        Assertions.assertEquals("672f7477-d3f0-443d-9bd5-2487ab0b6a4c", skills[0].id)
    }

    @Test
    fun testOverrideSkillListWithExperimentFlag() {
        val request = MegaMindRequest<Any>(
            clientInfo = ClientInfo(),
            scenarioMeta = Scenarios.DIALOGOVO,
            randomSeed = 42,
            experiments = mutableSetOf("dialogovo_test_override_skill_list_experiment_list"),
            serverTime = Instant.now(),
            input = Input.Unknown(""),
            requestId = "req-id",
            voiceSession = true
        )
        val skills = provider.getSkills(request.random, 1, request)
        Assertions.assertEquals(1, skills.size)
        Assertions.assertEquals("059fe34d-f446-4fc2-b7e2-6504fb89c27b", skills[0].id)
    }

    @TestConfiguration
    open class TestSkillProviderConfiguration {
        private fun skillProvider(): SkillProvider {
            return object : SkillProvider {
                override fun getSkill(skillId: String): Optional<SkillInfo> {
                    val skillInfo = TestSkills.cityGameSkill().copy(
                        id = skillId,
                        isRecommended = true,
                        automaticIsRecommended = true,
                        hideInStore = false,
                        onAir = true,
                        surfaces = setOf(Surface.QUASAR),
                    )
                    return Optional.of(skillInfo)
                }

                override fun getSkillDraft(skillId: String): Optional<SkillInfo> {
                    return Optional.empty()
                }

                override fun getActivationIntentFormNames(skillIds: List<String>): Map<String, Set<String>> {
                    return mapOf()
                }

                override fun getSkillsByTags(tagsKey: SkillTagsKey): List<SkillInfo> {
                    return emptyList()
                }

                override fun getSkillsByCategory(categoryKey: SkillCategoryKey): List<SkillInfo> {
                    return emptyList()
                }

                override fun findAllSkills(): MutableList<SkillInfo> {
                    return mutableListOf()
                }

                override fun isReady(): Boolean {
                    return true
                }

                override fun findSkillsByPhrases(phrases: Set<String>): Map<String, List<String>> {
                    return emptyMap()
                }
            }
        }

        @Bean("providerWithExperimentalSkillList")
        open fun providerWithExperimentalSkillList(
            surfaceChecker: SurfaceChecker
        ): StaticSkillListRecommendationProvider {
            return StaticSkillListRecommendationProvider(
                listOf("672f7477-d3f0-443d-9bd5-2487ab0b6a4c"),
                skillProvider(),
                surfaceChecker,
                mapOf("experiment_list" to listOf("059fe34d-f446-4fc2-b7e2-6504fb89c27b")),
                "dialogovo_test_override_skill_list_"
            )
        }
    }
}
