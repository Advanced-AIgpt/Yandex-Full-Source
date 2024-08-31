package ru.yandex.alice.paskill.dialogovo.providers.recommendation

import com.google.common.primitives.Doubles
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
import ru.yandex.alice.paskill.dialogovo.domain.show.ShowInfo
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType
import ru.yandex.alice.paskill.dialogovo.providers.skill.ShowProvider
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillCategoryKey
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillTagsKey
import ru.yandex.alice.paskill.dialogovo.scenarios.Scenarios
import ru.yandex.alice.paskill.dialogovo.service.SurfaceChecker
import ru.yandex.alice.paskill.dialogovo.test.TestSkills
import java.time.Instant
import java.util.Optional

@SpringBootTest(
    classes = [TestConfigProvider::class, SkillCategoryRecommendationProviderTest.TestSkillProviderConfiguration::class],
    webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT
)
internal class SkillCategoryRecommendationProviderTest {
    @Autowired
    @Qualifier("skillByScoreAndCategoryRecommendationProvider")
    private lateinit var provider: SkillByScoreAndCategoryRecommendationProvider

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
        Assertions.assertEquals("3", skills[0].id)
    }

    @TestConfiguration
    open class TestSkillProviderConfiguration {
        fun skillProvider(): SkillProvider {
            return object : SkillProvider {
                override fun getSkill(skillId: String): Optional<SkillInfo> {
                    val skillInfo = TestSkills.cityGameSkill().copy(
                        id = skillId,
                        isRecommended = true,
                        automaticIsRecommended = true,
                        hideInStore = false,
                        onAir = true,
                        score = Doubles.tryParse(skillId)!!,
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
                    return listOf(
                        TestSkills.cityGameSkill().copy(
                            id = "1",
                            isRecommended = true,
                            automaticIsRecommended = true,
                            hideInStore = false,
                            onAir = true,
                            score = 1.0,
                            surfaces = setOf(Surface.QUASAR),
                        ),
                        TestSkills.cityGameSkill().copy(
                            id = "2",
                            isRecommended = true,
                            automaticIsRecommended = true,
                            hideInStore = false,
                            onAir = true,
                            score = 2.0,
                            surfaces = setOf(Surface.QUASAR),
                        ),
                        TestSkills.cityGameSkill().copy(
                            id = "3",
                            isRecommended = true,
                            automaticIsRecommended = true,
                            hideInStore = false,
                            onAir = true,
                            score = 3.0,
                            surfaces = setOf(Surface.QUASAR),
                        )
                    )
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

        fun showProvider(): ShowProvider {
            return object : ShowProvider {
                override fun getShowFeed(feedId: String, showType: ShowType): Optional<ShowInfo> {
                    return Optional.empty()
                }

                override fun getShowFeedBySkillId(skillId: String, showType: ShowType): Optional<ShowInfo> {
                    return Optional.empty()
                }

                override fun getShowFeedsByIds(feedId: Collection<String>): List<ShowInfo> {
                    return emptyList()
                }

                override fun getActiveShowSkills(showType: ShowType): List<ShowInfo> {
                    return emptyList()
                }

                override fun getActivePersonalizedShowSkills(showType: ShowType): List<ShowInfo> {
                    return emptyList()
                }

                override fun isReady(): Boolean {
                    return true
                }
            }
        }

        @Bean("skillByScoreAndCategoryRecommendationProvider")
        open fun skillByScoreAndCategoryRecommendationProvider(
            surfaceChecker: SurfaceChecker
        ): SkillByScoreAndCategoryRecommendationProvider {
            return SkillByScoreAndCategoryRecommendationProvider(
                skillProvider(),
                surfaceChecker,
                1,
                SkillCategoryKey.GAMES_TRIVIA_ACCESSORIES
            )
        }
    }
}
