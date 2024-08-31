package ru.yandex.quasar.billing.dao;

import java.sql.Timestamp;
import java.time.Instant;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.stereotype.Repository;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;
import ru.yandex.alice.library.routingdatasource.ReadWriteTransactional;

/**
 * Repository for the special lock table used to prevent concurrent purchase requests (due to retries for example)
 * from a user.
 * The lock is acquired per provider so no two concurrent purchase processes for a single provider are possible.
 * The table is needed to face concurrency as the service runs in a cluster
 */
@Repository
@ReadOnlyTransactional
public class UserPurchaseLockDAO {
    private final JdbcTemplate jdbcTemplate;

    @Autowired
    public UserPurchaseLockDAO(JdbcTemplate jdbcTemplate) {
        this.jdbcTemplate = jdbcTemplate;
    }

    /**
     * @param uid          uid of the user
     * @param providerName content provider name
     * @return true if lock was acquired and false if it already exists
     */
    @ReadWriteTransactional
    public boolean acquireLock(Long uid, String providerName) {
        int rowsUpdated = jdbcTemplate.update(
                "INSERT INTO UserPurchaseLock (uid, provider, acquiredAt) VALUES (?, ?, ?) ON CONFLICT DO NOTHING",
                uid, providerName, new Timestamp(Instant.now().toEpochMilli())
        );
        return rowsUpdated > 0;
    }

    /**
     * cleansup the aquired lock
     *
     * @param uid          uid of the user
     * @param providerName content provider name
     * @return {@literal 1} if the lock existed and was removed, {@literal 0} otherwise
     */
    @ReadWriteTransactional
    public int removeLock(Long uid, String providerName) {
        return jdbcTemplate.update(
                "DELETE FROM UserPurchaseLock WHERE uid = ? and provider = ?",
                uid, providerName
        );
    }

    @ReadWriteTransactional
    public int removeAcuiredBeforeMoment(Instant moment) {
        return jdbcTemplate.update(
                "delete from UserPurchaseLock where acquiredAt < ?",
                new Timestamp(
                        moment.toEpochMilli()
                )
        );
    }


}
