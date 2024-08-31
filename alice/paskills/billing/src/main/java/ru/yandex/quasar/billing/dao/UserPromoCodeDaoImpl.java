package ru.yandex.quasar.billing.dao;

import java.util.List;
import java.util.Optional;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.BeanPropertyRowMapper;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.stereotype.Repository;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;
import ru.yandex.alice.library.routingdatasource.ReadWriteTransactional;

@Repository
class UserPromoCodeDaoImpl implements UserPromoCodeDao {
    private final JdbcTemplate jdbcTemplate;
    private final BeanPropertyRowMapper<UserPromoCode> rowMapper;
    private final ObjectMapper objectMapper;

    @Autowired
    UserPromoCodeDaoImpl(JdbcTemplate jdbcTemplate, ObjectMapper objectMapper) {
        this.jdbcTemplate = jdbcTemplate;
        this.rowMapper = new BillingBeanPropertyRowMapper<>(UserPromoCode.class, objectMapper);
        this.objectMapper = objectMapper;
    }

    @Override
    @ReadWriteTransactional
    public void save(UserPromoCode promoCode) {
        try {
            jdbcTemplate.update(
                    "INSERT INTO UserPromoCode (uid, provider, code, pricingOptionType, subscriptionPeriod, " +
                            "subscriptionPricingOption, activatedAt, subscriptionId) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                    promoCode.getUid(), promoCode.getProvider(), promoCode.getCode(),
                    promoCode.getPricingOptionType().toString(), promoCode.getSubscriptionPeriod(),
                    objectMapper.writeValueAsString(promoCode.getSubscriptionPricingOption()),
                    promoCode.getActivatedAt(), promoCode.getSubscriptionId()
            );
        } catch (JsonProcessingException e) {
            // serializations shouldn't cause errors
            throw new RuntimeException(e);
        }
    }

    @ReadOnlyTransactional
    @Override
    public Optional<UserPromoCode> findByUidAndProviderAndCode(Long uid, String provider, String promoCode) {
        List<UserPromoCode> result = jdbcTemplate.query(
                "SELECT * FROM UserPromoCode WHERE uid = ? AND code = ? and provider = ?",
                rowMapper,
                uid, promoCode, provider
        );
        return result.isEmpty() ? Optional.empty() : Optional.of(result.get(0));
    }

    @ReadOnlyTransactional
    @Override
    public List<UserPromoCode> findAllByUid(Long uid) {
        List<UserPromoCode> result = jdbcTemplate.query(
                "SELECT * FROM UserPromoCode WHERE uid = ?",
                rowMapper,
                uid
        );
        return result;
    }

}
