package ru.yandex.alice.paskill.dialogovo.domain.show

import com.fasterxml.jackson.annotation.JsonInclude
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType
import ru.yandex.alice.paskill.dialogovo.service.show.ShowService.EPISODE_WILL_REMAIN_VALID_THRESHOLD
import java.time.Instant

@JsonInclude(JsonInclude.Include.NON_ABSENT)
data class ShowEpisodeEntity(
    val id: String,
    val skillId: String,
    val skillSlug: String,
    val showType: ShowType,
    val text: String,
    val tts: String,
    val publicationDate: Instant,
    val expirationDate: Instant? = null
) {
    fun toMorningShowEpisodeEntity(userId: String?) =
        MorningShowEpisodeEntity(
            id, skillId, userId, text, tts, publicationDate,
            expirationDate ?: Instant.now().plus(EPISODE_WILL_REMAIN_VALID_THRESHOLD)
        )
}
