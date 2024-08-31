package ru.yandex.alice.paskill.dialogovo.utils.jetty;

import java.util.concurrent.atomic.LongAdder;

import org.eclipse.jetty.io.EndPoint;
import org.eclipse.jetty.server.Connector;
import org.eclipse.jetty.server.HttpConfiguration;
import org.eclipse.jetty.server.HttpConnection;

import ru.yandex.monlib.metrics.primitives.Histogram;
import ru.yandex.monlib.metrics.primitives.Rate;

public class InstrumentedHttpConnection extends HttpConnection {

    private final Rate acceptsOk;
    private final Histogram times;
    private final LongAdder concurrentConnections;

    public InstrumentedHttpConnection(HttpConfiguration config,
                                      Connector connector,
                                      EndPoint endPoint,
                                      Rate acceptsOk,
                                      Histogram times,
                                      LongAdder concurrentConnections) {
        super(config, connector, endPoint, null, true);
        this.acceptsOk = acceptsOk;
        this.times = times;
        this.concurrentConnections = concurrentConnections;
    }

    @Override
    public void onOpen() {
        acceptsOk.inc();
        super.onOpen();
        concurrentConnections.increment();
    }

    @Override
    public void onClose() {
        super.onClose();
        times.record(System.currentTimeMillis() - getCreatedTimeStamp());
        concurrentConnections.decrement();
    }
}
