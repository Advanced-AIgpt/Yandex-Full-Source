package ru.yandex.alice.paskill.dialogovo.providers.skill

import org.springframework.data.annotation.Id
import org.springframework.data.relational.core.mapping.Table
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType
import java.util.UUID

@Table("showFeeds")
data class ShowInfoDB(
    @field:Id val id: UUID,
    val skillId: UUID,
    val name: String,
    val nameTts: String?,
    val description: String,
    val onAir: Boolean,
    val type: ShowType
)
