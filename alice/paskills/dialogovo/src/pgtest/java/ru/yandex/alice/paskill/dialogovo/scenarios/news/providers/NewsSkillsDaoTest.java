package ru.yandex.alice.paskill.dialogovo.scenarios.news.providers;

import java.sql.Timestamp;
import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.stream.Collectors;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.JdbcTemplate;

import ru.yandex.alice.kronstadt.core.domain.Voice;
import ru.yandex.alice.paskill.dialogovo.domain.Channel;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillIdPhrasesEntity;
import ru.yandex.alice.paskill.dialogovo.test.BaseDatabaseTest;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;

class NewsSkillsDaoTest extends BaseDatabaseTest {

    @Autowired
    private NewsSkillsDao newskillsDao;
    @Autowired
    private JdbcTemplate jdbcTemplate;
    @Autowired
    private ObjectMapper objectMapper;

    @Test
    void testFindNothing() {
        assertFalse(newskillsDao.findAliceNewsSkillById(UUID.randomUUID()).isPresent());
    }

    @Test
    void findNewsSkillByActivationPhrases() throws JsonProcessingException {

        var uuid = UUID.randomUUID();
        var logoId = UUID.randomUUID();

        UUID salt = UUID.randomUUID();
        var userId = "123";

        jdbcTemplate.update("insert into users (id, name, \"createdAt\", \"updatedAt\", \"featureFlags\") values (?, " +
                        "?, now(), now(), '{\"abc\": 123}'::jsonb)",
                userId, "username");

        jdbcTemplate.update("insert into images " +
                        "(id, \"skillId\", \"url\", \"type\", \"size\", \"createdAt\", \"updatedAt\") " +
                        "values " +
                        "(?, ?, ?, ?::enum_images_type, 10, now(), now())",
                logoId, uuid, "https://avatars.mdst.yandex.net/get-dialogs/5182/9c9a06f5c3c5e5246ac9/orig",
                "skillSettings");

        UUID newsSkillId = UUID.randomUUID();
        jdbcTemplate.update("insert into skills (id, \"createdAt\", \"updatedAt\", salt, \"userId\", name, slug, " +
                        "\"onAir\"," +
                        " \"backendSettings\", \"logoId\", \"featureFlags\", \"inflectedActivationPhrases\", " +
                        "\"useStateStorage\", \"rsyPlatformId\",  channel) " +
                        "values (?, ?, ?, ?, ?, ?, ?, ?, (?)::json, ?, '{test_flag, test_flag2}', '{коммерсант, " +
                        "коммерсанту}', " +
                        "false, ?, (?)::enum_skills_channel)",
                newsSkillId,
                Timestamp.from(Instant.now()),
                Timestamp.from(Instant.now()),
                salt, userId, "test skill", "test-skill", true,
                objectMapper.writeValueAsString(new NewsSkillInfoDB.BackendSettings(null)),
                logoId,
                "rsyId",
                Channel.ALICE_NEWS_SKILL.getValue()
        );

        UUID aliceSkillId = UUID.randomUUID();
        jdbcTemplate.update("insert into skills (id, \"createdAt\", \"updatedAt\", salt, \"userId\", name, slug, " +
                        "\"onAir\"," +
                        " \"backendSettings\", \"logoId\", \"featureFlags\", \"inflectedActivationPhrases\", " +
                        "\"useStateStorage\", \"rsyPlatformId\",  channel) " +
                        "values (?, ?, ?, ?, ?, ?, ?, ?, (?)::json, ?, '{test_flag, test_flag2}', '{коммерсант, " +
                        "коммерсанту}', " +
                        "false, ?, (?)::enum_skills_channel)",
                aliceSkillId,
                Timestamp.from(Instant.now()),
                Timestamp.from(Instant.now()),
                salt, userId, "test skill", "test-skill", true,
                objectMapper.writeValueAsString(new NewsSkillInfoDB.BackendSettings(null)),
                logoId,
                "rsyId",
                Channel.ALICE_SKILL.getValue()
        );

        NewsSkillInfoDB newsSkill =
                newskillsDao.findAliceNewsSkillById(newsSkillId).orElseThrow(() -> new RuntimeException(
                        "skill not found"));

        assertEquals(newsSkillId, newsSkill.getId());

        List<NewsSkillInfoDB> allSkills = newskillsDao.findAllActiveAliceNewsSkill();

        assertEquals(
                List.of(newsSkill).stream().map(NewsSkillInfoDB::getId).collect(Collectors.toSet()),
                allSkills.stream().map(NewsSkillInfoDB::getId).collect(Collectors.toSet()));

        List<SkillIdPhrasesEntity> newsSkillByActivationPhrases = newskillsDao.findByActivationPhrases(new String[]{
                "коммерсант", "ящик",
                "красный шар"});

        assertEquals(List.of(new SkillIdPhrasesEntity(newsSkillId, List.of("коммерсант", "коммерсанту"))),
                newsSkillByActivationPhrases);
    }

    @Test
    void testQueryPhrases() {
        assertEquals(List.of(), newskillsDao.findByActivationPhrases(new String[]{"ботинок"}));
    }

    @Test
    void testSkillEquals() throws JsonProcessingException {

        var uuid = UUID.randomUUID();
        var logoId = UUID.randomUUID();

        UUID salt = UUID.randomUUID();
        var userId = "123";
        String slug = "slug";
        String name = "test skill";
        UUID skillId = UUID.randomUUID();

        jdbcTemplate.update("insert into users (id, name, \"createdAt\", \"updatedAt\", \"featureFlags\") values (?, " +
                        "?, now(), now(), '{\"abc\": 123}'::jsonb)",
                userId, "username");

        String logoUrl = "https://avatars.mdst.yandex.net/get-dialogs/5182/9c9a06f5c3c5e5246ac9/orig";
        jdbcTemplate.update("insert into images " +
                        "(id, \"skillId\", \"url\", \"type\", \"size\", \"createdAt\", \"updatedAt\") " +
                        "values " +
                        "(?, ?, ?, ?::enum_images_type, 10, now(), now())",
                logoId, uuid, logoUrl, "skillSettings");

        jdbcTemplate.update("insert into skills (id, \"createdAt\", \"updatedAt\", \"persistentUserIdSalt\", salt, " +
                        "\"userId\", name, \"onAir\", \"backendSettings\", \"logoId\", \"featureFlags\", " +
                        "\"requiredInterfaces\",  \"isRecommended\", slug, \"automaticIsRecommended\",  " +
                        "\"hideInStore\", channel, \"inflectedActivationPhrases\") " +
                        "values (?, ?, ?, ?, ?, ?, ?, ?, (?)::json, ?, '{test_flag, test_flag2}', '{screen, " +
                        "browser}', ?, ?, ?, ?, (?)::enum_skills_channel, '{коммерсант, коммерсанту}')",
                skillId,
                Timestamp.from(Instant.now()),
                Timestamp.from(Instant.now()),
                salt,
                salt,
                userId,
                name,
                true,
                objectMapper.writeValueAsString(new NewsSkillInfoDB.BackendSettings(null)),
                logoId,
                true,
                slug,
                true,
                false,
                Channel.ALICE_NEWS_SKILL.getValue()
        );

        NewsSkillInfoDB skill = NewsSkillInfoDB.builder()
                .id(skillId)
                .name(name)
                .onAir(true)
                .logoUrl(logoUrl)
                .isRecommended(true)
                .automaticIsRecommended(true)
                .hideInStore(false)
                .slug(slug)
                .voice(Voice.OKSANA.getCode())
                .backendSettings(new NewsSkillInfoDB.BackendSettings(null))
                .channel("aliceNewsSkill")
                .userFeatureFlags(new NewsSkillInfoDB.UserFeatures(Map.of("abc", 123)))
                .featureFlags(List.of("test_flag", "test_flag2"))
                .inflectedActivationPhrases(List.of("коммерсант", "коммерсанту"))
                .build();

        NewsSkillInfoDB actual =
                newskillsDao.findAliceNewsSkillById(skillId).orElseThrow(() -> new RuntimeException(
                        "skill not " +
                                "found"));

        assertEquals(skill, actual);
    }
}
