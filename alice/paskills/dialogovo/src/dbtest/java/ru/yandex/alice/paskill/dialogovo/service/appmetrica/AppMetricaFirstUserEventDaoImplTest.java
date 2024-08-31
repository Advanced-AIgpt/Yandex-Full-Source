package ru.yandex.alice.paskill.dialogovo.service.appmetrica;

import java.time.Instant;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.alice.paskill.dialogovo.utils.BaseYdbTest;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

class AppMetricaFirstUserEventDaoImplTest extends BaseYdbTest {

    private static final String APP_METRICA_API_KEY = "api_key";
    private static final String SKILL_ID = "skill-id";
    private static final String UUID = "1";
    private static final long DEFAULT_TIMESTAMP = 123123L;

    private AppMetricaFirstUserEventDaoImpl appMetricaFirstUserEventDao;

    @BeforeEach
    @Override
    public void setUp() throws Exception {
        super.setUp();
        appMetricaFirstUserEventDao = new AppMetricaFirstUserEventDaoImpl(new MetricRegistry(), ydbClient);
    }

    @AfterEach
    @Override
    public void tearDown() throws Exception {
        super.tearDown();
    }

    @Test
    public void createAndCheckExists() {
        appMetricaFirstUserEventDao.saveFirstUserEvent(APP_METRICA_API_KEY, SKILL_ID, UUID,
                Instant.ofEpochMilli(DEFAULT_TIMESTAMP));
        assertTrue(appMetricaFirstUserEventDao.firstUserEventExists(APP_METRICA_API_KEY, SKILL_ID, UUID));
    }

    @Test
    public void createUpsert() {
        appMetricaFirstUserEventDao.saveFirstUserEvent(APP_METRICA_API_KEY, SKILL_ID, UUID,
                Instant.ofEpochMilli(DEFAULT_TIMESTAMP));
        assertTrue(appMetricaFirstUserEventDao.firstUserEventExists(APP_METRICA_API_KEY, SKILL_ID, UUID));
        appMetricaFirstUserEventDao.saveFirstUserEvent(APP_METRICA_API_KEY, SKILL_ID, UUID,
                Instant.ofEpochMilli(DEFAULT_TIMESTAMP));
        assertTrue(appMetricaFirstUserEventDao.firstUserEventExists(APP_METRICA_API_KEY, SKILL_ID, UUID));
    }

    @Test
    public void createNotExistsCorrect() {
        assertFalse(appMetricaFirstUserEventDao.firstUserEventExists(APP_METRICA_API_KEY, SKILL_ID + "11", UUID));
    }
}
