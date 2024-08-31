package ru.yandex.alice.paskill.dialogovo.providers.skill

import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test
import org.mockito.Mockito
import org.springframework.boot.test.mock.mockito.MockBean
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig
import ru.yandex.alice.kronstadt.core.domain.Surface
import ru.yandex.alice.kronstadt.core.domain.Voice
import ru.yandex.alice.paskill.dialogovo.MockitoHelper
import ru.yandex.alice.paskill.dialogovo.domain.DeveloperType
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag.PERSONALIZED_MORNING_SHOW_ENABLED
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.domain.show.ShowInfo
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillInfoDB.PublishingSettings
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.util.Optional
import java.util.UUID

@SpringJUnitConfig
internal class DBSkillProviderTest {
    @MockBean
    private lateinit var skillsDao: SkillsDao

    @MockBean
    private lateinit var userAgreementDao: UserAgreementDao

    @MockBean
    private lateinit var showFeedDao: ShowFeedDao

    private lateinit var provider: DBSkillProvider

    @BeforeEach
    fun setUp() {
        provider = DBSkillProvider(
            "https://dialogs.yandex.ru",
            skillsDao,
            userAgreementDao,
            showFeedDao,
            MetricRegistry(),
            false
        )
    }

    fun getSkillInfoDbBuilderWithDefaults(id: UUID): SkillInfoDB {
        return SkillInfoDB(
            id = id,
            name = "skill name",
            logoUrl = "",
            backendSettings = SkillInfoDB.BackendSettings("...", null),
            onAir = true,
            publishingSettings =
            PublishingSettings(
                "", "", emptyList(),
                "", "", false, emptyList()
            ),
            salt = "salt",
            slug = "",
            look = SkillInfo.Look.EXTERNAL.code,
            voice = Voice.OKSANA.code,
            featureFlags = listOf(),
            automaticIsRecommended = true,
            developerType = "External",
            draft = false,
            exposeInternalFlags = false,
            persistentUserIdSalt = "salt",
            hideInStore = false,
            nameTts = null,
            skillAccess = "public",
            useNlu = true,
            useZora = true,
            userId = "123",
            monitoringType = SkillInfo.MonitoringType.NONMONITORED.code
        )
    }

    @Test
    fun noFeatureFlagsNotThrowsException() {
        val id = UUID.randomUUID()
        val skill: SkillInfoDB = getSkillInfoDbBuilderWithDefaults(id)
        Mockito.`when`(skillsDao.findAliceSkillById(MockitoHelper.anyObject())).thenReturn(Optional.of(skill))
        provider.getSkill(id.toString())
    }

    // see https://github.yandex-team.ru/paskills/api/blob/f7a35f26ce0f4342c34dfe9a905feb2090f52a9e/src/test/unit
    // /services/surface/surfacesFromSkill.ts
    @Test
    fun surfacesFromInterfaces() {
        val skill = aliceSkill()
        Assertions.assertEquals(
            setOf(
                Surface.DESKTOP,
                Surface.MOBILE,
                Surface.AUTO,
                Surface.NAVIGATOR,
                Surface.QUASAR
            ),
            provider.getSupportedSurfaces(skill)
        )
    }

    @Test
    fun surfaceFromInterfacesWithScreen() {
        val skill = aliceSkill().copy(requiredInterfaces = listOf("screen"))
        Assertions.assertEquals(setOf(Surface.DESKTOP, Surface.MOBILE), provider.getSupportedSurfaces(skill))
    }

    @Test
    fun surfacesFromExactSurfacesField() {
        val skill = aliceSkill().copy(exactSurfaces = listOf("auto"))
        Assertions.assertEquals(setOf(Surface.AUTO), provider.getSupportedSurfaces(skill))
    }

    @Test
    fun surfacesFromExactSurfacesField2() {
        val skill = aliceSkill().copy(exactSurfaces = listOf("auto"), surfaceWhitelist = listOf("navigator"))
        Assertions.assertEquals(setOf(Surface.AUTO), provider.getSupportedSurfaces(skill))
    }

    @Test
    fun surfacesWithWhitelist() {
        val skill = aliceSkill().copy(requiredInterfaces = listOf("screen"), surfaceWhitelist = listOf("watch"))
        Assertions.assertEquals(
            setOf(Surface.DESKTOP, Surface.MOBILE, Surface.WATCH),
            provider.getSupportedSurfaces(skill)
        )
    }

    @Test
    fun surfacesWithBlacklist() {
        val skill = aliceSkill().copy(surfaceBlacklist = listOf("station"))
        Assertions.assertEquals(
            setOf(Surface.DESKTOP, Surface.MOBILE, Surface.AUTO, Surface.NAVIGATOR),
            provider.getSupportedSurfaces(skill)
        )
    }

    @Test
    fun surfacesWithBlacklist2() {
        val skill = aliceSkill().copy(
            requiredInterfaces = listOf("screen"),
            surfaceWhitelist = listOf("station"),
            surfaceBlacklist = listOf("station")
        )
        Assertions.assertEquals(setOf(Surface.DESKTOP, Surface.MOBILE), provider.getSupportedSurfaces(skill))
    }

    @Test
    fun nullDeveloperTypeConvertsToExternal() {
        val id = UUID.randomUUID()
        val skillInfoDB = getSkillInfoDbBuilderWithDefaults(id).copy(developerType = null)
        Mockito.`when`(skillsDao.findAliceSkillById(MockitoHelper.anyObject())).thenReturn(Optional.of(skillInfoDB))
        val skill = provider.getSkill(id.toString()).get()
        Assertions.assertEquals(skill.developerType, DeveloperType.External)
    }

    @Test
    fun yandexDeveloperTypeConvertsToYandex() {
        val id = UUID.randomUUID()
        val skillInfoDB = getSkillInfoDbBuilderWithDefaults(id).copy(developerType = "yandex")
        Mockito.`when`(skillsDao.findAliceSkillById(MockitoHelper.anyObject())).thenReturn(Optional.of(skillInfoDB))
        val skill = provider.getSkill(id.toString()).get()
        Assertions.assertEquals(skill.developerType, DeveloperType.Yandex)
    }

    @Test
    fun externalDeveloperTypeConvertsToYandex() {
        val id = UUID.randomUUID()
        val skillInfoDB = getSkillInfoDbBuilderWithDefaults(id).copy(developerType = "external")
        Mockito.`when`(skillsDao.findAliceSkillById(MockitoHelper.anyObject())).thenReturn(Optional.of(skillInfoDB))
        val skill = provider.getSkill(id.toString()).get()
        Assertions.assertEquals(skill.developerType, DeveloperType.External)
    }

    @Test
    fun allowsSkillsWithSameActivationPhrase() {
        val firstId = UUID.randomUUID()
        val secondId = UUID.randomUUID()
        val activationPhrase = "метод помидора"
        Mockito.`when`(skillsDao.findByActivationPhrases(MockitoHelper.anyObject()))
            .thenReturn(
                listOf(
                    SkillIdPhrasesEntity(firstId, listOf(activationPhrase)),
                    SkillIdPhrasesEntity(secondId, listOf(activationPhrase))
                )
            )
        Assertions.assertEquals(
            mapOf(activationPhrase to listOf(firstId.toString(), secondId.toString())),
            provider.findSkillsByPhrases(setOf(activationPhrase))
        )
    }

    @Test
    fun morningShowFindActiveTest() {
        val skillId = UUID.randomUUID()
        val showId = UUID.randomUUID()
        val skillInfoDB = getSkillInfoDbBuilderWithDefaults(skillId)
        val showInfoDB = ShowInfoDB(
            id = showId,
            skillId = skillId,
            name = "show name",
            nameTts = "show name tts",
            description = "show description",
            onAir = true,
            type = ShowType.MORNING
        )

        Mockito.`when`(skillsDao.findAllActiveAliceSkill()).thenReturn(listOf(skillInfoDB))
        Mockito.`when`(showFeedDao.findAllActiveShowFeeds()).thenReturn(listOf(showInfoDB))
        provider.loadCaches()
        val skill = provider.getSkill(skillId.toString()).get()
        val expected = ShowInfo(
            id = showId.toString(),
            skillInfo = skill,
            showType = ShowType.MORNING,
            name = "show name",
            nameTts = "show name tts",
            description = "show description",
            onAir = true,
        )
        Assertions.assertEquals(listOf(expected), provider.getActiveShowSkills(ShowType.MORNING))
    }

    @Test
    fun morningShowFindPersonalizedShowEnabledTest() {
        val skillId = UUID.randomUUID()
        val showId = UUID.randomUUID()
        val skillInfoDB = getSkillInfoDbBuilderWithDefaults(skillId).copy(
            featureFlags = listOf("morning_show", PERSONALIZED_MORNING_SHOW_ENABLED)
        )
        val showInfoDB = ShowInfoDB(
            id = showId,
            skillId = skillId,
            name = "show name",
            nameTts = "show name tts",
            description = "show description",
            onAir = true,
            type = ShowType.MORNING,

        )

        Mockito.`when`(skillsDao.findAllActiveAliceSkill()).thenReturn(listOf(skillInfoDB))
        Mockito.`when`(showFeedDao.findAllActiveShowFeeds()).thenReturn(listOf(showInfoDB))
        provider.loadCaches()
        val skill = provider.getSkill(skillId.toString()).get()
        val expected = ShowInfo(
            id = showId.toString(),
            skillInfo = skill,
            showType = ShowType.MORNING,
            name = "show name",
            nameTts = "show name tts",
            description = "show description",
            onAir = true,
            personalizationEnabled = true
        )
        Assertions.assertEquals(listOf(expected), provider.getActivePersonalizedShowSkills(ShowType.MORNING))
    }

    @Test
    fun getSkillsByTagsTest() {
        val id1 = UUID.randomUUID()
        val skillInfoDB1 = getSkillInfoDbBuilderWithDefaults(id1).copy(
            developerType = "yandex",
            tags = listOf("none")
        )
        val id2 = UUID.randomUUID()
        val skillInfoDB2 = getSkillInfoDbBuilderWithDefaults(id2).copy(
            developerType = "yandex",
        )
        val id3 = UUID.randomUUID()
        val tags: MutableList<String> = SkillTagsKey.KIDS_GAMES_ONBOARDING.tags.toMutableList()
        tags.add("another")
        val skillInfoDB3 = getSkillInfoDbBuilderWithDefaults(id3).copy(
            developerType = "yandex",
            tags = tags
        )
        Mockito.`when`(skillsDao.findAllActiveAliceSkill())
            .thenReturn(listOf(skillInfoDB1, skillInfoDB2, skillInfoDB3))

        provider.loadCaches()
        Assertions.assertEquals(
            provider.getSkillsByTags(SkillTagsKey.KIDS_GAMES_ONBOARDING).map { it.id },
            listOf(id3.toString())
        )
    }

    private fun aliceSkill(): SkillInfoDB {
        return getSkillInfoDbBuilderWithDefaults(UUID.randomUUID())
            .copy(channel = "aliceSkill")
    }
}
