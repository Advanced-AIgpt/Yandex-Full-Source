package ru.yandex.alice.paskill.dialogovo.scenarios.news.providers;

import java.util.List;
import java.util.Optional;
import java.util.UUID;

import org.springframework.data.jdbc.repository.query.Query;
import org.springframework.data.repository.Repository;
import org.springframework.data.repository.query.Param;
import org.springframework.transaction.annotation.Transactional;

import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillIdPhrasesEntity;

@Transactional(readOnly = true)
public interface NewsSkillsDao extends Repository<NewsSkillInfoDB, UUID> {
    @Query("SELECT\n" +
            "    s.id\n" +
            "    , s.name\n" +
            "    , s.\"nameTts\" as name_tts\n" +
            "    , s.slug" +
            "    , s.\"backendSettings\" as backend_settings\n" +
            "    , s.\"onAir\" as on_air\n" +
            "    , s.voice\n" +
            "    , s.channel\n" +
            "    , i.url as logo_url\n" +
            "    , s.\"featureFlags\" as feature_flags\n" +
            "    , u.\"featureFlags\" as user_feature_flags\n" +
            "    , s.\"appMetricaApiKey\" as app_metrica_api_key\n" +
            "    , s.\"isRecommended\" as is_recommended\n" +
            "    , s.\"rsyPlatformId\" as rsy_platform_id\n" +
            "    , s.\"automaticIsRecommended\" as automatic_is_recommended\n" +
            "    , s.\"inflectedActivationPhrases\" as inflected_activation_phrases\n" +
            "    , s.\"hideInStore\" as hide_in_store\n" +
            "    , false as draft\n" +
            "FROM\n" +
            "    skills s\n" +
            "    LEFT JOIN images i ON i.id = s.\"logoId\" AND i.\"type\" = 'skillSettings'\n" +
            "    LEFT JOIN \"users\" u ON u.id = s.\"userId\"\n" +
            "WHERE\n" +
            "    s.id = :id\n" +
            "    AND s.channel = 'aliceNewsSkill'")
    Optional<NewsSkillInfoDB> findAliceNewsSkillById(@Param("id") UUID id);

    @Query("SELECT\n" +
            "    s.id\n" +
            "    , s.name\n" +
            "    , s.\"nameTts\" as name_tts\n" +
            "    , s.slug" +
            "    , s.\"backendSettings\" as backend_settings\n" +
            "    , s.\"onAir\" as on_air\n" +
            "    , s.voice\n" +
            "    , s.channel\n" +
            "    , i.url as logo_url\n" +
            "    , s.\"featureFlags\" as feature_flags\n" +
            "    , u.\"featureFlags\" as user_feature_flags\n" +
            "    , s.\"appMetricaApiKey\" as app_metrica_api_key\n" +
            "    , s.\"isRecommended\" as is_recommended\n" +
            "    , s.\"rsyPlatformId\" as rsy_platform_id\n" +
            "    , s.\"automaticIsRecommended\" as automatic_is_recommended\n" +
            "    , s.\"inflectedActivationPhrases\" as inflected_activation_phrases\n" +
            "    , s.\"hideInStore\" as hide_in_store\n" +
            "    , false as draft\n" +
            "FROM\n" +
            "    skills s\n" +
            "    LEFT JOIN images i ON i.id = s.\"logoId\" AND i.\"type\" = 'skillSettings'\n" +
            "    LEFT JOIN \"users\" u ON u.id = s.\"userId\"\n" +
            "WHERE\n" +
            "    s.channel = 'aliceNewsSkill'\n" +
            "    AND s.\"onAir\" = true\n" +
            "    AND s.\"deletedAt\" is null")
    List<NewsSkillInfoDB> findAllActiveAliceNewsSkill();

    // Need index ofr better performance:
    // CREATE INDEX "
    // skills_alice_inflected_activation_phrases_idx" on skills using gin ("inflectedActivationPhrases")
    // WHERE "channel" = 'aliceSkill' AND "onAir" = true AND "deletedAt" IS NULL
    @Query("SELECT\n" +
            "    s.id\n" +
            "    , s.\"inflectedActivationPhrases\" as inflected_activation_phrases\n" +
            "FROM\n" +
            "    skills s\n" +
            "WHERE\n" +
            "    s.channel = 'aliceNewsSkill'\n" +
            "    AND s.\"onAir\" = true\n" +
            "    AND s.\"deletedAt\" is null\n" +
            "    AND s.\"inflectedActivationPhrases\" && (:phrases)\\:\\:text[]")
    List<SkillIdPhrasesEntity> findByActivationPhrases(@Param("phrases") String[] phrases);
}

