package ru.yandex.alice.paskill.dialogovo.service.show

import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test
import org.mockito.Mock
import org.mockito.Mockito
import org.mockito.kotlin.any
import org.mockito.kotlin.mock
import org.mockito.kotlin.times
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.layout.TextCard
import ru.yandex.alice.paskill.dialogovo.config.ShowConfig
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult
import ru.yandex.alice.paskill.dialogovo.domain.show.MorningShowEpisodeEntity
import ru.yandex.alice.paskill.dialogovo.domain.show.ShowInfo
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowItemMeta
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor
import ru.yandex.alice.paskill.dialogovo.providers.skill.ShowProvider
import ru.yandex.alice.paskill.dialogovo.test.TestSkills
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.time.Duration
import java.time.Instant
import java.util.Optional

@SpringJUnitConfig
internal class ShowServiceImplTest {

    @Mock
    lateinit var showProvider: ShowProvider

    @Mock
    lateinit var skillRequestProcessor: SkillRequestProcessor

    @Mock
    lateinit var store: ShowEpisodeStoreDao

    @Mock
    lateinit var morningShowEpisodeDao: MorningShowEpisodeDao

    lateinit var showService: ShowService

    @BeforeEach
    fun setUp() {
        val metricRegistry = MetricRegistry()
        val executorsFactory = ExecutorsFactory(metricRegistry, RequestContext(), DialogovoRequestContext())
        val executorFetchService = executorsFactory.fixedThreadPool(60, 1000000, "test")

        showService = ShowServiceImpl(
            showProvider, skillRequestProcessor, store, executorFetchService, ShowConfig(
                mock(),
                listOf(), 10000, 10000, 1000
            ), metricRegistry, morningShowEpisodeDao
        )

        Mockito.`when`(skillRequestProcessor.process(any(), Mockito.argThat { req -> req.clientInfo.uuid == userId }))
            .thenReturn(skillProcessPersonalizedEpisodeResult)
        Mockito.`when`(skillRequestProcessor.process(any(), Mockito.argThat { req -> req.clientInfo.uuid == DEFAULT_UUID }))
            .thenReturn(skillProcessUnpersonalizedEpisodeResult)
    }

    @Test
    fun updateUnpersonalizedShowsTest() {
        Mockito.`when`(showProvider.getActiveShowSkills(showType)).thenReturn(listOf(showInfo))
        Mockito.`when`(showProvider.getShowFeedBySkillId(skillId, showType)).thenReturn(Optional.of(showInfo))
        Mockito.`when`(morningShowEpisodeDao.getUnpersonalizedEpisode(skillId, null)).thenReturn(unpersonalizedEpisode)

        showService.updateUnpersonalizedShows(showType)
        Mockito.verify(morningShowEpisodeDao, times(1)).storeEpisodeAsync(unpersonalizedEpisode)

        val actual = showService.getRelevantEpisode(showType, skillId, ClientInfo(uuid = userId))
        Assertions.assertTrue(actual.isPresent)
        Assertions.assertEquals(unpersonalizedEpisode.toShowEpisodeEntity(), actual.get())
    }

    @Test
    fun getPersonalizedShowConfigAndStartPrepareTest() {
        Mockito.`when`(showProvider.getActivePersonalizedShowSkills(showType)).thenReturn(listOf(showInfo))
        Mockito.`when`(showProvider.getShowFeedBySkillId(skillId, showType)).thenReturn(Optional.of(showInfo))

        val actual = showService.getPersonalizedShowConfigAndStartPrepare(
            showType,
            mutableListOf(skillId),
            ClientInfo(uuid = userId)
        )
        Assertions.assertEquals(listOf(skillId), actual)
        Thread.sleep(1000) //Wait for async episodes prepare
        Mockito.verify(morningShowEpisodeDao, times(1)).storeEpisodeAsync(personalizedEpisode)
    }

    @Test
    fun getPersonalizedShowConfigAndStartPrepareTest_DontPullNewEpisodeIfValidExists() {
        Mockito.`when`(showProvider.getActivePersonalizedShowSkills(showType)).thenReturn(listOf(showInfo))
        Mockito.`when`(showProvider.getShowFeedBySkillId(skillId, showType)).thenReturn(Optional.of(showInfo))

        Mockito.`when`(morningShowEpisodeDao.getEpisode(skillId, userId, null))
            .thenReturn(personalizedEpisode.copy(validUntil = Instant.MAX))

        val actual = showService.getPersonalizedShowConfigAndStartPrepare(
            showType,
            mutableListOf(skillId),
            ClientInfo(uuid = userId)
        )
        Assertions.assertEquals(listOf(skillId), actual)
        Thread.sleep(1000) //Wait for async episodes prepare
        Mockito.verify(morningShowEpisodeDao, times(0)).storeEpisodeAsync(any())
    }

    @Test
    fun getPersonalizedShowConfigAndStartPrepareTest_PullNewEpisodeIfExistingExpireSoon() {
        Mockito.`when`(showProvider.getActivePersonalizedShowSkills(showType)).thenReturn(listOf(showInfo))
        Mockito.`when`(showProvider.getShowFeedBySkillId(skillId, showType)).thenReturn(Optional.of(showInfo))

        Mockito.`when`(morningShowEpisodeDao.getEpisode(skillId, userId, null)).thenReturn(
            personalizedEpisode.copy(
                validUntil = now.plus(episode_validation_threshold).minusSeconds(1)
            )
        )

        val actual = showService.getPersonalizedShowConfigAndStartPrepare(
            showType,
            mutableListOf(skillId),
            ClientInfo(uuid = userId)
        )
        Assertions.assertEquals(listOf(skillId), actual)
        Thread.sleep(1000) //Wait for async episodes prepare
        Mockito.verify(morningShowEpisodeDao, times(1)).storeEpisodeAsync(personalizedEpisode)
    }

    @Test
    fun getPersonalizedShowConfigAndStartPrepareTest_FilterOutNonActiveShows() {
        val skillInfo = TestSkills.cityGameSkill()
        val skillId = skillInfo.id

        Mockito.`when`(showProvider.getActivePersonalizedShowSkills(showType)).thenReturn(emptyList())

        val actual = showService.getPersonalizedShowConfigAndStartPrepare(
            showType,
            mutableListOf(skillId),
            ClientInfo(uuid = userId)
        )
        Assertions.assertEquals(emptyList<SkillInfo>(), actual)
    }

    @Test
    fun getRelevantEpisodeTest_PersonalizedExists() {
        Mockito.`when`(showProvider.getActivePersonalizedShowSkills(showType)).thenReturn(listOf(showInfo))
        Mockito.`when`(showProvider.getShowFeedBySkillId(skillId, showType)).thenReturn(Optional.of(showInfo))
        Mockito.`when`(morningShowEpisodeDao.getEpisode(skillId, userId, null)).thenReturn(personalizedEpisode)

        val actual = showService.getRelevantEpisode(showType, skillId, ClientInfo(uuid = userId))
        Assertions.assertTrue(actual.isPresent)
        Assertions.assertEquals(personalizedEpisode.toShowEpisodeEntity(), actual.get())
    }

    @Test
    fun getRelevantEpisodeTest_OnlyUnpersonalizedExists() {
        Mockito.`when`(showProvider.getActivePersonalizedShowSkills(showType)).thenReturn(listOf(showInfo))
        Mockito.`when`(showProvider.getShowFeedBySkillId(skillId, showType)).thenReturn(Optional.of(showInfo))

        Mockito.`when`(morningShowEpisodeDao.getEpisode(skillId, userId, null)).thenReturn(null)
        Mockito.`when`(morningShowEpisodeDao.getUnpersonalizedEpisode(skillId, null)).thenReturn(unpersonalizedEpisode)

        val actual = showService.getRelevantEpisode(showType, skillId, ClientInfo(uuid = userId))
        Assertions.assertTrue(actual.isPresent)
        Assertions.assertEquals(unpersonalizedEpisode.toShowEpisodeEntity(), actual.get())
    }

    companion object {
        private const val DEFAULT_UUID = "4E148F90712F1EC2EE67301BCF579E4E4E256ED91B789947C22E2EC3FA7CA98D"
        private val showType = ShowType.MORNING
        private const val episodeId = "episode-id"
        private const val personalizedEpisodeText = "personalized-episode-text"
        private const val unpersonalizedEpisodeText = "unpersonalized-episode-text"
        private const val personalizedEpisodeTts = "personalized-episode-tts"
        private const val unpersonalizedEpisodeTts = "unpersonalized-episode-tts"
        private const val userId = "user-uuid"
        private val skillInfo = TestSkills.cityGameSkill()
        private val skillId = skillInfo.id
        private val now = Instant.now()
        private val publicationDate = now
        private val validUntil = publicationDate.plusSeconds(9999)
        private val showInfo = ShowInfo(
            episodeId, "name", null, "description", skillInfo,
            showType, onAir = true, personalizationEnabled = true
        )
        private val unpersonalizedEpisode = MorningShowEpisodeEntity(
            episodeId,
            skillId,
            null,
            unpersonalizedEpisodeText,
            unpersonalizedEpisodeTts,
            publicationDate,
            validUntil
        )
        private val personalizedEpisode = MorningShowEpisodeEntity(
            episodeId,
            skillId,
            userId,
            personalizedEpisodeText,
            personalizedEpisodeTts,
            publicationDate,
            validUntil
        )

        private val skillProcessResultBuilder = SkillProcessResult.builder(
            response = null,
            skill = skillInfo,
            webhookRequest = null
        ).showEpisode(
            Optional.of(
                ShowItemMeta(
                    episodeId,
                    null,
                    null,
                    publicationDate,
                    validUntil
                )
            )
        )

        private val skillProcessPersonalizedEpisodeResult = skillProcessResultBuilder.apply {
            getLayout().cards(listOf(TextCard(personalizedEpisodeText)))
        }.setTts(personalizedEpisodeTts).build()

        private val skillProcessUnpersonalizedEpisodeResult = skillProcessResultBuilder.apply {
            getLayout().cards(listOf(TextCard(unpersonalizedEpisodeText)))
        }.setTts(unpersonalizedEpisodeTts).build()

        private val episode_validation_threshold = Duration.ofHours(1)
    }
}
