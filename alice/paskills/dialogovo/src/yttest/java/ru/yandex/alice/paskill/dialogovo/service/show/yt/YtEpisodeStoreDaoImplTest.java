package ru.yandex.alice.paskill.dialogovo.service.show.yt;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

import javax.annotation.Nullable;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.alice.paskill.dialogovo.domain.show.ShowEpisodeEntity;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType;
import ru.yandex.alice.paskill.dialogovo.utils.BaseYtTest;
import ru.yandex.inside.yt.kosher.cypress.YPath;
import ru.yandex.inside.yt.kosher.tables.YTableEntryTypes;
import ru.yandex.inside.yt.kosher.ytree.YTreeMapNode;
import ru.yandex.inside.yt.kosher.ytree.YTreeNode;

class YtEpisodeStoreDaoImplTest extends BaseYtTest {
    private static final String EPISODE_ID = "episode-id";
    private static final String SKILL_ID = "skill-id";
    private static final String SKILL_SLUG = "skill-slug";
    private static final ShowType SHOW_TYPE = ShowType.MORNING;
    private static final String EPISODE_TEXT = "episode-text";
    private static final String EPISODE_TTS = "episode-tts";
    private final YtShowEpisodeMapper ytShowEpisodeMapper = new YtShowEpisodeMapper();
    private ShowEpisodeEntity defaultEntity;
    private Instant farPastTimeStamp;
    private Instant pastTimeStamp;

    @BeforeEach
    @Override
    public void setUp() throws Exception {
        super.setUp();
        Instant requestTime = Instant.now();
        farPastTimeStamp = requestTime.minusSeconds(200000L);
        pastTimeStamp = requestTime.minusSeconds(100000L);
        Instant futureTimestamp = requestTime.plusSeconds(100000L);
        defaultEntity = buildEntity(farPastTimeStamp, futureTimestamp);
    }

    @Test
    void testFetchRecord() {
        prepareState();
        YPath table = storeDao.getTableForInstant(pastTimeStamp);
        List<ShowEpisodeEntity> actual = new ArrayList<>();
        yt.tables().read(
                table, YTableEntryTypes.YSON,
                (Consumer<YTreeMapNode>) (show -> actual.add(ytShowEpisodeMapper.map(show)))
        );
        Assertions.assertEquals(List.of(defaultEntity), actual);
    }

    @Test
    void testSaveRecordWithNoExpirationDate() {
        ShowEpisodeEntity testEntity = buildEntity(farPastTimeStamp, null);
        storeDao.storeSyncShowEpisodeEntity(List.of(testEntity), pastTimeStamp);
        YPath table = storeDao.getTableForInstant(pastTimeStamp);
        List<ShowEpisodeEntity> actual = new ArrayList<>();
        yt.tables().read(
                table, YTableEntryTypes.YSON,
                (Consumer<YTreeMapNode>) (show -> actual.add(ytShowEpisodeMapper.map(show)))
        );
        Assertions.assertEquals(List.of(testEntity), actual);
    }


    private void prepareState() {
        storeDao.storeSyncShowEpisodeEntity(List.of(defaultEntity), pastTimeStamp);
    }

    private static class YtShowEpisodeMapper {
        public ShowEpisodeEntity map(YTreeMapNode yTreeMapNode) {
            return new ShowEpisodeEntity(
                    yTreeMapNode.getString("uid"),
                    yTreeMapNode.getString("skill_id"),
                    SKILL_SLUG,
                    ShowType.valueOf(yTreeMapNode.getString("show_type")),
                    yTreeMapNode.getString("text"),
                    yTreeMapNode.getString("tts"),
                    Instant.parse(yTreeMapNode.getString("pub_date")),
                    yTreeMapNode.get("exp_date")
                            .filter(YTreeNode::isStringNode)
                            .map(YTreeNode::stringValue)
                            .map(Instant::parse)
                            .orElse(null)
            );
        }
    }

    private static ShowEpisodeEntity buildEntity(Instant publicationDate, @Nullable Instant expirationDate) {
        return new ShowEpisodeEntity(
                EPISODE_ID,
                SKILL_ID,
                SKILL_SLUG,
                SHOW_TYPE,
                EPISODE_TEXT,
                EPISODE_TTS,
                publicationDate,
                expirationDate
        );
    }
}
