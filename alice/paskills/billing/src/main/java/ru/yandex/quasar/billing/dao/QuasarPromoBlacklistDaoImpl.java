package ru.yandex.quasar.billing.dao;

import java.util.Set;

import javax.annotation.Nullable;

import com.google.common.collect.ImmutableSet;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Repository;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;

@Repository
class QuasarPromoBlacklistDaoImpl implements QuasarPromoBlacklistDao {

    private final JdbcTemplate jdbcTemplate;
    @Nullable
    private volatile Set<String> blacklistUids = null;

    QuasarPromoBlacklistDaoImpl(JdbcTemplate jdbcTemplate) {
        this.jdbcTemplate = jdbcTemplate;
    }

    @Override
    public boolean userInBlacklist(String uid) {
        if (blacklistUids == null) {
            refreshBlacklist();
        }
        return blacklistUids.contains(uid);
    }

    @ReadOnlyTransactional
    @Scheduled(fixedDelay = 1000 * 60)
    synchronized void refreshBlacklist() {
        this.blacklistUids = ImmutableSet.copyOf(jdbcTemplate.queryForList("select uid from quasarpromoblacklist",
                String.class));
    }
}
