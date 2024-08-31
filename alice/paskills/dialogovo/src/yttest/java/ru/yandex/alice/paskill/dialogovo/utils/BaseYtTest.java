package ru.yandex.alice.paskill.dialogovo.utils;

import java.io.IOException;
import java.time.Duration;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.alice.paskill.dialogovo.service.show.yt.YtEpisodeStoreDaoImpl;
import ru.yandex.alice.paskill.dialogovo.service.show.yt.YtServiceClient;
import ru.yandex.inside.yt.kosher.Yt;
import ru.yandex.inside.yt.kosher.cypress.CypressNodeType;
import ru.yandex.inside.yt.kosher.cypress.YPath;
import ru.yandex.inside.yt.kosher.impl.YtUtils;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class BaseYtTest {
    protected static Yt yt;
    protected static YtServiceClient ytClient;
    protected static YtEpisodeStoreDaoImpl storeDao;
    protected static YPath directory;

    @BeforeAll
    public static void beforeAll() throws Exception {
        String ytProxy = System.getenv("YT_PROXY");
        yt = YtUtils.http(ytProxy, "");
        ytClient = new YtServiceClient(yt);
        directory = YPath.simple("//tmp");
        storeDao = new YtEpisodeStoreDaoImpl(directory, ytClient, Duration.ofMillis(10000), Duration.ofHours(148));
    }


    @Test
    public void testRecipeWorks() {
        YPath node = YPath.simple("//tmp/table");
        assertFalse(yt.cypress().exists(node));
        yt.cypress().create(node, CypressNodeType.TABLE);
        assertTrue(yt.cypress().exists(node));
        yt.cypress().remove(node);
        assertFalse(yt.cypress().exists(node));
    }


    @BeforeEach
    protected void setUp() throws Exception {
        clearAllTables();
    }

    @AfterEach
    protected void tearDown() throws Exception {
        clearAllTables();
    }

    public void clearAllTables() throws IOException {

    }
}
