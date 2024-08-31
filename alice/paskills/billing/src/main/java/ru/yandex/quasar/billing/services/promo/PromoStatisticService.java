package ru.yandex.quasar.billing.services.promo;

import java.util.List;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.Data;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.stereotype.Component;

import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.dao.PromoStatistics;
import ru.yandex.quasar.billing.dao.UsedDevicePromoDao;

@Component
public class PromoStatisticService {
    private static final String DEFAULT_LABEL_VALUE_IF_NULL = "any";

    private final UsedDevicePromoDao usedDevicePromoDao;
    private final MetricRegistry promoMetricRegistry;

    private final Set<RecordKey> recordsToSkip = Set.of(
            new RecordKey(PromoType.amediateka90.name(), null),
            new RecordKey("ivi60", null),
            new RecordKey(PromoType.kinopoisk_a6m_plus6m.name(), null),
            new RecordKey(PromoType.plus90_kz.name(), null),
            new RecordKey(PromoType.plus360_kz.name(), null),
            new RecordKey(PromoType.plus90_by.name(), null),
            new RecordKey(PromoType.plus180_by.name(), null),
            new RecordKey(PromoType.plus360_by.name(), Platform.YANDEXSTATION_2.getName())
    );

    public PromoStatisticService(
            UsedDevicePromoDao usedDevicePromoDao,
            @Qualifier("promoMetricRegistry") MetricRegistry promoMetricRegistry
    ) {
        this.usedDevicePromoDao = usedDevicePromoDao;
        this.promoMetricRegistry = promoMetricRegistry;
    }

    public void writePromoStatisticsToSolomon() {
        List<PromoStatistics> promoStatistics = usedDevicePromoDao.getPromoStatistics();

        for (var promoStat : promoStatistics) {
            String platform = promoStat.getPlatform() != null ? promoStat.getPlatform() :
                    DEFAULT_LABEL_VALUE_IF_NULL;

            // Пишем -1 в total для recordsToSkip и промокодов с прототипом, чтобы отфильтровать лишние пачки
            // промокодов в алерте
            long totalCount =
                    (recordsToSkip.stream().noneMatch(key -> key.check(promoStat))
                            && promoStat.getPrototypeId() == null)
                            ? promoStat.getTotalCount() : -1;

            promoMetricRegistry.gaugeInt64("billing.promo.code.statistic.total.count.gaugei",
                    Labels.of(
                            "promoType", promoStat.getPromoType(),
                            "platform", platform,
                            "provider", promoStat.getProvider()
                    )).set(totalCount);

            promoMetricRegistry.gaugeInt64("billing.promo.code.statistic.left.count.gaugei",
                    Labels.of(
                            "promoType", promoStat.getPromoType(),
                            "platform", platform,
                            "provider", promoStat.getProvider()
                    )).set(promoStat.getLeftCount());
        }
    }

    @Data
    private static class RecordKey {
        private final String promoType;
        @Nullable
        private final String platform;

        boolean check(PromoStatistics statistics) {
            return promoType.equals(statistics.getPromoType()) &&
                    Objects.equals(platform, statistics.getPlatform());
        }
    }
}
