package ru.yandex.alice.paskills.common.apphost.spring.solomon;

import io.grpc.ForwardingServerCallListener;
import io.grpc.ServerCall;

public class MonitoringServerCallListener<ReqT, RespT>
        extends ForwardingServerCallListener.SimpleForwardingServerCallListener<ReqT> {
    private final ServerMetrics<ReqT, RespT> serverMetrics;

    protected MonitoringServerCallListener(
            ServerCall.Listener<ReqT> delegate,
            ServerMetrics<ReqT, RespT> serverMetrics
    ) {
        super(delegate);
        this.serverMetrics = serverMetrics;
    }

    @Override
    public void onMessage(ReqT message) {
        serverMetrics.onMessageReceived(message);
        super.onMessage(message);
    }
}
