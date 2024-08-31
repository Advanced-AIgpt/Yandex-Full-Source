package ru.yandex.alice.paskill.dialogovo.providers.skill

import org.springframework.data.jdbc.repository.query.Query
import org.springframework.data.repository.Repository
import org.springframework.transaction.annotation.Transactional
import java.util.UUID

@Transactional(readOnly = true)
interface ShowFeedDao : Repository<ShowInfoDB, UUID> {
    @Query(
        value = """
            SELECT
            sf.id
            , sf."skillId" as skill_id
            , sf.name
            , sf."nameTts" as name_tts
            , sf.description
            , sf."onAir" as on_air
            , upper(sf."type"::text) as type
        FROM
            "showFeeds" as sf
        INNER JOIN skills on skills.id = sf."skillId"
        WHERE sf."onAir" and skills."onAir" and skills."deletedAt" is null
        ORDER BY
            skills."createdAt" ASC
        """
    )
    fun findAllActiveShowFeeds(): List<ShowInfoDB>
}
