package ru.yandex.alice.paskills.common.tvm.solomon;

import java.util.function.Supplier;

import ru.yandex.alice.paskills.common.solomon.utils.RequestSensors;
import ru.yandex.monlib.metrics.histogram.HistogramCollector;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.passport.tvmauth.BlackboxEnv;
import ru.yandex.passport.tvmauth.CheckedServiceTicket;
import ru.yandex.passport.tvmauth.CheckedUserTicket;
import ru.yandex.passport.tvmauth.ClientStatus;
import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.passport.tvmauth.roles.Roles;

public class TvmClientWithSolomon implements TvmClient {
    private final TvmClient delegate;
    private final RequestSensors getStatusSensor;
    private final RequestSensors getServiceTicketSensor;
    private final RequestSensors checkServiceTicketSensor;
    private final RequestSensors checkUserTicketSensor;

    public TvmClientWithSolomon(TvmClient tvmClient, MetricRegistry registry) {
        this.delegate = tvmClient;
        Supplier<HistogramCollector> bins =
                () -> Histograms.exponential(19, 1.5d, 1d);
        this.getStatusSensor =
                RequestSensors.withLabels(registry,
                        Labels.of("target", "tvm_client", "method", "status"), null, bins);
        this.getServiceTicketSensor =
                RequestSensors.withLabels(registry,
                        Labels.of("target", "tvm_client", "method", "get_service_ticket"), null, bins);
        this.checkServiceTicketSensor =
                RequestSensors.withLabels(registry,
                        Labels.of("target", "tvm_client", "method", "check_service_ticket"), null, bins);
        this.checkUserTicketSensor =
                RequestSensors.withLabels(registry,
                        Labels.of("target", "tvm_client", "method", "check_user_ticket"), null, bins);
    }

    @Override
    public ClientStatus getStatus() {
        return getStatusSensor.measure(delegate::getStatus);
    }

    @Override
    public String getServiceTicketFor(String alias) {
        return getServiceTicketSensor.measure(() -> delegate.getServiceTicketFor(alias));
    }

    @Override
    public String getServiceTicketFor(int clientId) {
        return getServiceTicketSensor.measure(() -> delegate.getServiceTicketFor(clientId));
    }

    @Override
    public CheckedServiceTicket checkServiceTicket(String ticketBody) {
        return checkServiceTicketSensor.measure(() -> delegate.checkServiceTicket(ticketBody));
    }

    @Override
    public CheckedUserTicket checkUserTicket(String ticketBody) {
        return checkUserTicketSensor.measure(() -> delegate.checkUserTicket(ticketBody));
    }

    @Override
    public CheckedUserTicket checkUserTicket(String ticketBody, BlackboxEnv env) {
        return checkUserTicketSensor.measure(() -> delegate.checkUserTicket(ticketBody, env));
    }

    @Override
    public Roles getRoles() {
        return null;
    }

    @Override
    public void close() {
        delegate.close();
    }
}
