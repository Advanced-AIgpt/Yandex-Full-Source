package ru.yandex.alice.paskill.dialogovo.service.show

import org.junit.jupiter.api.AfterEach
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test
import org.junit.jupiter.params.ParameterizedTest
import org.junit.jupiter.params.provider.MethodSource
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.paskill.dialogovo.domain.show.MorningShowEpisodeEntity
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext
import ru.yandex.alice.paskill.dialogovo.utils.BaseYdbTest
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.time.Duration
import java.time.Instant
import java.time.temporal.ChronoUnit
import java.util.stream.Stream

internal class MorningShowEpisodeDaoImplTest : BaseYdbTest() {
    private lateinit var episodeDao: MorningShowEpisodeDaoImpl
    private lateinit var executorService: DialogovoInstrumentedExecutorService

    @BeforeEach
    @Throws(Exception::class)
    public override fun setUp() {
        super.setUp()
        val executorsFactory = ExecutorsFactory(MetricRegistry(), RequestContext(), DialogovoRequestContext())

        executorService = executorsFactory.fixedThreadPool(3, "test")
        episodeDao = MorningShowEpisodeDaoImpl(ydbClient, MetricRegistry())
    }

    @AfterEach
    @Throws(Exception::class)
    public override fun tearDown() {
        executorService.shutdownNow()
        super.tearDown()
    }

    @Test
    fun testGetNonExistingEpisodes() {
        Assertions.assertNull(episodeDao.getEpisode("", "", ""))
        Assertions.assertNull(episodeDao.getEpisode("", ""))
        Assertions.assertNull(episodeDao.getUnpersonalizedEpisode("", ""))
        Assertions.assertNull(episodeDao.getUnpersonalizedEpisode(""))
    }

    @Test
    fun testWarmUpDoesntFail() {
        episodeDao.prepareQueries()
    }

    @ParameterizedTest
    @MethodSource("provideEpisodes")
    fun testStoreAndGetEpisode(episode: MorningShowEpisodeEntity) {
        episodeDao.storeEpisodeAsync(episode).join()
        val actual = episodeDao.getEpisode(episode.skillId, episode.userId)
        Assertions.assertEquals(episode, actual)
    }

    @ParameterizedTest
    @MethodSource("provideEpisodes")
    fun testCantGetNonExistingEpisode(episode: MorningShowEpisodeEntity) {
        Assertions.assertNull(episodeDao.getEpisode(episode.skillId, episode.userId))
    }

    @ParameterizedTest
    @MethodSource("provideEpisodes")
    fun testCantGetNonExistingEpisodeUnknownId(episode: MorningShowEpisodeEntity) {
        episodeDao.storeEpisodeAsync(episode).join()
        Assertions.assertNull(
            episodeDao.getEpisode(
                episode.skillId,
                episode.userId,
                "unknownId"
            )
        )
    }

    @ParameterizedTest
    @MethodSource("provideEpisodes")
    fun testDontReturnExpiredEpisode(episode: MorningShowEpisodeEntity) {
        val expiredEpisode = episode.copy(validUntil = Instant.now().minusSeconds(1))
        episodeDao.storeEpisodeAsync(expiredEpisode).join()

        Assertions.assertNull(episodeDao.getEpisode(episode.skillId))
    }

    @ParameterizedTest
    @MethodSource("provideEpisodes")
    fun testStoreEpisodeReplacesExisting(episode: MorningShowEpisodeEntity) {
        val newTts = "newTts"
        episodeDao.storeEpisodeAsync(episode).join()
        val actual = episodeDao.getEpisode(episode.skillId, episode.userId)
        Assertions.assertNotNull(actual)
        Assertions.assertEquals(episode, actual!!)

        val episodeWithNewTts = episode.copy(tts = newTts)
        episodeDao.storeEpisodeAsync(episodeWithNewTts).join()
        val actualAfterReplace =
            episodeDao.getEpisode(episode.skillId, episode.userId)
        Assertions.assertNotNull(actualAfterReplace)
        Assertions.assertEquals(episodeWithNewTts, actualAfterReplace!!)
    }

    @ParameterizedTest
    @MethodSource("provideEpisodes")
    fun testGetLatestByPublicationDateEpisode(episode: MorningShowEpisodeEntity) {
        val latestEpisode = episode.copy(
            episodeId = "new_id",
            publicationDate = publicationDate.plusSeconds(1000)
        )
        episodeDao.storeEpisodeAsync(latestEpisode).join()
        episodeDao.storeEpisodeAsync(episode).join()

        val actual = episodeDao.getEpisode(episode.skillId, episode.userId)
        Assertions.assertNotNull(actual)
        Assertions.assertEquals(latestEpisode, actual!!)
    }

    @ParameterizedTest
    @MethodSource("provideEpisodes")
    fun testStoreEpisodeAsyncEpisodeTruncateValidUntilToMaxValue(episode: MorningShowEpisodeEntity) {
        val tooBigValidUntil = Instant.now().plus(maxEpisodeExpirationPeriod).plusSeconds(10)
        val invalidEpisode = episode.copy(validUntil = tooBigValidUntil)
        episodeDao.storeEpisodeAsync(invalidEpisode).join()

        val actual = episodeDao.getEpisode(invalidEpisode.skillId, invalidEpisode.userId)
        Assertions.assertNotNull(actual)
        Assertions.assertTrue(actual!!.validUntil.isBefore(tooBigValidUntil))
    }

    @Test
    fun testStoreAndGetManyEpisodes() {
        val newId = "newId"
        val newTts = "tts"
        val newUnpersonalizedEpisode = unpersonalizedEpisode.copy(episodeId = newId, tts = newTts)
        val newPersonalizedEpisode = personalizedEpisode.copy(episodeId = newId, tts = newTts)

        episodeDao.storeEpisodeAsync(unpersonalizedEpisode).join()
        episodeDao.storeEpisodeAsync(personalizedEpisode).join()
        episodeDao.storeEpisodeAsync(newUnpersonalizedEpisode).join()
        episodeDao.storeEpisodeAsync(newPersonalizedEpisode).join()

        Assertions.assertEquals(
            unpersonalizedEpisode, episodeDao.getEpisode(
                unpersonalizedEpisode.skillId,
                unpersonalizedEpisode.userId,
                unpersonalizedEpisode.episodeId
            )
        )

        Assertions.assertEquals(
            personalizedEpisode, episodeDao.getEpisode(
                personalizedEpisode.skillId,
                personalizedEpisode.userId,
                personalizedEpisode.episodeId
            )
        )

        Assertions.assertEquals(
            newUnpersonalizedEpisode, episodeDao.getEpisode(
                newUnpersonalizedEpisode.skillId,
                newUnpersonalizedEpisode.userId,
                newUnpersonalizedEpisode.episodeId
            )
        )

        Assertions.assertEquals(
            newPersonalizedEpisode, episodeDao.getEpisode(
                newPersonalizedEpisode.skillId,
                newPersonalizedEpisode.userId,
                newPersonalizedEpisode.episodeId
            )
        )
    }

    companion object {
        private val maxEpisodeExpirationPeriod = Duration.ofDays(7)

        private lateinit var publicationDate: Instant
        private lateinit var validUntil: Instant

        private lateinit var unpersonalizedEpisode: MorningShowEpisodeEntity
        private lateinit var personalizedEpisode: MorningShowEpisodeEntity

        @JvmStatic
        fun provideEpisodes(): Stream<MorningShowEpisodeEntity> {
            publicationDate = Instant.now().truncatedTo(ChronoUnit.SECONDS)
            validUntil = publicationDate.plusSeconds(100000)

            unpersonalizedEpisode = MorningShowEpisodeEntity(
                "personalized-episode-id",
                "personalized-episode-skill-id",
                "personalized-episode-user-id",
                "personalized-episode-text",
                "personalized-episode-tts",
                publicationDate,
                validUntil
            )
            personalizedEpisode =
                MorningShowEpisodeEntity(
                    "personalized-episode-id",
                    "personalized-episode-skill-id",
                    text = "personalized-episode-texts",
                    tts = "personalized-episode-tts",
                    publicationDate = publicationDate,
                    validUntil = validUntil
                )

            return Stream.of(personalizedEpisode, unpersonalizedEpisode)
        }
    }
}
