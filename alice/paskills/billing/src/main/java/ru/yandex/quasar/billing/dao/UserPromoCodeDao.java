package ru.yandex.quasar.billing.dao;

import java.util.List;
import java.util.Optional;

public interface UserPromoCodeDao {
    void save(UserPromoCode promoCode);

    Optional<UserPromoCode> findByUidAndProviderAndCode(Long uid, String provider, String promoCode);

    List<UserPromoCode> findAllByUid(Long uid);
}
