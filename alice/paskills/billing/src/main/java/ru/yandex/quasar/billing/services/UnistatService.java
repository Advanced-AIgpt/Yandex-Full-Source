package ru.yandex.quasar.billing.services;

import java.util.Map;
import java.util.TreeMap;
import java.util.concurrent.ConcurrentHashMap;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.util.concurrent.AtomicDouble;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.json.JSONArray;
import org.springframework.stereotype.Component;

@Component
public class UnistatService {

    private static final Logger log = LogManager.getLogger();

    // аварийный предохранитель от разрастания
    private static final int CRITICAL_SIZE = 1000;

    private static final double LOG_OF_ONE_AND_A_HALF = Math.log(1.5);

    private final Map<String, AtomicDouble> singleValues = new ConcurrentHashMap<>();
    private final Map<String, Map<Double, AtomicDouble>> histograms = new ConcurrentHashMap<>();

    /**
     * Возвращает нижнюю границу диапазона для логарифмической шкалы по основанию 1.5.
     * По мотивам
     * <a href="https://wiki.yandex-team.ru/golovan/datatypes/hgram/">
     * https://wiki.yandex-team.ru/golovan/datatypes/hgram/</a>
     */
    @VisibleForTesting
    static double rangeLowerBound(long duration) {
        return Math.pow(1.5, Math.floor(Math.log(duration) / LOG_OF_ONE_AND_A_HALF));
    }

    public String getAllInUnistatFormat() {
        JSONArray result = new JSONArray()
                .put(
                        new JSONArray()
                                .put("quasar_billing_node_alive_ammv")
                                .put(1.0)
                );

        for (Map.Entry<String, AtomicDouble> entry : singleValues.entrySet()) {
            result.put(
                    new JSONArray()
                            .put(entry.getKey())
                            .put(entry.getValue().get())
            );
        }

        for (Map.Entry<String, Map<Double, AtomicDouble>> entry : histograms.entrySet()) {
            String signal = entry.getKey();
            Map<Double, AtomicDouble> histogram = new TreeMap<>(entry.getValue());

            JSONArray histogramJson = new JSONArray();

            for (Map.Entry<Double, AtomicDouble> innerEntry : histogram.entrySet()) {
                Double rangeLowerBound = innerEntry.getKey();
                AtomicDouble rangeWeight = innerEntry.getValue();
                histogramJson.put(
                        new JSONArray()
                                .put(rangeLowerBound)
                                .put(rangeWeight)
                );
            }

            result.put(
                    new JSONArray()
                            .put(signal)
                            .put(histogramJson)
            );
        }

        result.put(
                new JSONArray()
                        .put("quasar_billing_number_of_single_valued_signals_ammv")
                        .put(singleValues.size())
        );

        result.put(
                new JSONArray()
                        .put("quasar_billing_number_of_histogram_signals_ammv")
                        .put(histograms.size())
        );

        // put total number of signals in map
        result.put(
                new JSONArray()
                        .put("quasar_billing_number_of_signals_ammv")
                        // +1 as count this signal itself
                        .put(result.length() + 1)
        );

        return result.toString();
    }

    public void incrementStatValue(String signal) {
        incrementStatValue(signal, 1.0);
    }

    public void incrementStatValue(String signal, double delta) {
        if (singleValues.size() > CRITICAL_SIZE) {
            log.error("Overflow - clearing");
            singleValues.clear();
        }

        AtomicDouble atomic = singleValues.computeIfAbsent(
                signal,
                key -> new AtomicDouble()
        );

        atomic.addAndGet(delta);
    }

    public void setStatValue(String signal, double value) {
        if (singleValues.size() > CRITICAL_SIZE) {
            log.error("Overflow - clearing");
            singleValues.clear();
        }

        AtomicDouble atomic = singleValues.computeIfAbsent(
                signal,
                key -> new AtomicDouble()
        );

        atomic.set(value);
    }

    public void addHistStatValue(String signal, double lowerBound) {
        addHistStatValue(signal, lowerBound, 1.0);
    }

    public void addHistStatValue(String signal, double lowerBound, double weight) {
        if (histograms.size() > CRITICAL_SIZE) {
            log.error("Overflow - clearing");
            histograms.clear();
        }

        Map<Double, AtomicDouble> histogram = histograms.computeIfAbsent(
                signal,
                key -> new ConcurrentHashMap<>()
        );

        AtomicDouble atomic = histogram.computeIfAbsent(
                lowerBound,
                key -> new AtomicDouble()
        );

        atomic.addAndGet(weight);
    }

    public void logOperationDurationHist(String signal, long duration) {
        addHistStatValue(signal, rangeLowerBound(duration));
    }
}
