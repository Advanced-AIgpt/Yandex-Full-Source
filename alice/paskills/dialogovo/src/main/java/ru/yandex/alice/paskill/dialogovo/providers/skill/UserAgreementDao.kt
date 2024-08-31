package ru.yandex.alice.paskill.dialogovo.providers.skill

import org.springframework.data.jdbc.repository.query.Query
import org.springframework.data.repository.Repository
import org.springframework.data.repository.query.Param
import org.springframework.transaction.annotation.Transactional
import java.util.UUID

@Transactional(readOnly = true)
interface UserAgreementDao : Repository<UserAgreementDb, UUID> {
    @Query(
        """SELECT
            id
            , "skillId" as skill_id
            , "order"
            , name
            , url
        FROM
            "publishedUserAgreements"
        ORDER BY
            "skill_id"
            , "order""""
    )
    fun findAllPublishedUserAgreements(): List<UserAgreementDb>

    @Query(
        """SELECT
            id
            , "skillId" as skill_id
            , "order"
            , name
            , url
        FROM
            "draftUserAgreements"
        WHERE
            "skillId" = :skillId::uuid
        ORDER BY
            "order""""
    )
    fun findDraftUserAgreements(@Param("skillId") skillId: String): List<UserAgreementDb>
}
