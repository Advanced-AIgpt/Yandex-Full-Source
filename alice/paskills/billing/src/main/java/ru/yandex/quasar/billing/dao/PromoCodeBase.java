package ru.yandex.quasar.billing.dao;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;
import lombok.ToString;

import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.services.promo.Platform;

@Getter
@Setter
@NoArgsConstructor
@AllArgsConstructor
@ToString
public class PromoCodeBase {
    @Nullable
    private Long id;
    private String provider;
    private PromoType promoType;
    private String code;
    @Nullable
    private Platform platform;
    @Nullable
    private Integer prototypeId;

    @Nullable
    private String taskId;

    public static PromoCodeBase create(String provider, PromoType promoType, String code) {
        return new PromoCodeBase(null, provider, promoType, code, null, null, null);
    }

    public static PromoCodeBase create(String provider, PromoType promoType, String code, Platform platform) {
        return new PromoCodeBase(null, provider, promoType, code, platform, null, null);
    }

    public static PromoCodeBase create(String provider, PromoType promoType, String code, Platform platform,
                                       Integer prototypeId) {
        return new PromoCodeBase(null, provider, promoType, code, platform, prototypeId, null);
    }

    public static PromoCodeBase create(String provider, PromoType promoType, String code, Platform platform,
                                       Integer prototypeId, String taskId) {
        return new PromoCodeBase(null, provider, promoType, code, platform, prototypeId, taskId);
    }
}
