package ru.yandex.quasar.billing.dao;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.jdbc.core.JdbcTemplate;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.config.TestConfigProvider;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = {TestConfigProvider.class})
class QuasarPromoBlacklistDaoImplTest {

    @Autowired
    private QuasarPromoBlacklistDaoImpl promoUserBlacklistDao;

    @Autowired
    private JdbcTemplate jdbcTemplate;

    @BeforeEach
    void setUp() {
        promoUserBlacklistDao.refreshBlacklist();
    }

    @Test
    void userInBlacklist() {
        assertFalse(promoUserBlacklistDao.userInBlacklist("1"));
    }

    @Test
    void userInBlacklistTrue() {
        // given
        jdbcTemplate.execute("insert into quasarpromoblacklist values ('1')");

        // when
        promoUserBlacklistDao.refreshBlacklist();

        // then
        assertTrue(promoUserBlacklistDao.userInBlacklist("1"));
        assertFalse(promoUserBlacklistDao.userInBlacklist("2"));
    }
}
