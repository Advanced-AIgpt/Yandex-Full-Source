package ru.yandex.quasar.billing.dao;

import java.util.Optional;

import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.services.promo.Platform;

public interface PromoCodeBaseDao {
    PromoCodeBase save(PromoCodeBase promoCodeBase);

    Optional<PromoCodeBase> findById(Long id);

    Optional<PromoCodeBase> queryNextUnusedCode(String providerName, PromoType promoType, Platform platform);
}
