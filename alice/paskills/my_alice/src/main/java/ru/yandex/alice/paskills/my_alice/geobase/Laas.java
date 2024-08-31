package ru.yandex.alice.paskills.my_alice.geobase;

import com.google.gson.annotations.SerializedName;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.ToString;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.lang.Nullable;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskills.my_alice.solomon.MethodRate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.web.apphost.api.request.RequestContext;

@Component
public class Laas {

    private static final Logger logger = LogManager.getLogger();
    private static final int MOSCOW = 213;

    private final MethodRate regionIdRate;

    public Laas(MetricRegistry metricRegistry) {
        this.regionIdRate = new MethodRate(metricRegistry, "laas", "regionId");
    }

    public int regionId(RequestContext context, int defaultRegion) {
        try {
            Region region = context.getSingleRequestItem("region").getJsonData(Region.class);
            // parse regionId to unboxed int before calling regionIdRate.ok() to prevent false positives
            int regionId = region.getRegionId();
            logger.debug("Parsed region from context: {}", region);
            regionIdRate.ok();
            return regionId;
        } catch (Exception e) {
            logger.error("Failed to parse region from request context", e);
            regionIdRate.error();
            return defaultRegion;
        }
    }

    public int regionId(RequestContext context) {
        return regionId(context, MOSCOW);
    }

    @AllArgsConstructor
    @ToString
    @EqualsAndHashCode
    private static class Region {
        // https://a.yandex-team.ru/arc/trunk/arcadia/search/begemot/rules/init/region/proto/region.proto
        // https://a.yandex-team.ru/arc/trunk/arcadia/frontend/packages/frontend-apphost-context/src/sources/region.ts

        @SerializedName("default")
        private final RegionData defaultData;

        @SerializedName("laas_real")
        @Nullable
        private final RegionData laasReal;

        @Nullable
        private final RegionData real;

        int getRegionId() {
            if (laasReal != null) {
                return laasReal.getId();
            } else if (real != null) {
                return real.getId();
            } else {
                return defaultData.getId();
            }
        }

    }

    @Data
    private static class RegionData {

        private final int id;
        private final String name;

    }

}
