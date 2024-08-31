package ru.yandex.alice.paskill.dialogovo.utils.jetty;

import java.util.concurrent.atomic.LongAdder;

import org.eclipse.jetty.io.Connection;
import org.eclipse.jetty.io.EndPoint;
import org.eclipse.jetty.server.Connector;
import org.eclipse.jetty.server.HttpConfiguration;
import org.eclipse.jetty.server.HttpConnectionFactory;

import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry;
import ru.yandex.monlib.metrics.primitives.Histogram;
import ru.yandex.monlib.metrics.primitives.Rate;


public class InstrumentedHttpConnectionFactory extends HttpConnectionFactory {

    private final Rate acceptsOk;
    private final Histogram times;
    private final LongAdder concurrentConnections = new LongAdder();

    public InstrumentedHttpConnectionFactory(HttpConfiguration config, NamedSensorsRegistry sensorsRegistry) {
        super(config);
        sensorsRegistry.lazyGauge("activeConnections", concurrentConnections::sum);
        this.acceptsOk = sensorsRegistry.rate("acceptsOk");
        this.times = sensorsRegistry.histogram("times");
    }

    @Override
    public Connection newConnection(Connector connector, EndPoint endPoint) {
        return configure(
                new InstrumentedHttpConnection(
                        getHttpConfiguration(),
                        connector,
                        endPoint,
                        acceptsOk,
                        times,
                        concurrentConnections),
                connector,
                endPoint);
    }
}
