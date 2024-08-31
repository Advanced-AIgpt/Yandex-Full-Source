package ru.yandex.alice.paskill.dialogovo.scenarios.news.providers;

import java.util.List;
import java.util.UUID;

import org.springframework.data.jdbc.repository.query.Query;
import org.springframework.data.repository.Repository;
import org.springframework.data.repository.query.Param;
import org.springframework.transaction.annotation.Transactional;

@Transactional(readOnly = true)
public interface NewsContentsDao extends Repository<NewsContentDB, UUID> {

    @Query("SELECT\n" +
            "    c.id\n" +
            "    , c.\"feedId\" as feed_id\n" +
            "    , c.uid\n" +
            "    , c.\"pubDate\" as pub_date\n" +
            "    , c.title\n" +
            "    , c.\"streamUrl\" as stream_url\n" +
            "    , c.\"mainText\" as main_text" +
            "    , c.\"soundId\" as sound_id" +
            "    , c.\"imageUrl\" as image_url\n" +
            "    , c.\"detailsUrl\" details_url\n" +
            "    , c.\"detailsText\" details_text\n" +
            "FROM\n" +
            "    \"newsContents\" c \n" +
            "WHERE\n" +
            "    c.\"feedId\" = :feedId\n " +
            "ORDER BY\n" +
            "   c.\"pubDate\" DESC\n" +
            "LIMIT :limit"
    )
    List<NewsContentDB> getTopNewsByFeed(@Param("feedId") UUID feedId, @Param("limit") int limit);

    @Query("SELECT\n" +
            "    c.id\n" +
            "    , c.\"feedId\" as feed_id\n" +
            "    , c.uid\n" +
            "    , c.\"pubDate\" as pub_date\n" +
            "    , c.title\n" +
            "    , c.\"streamUrl\" as stream_url\n" +
            "    , c.\"mainText\" as main_text" +
            "    , c.\"soundId\" as sound_id" +
            "    , c.\"imageUrl\" as image_url\n" +
            "    , c.\"detailsUrl\" details_url\n" +
            "    , c.\"detailsText\" details_text\n" +
            "FROM\n" +
            "   (SELECT\n " +
            "       c.*\n" +
            "       ,ROW_NUMBER() OVER (PARTITION BY \"feedId\" ORDER BY c.\"pubDate\" DESC) as in_feed_num\n" +
            "    FROM\n" +
            "       \"newsContents\" c\n" +
            "       JOIN  \"newsFeeds\" f ON f.id = c.\"feedId\"\n" +
            "       WHERE\n" +
            "           f.enabled = true) as c\n" +
            "WHERE\n" +
            "   in_feed_num <= :limitPerFeed"
    )
    List<NewsContentDB> getAllNewsContentsTopPerFeed(@Param("limitPerFeed") int limitPerFeed);
}
