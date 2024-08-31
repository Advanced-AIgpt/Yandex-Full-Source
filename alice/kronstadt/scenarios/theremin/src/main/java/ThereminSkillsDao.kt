package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import org.springframework.data.jdbc.repository.query.Query
import org.springframework.data.repository.Repository
import org.springframework.data.repository.query.Param
import org.springframework.transaction.annotation.Transactional
import java.util.UUID

@Transactional(readOnly = true)
interface ThereminSkillsDao : Repository<ThereminSkillInfoDB, UUID> {
    @Query(
        """SELECT id
        ,"onAir" as on_air
        ,"userId" as user_id
        ,name
        ,"inflectedActivationPhrases" as inflected_activation_phrases
        ,"backendSettings" as backend_settings
        ,"developerType" as developer_type
        ,"hideInStore" as hide_in_store
        FROM skills s
        WHERE s."onAir" = true AND channel = 'thereminvox' AND "deletedAt" is null AND "userId" = :uid
        """
    )
    fun findThereminPrivateSkillsByUser(@Param("uid") uid: String): List<ThereminSkillInfoDB>

    @Query(
        """SELECT id
        ,"onAir" as on_air
        ,"userId" as user_id
        ,name
        ,"inflectedActivationPhrases" as inflected_activation_phrases
        ,"backendSettings" as backend_settings
        ,"developerType" as developer_type
        ,"hideInStore" as hide_in_store
        FROM skills s
        WHERE s."onAir" = true AND channel = 'thereminvox' AND "deletedAt" is null AND s."hideInStore" = false
        """
    )
    fun findThereminAllPublicSkills(): List<ThereminSkillInfoDB>
}
