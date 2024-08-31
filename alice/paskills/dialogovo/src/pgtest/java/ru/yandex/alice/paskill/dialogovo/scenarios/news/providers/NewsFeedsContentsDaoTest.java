package ru.yandex.alice.paskill.dialogovo.scenarios.news.providers;

import java.sql.SQLException;
import java.sql.Timestamp;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.Collections;
import java.util.List;
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
import static org.junit.jupiter.api.Assertions.assertTrue;

class NewsFeedsContentsDaoTest extends BaseDatabaseTest {

    @Autowired
    private NewsContentsDao newsContentsDao;
    @Autowired
    private JdbcTemplate jdbcTemplate;

    @Test
    void findFeedByIdEmpty() {
        assertTrue(newsContentsDao.getTopNewsByFeed(UUID.randomUUID(), 1).isEmpty());
    }

    @Test
    void topNewsByFeed() throws JsonProcessingException, SQLException {
        UUID skillId = createSkill();

        var feedId = UUID.randomUUID();
        var newsItem1 = UUID.randomUUID();
        var newsItem2 = UUID.randomUUID();

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

        Instant newsItem1PubDate = Instant.now().truncatedTo(ChronoUnit.MICROS);

        jdbcTemplate.update("INSERT INTO public.\"newsContents\" (id, \"feedId\", \"uid\", \"pubDate\", " +
                        "title, \"streamUrl\", \"mainText\", \"imageUrl\", \"detailsUrl\") VALUES " +
                        "(?, ?, ?, ?, ?, ?, ?, ?, ?);",
                newsItem1,
                feedId,
                "https://www.kommersant.ru/doc/4327837",
                Timestamp.from(newsItem1PubDate),
                "Уйти-компании // Падение доходов IT-отрасли может привести к сокращениям сотрудников",
                null,
                "Российские IT-компании начинают оптимизировать расходы и готовиться к сокращению штата. Только 16,5%" +
                        " из них уверены, что смогут выплачивать зарплату всем действующим сотрудникам в течение " +
                        "второго квартала 2020 года. Компании жалуются на снижение спроса и заморозку бюджетов " +
                        "крупных государственных и коммерческих структур.",
                null,
                null
        );

        Instant newsItem2PubDate = newsItem1PubDate.plusSeconds(10);

        String news2Title = "Посторонние болезни нарушают карантин // Пациентов с редкими заболеваниями ограничили в " +
                "медпомощи" +
                " из-за коронавируса";

        String news2Text = "Российские больницы ограничили лечение пациентов не с коронавирусной инфекцией в дневных " +
                "стационарах, от чего могут пострадать по меньшей мере несколько тысяч человек. Это следует " +
                "из заявлений ряда пациентских организаций, оценивших первые итоги перепрофилирования " +
                "мощностей системы ОМС для борьбы с эпидемией коронавируса. В первую очередь с приостановкой " +
                "плановых процедур столкнулись больные редкими заболеваниями и гепатитами — однако в ряде " +
                "случаев сроки ожидания лечения выросли и для пациентов с онкологическими заболеваниями.";

        String news2Guid = "https://www.kommersant.ru/doc/4327842";

        jdbcTemplate.update("INSERT INTO public.\"newsContents\" (id, \"feedId\", \"uid\", \"pubDate\", " +
                        "title, \"streamUrl\", \"mainText\", \"imageUrl\", \"detailsUrl\") VALUES " +
                        "(?, ?, ?, ?, ?, ?, ?, ?, ?);",
                newsItem2,
                feedId,
                news2Guid,
                Timestamp.from(newsItem2PubDate),
                news2Title,
                null,
                news2Text,
                null,
                null
        );

        List<NewsContentDB> topNewsByFeed = newsContentsDao.getTopNewsByFeed(feedId, 1);

        assertEquals(topNewsByFeed.size(), 1);

        NewsContentDB newsContent = topNewsByFeed.get(0);

        assertEquals(
                new NewsContentDB(
                        newsItem2.toString(),
                        feedId.toString(),
                        news2Guid,
                        newsItem2PubDate.truncatedTo(ChronoUnit.MICROS),
                        news2Title,
                        null,
                        news2Text,
                        null,
                        null,
                        null,
                        null
                ),
                newsContent);
    }

    @Test
    void getAllNewsContentsTopPerFeed() throws JsonProcessingException, SQLException {
        UUID skillId = createSkill();
        var feedId1 = UUID.randomUUID();
        var feedId2 = UUID.randomUUID();
        var newsFeed1Item1 = UUID.randomUUID();
        var newsFeed1Item2 = UUID.randomUUID();
        var newsFeed2Item1 = UUID.randomUUID();
        var newsFeed2Item2 = UUID.randomUUID();

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

        List<NewsContentDB> allNewsContentsTopPerFeed = newsContentsDao.getAllNewsContentsTopPerFeed(1);

        assertEquals(allNewsContentsTopPerFeed.size(), 2);
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
                DeveloperType.Yandex,
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
