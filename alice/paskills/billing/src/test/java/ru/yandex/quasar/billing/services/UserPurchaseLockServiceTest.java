package ru.yandex.quasar.billing.services;

import java.sql.Timestamp;
import java.time.Instant;
import java.util.Collections;
import java.util.List;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.jdbc.DataSourceTransactionManagerAutoConfiguration;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.dao.UserPurchaseLockDAO;

import static org.junit.jupiter.api.Assertions.assertEquals;

@ExtendWith(SpringExtension.class)
@ExtendWith(EmbeddedPostgresExtension.class)
@ContextConfiguration(classes = {
        UserPurchaseLockService.class,
        UserPurchaseLockDAO.class,
        TestConfigProvider.class,
        DataSourceTransactionManagerAutoConfiguration.class
})
public class UserPurchaseLockServiceTest {

    @Autowired
    private UserPurchaseLockService userPurchaseLockService;
    @Autowired
    private JdbcTemplate jdbcTemplate;

    @BeforeEach
    public void setUp() throws Exception {
        jdbcTemplate.update("delete from UserPurchaseLock");
    }

    @AfterEach
    public void tearDown() throws Exception {
        jdbcTemplate.update("delete from UserPurchaseLock");
    }

    @Test
    public void removeExpiredLocks() {
        jdbcTemplate.update("INSERT INTO UserPurchaseLock (uid, provider, acquiredAt) VALUES (?, ?, ?)",
                1L, "test", new Timestamp(Instant.now().minusSeconds(60 * 10).toEpochMilli())
        );
        jdbcTemplate.update("INSERT INTO UserPurchaseLock (uid, provider, acquiredAt) VALUES (?, ?, ?)",
                2L, "test", new Timestamp(Instant.now().toEpochMilli())
        );

        userPurchaseLockService.removeExpiredLocks();

        List<Long> rows = jdbcTemplate.queryForList("select uid from UserPurchaseLock", Long.class);

        // row for uid=1 must be deleted
        assertEquals(Collections.singletonList(2L), rows);
    }
}
