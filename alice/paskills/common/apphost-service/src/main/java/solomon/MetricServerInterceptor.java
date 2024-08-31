package ru.yandex.alice.paskills.common.apphost.spring.solomon;

import io.grpc.Metadata;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;

import ru.yandex.monlib.metrics.registry.MetricRegistry;

public class MetricServerInterceptor implements ServerInterceptor {

    private final MetricRegistry metricRegistry;

    public MetricServerInterceptor(MetricRegistry metricRegistry) {
        this.metricRegistry = metricRegistry;
    }

    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(
            ServerCall<ReqT, RespT> call, Metadata headers, ServerCallHandler<ReqT, RespT> next
    ) {
        ServerMetrics<ReqT, RespT> serverMetrics = new ServerMetrics<>(metricRegistry);

        var monitoringCall = new MonitoringServerCall<>(call, serverMetrics);
        var listener = next.startCall(monitoringCall, headers);

        return new MonitoringServerCallListener<>(listener, serverMetrics);
    }
}
