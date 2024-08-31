package ru.yandex.quasar.billing.dao;

import java.sql.Timestamp;
import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.jdbc.core.BeanPropertyRowMapper;
import org.springframework.jdbc.core.ColumnMapRowMapper;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;
import org.springframework.transaction.annotation.Propagation;
import org.springframework.transaction.annotation.Transactional;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;
import ru.yandex.alice.library.routingdatasource.ReadWriteTransactional;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.services.UnistatService;

@Component
@ReadOnlyTransactional
public class UserPurchasesDAO {

    private static final Logger log = LogManager.getLogger();

    private final JdbcTemplate jdbcTemplate;
    private final ObjectMapper objectMapper;
    private final UnistatService unistatService;
    private final BeanPropertyRowMapper<PurchaseInfo> purchaseInfoRowMapper;

    public UserPurchasesDAO(JdbcTemplate jdbcTemplate, UnistatService unistatService, ObjectMapper objectMapper) {
        this.jdbcTemplate = jdbcTemplate;
        this.unistatService = unistatService;
        this.objectMapper = objectMapper;

        // a special row mapper to read nested field from serialized data
        this.purchaseInfoRowMapper = new BillingBeanPropertyRowMapper<>(PurchaseInfo.class, objectMapper);
    }

    @ReadWriteTransactional
    public Long getNextPurchaseId() {
        List<Map<String, Object>> purchaseIdRes = jdbcTemplate.query(
                "SELECT nextval('PurchaseIdsSeq')",
                new ColumnMapRowMapper()
        );

        return (Long) purchaseIdRes.get(0).get("nextval");
    }

    @ReadWriteTransactional
    public PurchaseInfo savePurchaseInfo(PurchaseInfo purchaseInfo) {
        String contentItemStr;
        String selectedOptionStr;
        try {
            contentItemStr = purchaseInfo.getContentItem() != null ?
                    objectMapper.writeValueAsString(purchaseInfo.getContentItem()) : null;
            selectedOptionStr = objectMapper.writeValueAsString(purchaseInfo.getSelectedOption());
        } catch (JsonProcessingException e) {
            // serializations shouldn't cause errors
            throw new RuntimeException(e);
        }
        jdbcTemplate.update(
                "INSERT INTO UserPurchases (purchaseId, uid, purchaseToken, purchaseDate, contentItem, contentType, " +
                        "selectedOption, status, subscriptionId, securityToken, provider, userPrice, originalPrice, " +
                        "currencyCode, partnerId, skillInfoId, purchaseOfferId, userskillproductid, paymentProcessor," +
                        "merchantId, merchantName) " +
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                purchaseInfo.getPurchaseId(),
                purchaseInfo.getUid(),
                purchaseInfo.getPurchaseToken(),
                Timestamp.from(purchaseInfo.getPurchaseDate()),
                contentItemStr,
                purchaseInfo.getContentItem() != null ? purchaseInfo.getContentItem().getContentType().getName() :
                        ContentType.OTHER.getName(),
                selectedOptionStr,
                purchaseInfo.getStatus().toString(),
                purchaseInfo.getSubscriptionId(),
                purchaseInfo.getSecurityToken(),
                purchaseInfo.getProvider(),
                purchaseInfo.getUserPrice(),
                purchaseInfo.getOriginalPrice(),
                purchaseInfo.getCurrencyCode(),
                purchaseInfo.getPartnerId(),
                purchaseInfo.getSkillInfoId(),
                purchaseInfo.getPurchaseOfferId(),
                purchaseInfo.getUserSkillProductId(),
                purchaseInfo.getPaymentProcessor().name(),
                purchaseInfo.getMerchantId(),
                purchaseInfo.getMerchantName()
        );
        return purchaseInfo;
    }

    @ReadWriteTransactional
    public void updatePurchaseStatus(Long purchaseId, @Nonnull PurchaseInfo.Status status) {
        jdbcTemplate.update(
                "UPDATE UserPurchases SET status = ? WHERE purchaseId = ?",
                status.toString(), purchaseId
        );

        if (status.isError()) {
            unistatService.incrementStatValue("quasar_billing_purchase_errors_dmmm");
            log.warn(String.format("User purchase with id=%d gets status='%s'", purchaseId, status));
        }
    }

    @ReadWriteTransactional
    public int updateClearedStatus(Long purchaseId, Instant clearDate) {
        return jdbcTemplate.update(
                "UPDATE UserPurchases SET status = ?, cleardate = ? WHERE purchaseId = ?",
                PurchaseInfo.Status.CLEARED.toString(), Timestamp.from(clearDate), purchaseId
        );
    }

    @ReadWriteTransactional
    public void setRefunded(Long purchaseId) {
        jdbcTemplate.update(
                "UPDATE UserPurchases SET status = ?, refundDate = ? WHERE purchaseId = ?",
                PurchaseInfo.Status.REFUNDED.toString(), Timestamp.from(Instant.now()), purchaseId
        );
    }

    @ReadWriteTransactional
    public void incrementPurchaseRetries(Long purchaseId) {
        jdbcTemplate.update(
                "UPDATE UserPurchases SET retriesCount = retriesCount+1 WHERE purchaseId = ?",
                purchaseId
        );
    }

    public Optional<PurchaseInfo> getPurchaseInfo(Long purchaseId) {
        List<PurchaseInfo> result = jdbcTemplate.query(
                "SELECT * FROM UserPurchases WHERE purchaseId = ?",
                purchaseInfoRowMapper,
                purchaseId
        );

        return result.isEmpty() ? Optional.empty() : Optional.ofNullable(result.get(0));
    }

    public Optional<PurchaseInfo> getPurchaseInfo(Long uid, Long purchaseId) {
        List<PurchaseInfo> result = jdbcTemplate.query(
                "SELECT * FROM UserPurchases WHERE uid = ? AND purchaseId = ?",
                purchaseInfoRowMapper,
                uid,
                purchaseId
        );

        return result.isEmpty() ? Optional.empty() : Optional.ofNullable(result.get(0));
    }

    @Nullable
    public PurchaseInfo getPurchaseInfo(Long uid, String purchaseToken) {
        List<PurchaseInfo> result = jdbcTemplate.query(
                "SELECT * FROM UserPurchases WHERE uid = ? AND purchaseToken = ?",
                purchaseInfoRowMapper,
                uid, purchaseToken
        );

        return result.isEmpty() ? null : result.get(0);
    }

    public Optional<PurchaseInfo> getPurchaseInfo(String purchaseToken) {
        List<PurchaseInfo> result = jdbcTemplate.query(
                "SELECT * FROM UserPurchases WHERE purchaseToken = ?",
                purchaseInfoRowMapper,
                purchaseToken
        );

        return result.isEmpty() ? Optional.empty() : Optional.ofNullable(result.get(0));
    }

    public Optional<PurchaseInfo> findByPurchaseTokenAndPaymentProcessor(String purchaseToken,
                                                                         PaymentProcessor processor) {
        List<PurchaseInfo> result = jdbcTemplate.query(
                "SELECT * FROM UserPurchases WHERE purchaseToken = ? and paymentProcessor = ?",
                purchaseInfoRowMapper,
                purchaseToken, processor.name()
        );

        return result.isEmpty() ? Optional.empty() : Optional.ofNullable(result.get(0));
    }

    @Nullable
    public PurchaseInfo.Status getPurchaseStatus(Long uid, String purchaseToken) {
        List<Map<String, Object>> result = jdbcTemplate.query(
                "SELECT status FROM UserPurchases WHERE uid = ? AND purchaseToken = ?",
                new ColumnMapRowMapper(),
                uid, purchaseToken
        );

        return result.isEmpty() ? null : PurchaseInfo.Status.valueOf(result.get(0).get("status").toString());
    }

    public Integer getPurchaseRetries(Long uid) {
        List<Map<String, Object>> result = jdbcTemplate.query(
                "SELECT coalesce(retriesCount,0) as retriesCount FROM UserPurchases WHERE uid = ?",
                new ColumnMapRowMapper(),
                uid
        );

        return result.isEmpty() ? 0 : (Integer) result.get(0).get("retriesCount");
    }

    @ReadWriteTransactional
    public void updatePurchaseCallbackDate(Long purchaseId, Timestamp callbackDate) {
        jdbcTemplate.update(
                "UPDATE UserPurchases SET callbackDate = ? WHERE purchaseId = ?",
                callbackDate, purchaseId
        );
    }

    @Scheduled(fixedDelay = 1000 * 60, initialDelay = 1000 * 5)
    void logStuckPurchasesCount() {
        List<Map<String, Object>> queryRes = jdbcTemplate.query(
                "SELECT COUNT(*) AS cnt FROM UserPurchases WHERE status IN ('STARTED','PROCESSED') AND purchaseDate <" +
                        " ?",
                new ColumnMapRowMapper(),
                new Timestamp(System.currentTimeMillis() - TimeUnit.MINUTES.toMillis(5))
        );

        Long cnt = (Long) queryRes.get(0).get("cnt");

        unistatService.setStatValue(
                "quasar_billing_stuck-purchases-count_axxv",
                cnt
        );
    }

    //TODO: сделать поддержку полноценного paging-а
    public List<PurchaseInfo> getLastPurchases(Long uid) {
        return jdbcTemplate.query(
                "SELECT * FROM UserPurchases WHERE uid = ? AND status in ('WAITING_FOR_CLEARING', 'CLEARED') AND " +
                        "(paymentProcessor in ('TRUST','MEDIABILLING') or paymentProcessor is null) ORDER BY " +
                        "purchaseDate DESC LIMIT 25",
                purchaseInfoRowMapper,
                uid
        );
    }

    public List<PurchaseInfo> getPurchases(Long uid, long limit, long offset) {
        return jdbcTemplate.query(
                "select\n" +
                        "    *\n" +
                        "from UserPurchases\n" +
                        "where uid = ?\n" +
                        "    and (paymentProcessor in ('TRUST', 'YANDEX_PAY', 'FREE') or paymentProcessor is null)\n" +
                        "order by purchaseDate desc\n" +
                        "limit ? offset ?",
                purchaseInfoRowMapper,
                uid,
                limit,
                offset
        );
    }

    public List<PurchaseInfo> getLastPurchases(Long uid, Timestamp after) {
        return jdbcTemplate.query(
                "SELECT * FROM UserPurchases WHERE uid = ? AND status in ('WAITING_FOR_CLEARING', 'CLEARED') AND " +
                        "(paymentProcessor in ('TRUST','MEDIABILLING') or paymentProcessor is null) AND purchaseDate " +
                        "> ? ORDER BY purchaseDate DESC LIMIT 25",
                purchaseInfoRowMapper,
                uid, after
        );
    }

    //TODO: сделать поддержку полноценного paging-а
    public List<PurchaseInfo> getLastContentPurchases(Long uid) {
        return jdbcTemplate.query(
                "SELECT * FROM UserPurchases WHERE uid = ? AND status in ('WAITING_FOR_CLEARING', 'CLEARED') AND " +
                        "(paymentProcessor in ('TRUST','MEDIABILLING') or paymentProcessor is null) AND contentType " +
                        "!= 'subscription' ORDER BY purchaseDate DESC LIMIT 25",
                purchaseInfoRowMapper,
                uid
        );
    }

    public List<PurchaseInfo> findAllByUid(Long uid) {
        return jdbcTemplate.query(
                "SELECT * FROM UserPurchases WHERE uid = ?",
                purchaseInfoRowMapper,
                uid
        );
    }

    public List<PurchaseInfo> getAllSucceededByPurchaseOfferId(Long purchaseOfferId) {
        return jdbcTemplate.query(
                "select * from UserPurchases where purchaseOfferId = ? and status in ('WAITING_FOR_CLEARING', " +
                        "'CLEARED') order by purchaseDate asc",
                purchaseInfoRowMapper,
                purchaseOfferId
        );
    }

    public List<PurchaseInfo> getAllPurchaseOfferId(Long purchaseOfferId) {
        return jdbcTemplate.query(
                "select * from UserPurchases where purchaseOfferId = ? order by purchaseDate",
                purchaseInfoRowMapper,
                purchaseOfferId
        );
    }

    @Transactional(readOnly = true, propagation = Propagation.MANDATORY)
    public List<PurchaseInfo> findAllWaitingForClearing(long startFromId, int limit) {
        return jdbcTemplate.query(
                "select * from UserPurchases where status = 'WAITING_FOR_CLEARING' and purchaseId > ? ORDER BY " +
                        "purchaseId LIMIT ? FOR UPDATE SKIP LOCKED",
                purchaseInfoRowMapper,
                startFromId, limit
        );
    }
}
