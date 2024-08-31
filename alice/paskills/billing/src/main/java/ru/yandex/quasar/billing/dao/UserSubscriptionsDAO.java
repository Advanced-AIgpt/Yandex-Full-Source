package ru.yandex.quasar.billing.dao;

import java.sql.Timestamp;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.BeanPropertyRowMapper;
import org.springframework.jdbc.core.ColumnMapRowMapper;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.stereotype.Component;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;
import ru.yandex.alice.library.routingdatasource.ReadWriteTransactional;
import ru.yandex.quasar.billing.services.UnistatService;

@Component
@ReadOnlyTransactional
public class UserSubscriptionsDAO {

    private final JdbcTemplate jdbcTemplate;
    private final ObjectMapper objectMapper;
    private final BeanPropertyRowMapper<SubscriptionInfo> subscriptionInfoRowMapper;
    private final UnistatService unistatService;

    @Autowired
    public UserSubscriptionsDAO(JdbcTemplate jdbcTemplate, ObjectMapper objectMapper, UnistatService unistatService) {
        this.jdbcTemplate = jdbcTemplate;
        this.objectMapper = objectMapper;
        this.subscriptionInfoRowMapper = new BillingBeanPropertyRowMapper<>(SubscriptionInfo.class, objectMapper);
        this.unistatService = unistatService;
    }

    @ReadWriteTransactional
    public Long getNextSubscriptionId() {
        List<Map<String, Object>> purchaseIdRes = jdbcTemplate.query(
                "SELECT nextval('SubscriptionIdsSeq')",
                new ColumnMapRowMapper()
        );

        return (Long) purchaseIdRes.get(0).get("nextval");
    }

    @ReadWriteTransactional
    public SubscriptionInfo save(SubscriptionInfo subscriptionInfo) {
        String selectedOptionStr;
        String contentItemStr;
        String purchasedContentItem;
        try {
            contentItemStr = subscriptionInfo.getContentItem() != null ?
                    objectMapper.writeValueAsString(subscriptionInfo.getContentItem()) : null;
            selectedOptionStr = subscriptionInfo.getSelectedOption() != null ?
                    objectMapper.writeValueAsString(subscriptionInfo.getSelectedOption()) : null;
            purchasedContentItem = subscriptionInfo.getPurchasedContentItem() != null ?
                    objectMapper.writeValueAsString(subscriptionInfo.getPurchasedContentItem()) : null;

        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }

        jdbcTemplate.update(
                "INSERT INTO UserSubscriptions (subscriptionId, uid, subscriptionDate, contentItem, selectedOption, " +
                        "status, activeTill, securityToken, provider, userPrice, originalPrice, subscriptionPeriod, " +
                        "trialPeriod, productCode, currencyCode, partnerId, purchasedContentItem, paymentProcessor) " +
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                subscriptionInfo.getSubscriptionId(),
                subscriptionInfo.getUid(),
                subscriptionInfo.getSubscriptionDate(),
                contentItemStr,
                selectedOptionStr,
                subscriptionInfo.getStatus().toString(),
                subscriptionInfo.getActiveTill(),
                subscriptionInfo.getSecurityToken(),
                subscriptionInfo.getProvider(),
                subscriptionInfo.getUserPrice(),
                subscriptionInfo.getOriginalPrice(),
                subscriptionInfo.getSubscriptionPeriod(),
                subscriptionInfo.getTrialPeriod(),
                subscriptionInfo.getProductCode(),
                subscriptionInfo.getCurrencyCode(),
                subscriptionInfo.getPartnerId(),
                purchasedContentItem,
                subscriptionInfo.getPaymentProcessor().name()
        );
        return subscriptionInfo;
    }

    @Nullable
    public SubscriptionInfo getSubscriptionInfo(Long subscriptionId) {
        List<SubscriptionInfo> result = jdbcTemplate.query(
                "SELECT * FROM UserSubscriptions WHERE subscriptionId = ?",
                subscriptionInfoRowMapper,
                subscriptionId
        );

        return result.isEmpty() ? null : result.get(0);
    }

    // for tests
    @ReadWriteTransactional
    void updateSubscriptionStatusAndActiveTill(Long subscriptionId, SubscriptionInfo.Status status,
                                               Timestamp activeTill) {
        jdbcTemplate.update(
                "UPDATE UserSubscriptions SET status = ?, activeTill = ? WHERE subscriptionId = ?",
                status.toString(), activeTill, subscriptionId
        );
    }

    public List<SubscriptionInfo> getActiveSubscriptions(Long uid) {
        // Don't filter by status as even if subscription is DISMISSED and its activetill date is after now
        // then its still active right now
        return jdbcTemplate.query(
                "SELECT * FROM UserSubscriptions WHERE uid = ? AND activeTill >= ? ORDER BY activeTill DESC",
                subscriptionInfoRowMapper,
                uid, new Timestamp(System.currentTimeMillis())
        );
    }

    public List<SubscriptionInfo> getPendingSubscriptions(Long uid) {
        return jdbcTemplate.query(
                // don't filter by activeTill as when created subscription has activeTill equal to its creation moment
                "SELECT * FROM UserSubscriptions WHERE uid = ? AND status IN ('CREATED')",
                subscriptionInfoRowMapper,
                uid
        );
    }

    public List<SubscriptionInfo> findAllByUid(Long uid) {
        List<SubscriptionInfo> result = jdbcTemplate.query(
                "SELECT * FROM UserSubscriptions WHERE uid = ?",
                subscriptionInfoRowMapper,
                uid
        );
        return result;
    }

}
