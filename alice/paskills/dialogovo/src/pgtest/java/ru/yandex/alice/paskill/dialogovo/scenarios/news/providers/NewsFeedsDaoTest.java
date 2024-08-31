package ru.yandex.alice.paskill.dialogovo.scenarios.news.providers;

import java.sql.SQLException;
import java.sql.Timestamp;
import java.time.Instant;
import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.UUID;

import com.fasterxml.jackson.core.JsonProcessingException;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.JdbcTemplate;

import ru.yandex.alice.paskill.dialogovo.domain.Channel;
import ru.yandex.alice.paskill.dialogovo.domain.DeveloperType;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillInfoDB;
import ru.yandex.alice.paskill.dialogovo.test.BaseDatabaseTest;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class NewsFeedsDaoTest extends BaseDatabaseTest {

    @Autowired
    private NewsFeedsDao newsFeedsDao;
    @Autowired
    private JdbcTemplate jdbcTemplate;

    @Test
    void findFeedByIdEmpty() {
        assertFalse(newsFeedsDao.findFeedById(UUID.randomUUID()).isPresent());
    }

    @Test
    void findFeedById() throws JsonProcessingException, SQLException {
        var feedId = UUID.randomUUID();
        var skillId = createSkill();

        jdbcTemplate.update("INSERT INTO public.\"newsFeeds\"(id, \"skillId\", \"createdAt\", \"updatedAt\", " +
                        "preamble, name, description, topic, \"iconUrl\", enabled) VALUES " +
                        "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                feedId,
                skillId,
                Timestamp.from(Instant.now()),
                Timestamp.from(Instant.now()),
                "Новости от коммерсант ФМ",
                "Новости от коммерсант ФМ",
                "Новости от коммерсант ФМ",
                "разное",
                null,
                true
        );

        Optional<NewsFeedDB> feedO = newsFeedsDao.findFeedById(feedId);
        assertTrue(feedO.isPresent());

        assertEquals(feedO.get(),
                new NewsFeedDB(
                        feedId.toString(),
                        skillId.toString(),
                        "Новости от коммерсант ФМ",
                        "Новости от коммерсант ФМ",
                        "разное",
                        true
                ));
    }

    @Test
    void findAllEnabled() throws JsonProcessingException, SQLException {
        var feedId = UUID.randomUUID();
        var skillId = createSkill();

        jdbcTemplate.update("INSERT INTO public.\"newsFeeds\"(id, \"skillId\", \"createdAt\", \"updatedAt\", " +
                        "preamble, name, description, topic, \"iconUrl\", enabled) VALUES " +
                        "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                feedId,
                skillId,
                Timestamp.from(Instant.now()),
                Timestamp.from(Instant.now()),
                "Новости от коммерсант ФМ",
                "Новости от коммерсант ФМ",
                "Новости от коммерсант ФМ",
                "разное",
                null,
                true
        );

        List<NewsFeedDB> feeds = newsFeedsDao.findAllEnabled();
        assertFalse(feeds.isEmpty());

        assertEquals(feeds.get(0),
                new NewsFeedDB(
                        feedId.toString(),
                        skillId.toString(),
                        "Новости от коммерсант ФМ",
                        "Новости от коммерсант ФМ",
                        "разное",
                        true
                ));
    }

    private UUID createSkill() throws JsonProcessingException, SQLException {
        var logoId = UUID.randomUUID();
        var salt = UUID.randomUUID();
        var userId = "123";

        var skillId = UUID.randomUUID();

        insertUser(userId, "username");
        insertSkillLogo(skillId, logoId, "https://avatars.mdst.yandex.net/get-dialogs/5182/9c9a06f5c3c5e5246ac9/orig");

        var skillFeatureFlags = List.of("test_flag", "test_flag2");
        var inflectedActivationPhrases = List.of("ботинок", "ботинка");
        insertSkill(
                skillId,
                salt,
                salt,
                userId,
                "test skill",
                "slug",
                true,
                new SkillInfoDB.BackendSettings("http://localhost/testskill", null),
                BaseDatabaseTest.EMPTY_PUBLISHING_SETTINGS,
                logoId,
                skillFeatureFlags,
                Channel.ALICE_NEWS_SKILL,
                DeveloperType.External,
                false,
                null,
                true,
                inflectedActivationPhrases,
                null,
                1.0,
                Collections.emptyList(),
                null,
                true,
                List.of(),
                null
        );
        return skillId;
    }
}
