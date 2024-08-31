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
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillCategoryKey
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillTagsKey
import ru.yandex.alice.paskill.dialogovo.scenarios.Scenarios
import ru.yandex.alice.paskill.dialogovo.service.SurfaceChecker
import ru.yandex.alice.paskill.dialogovo.test.TestSkills
import java.time.Instant
import java.util.Optional

@SpringBootTest(
    classes = [TestConfigProvider::class, SkillTagsRecommendationProviderTest.TestSkillProviderConfiguration::class],
    webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT
)
internal class SkillTagsRecommendationProviderTest {
    @Autowired
    @Qualifier("testTagsRecommendationProvider")
    private lateinit var provider: SkillTagsRecommendationProvider

    @Test
    fun testTakeSkillId() {
        val request = MegaMindRequest<Any>(
            requestId = "req-id",
            scenarioMeta = Scenarios.DIALOGOVO,
            serverTime = Instant.now(),
            randomSeed = 42,
            experiments = mutableSetOf(),
            clientInfo = ClientInfo(),
            input = Input.Unknown(""),
            voiceSession = true,
        )
        val skills = provider.getSkills(request.random, 1, request)
        Assertions.assertEquals(1, skills.size)
    }

    @TestConfiguration
    open class TestSkillProviderConfiguration {
        fun skillProvider(): SkillProvider {
            return object : SkillProvider {
                override fun getSkill(skillId: String): Optional<SkillInfo> {
                    val skillInfo = TestSkills.cityGameSkill().copy(
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
                    return listOf(
                        TestSkills.cityGameSkill().copy(
                            isRecommended = true,
                            automaticIsRecommended = true,
                            hideInStore = false,
                            onAir = true,
                            surfaces = setOf(Surface.QUASAR),
                        )
                    )
                }

                override fun getSkillsByCategory(categoryKey: SkillCategoryKey): List<SkillInfo> {
                    return mutableListOf()
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

        @Bean("testTagsRecommendationProvider")
        open fun testTagsRecommendationProvider(
            surfaceChecker: SurfaceChecker
        ): SkillTagsRecommendationProvider {
            return SkillTagsRecommendationProvider(
                skillProvider(),
                surfaceChecker,
                SkillTagsKey.KIDS_GAMES_ONBOARDING
            )
        }
    }
}
