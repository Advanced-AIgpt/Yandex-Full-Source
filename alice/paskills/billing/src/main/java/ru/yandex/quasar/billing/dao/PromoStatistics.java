package ru.yandex.quasar.billing.dao;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Data;

@Data
@Builder
public class PromoStatistics {
    private final String promoType;
    @Nullable
    private final String platform;
    private final String provider;
    @Nullable
    private final Integer prototypeId;
    private final long totalCount;
    private final long leftCount;
}
