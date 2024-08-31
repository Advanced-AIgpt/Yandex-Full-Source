package ru.yandex.alice.paskill.dialogovo.service.show

import com.yandex.ydb.core.Status
import ru.yandex.alice.paskill.dialogovo.domain.show.MorningShowEpisodeEntity
import java.util.concurrent.CompletableFuture

interface MorningShowEpisodeDao {
    fun storeEpisodeAsync(episode: MorningShowEpisodeEntity): CompletableFuture<out Status>

    fun getUnpersonalizedEpisode(
        skillId: String,
        episodeId: String? = null
    ) = getEpisode(skillId, null, episodeId)

    fun getEpisode(
        skillId: String,
        userId: String? = null,
        episodeId: String? = null
    ): MorningShowEpisodeEntity?
}
