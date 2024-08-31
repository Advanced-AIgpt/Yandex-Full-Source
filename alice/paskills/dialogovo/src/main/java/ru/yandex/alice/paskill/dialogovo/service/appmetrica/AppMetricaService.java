package ru.yandex.alice.paskill.dialogovo.service.appmetrica;

import java.util.List;
import java.util.concurrent.CompletableFuture;

import ru.yandex.alice.paskill.dialogovo.scenarios.news.processor.AppmetricaCommitArgs;
import ru.yandex.metrika.appmetrica.proto.AppMetricaProto;

interface AppMetricaService {
    CompletableFuture<AppMetricaServiceImpl.AppMetricaResponse> sendEventsAsync(
            String uuid,
            String apiKeyEncrypted,
            AppMetricaEvent event,
            List<AppMetricaProto.ReportMessage.Session.Event> events,
            boolean useTimestampCounter);

    CompletableFuture<AppMetricaServiceImpl.AppMetricaResponse> sendEventsAsync(
            String uuid,
            String skillId,
            String apiKeyEncrypted,
            String uri,
            Long eventEpochTme,
            AppMetricaProto.ReportMessage reportMessage,
            boolean useTimestampCounter
    );

    AppmetricaCommitArgs prepareClientEventsForCommit(String uuid,
                                                      String apiKeyEncrypted,
                                                      AppMetricaEvent appMetricaEvent,
                                                      List<AppMetricaProto.ReportMessage.Session.Event> events)
            throws AppMetricaServiceException;
}
