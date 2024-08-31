package ru.yandex.alice.paskill.dialogovo.providers.skill

import org.springframework.data.jdbc.repository.query.Query
import org.springframework.data.repository.Repository
import org.springframework.data.repository.query.Param
import org.springframework.transaction.annotation.Transactional
import java.util.Optional
import java.util.UUID

@Transactional(readOnly = true)
interface SkillsDao : Repository<SkillInfoDB, UUID> {
    @Query(
        """
SELECT
    s.id
    , s.name
    , s."userId" as user_id
    , s."nameTts" as name_tts
    , s.channel
    , s.slug
    , s."backendSettings" as backend_settings
    , s."publishingSettings" as publishing_settings
    , s."onAir" as on_air
    , s."useZora" as use_zora
    , s.salt
    , s."persistentUserIdSalt" as persistent_user_id_salt
    , s.voice
    , s.look
    , s."developerType" as developer_type
    , s."grammarsBase64" as grammars_base64
    , s."exactSurfaces" as exact_surfaces
    , s."surfaceWhitelist" as surface_whitelist
    , s."surfaceBlacklist" as surface_blacklist
    , s."requiredInterfaces" as required_interfaces
    , i.url as logo_url
    , s."featureFlags" as feature_flags
    , "useNLU" as use_nlu
    , o."socialAppName" as social_app_name
    , u."featureFlags" as user_feature_flags
    , s."appMetricaApiKey" as app_metrica_api_key
    , s."isRecommended" as is_recommended
    , s."useStateStorage" as use_state_storage
    , s."rsyPlatformId" as rsy_platform_id
    , s."automaticIsRecommended" as automatic_is_recommended
    , s."hideInStore" as hide_in_store
    , s."exposeInternalFlags" as expose_internal_flags
    , s."inflectedActivationPhrases" as inflected_activation_phrases
    , s."score" as score
    , false as draft
    , s."skillAccess" as skill_access
    , s.tags
    , s."editorDescription" as editor_description
    , (select coalesce(array_agg(sus.user_id),'{}') as shared_users
           from skill_user_share sus where sus.skill_id = s.id) as shared_access
    , sc."publicKey" as public_key
    , sc."privateKey" as private_key
    , s."safeForKids" as safe_for_kids
    , s."monitoringType" as monitoring_type
FROM
    skills s
    LEFT JOIN images i ON i.id = s."logoId" AND i."type" = 'skillSettings'
    LEFT JOIN "oauthApps" o ON o.id = s."oauthAppId"
    LEFT JOIN "users" u ON u.id = s."userId"
    LEFT JOIN "skillsCrypto" sc ON sc."skillId" = s.id
WHERE
    s.id = :id
    AND s.channel  = 'aliceSkill'"""
    )
    fun findAliceSkillById(@Param("id") id: UUID): Optional<SkillInfoDB>

    @Query(
        """
SELECT
    s.id
    , d.name
    , s."userId" as user_id
    , d."nameTts" as name_tts
    , s.channel
    , s.slug
    , d."backendSettings" as backend_settings
    , d."publishingSettings" as publishing_settings
    , true as on_air
    , s."useZora" as use_zora
    , s.salt
    , s."persistentUserIdSalt" as persistent_user_id_salt
    , d.voice
    , s.look
    , s."developerType" as developer_type
    , d."grammarsBase64" as grammars_base64
    , d."exactSurfaces" as exact_surfaces
    , d."surfaceWhitelist" as surface_whitelist
    , d."surfaceBlacklist" as surface_blacklist
    , d."requiredInterfaces" as required_interfaces
    , i.url as logo_url
    , s."featureFlags" as feature_flags
    , "useNLU" as use_nlu
    , o."socialAppName" as social_app_name
    , u."featureFlags" as user_feature_flags
    , d."appMetricaApiKey" as app_metrica_api_key
    , s."isRecommended" as is_recommended
    , d."useStateStorage" as use_state_storage
    , d."rsyPlatformId" as rsy_platform_id
    , s."automaticIsRecommended" as automatic_is_recommended
    , d."hideInStore" as hide_in_store
    , s."exposeInternalFlags" as expose_internal_flags
    , d."inflectedActivationPhrases" as inflected_activation_phrases
    , s."score" as score
    , true as draft
    , d."skillAccess" as skill_access
    , s.tags
    , s."editorDescription" as editor_description
    , (select coalesce(array_agg(sus.user_id),'{}') as shared_users
        from skill_user_share sus where sus.skill_id = s.id) as shared_access
    , sc."publicKey" as public_key
    , sc."privateKey" as private_key
    , s."safeForKids" as safe_for_kids
    , s."monitoringType" as monitoring_type
FROM
    skills s
    JOIN drafts d on d."skillId" = s.id
    LEFT JOIN images i ON i.id = d."logoId" AND i."type" = 'skillSettings'
    LEFT JOIN "oauthApps" o ON o.id = d."oauthAppId"
    LEFT JOIN "users" u ON u.id = s."userId"
    LEFT JOIN "skillsCrypto" sc ON sc."skillId" = s.id
WHERE
    s.id = :id
    AND s.channel = 'aliceSkill'"""
    )
    fun findAliceSkillDraftById(@Param("id") id: UUID): Optional<SkillInfoDB>

    @Query(
        """
SELECT
    s.id
    , s.name
    , s."userId" as user_id
    , s."nameTts" as name_tts
    , s.channel
    , s.slug
    , s."backendSettings" as backend_settings
    , s."publishingSettings" as publishing_settings
    , s."onAir" as on_air
    , s."useZora" as use_zora
    , s.salt
    , s."persistentUserIdSalt" as persistent_user_id_salt
    , s.voice
    , s.look
    , s."exactSurfaces" as exact_surfaces
    , s."surfaceWhitelist" as surface_whitelist
    , s."surfaceBlacklist" as surface_blacklist
    , s."requiredInterfaces" as required_interfaces
    , s."developerType" as developer_type
    , s."grammarsBase64" as grammars_base64
    , i.url as logo_url
    , s."featureFlags" as feature_flags
    , s."useNLU" as use_nlu
    , o."socialAppName" as social_app_name
    , u."featureFlags" as user_feature_flags
    , s."appMetricaApiKey" as app_metrica_api_key
    , s."isRecommended" as is_recommended
    , s."useStateStorage" as use_state_storage
    , s."rsyPlatformId" as rsy_platform_id
    , s."automaticIsRecommended" as automatic_is_recommended
    , s."hideInStore" as hide_in_store
    , s."exposeInternalFlags" as expose_internal_flags
    , s."inflectedActivationPhrases" as inflected_activation_phrases
    , s."score" as score
    , false as draft
    , s."skillAccess" as skill_access
    , s.tags
    , s."editorDescription" as editor_description
    , COALESCE(sus.shared_users, '{}') as shared_access
    , sc."publicKey" as public_key
    , sc."privateKey" as private_key
    , s."safeForKids" as safe_for_kids
    , s."monitoringType" as monitoring_type
FROM
    skills s
    LEFT JOIN images i ON i.id = s."logoId" AND i."type" = 'skillSettings'
    LEFT JOIN "oauthApps" o ON o.id = s."oauthAppId"
    LEFT JOIN "users" u ON u.id = s."userId"
    LEFT JOIN (select skill_id, array_agg(user_id) as shared_users
        from skill_user_share group by skill_id) sus ON s.id = sus.skill_id
    LEFT JOIN "skillsCrypto" sc ON sc."skillId" = s.id
WHERE
    s.channel = 'aliceSkill'
    AND s."onAir" = true
    AND s."deletedAt" is null"""
    )
    fun findAllActiveAliceSkill(): List<SkillInfoDB>

    // Need index ofr better performance:
    // CREATE INDEX "
    // skills_alice_inflected_activation_phrases_idx" on skills using gin ("inflectedActivationPhrases")
    // WHERE "channel" = 'aliceSkill' AND "onAir" = true AND "deletedAt" IS NULL
    @Query(
        """
SELECT
    s.id
    , s."inflectedActivationPhrases" as inflected_activation_phrases
FROM
    skills s
WHERE
    s.channel = 'aliceSkill'
    AND s."onAir" = true
    AND s."deletedAt" is null
    AND s."inflectedActivationPhrases" && (:phrases)\:\:text[]"""
    )
    fun findByActivationPhrases(@Param("phrases") phrases: Array<String>): List<SkillIdPhrasesEntity>

    @Query(
        """
SELECT
    i."skillId" as id,
    array_agg(i."formName") as form_names
FROM
    "publishedIntents" i
WHERE
    i."isActivation"
GROUP BY i."skillId""""
    )
    fun findAllActivationIntents(): List<SkillActivationIntentsEntity>
}
