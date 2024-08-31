package ru.yandex.alice.paskill.dialogovo.service.show

import com.yandex.ydb.core.Result
import com.yandex.ydb.core.Status
import com.yandex.ydb.table.Session
import com.yandex.ydb.table.query.DataQueryResult
import com.yandex.ydb.table.query.Params
import com.yandex.ydb.table.result.ResultSetReader
import com.yandex.ydb.table.transaction.TxControl
import com.yandex.ydb.table.values.PrimitiveValue
import org.apache.logging.log4j.LogManager
import ru.yandex.alice.paskill.dialogovo.domain.show.MorningShowEpisodeEntity
import ru.yandex.alice.paskill.dialogovo.ydb.YdbClient
import ru.yandex.alice.paskills.common.solomon.utils.Instrument
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.nio.charset.StandardCharsets
import java.time.Duration
import java.time.Instant
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit
import java.util.function.Function

private const val SELECT_NEWEST_PERSONALIZED_EPISODE_BY_SKILL_AND_USER_ID = """--!syntax_v1
                    DECLARE ${"$"}skillId AS String;
                    DECLARE ${"$"}userId AS String;

                    ${"$"}hashedUserId = Digest::CityHash(${"$"}userId);

                    SELECT *
                    FROM morning_show_personalized_episodes
                    WHERE user_id_uuid_hash == ${"$"}hashedUserId
                    AND user_id == ${"$"}userId
                    AND skill_id == ${"$"}skillId
                    AND CurrentUtcTimestamp() < valid_until
                    ORDER BY publication_date DESC LIMIT 1;"""

private const val SELECT_NEWEST_UNPERSONALIZED_EPISODE_BY_SKILL_ID = """--!syntax_v1
                    DECLARE ${"$"}skillId AS String;

                    ${"$"}hashedSkillId = Digest::CityHash(${"$"}skillId);

                    SELECT *
                    FROM morning_show_unpersonalized_episodes
                    WHERE skill_id_uuid_hash == ${"$"}hashedSkillId
                    AND skill_id == ${"$"}skillId
                    AND CurrentUtcTimestamp() < valid_until
                    ORDER BY publication_date DESC LIMIT 1;"""

private const val SELECT_NEWEST_PERSONALIZED_EPISODE_BY_SKILL_USER_AND_EPISODE_ID = """--!syntax_v1
                    DECLARE ${"$"}skillId AS String;
                    DECLARE ${"$"}userId AS String;
                    DECLARE ${"$"}episodeId AS String;

                    ${"$"}hashedUserId = Digest::CityHash(${"$"}userId);

                    SELECT *
                    FROM morning_show_personalized_episodes
                    WHERE user_id_uuid_hash == ${"$"}hashedUserId
                    AND user_id == ${"$"}userId
                    AND skill_id == ${"$"}skillId
                    AND episode_id == ${"$"}episodeId
                    AND CurrentUtcTimestamp() < valid_until;"""

private const val SELECT_NEWEST_UNPERSONALIZED_EPISODE_BY_SKILL_AND_EPISODE_ID = """--!syntax_v1
                    DECLARE ${"$"}skillId AS String;
                    DECLARE ${"$"}episodeId AS String;

                    ${"$"}hashedSkillId = Digest::CityHash(${"$"}skillId);

                    SELECT *
                    FROM morning_show_unpersonalized_episodes
                    WHERE skill_id_uuid_hash == ${"$"}hashedSkillId
                    AND skill_id == ${"$"}skillId
                    AND episode_id == ${"$"}episodeId
                    AND CurrentUtcTimestamp() < valid_until;"""

private const val REPLACE_PERSONALIZED_EPISODE = """--!syntax_v1
                    DECLARE ${"$"}episodeId AS String;
                    DECLARE ${"$"}skillId AS String;
                    DECLARE ${"$"}userId AS String;
                    DECLARE ${"$"}text AS String;
                    DECLARE ${"$"}tts AS String;
                    DECLARE ${"$"}publicationDate AS Timestamp;
                    DECLARE ${"$"}validUntil AS Timestamp;

                    ${"$"}hashedUserId = Digest::CityHash(${"$"}userId);

                    REPLACE INTO morning_show_personalized_episodes (
                        user_id_uuid_hash, user_id, skill_id, episode_id,
                        text, tts, publication_date, valid_until
                    ) VALUES(
                        ${"$"}hashedUserId, ${"$"}userId, ${"$"}skillId, ${"$"}episodeId,
                        ${"$"}text, ${"$"}tts, ${"$"}publicationDate, ${"$"}validUntil
                    );
                """

private const val REPLACE_UNPERSONALIZED_EPISODE = """--!syntax_v1
                    DECLARE ${"$"}episodeId AS String;
                    DECLARE ${"$"}skillId AS String;
                    DECLARE ${"$"}text AS String;
                    DECLARE ${"$"}tts AS String;
                    DECLARE ${"$"}publicationDate AS Timestamp;
                    DECLARE ${"$"}validUntil AS Timestamp;

                    ${"$"}hashedSkillId = Digest::CityHash(${"$"}skillId);

                    REPLACE INTO morning_show_unpersonalized_episodes (
                        skill_id_uuid_hash, skill_id, episode_id,
                        text, tts, publication_date, valid_until
                    ) VALUES(
                        ${"$"}hashedSkillId, ${"$"}skillId, ${"$"}episodeId,
                        ${"$"}text, ${"$"}tts, ${"$"}publicationDate, ${"$"}validUntil
                    );
                """

internal class MorningShowEpisodeDaoImpl(
    private val ydbClient: YdbClient,
    metricRegistry: MetricRegistry
) : MorningShowEpisodeDao {
    private val morningShowEpisodeYdbEntityRowMapper: MorningShowEpisodeYdbEntityRowMapper
    private val storeMorningShowEpisodeInstrument: Instrument
    private val getMorningShowEpisodeInstrument: Instrument

    fun prepareQueries() = ydbClient.warmUpSessionPool(
        listOf(
            SELECT_NEWEST_PERSONALIZED_EPISODE_BY_SKILL_AND_USER_ID,
            SELECT_NEWEST_PERSONALIZED_EPISODE_BY_SKILL_USER_AND_EPISODE_ID,
            SELECT_NEWEST_UNPERSONALIZED_EPISODE_BY_SKILL_ID,
            SELECT_NEWEST_UNPERSONALIZED_EPISODE_BY_SKILL_AND_EPISODE_ID,
            REPLACE_PERSONALIZED_EPISODE,
            REPLACE_UNPERSONALIZED_EPISODE
        )
    )

    override fun storeEpisodeAsync(episode: MorningShowEpisodeEntity): CompletableFuture<out Status> {
        logger.debug("Storing morning show episode: {}", episode)
        val maxValidUntil = Instant.now().plus(Duration.ofDays(7))

        val txControl = TxControl.serializableRw()
        val episodeFuture = storeMorningShowEpisodeInstrument.timeFuture {
            ydbClient.executeAsync("content.store-morning-show-episode") { session: Session ->
                insertEpisode(
                    session,
                    txControl,
                    if (episode.validUntil.isBefore(maxValidUntil)) episode else episode.apply {
                        logger.debug(
                            "Episode has too big validUntil value. Setting validUntil to maximum valid value: {}\nEpisode: {}",
                            maxValidUntil, episode
                        )
                    }.copy(validUntil = maxValidUntil)
                )
            }.orTimeout(TIMEOUT.toMillis(), TimeUnit.MILLISECONDS)
        } ?: CompletableFuture.completedFuture(null)

        return episodeFuture.whenComplete { _, ex: Throwable? ->
            ex?.let { logger.error("Error on storing morning show episode", ex) }
                ?: logger.debug("Morning show episode stored")
        }
    }

    private fun insertEpisode(
        ydbSession: Session,
        txControl: TxControl<*>,
        episode: MorningShowEpisodeEntity
    ): CompletableFuture<Result<Status>> {
        val params = Params.create().apply {
            put("\$episodeId", PrimitiveValue.string(episode.episodeId.toByteArray()))
            put("\$skillId", PrimitiveValue.string(episode.skillId.toByteArray()))
            put("\$text", PrimitiveValue.string(episode.text.toByteArray()))
            put("\$tts", PrimitiveValue.string(episode.tts.toByteArray()))
            put("\$publicationDate", PrimitiveValue.timestamp(episode.publicationDate))
            put("\$validUntil", PrimitiveValue.timestamp(episode.validUntil))
            episode.userId?.let {
                put("\$userId", PrimitiveValue.string(it.toByteArray()))
            }
        }
        return ydbSession.executeDataQuery(
            episode.userId?.let { REPLACE_PERSONALIZED_EPISODE } ?: REPLACE_UNPERSONALIZED_EPISODE,
            txControl,
            params,
            ydbClient.keepInQueryCache()
        ).thenApply { it.map { _ -> it.toStatus() } }
    }

    override fun getEpisode(
        skillId: String,
        userId: String?,
        episodeId: String?
    ): MorningShowEpisodeEntity? {
        logger.debug(
            "Getting morning show episode by userId: {}, skillId: {}, episodeId: {}",
            userId,
            skillId,
            episodeId
        )

        val params = Params.create().apply {
            put("\$skillId", PrimitiveValue.string(skillId.toByteArray()))
            userId?.let {
                put("\$userId", PrimitiveValue.string(it.toByteArray()))
            }
            episodeId?.let {
                put("\$episodeId", PrimitiveValue.string(it.toByteArray()))
            }
        }

        val txControl = TxControl.onlineRo()
        val query = if (episodeId == null) {
            userId?.let { SELECT_NEWEST_PERSONALIZED_EPISODE_BY_SKILL_AND_USER_ID }
                ?: SELECT_NEWEST_UNPERSONALIZED_EPISODE_BY_SKILL_ID
        } else {
            userId?.let { SELECT_NEWEST_PERSONALIZED_EPISODE_BY_SKILL_USER_AND_EPISODE_ID }
                ?: SELECT_NEWEST_UNPERSONALIZED_EPISODE_BY_SKILL_AND_EPISODE_ID
        }

        val dataQueryResult = getMorningShowEpisodeInstrument.time<DataQueryResult> {
            ydbClient.execute("content.get-morning-show-episode", TIMEOUT) { session ->
                session.executeDataQuery(query, txControl, params, ydbClient.keepInQueryCache())
            }
        }

        return ydbClient.readFirstResultSet(dataQueryResult, morningShowEpisodeYdbEntityRowMapper).firstOrNull()
    }

    private inner class MorningShowEpisodeYdbEntityRowMapper : Function<ResultSetReader, MorningShowEpisodeEntity> {
        override fun apply(reader: ResultSetReader) = MorningShowEpisodeEntity(
            reader.getColumn("episode_id").getString(StandardCharsets.UTF_8),
            reader.getColumn("skill_id").getString(StandardCharsets.UTF_8),
            if (reader.getColumnIndex("user_id") != -1)
                reader.getColumn("user_id").getString(StandardCharsets.UTF_8)
            else null,
            reader.getColumn("text").getString(StandardCharsets.UTF_8),
            reader.getColumn("tts").getString(StandardCharsets.UTF_8),
            reader.getColumn("publication_date").timestamp,
            reader.getColumn("valid_until").timestamp
        )
    }

    companion object {
        private val logger = LogManager.getLogger()
        private val TIMEOUT = Duration.ofMillis(500)
    }

    init {
        val registry = NamedSensorsRegistry(metricRegistry, "ydb")
            .withLabels(Labels.of("target", "morning-show"))
        storeMorningShowEpisodeInstrument = registry.instrument("storeMorningShowEpisode")
        getMorningShowEpisodeInstrument = registry.instrument("getMorningShowEpisode")
        morningShowEpisodeYdbEntityRowMapper = MorningShowEpisodeYdbEntityRowMapper()
    }
}
