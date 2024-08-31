package ru.yandex.alice.paskill.dialogovo.scenarios.news.providers;

import java.sql.Timestamp;
import java.time.Instant;
import java.util.Optional;
import java.util.UUID;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.JdbcTemplate;

import ru.yandex.alice.paskill.dialogovo.domain.Channel;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.FlashBriefingType;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.overrides.NewsSkillInflectedNamesOverrides;
import ru.yandex.alice.paskill.dialogovo.test.BaseDatabaseTest;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

class DBNewsSkillsProviderTest extends BaseDatabaseTest {

    @Autowired
    private JdbcTemplate jdbcTemplate;
    @Autowired
    private NewsSkillsDao newskillsDao;
    @Autowired
    private NewsFeedsDao newsFeedsDao;
    @Autowired
    private NewsContentsDao newsContentsDao;
    private NewsSkillInflectedNamesOverrides inflectedNamesOverrides = new NewsSkillInflectedNamesOverrides();

    private DBNewsSkillProvider newsSkillProvider;

    @BeforeEach
    void setUp() {
        newsSkillProvider = new DBNewsSkillProvider(
                newskillsDao,
                MetricRegistry.root(),
                false,
                newsFeedsDao,
                newsContentsDao,
                inflectedNamesOverrides);
    }

    @Test
    @SuppressWarnings("MethodLength")
    public void getSkill() throws JsonProcessingException {
        var skillId = UUID.randomUUID();
        var feedId1 = UUID.randomUUID();
        var feedId2 = UUID.randomUUID();
        var newsFeed1Item1 = UUID.randomUUID();
        var newsFeed1Item2 = UUID.randomUUID();
        var newsFeed2Item1 = UUID.randomUUID();
        var newsFeed2Item2 = UUID.randomUUID();
        var userId = "123";
        var uuid = UUID.randomUUID();
        var logoId = UUID.randomUUID();

        UUID salt = UUID.randomUUID();

        jdbcTemplate.update("insert into users (id, name, \"createdAt\", \"updatedAt\", \"featureFlags\") values (?, " +
                        "?, now(), now(), '{\"abc\": 123}'::jsonb)",
                userId, "username");

        jdbcTemplate.update("insert into images " +
                        "(id, \"skillId\", \"url\", \"type\", \"size\", \"createdAt\", \"updatedAt\") " +
                        "values " +
                        "(?, ?, ?, ?::enum_images_type, 10, now(), now())",
                logoId, uuid, "https://avatars.mdst.yandex.net/get-dialogs/5182/9c9a06f5c3c5e5246ac9/orig",
                "skillSettings");

        ObjectMapper objectMapper = new ObjectMapper();
        jdbcTemplate.update("insert into skills (id, \"createdAt\", \"updatedAt\", salt, \"userId\", name, \"onAir\"," +
                        " \"backendSettings\", \"logoId\", \"featureFlags\", \"inflectedActivationPhrases\", " +
                        "\"useStateStorage\", \"rsyPlatformId\",  channel) " +
                        "values (?, ?, ?, ?, ?, ?, ?, (?)::json, ?, '{test_flag, test_flag2}', '{коммерсант, " +
                        "коммерсанту}', " +
                        "false, ?, (?)::enum_skills_channel)",
                skillId,
                Timestamp.from(Instant.now()),
                Timestamp.from(Instant.now()),
                salt, userId, "test skill", true,
                objectMapper.writeValueAsString(new NewsSkillInfoDB.BackendSettings(feedId1.toString(),
                        FlashBriefingType.RADIONEWS.getValue())),
                logoId,
                "rsyId",
                Channel.ALICE_NEWS_SKILL.getValue()
        );

        jdbcTemplate.update("INSERT INTO public.\"newsFeeds\"(id, \"skillId\", \"createdAt\", \"updatedAt\", " +
                        "preamble, name, description, topic, \"iconUrl\", enabled) VALUES " +
                        "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                feedId1,
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

        jdbcTemplate.update("INSERT INTO public.\"newsFeeds\"(id, \"skillId\", \"createdAt\", \"updatedAt\", " +
                        "preamble, name, description, topic, \"iconUrl\", enabled) VALUES " +
                        "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                feedId2,
                skillId,
                Timestamp.from(Instant.now()),
                Timestamp.from(Instant.now()),
                "Новости от коммерсант ФМ",
                "Новости от коммерсант ФМ",
                "Новости от коммерсант ФМ",
                "спорт",
                null,
                true
        );

        Instant newsFeed1Item1PubDate = Instant.now();

        jdbcTemplate.update("INSERT INTO public.\"newsContents\" (id, \"feedId\", \"uid\", \"pubDate\", " +
                        "title, \"streamUrl\", \"mainText\", \"imageUrl\", \"detailsUrl\") VALUES " +
                        "(?, ?, ?, ?, ?, ?, ?, ?, ?);",
                newsFeed1Item1,
                feedId1,
                "https://www.kommersant.ru/doc/4327837",
                Timestamp.from(newsFeed1Item1PubDate),
                "Уйти-компании // Падение доходов IT-отрасли может привести к сокращениям сотрудников",
                null,
                "Российские IT-компании начинают оптимизировать расходы и готовиться к сокращению штата. Только 16,5%" +
                        " из них уверены, что смогут выплачивать зарплату всем действующим сотрудникам в течение " +
                        "второго квартала 2020 года. Компании жалуются на снижение спроса и заморозку бюджетов " +
                        "крупных государственных и коммерческих структур.",
                null,
                null
        );

        Instant newsFeed1Item2PubDate = newsFeed1Item1PubDate.plusSeconds(10);

        jdbcTemplate.update("INSERT INTO public.\"newsContents\" (id, \"feedId\", \"uid\", \"pubDate\", " +
                        "title, \"streamUrl\", \"mainText\", \"imageUrl\", \"detailsUrl\") VALUES " +
                        "(?, ?, ?, ?, ?, ?, ?, ?, ?);",
                newsFeed1Item2,
                feedId1,
                "https://www.kommersant.ru/doc/4327837",
                Timestamp.from(newsFeed1Item2PubDate),
                "Уйти-компании // Падение доходов IT-отрасли может привести к сокращениям сотрудников",
                null,
                "Российские IT-компании начинают оптимизировать расходы и готовиться к сокращению штата. Только 16,5%" +
                        " из них уверены, что смогут выплачивать зарплату всем действующим сотрудникам в течение " +
                        "второго квартала 2020 года. Компании жалуются на снижение спроса и заморозку бюджетов " +
                        "крупных государственных и коммерческих структур.",
                null,
                null
        );

        jdbcTemplate.update("INSERT INTO public.\"newsContents\" (id, \"feedId\", \"uid\", \"pubDate\", " +
                        "title, \"streamUrl\", \"mainText\", \"imageUrl\", \"detailsUrl\") VALUES " +
                        "(?, ?, ?, ?, ?, ?, ?, ?, ?);",
                newsFeed2Item1,
                feedId2,
                "https://www.kommersant.ru/doc/4327837",
                Timestamp.from(newsFeed1Item1PubDate),
                "Уйти-компании // Падение доходов IT-отрасли может привести к сокращениям сотрудников",
                null,
                "Российские IT-компании начинают оптимизировать расходы и готовиться к сокращению штата. Только 16,5%" +
                        " из них уверены, что смогут выплачивать зарплату всем действующим сотрудникам в течение " +
                        "второго квартала 2020 года. Компании жалуются на снижение спроса и заморозку бюджетов " +
                        "крупных государственных и коммерческих структур.",
                null,
                null
        );

        Instant newsFeed2Item2PubDate = newsFeed1Item1PubDate.plusSeconds(10);

        jdbcTemplate.update("INSERT INTO public.\"newsContents\" (id, \"feedId\", \"uid\", \"pubDate\", " +
                        "title, \"streamUrl\", \"mainText\", \"imageUrl\", \"detailsUrl\") VALUES " +
                        "(?, ?, ?, ?, ?, ?, ?, ?, ?);",
                newsFeed2Item2,
                feedId2,
                "https://www.kommersant.ru/doc/4327837",
                Timestamp.from(newsFeed2Item2PubDate),
                "Уйти-компании // Падение доходов IT-отрасли может привести к сокращениям сотрудников",
                null,
                "Российские IT-компании начинают оптимизировать расходы и готовиться к сокращению штата. Только 16,5%" +
                        " из них уверены, что смогут выплачивать зарплату всем действующим сотрудникам в течение " +
                        "второго квартала 2020 года. Компании жалуются на снижение спроса и заморозку бюджетов " +
                        "крупных государственных и коммерческих структур.",
                null,
                null
        );

        newsSkillProvider.loadCache();

        Optional<NewsSkillInfo> skill = newsSkillProvider.getSkill(skillId.toString());

        assertTrue(skill.isPresent());

        NewsSkillInfo newsSkillInfo = skill.get();
        assertEquals(newsSkillInfo.getFeeds().size(), 2);
        assertEquals(newsSkillInfo.getFeeds().get(0).getTopContents().size(), 2);
        assertEquals(newsSkillInfo.getFeeds().get(1).getTopContents().size(), 2);
        assertTrue(newsSkillInfo.getDefaultFeed().isPresent());
        assertEquals(newsSkillInfo.getDefaultFeed().get().getId(), feedId1.toString());
    }
}
