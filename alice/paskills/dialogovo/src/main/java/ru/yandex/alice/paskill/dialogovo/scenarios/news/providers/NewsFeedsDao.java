package ru.yandex.alice.paskill.dialogovo.scenarios.news.providers;

import java.util.List;
import java.util.Optional;
import java.util.UUID;

import org.springframework.data.jdbc.repository.query.Query;
import org.springframework.data.repository.Repository;
import org.springframework.data.repository.query.Param;
import org.springframework.transaction.annotation.Transactional;

@Transactional(readOnly = true)
public interface NewsFeedsDao extends Repository<NewsFeedDB, UUID> {
    @Query("SELECT\n" +
            "    f.id\n" +
            "    , f.\"skillId\" as skill_id\n" +
            "    , f.preamble\n" +
            "    , f.name\n" +
            "    , f.topic\n" +
            "    , f.enabled\n" +
            "FROM\n" +
            "    \"newsFeeds\" f\n" +
            "WHERE\n" +
            "    f.id = :id")
    Optional<NewsFeedDB> findFeedById(@Param("id") UUID id);

    @Query("SELECT\n" +
            "    f.id\n" +
            "    , f.\"skillId\" as skill_id\n" +
            "    , f.preamble\n" +
            "    , f.name\n" +
            "    , f.topic\n" +
            "    , f.enabled\n" +
            "FROM\n" +
            "    \"newsFeeds\" f\n" +
            "WHERE\n" +
            "    f.enabled = true")
    List<NewsFeedDB> findAllEnabled();
}
