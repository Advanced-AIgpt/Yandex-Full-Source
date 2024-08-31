package ru.yandex.alice.paskills.common.apphost.spring.solomon;

import io.grpc.ForwardingServerCall;
import io.grpc.Metadata;
import io.grpc.ServerCall;
import io.grpc.Status;

public class MonitoringServerCall<ReqT, RespT> extends ForwardingServerCall.SimpleForwardingServerCall<ReqT, RespT> {
    private final ServerMetrics<ReqT, RespT> serverMetrics;

    protected MonitoringServerCall(ServerCall<ReqT, RespT> delegate, ServerMetrics<ReqT, RespT> serverMetrics) {
        super(delegate);
        this.serverMetrics = serverMetrics;
    }

    @Override
    public void close(Status status, Metadata trailers) {
        super.close(status, trailers);
        serverMetrics.onClose();
    }

    @Override
    public void sendMessage(RespT message) {
        super.sendMessage(message);
        serverMetrics.onMessageSent(message);
    }
}
