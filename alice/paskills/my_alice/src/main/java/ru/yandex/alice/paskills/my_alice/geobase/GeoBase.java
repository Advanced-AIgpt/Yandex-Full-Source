package ru.yandex.alice.paskills.my_alice.geobase;

import java.io.IOException;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.alice.paskills.my_alice.solomon.MethodRate;
import ru.yandex.geobase6.Lookup;
import ru.yandex.geobase6.Timezone;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.web.apphost.api.request.RequestContext;

public class GeoBase {

    private static final Logger logger = LogManager.getLogger();

    private final Laas laas;
    private final Lookup lookup;
    private final Timezone moscow;

    private final MethodRate timezoneParse;

    GeoBase(Laas laas, String path, MetricRegistry metricRegistry) {
        this.laas = laas;
        this.lookup = new Lookup(path);
        try {
            this.moscow = lookup.getTimezoneById(213);
        } catch (Exception e) {
            logger.error("Failed to create Geobase bean", e);
            throw e;
        }
        this.timezoneParse = new MethodRate(metricRegistry, "geobase", "timezoneParse");
    }

    public Timezone timezone(int regionId, Timezone defaultTimezone) {
        try {
            Timezone tz = lookup.getTimezoneById(regionId);
            timezoneParse.ok();
            return tz;
        } catch (Exception e) {
            logger.error("Failed to parse timezone for region {}", regionId);
            timezoneParse.error();
            return defaultTimezone;
        }
    }

    public Timezone timezone(int regionId) {
        return timezone(regionId, moscow);
    }

    public Timezone timezone(RequestContext context) {
        int regionId = laas.regionId(context);
        return timezone(regionId);
    }

    public void close() throws IOException {
        this.lookup.close();
    }


}
