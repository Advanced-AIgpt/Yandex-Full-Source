package ru.yandex.alice.paskills.common.apphost.spring.solomon;

import java.util.function.Supplier;

import ru.yandex.monlib.metrics.histogram.HistogramCollector;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.web.apphost.grpc.proto.TServiceRequest;
import ru.yandex.web.apphost.grpc.proto.TServiceResponse;

public class ServerMetrics<ReqT, RespT> {
    private static final String REQUESTS_RATE = "grpc.total.requests";
    private static final String REQUEST_DURATION_FULL = "grpc.request.duration.full";
    private static final String REQUEST_DURATION_LOGIC = "grpc.request.duration.logic";
    private static final String REQUEST_DURATION_DECOMPR = "grpc.request.duration.decompr";
    private static final String REQUEST_DURATION_COMPR = "grpc.request.duration.compr";
    private static final String REQUEST_DURATION_SERIAL = "grpc.request.duration.serial";
    private static final String REQUEST_DURATION_DESERIAL = "grpc.request.duration.deserial";

    private static final Supplier<HistogramCollector> SENSOR_COLLECTOR_SUPPLIER =
            () -> Histograms.exponential(22, 2.0);

    private final MetricRegistry registry;
    private final long startNanos;
    private Labels commonLabels;

    public ServerMetrics(MetricRegistry registry) {
        this.registry = registry;
        this.commonLabels = Labels.empty();
        this.startNanos = System.nanoTime();
    }

    public void onMessageReceived(ReqT message) {
        if (message instanceof TServiceRequest) {
            String requestPath = ((TServiceRequest) message).getPath()
                    .replaceAll("[-/]", "_")
                    .replaceFirst("^_", "");
            if (!requestPath.isEmpty()) {
                commonLabels = commonLabels.add("path", requestPath);
            }
        }
    }

    public void onMessageSent(RespT message) {
        if (message instanceof TServiceResponse) {
            TServiceResponse response = ((TServiceResponse) message);
            registry
                    .histogramRate(REQUEST_DURATION_SERIAL, commonLabels,
                            SENSOR_COLLECTOR_SUPPLIER)
                    .record(response.getSerialTime());
            registry
                    .histogramRate(REQUEST_DURATION_DESERIAL, commonLabels,
                            SENSOR_COLLECTOR_SUPPLIER)
                    .record(response.getDeserialTime());
            registry
                    .histogramRate(REQUEST_DURATION_COMPR, commonLabels,
                            SENSOR_COLLECTOR_SUPPLIER)
                    .record(response.getComprTime());
            registry
                    .histogramRate(REQUEST_DURATION_DECOMPR, commonLabels,
                            SENSOR_COLLECTOR_SUPPLIER)
                    .record(response.getDecomprTime());
            registry
                    .histogramRate(REQUEST_DURATION_LOGIC, commonLabels,
                            SENSOR_COLLECTOR_SUPPLIER)
                    .record(response.getLogicTime());
        }
    }


    public void onClose() {
        long durationNanos = System.nanoTime() - startNanos;
        registry
                .histogramRate(REQUEST_DURATION_FULL, commonLabels,
                        SENSOR_COLLECTOR_SUPPLIER)
                .record(durationNanos / 1000);

        registry.rate(REQUESTS_RATE, commonLabels).inc();
    }
}

