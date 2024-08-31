package ru.yandex.quasar.billing.dao;

import java.util.List;
import java.util.Map;
import java.util.Optional;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.base.Stopwatch;
import org.springframework.dao.EmptyResultDataAccessException;
import org.springframework.jdbc.core.BeanPropertyRowMapper;
import org.springframework.jdbc.core.ColumnMapRowMapper;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.stereotype.Repository;
import org.springframework.transaction.annotation.Propagation;
import org.springframework.transaction.annotation.Transactional;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;
import ru.yandex.alice.library.routingdatasource.ReadWriteTransactional;
import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.services.UnistatService;
import ru.yandex.quasar.billing.services.promo.Platform;

@Repository
@ReadOnlyTransactional
class PromoCodeBaseDaoImpl implements PromoCodeBaseDao {
    private final JdbcTemplate jdbcTemplate;
    private final BeanPropertyRowMapper<PromoCodeBase> rowMapper;
    private final UnistatService unistatService;

    PromoCodeBaseDaoImpl(JdbcTemplate jdbcTemplate, ObjectMapper objectMapper, UnistatService unistatService) {
        this.jdbcTemplate = jdbcTemplate;
        this.rowMapper = new BillingBeanPropertyRowMapper<>(PromoCodeBase.class, objectMapper);
        this.unistatService = unistatService;
    }

    @Override
    @ReadWriteTransactional
    public PromoCodeBase save(PromoCodeBase promoCodeBase) {
        if (promoCodeBase.getId() == null) {
            promoCodeBase.setId(getNextId());
            Stopwatch sw = Stopwatch.createStarted();
            try {
                jdbcTemplate.update(
                        "insert into PromoCodeBase (id, provider, promotype, code, platform, task_id, prototype_id) " +
                                "values (?, ?, ?, ?, ?, ?, ?)",
                        promoCodeBase.getId(),
                        promoCodeBase.getProvider(),
                        promoCodeBase.getPromoType().name(),
                        promoCodeBase.getCode(),
                        promoCodeBase.getPlatform() != null ? promoCodeBase.getPlatform().toString() : null,
                        promoCodeBase.getTaskId(),
                        promoCodeBase.getPrototypeId()
                );
            } finally {
                unistatService.logOperationDurationHist(
                        "quasar_billing_pg_promocodebasedao_save_dhhh", sw.elapsed().toMillis());
            }

            return promoCodeBase;
        } else {
            throw new IllegalArgumentException("update not supported");
        }
    }

    @ReadWriteTransactional
    Long getNextId() {
        Stopwatch sw = Stopwatch.createStarted();
        try {
            List<Map<String, Object>> purchaseIdRes = jdbcTemplate.query(
                    "SELECT nextval('PromoCodeBaseSeq')",
                    new ColumnMapRowMapper()
            );

            return (Long) purchaseIdRes.get(0).get("nextval");
        } finally {
            unistatService.logOperationDurationHist(
                    "quasar_billing_pg_promocodebasedao_getNextId_dhhh", sw.elapsed().toMillis());
        }
    }

    @Override
    @ReadOnlyTransactional
    public Optional<PromoCodeBase> findById(Long id) {
        Stopwatch sw = Stopwatch.createStarted();
        try {

            return Optional.ofNullable(
                    jdbcTemplate.queryForObject("select * from PromoCodeBase where id = ?",
                            rowMapper,
                            id)
            );

        } catch (EmptyResultDataAccessException e) {
            return Optional.empty();
        } finally {
            unistatService.logOperationDurationHist(
                    "quasar_billing_pg_promocodebasedao_findById_dhhh", sw.elapsed().toMillis());
        }
    }

    @Override
    @Transactional(propagation = Propagation.MANDATORY)
    public Optional<PromoCodeBase> queryNextUnusedCode(String providerName, PromoType promoType, Platform platform) {
        Stopwatch sw = Stopwatch.createStarted();
        PromoCodeBase promoCodeBase;
        try {
            /*
            Для этих запросов критически важно, чтобы максимально эффективо использовался индекс,
            весь запрос должен отрабатывать на индексе по promocodebase без обращения к таблицу, а поле code
            в индексе должно быть последним, чтобы он же использовался для сортировки. Так подбор очередного промокода
            выпоняется мгновенно и не приводит к блокировкам при нагрузке.
            https://st.yandex-team.ru/SPI-9195 - пример того, что бывает, если индексов нет.
            В рамках исправления были созданы два индекса:
                create index promocodebase_idx1 on PromoCodeBase(provider, promoType, code) where platform is null;
                create index promocodebase_idx2 on PromoCodeBase(provider, promoType, platform, code);
             */
            promoCodeBase = jdbcTemplate.queryForObject("select p.* from PromoCodeBase p " +
                            "left join useddevicepromo d on d.codeid = p.id " +
                            "where p.provider = ? " +
                            "and p.promoType = ? " +
                            "and p.platform = ? " +
                            "and d.codeid is null " +
                            "for update of p skip locked limit 1",
                    rowMapper,
                    providerName, promoType.name(), platform.toString());
        } catch (EmptyResultDataAccessException e) {
            try {
                promoCodeBase = jdbcTemplate.queryForObject("select p.* from PromoCodeBase p " +
                                "left join useddevicepromo d on d.codeid = p.id " +
                                "where p.provider = ? " +
                                "and p.promoType = ? " +
                                "and p.platform is null " +
                                "and d.codeid is null " +
                                "for update of p skip locked limit 1",
                        rowMapper,
                        providerName, promoType.name());
            } catch (EmptyResultDataAccessException ignored) {
                return Optional.empty();
            }
        } finally {
            unistatService.logOperationDurationHist(
                    "quasar_billing_pg_promocodebasedao_queryNextUnusedCode_dhhh", sw.elapsed().toMillis());
        }

        return Optional.ofNullable(promoCodeBase);
    }
}
