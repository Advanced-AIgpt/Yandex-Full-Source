package ru.yandex.alice.paskill.dialogovo.providers.skill

import ru.yandex.alice.paskill.dialogovo.domain.show.ShowInfo
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType
import java.util.Optional

interface ShowProvider {
    fun getShowFeed(feedId: String, showType: ShowType = ShowType.MORNING): Optional<ShowInfo>
    fun getShowFeedBySkillId(skillId: String, showType: ShowType = ShowType.MORNING): Optional<ShowInfo>
    fun getShowFeedsByIds(feedId: Collection<String>): List<ShowInfo>
    fun getActiveShowSkills(showType: ShowType): List<ShowInfo>
    fun getActivePersonalizedShowSkills(showType: ShowType): List<ShowInfo>
    fun isReady(): Boolean
}
