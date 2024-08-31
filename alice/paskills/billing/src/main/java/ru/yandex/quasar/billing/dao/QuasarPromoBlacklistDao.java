package ru.yandex.quasar.billing.dao;

/**
 * DAO for blacklist of users for promoperiod activations
 */

public interface QuasarPromoBlacklistDao {
    /**
     * check if user is in blacklist for promo
     *
     * @param uid user identifier
     * @return if user is in blacklist
     */
    boolean userInBlacklist(String uid);
}
