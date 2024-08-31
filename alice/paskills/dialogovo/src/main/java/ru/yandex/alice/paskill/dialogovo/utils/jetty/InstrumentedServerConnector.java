package ru.yandex.alice.paskill.dialogovo.utils.jetty;

import java.io.IOException;

import org.eclipse.jetty.server.ConnectionFactory;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;

import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry;
import ru.yandex.monlib.metrics.primitives.Rate;

public class InstrumentedServerConnector extends ServerConnector {

    private final Rate accepts;

    public InstrumentedServerConnector(Server server, NamedSensorsRegistry sensorsRegistry,
                                       ConnectionFactory... factories) {
        super(server, factories);
        this.accepts = sensorsRegistry.rate("accepts");
    }

    @Override
    public void accept(int acceptorID) throws IOException {
        accepts.inc();
        super.accept(acceptorID);
    }
}
