package ru.yandex.quasar.billing.dao;

import java.time.Instant;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;

import ru.yandex.quasar.billing.services.promo.Platform;
import ru.yandex.quasar.billing.services.promo.PromoProvider;

@Data
@Builder
@NoArgsConstructor
@AllArgsConstructor
public class UsedDevicePromo {
    private Long id;
    private String deviceId;
    private Platform platform;
    @Nullable
    private Long uid;
    private PromoProvider provider;
    @Nullable
    private Long codeId;
    @Nullable
    private Instant promoActivationTime;

    /**
     * if promoActivationTime is null then promo is associated with the device but not yet used
     */
    public boolean isPromoActivated() {
        return promoActivationTime != null && uid != null;
    }

}
