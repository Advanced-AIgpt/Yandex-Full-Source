package ru.yandex.quasar.billing.dao;

import java.math.BigDecimal;
import java.util.List;
import java.util.Map;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.ColumnMapRowMapper;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.stereotype.Repository;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;
import ru.yandex.alice.library.routingdatasource.ReadWriteTransactional;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;

@Repository
public class SubscriptionProductsDAO {

    private final JdbcTemplate jdbcTemplate;

    @Autowired
    public SubscriptionProductsDAO(JdbcTemplate jdbcTemplate) {
        this.jdbcTemplate = jdbcTemplate;
    }

    @ReadOnlyTransactional
    public Long getSubscriptionProductCode(String provider, int subscriptionPeriodDays, int trialPeriodDays,
                                           BigDecimal price) {
        List<Map<String, Object>> result = jdbcTemplate.query(
                "SELECT productCode FROM SubscriptionProducts WHERE provider = ? AND subscriptionPeriodDays = ? AND " +
                        "trialPeriodDays = ? AND price = ?",
                new ColumnMapRowMapper(),
                provider, subscriptionPeriodDays, trialPeriodDays, price
        );

        return result.isEmpty()
                ? null
                : (Long) result.get(0).get("productCode");

    }

    @ReadWriteTransactional
    public Long getNextSubscriptionProductCode() {
        List<Map<String, Object>> purchaseIdRes = jdbcTemplate.query(
                "SELECT nextval('SubscriptionProductCodesSeq')",
                new ColumnMapRowMapper()
        );

        return (Long) purchaseIdRes.get(0).get("nextval");
    }

    @ReadWriteTransactional
    public void saveSubscriptionProductCode(String provider, int subscriptionPeriodDays, int trialPeriodDays,
                                            BigDecimal price, TrustCurrency currency, long productCode) {
        jdbcTemplate.update(
                "INSERT INTO SubscriptionProducts (provider, subscriptionPeriodDays, trialPeriodDays, price, " +
                        "currency, productCode) VALUES (?, ?, ?, ?, ?, ?) ON CONFLICT DO NOTHING",
                provider, subscriptionPeriodDays, trialPeriodDays, price, currency.name(), productCode
        );
    }
}
