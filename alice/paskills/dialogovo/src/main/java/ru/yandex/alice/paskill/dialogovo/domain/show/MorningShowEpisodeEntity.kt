package ru.yandex.alice.paskill.dialogovo.domain.show

import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType
import java.time.Instant

data class MorningShowEpisodeEntity(
    val episodeId: String,
    val skillId: String,
    val userId: String? = null,
    val text: String,
    val tts: String,
    val publicationDate: Instant,
    val validUntil: Instant
) {
    fun toShowEpisodeEntity() = ShowEpisodeEntity(
        id = episodeId,
        skillId = skillId,
        skillSlug = "",
        text = text,
        tts = tts,
        publicationDate = publicationDate,
        expirationDate = validUntil,
        showType = ShowType.MORNING
    )
}
