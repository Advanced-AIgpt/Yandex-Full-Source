package ru.yandex.quasar.billing.dao;

import java.sql.Timestamp;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.ColumnMapRowMapper;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;
import ru.yandex.alice.library.routingdatasource.ReadWriteTransactional;

/**
 * Табличка в которую можно что-то сохранить, а потом вытащить - как буфер обмена.
 * Используется например для случая когда надо что-то протащить через процесс покупки в вебвью,
 * а размер этого чего-то может быть довольно большим.
 * <br/>
 * Записи старше суток автоматически подчищаются.
 */
@Component
public class UserPastesDAO {

    private final JdbcTemplate jdbcTemplate;

    @Autowired
    public UserPastesDAO(JdbcTemplate jdbcTemplate) {
        this.jdbcTemplate = jdbcTemplate;
    }

    @ReadWriteTransactional
    Long getNextPasteId() {
        List<Map<String, Object>> purchaseIdRes = jdbcTemplate.query(
                "SELECT nextval('PasteIdsSeq')",
                new ColumnMapRowMapper()
        );

        return (Long) purchaseIdRes.get(0).get("nextval");
    }

    @ReadWriteTransactional
    public Long savePaste(Long uid, String paste) {
        Long pasteId = getNextPasteId();

        jdbcTemplate.update(
                "INSERT INTO UserPastes (pasteId, uid, paste, pasteDate) VALUES (?, ?, ?, ?)",
                pasteId, uid, paste, new Timestamp(System.currentTimeMillis())
        );

        return pasteId;
    }

    @ReadOnlyTransactional
    public String getPaste(Long pasteId, Long uid) {
        List<Map<String, Object>> result = jdbcTemplate.query(
                "SELECT paste FROM UserPastes WHERE pasteId = ? AND uid = ?",
                new ColumnMapRowMapper(),
                pasteId, uid
        );

        return result.isEmpty() ? null : result.get(0).get("paste").toString();
    }

    @ReadWriteTransactional
    @Scheduled(fixedDelay = 1000 * 60 * 60 * 24, initialDelay = 1000 * 5)
    void deleteOldPastes() {
        jdbcTemplate.update(
                "DELETE FROM UserPastes WHERE pasteDate < ?",
                new Timestamp(System.currentTimeMillis() - TimeUnit.DAYS.toMillis(1))
        );
    }
}
