package ru.yandex.alice.paskill.dialogovo.service.appmetrica;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.node.ObjectNode;
import com.google.protobuf.ByteString;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.domain.LocationInfo;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ResponseAnalytics;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.processor.AppmetricaCommitArgs;
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookRequestResult;
import ru.yandex.metrika.appmetrica.proto.AppMetricaProto;

@Component
public class AppMetricaEventSender {
    private static final Logger logger = LogManager.getLogger();

    private final AppMetricaService appMetricaService;
    private final RequestContext requestContext;

    public AppMetricaEventSender(AppMetricaService appMetricaService, RequestContext requestContext) {
        this.appMetricaService = appMetricaService;
        this.requestContext = requestContext;
    }

    @SuppressWarnings("ParameterNumber")
    public void sendEndSessionEvents(SourceType source,
                                     String skillId,
                                     SkillInfo.Look look,
                                     Optional<String> encryptedAppMetricaApiKey,
                                     ClientInfo clientInfo,
                                     Optional<LocationInfo> locationInfo,
                                     Instant time,
                                     Optional<Session> session,
                                     boolean isTestRequest,
                                     boolean useTimestampCounter) {
        if (isSendMetrica(source, encryptedAppMetricaApiKey, session, isTestRequest)) {
            var appMetricaEvent = buildAppMetricaEvent(
                    clientInfo,
                    locationInfo,
                    Optional.ofNullable(requestContext.getCurrentUserId()),
                    session.get(),
                    List.of(MetricaEventConstants.INSTANCE.getEndSessionMetricaEvent()),
                    time,
                    skillId,
                    look,
                    true
            ).build();
            var eventsToSend = buildEventsToSend(appMetricaEvent, useTimestampCounter);

            appMetricaService.sendEventsAsync(
                    clientInfo.getUuid(),
                    encryptedAppMetricaApiKey.get(),
                    appMetricaEvent,
                    eventsToSend,
                    useTimestampCounter
            );
        }
    }

    @SuppressWarnings("ParameterNumber")
    public long sendClientEvents(SourceType source,
                                 String skillId,
                                 SkillInfo.Look look,
                                 Optional<String> encryptedAppMetricaApiKey,
                                 ClientInfo clientInfo,
                                 Optional<LocationInfo> locationInfo,
                                 Instant time,
                                 Optional<Session> session,
                                 MetricaEvent event,
                                 Optional<WebhookRequestResult> result,
                                 boolean isTestRequest,
                                 boolean useTimestampCounter) {
        if (isSendMetrica(source, encryptedAppMetricaApiKey, session, isTestRequest)) {
            List<MetricaEvent> events = new ArrayList<>();
            events.add(event);

            result.flatMap(WebhookRequestResult::getResponse)
                    .flatMap(WebhookResponse::getAnalytics)
                    .map(ResponseAnalytics::getEvents)
                    .ifPresent(events::addAll);

            var builder = buildAppMetricaEvent(
                    clientInfo,
                    locationInfo,
                    Optional.ofNullable(requestContext.getCurrentUserId()),
                    session.get(),
                    events,
                    time,
                    skillId,
                    look,
                    false
            );
            var appMetricaEvent = builder.build();
            var eventsToSend = buildEventsToSend(appMetricaEvent, useTimestampCounter);

            appMetricaService.sendEventsAsync(
                    clientInfo.getUuid(),
                    encryptedAppMetricaApiKey.get(),
                    appMetricaEvent,
                    eventsToSend,
                    useTimestampCounter
            );
            return session.get().getAppMetricaEventCounter() + eventsToSend.size();
        }
        //Если отправлять ивенты не нужно, то возвращаемся тот же счетчик ивентов, либо значение по умолчанию
        return session.map(Session::getAppMetricaEventCounter).orElse(1L);
    }

    @Nullable
    @SuppressWarnings("ParameterNumber")
    public AppmetricaCommitArgs prepareClientEventsForCommit(
            SourceType source,
            String skillId,
            SkillInfo.Look look,
            Optional<String> encryptedAppMetricaApiKey,
            ClientInfo clientInfo,
            Optional<LocationInfo> locationInfo,
            Instant time,
            Optional<Session> session,
            MetricaEvent event,
            Optional<WebhookRequestResult> result,
            boolean isTestRequest,
            boolean useTimestampCounter
    ) {
        if (isSendMetrica(source, encryptedAppMetricaApiKey, session, isTestRequest)) {
            List<MetricaEvent> events = new ArrayList<>();
            events.add(event);

            result.flatMap(WebhookRequestResult::getResponse)
                    .flatMap(WebhookResponse::getAnalytics)
                    .map(ResponseAnalytics::getEvents)
                    .ifPresent(events::addAll);

            var builder = buildAppMetricaEvent(
                    clientInfo,
                    locationInfo,
                    Optional.ofNullable(requestContext.getCurrentUserId()),
                    session.get(),
                    events,
                    time,
                    skillId,
                    look,
                    false
            );
            var appMetricaEvent = builder.build();
            var eventsToSend = buildEventsToSend(appMetricaEvent, useTimestampCounter);

            try {
                return appMetricaService.prepareClientEventsForCommit(
                        clientInfo.getUuid(),
                        encryptedAppMetricaApiKey.get(),
                        appMetricaEvent,
                        eventsToSend);
            } catch (AppMetricaServiceException e) {
                logger.error("Failed prepare appmetrica event for commit: {}" + e.getMessage(), e);
                return null;
            }
        }
        return null;
    }


    @SuppressWarnings("ParameterNumber")
    private AppMetricaEvent.AppMetricaEventBuilder buildAppMetricaEvent(ClientInfo clientInfo,
                                                                        Optional<LocationInfo> locationInfo,
                                                                        Optional<String> uid,
                                                                        Session session,
                                                                        List<MetricaEvent> events,
                                                                        Instant eventTime,
                                                                        String skillId,
                                                                        SkillInfo.Look skillLook,
                                                                        boolean endSession) {
        return AppMetricaEvent.builder()
                .clientInfo(clientInfo)
                .locationInfo(locationInfo)
                .uid(uid)
                .session(session)
                .events(events)
                .eventTime(eventTime)
                .skillId(skillId)
                .skillLook(skillLook)
                .endSession(endSession);
    }

    public void sendClientEvents(
            String uuid,
            String skillId,
            String apiKeyEncrypted,
            String uri,
            Long eventEpochTime,
            AppMetricaProto.ReportMessage reportMessage,
            boolean useTimestampCounter
    ) {
        //Ничего не делаем, если нам не пришла сессия для отправки
        if (reportMessage.getSessionsCount() == 0) {
            return;
        }
        appMetricaService.sendEventsAsync(uuid, skillId, apiKeyEncrypted,
                uri, eventEpochTime, reportMessage, useTimestampCounter);
    }

    private List<AppMetricaProto.ReportMessage.Session.Event> buildEventsToSend(AppMetricaEvent appMetricaEvent,
                                                                                boolean useTimestampCounter) {
        boolean isNewSession = appMetricaEvent.getSession().isNew();
        long timeFromSessionStart = getTimeFromSessionStart(
                appMetricaEvent.getSession().getStartTimestamp().getEpochSecond(),
                appMetricaEvent.getEventTime().getEpochSecond(),
                isNewSession);
        // epochMilli резервируется для EVENT_INIT_VALUE, поэтому добавляем + 1
        long inSessionEventCounter = useTimestampCounter ? appMetricaEvent.getEventTime().toEpochMilli() + 1
                : appMetricaEvent.getSession().getAppMetricaEventCounter();

        List<AppMetricaProto.ReportMessage.Session.Event> events = new ArrayList<>();

        if (isNewSession) {
            events.add(buildEvent(AppMetricaProto.ReportMessage.Session.Event.EventType.EVENT_START_VALUE,
                    inSessionEventCounter++, timeFromSessionStart, Optional.empty(), Optional.empty(),
                    appMetricaEvent.getLocationInfo()));
        } else {
            events.add(buildEvent(AppMetricaProto.ReportMessage.Session.Event.EventType.EVENT_ALIVE_VALUE,
                    inSessionEventCounter++, timeFromSessionStart, Optional.empty(), Optional.empty(),
                    appMetricaEvent.getLocationInfo()));
        }

        for (MetricaEvent event : appMetricaEvent.getEvents()) {
            events.add(buildEvent(
                    AppMetricaProto.ReportMessage.Session.Event.EventType.EVENT_CLIENT_VALUE,
                    inSessionEventCounter++,
                    timeFromSessionStart,
                    Optional.of(event.getName()),
                    Optional.ofNullable(event.getValue()),
                    appMetricaEvent.getLocationInfo()));
        }

        return events;
    }

    private static AppMetricaProto.ReportMessage.Session.Event buildEvent(int eventType,
                                                                          long number,
                                                                          long time,
                                                                          Optional<String> name,
                                                                          Optional<ObjectNode> value,
                                                                          Optional<LocationInfo> locationInfo) {
        var builder = AppMetricaProto.ReportMessage.Session.Event.newBuilder()
                .setType(eventType)
                .setTime(time)
                .setNumberInSession(number);

        name.ifPresent(builder::setName);
        locationInfo.ifPresent(info -> builder.setLocation(
                AppMetricaProto.ReportMessage.Location.newBuilder()
                        .setLat(info.getLat())
                        .setLon(info.getLon())
                        .build()
        ));
        value.ifPresent(jsonNodes -> builder.setValue(ByteString.copyFromUtf8(jsonNodes.toString())));

        return builder.build();
    }

    private static long getTimeFromSessionStart(long startTime, long actTime, boolean isNewSession) {
        long fromSessionStartEventTime = Math.max(0L, actTime - startTime);
        return isNewSession ? 0 : fromSessionStartEventTime;
    }

    private boolean isSendMetrica(SourceType source,
                                  Optional<String> encryptedAppMetricaApiKey,
                                  Optional<Session> session,
                                  boolean isTestRequest) {
        return !isTestRequest
                && session.isPresent()
                && source == SourceType.USER
                && encryptedAppMetricaApiKey.isPresent();
    }

    private boolean isSendMetrica(SourceType source, NewsSkillInfo skill) {
        return source == SourceType.USER
                && skill.getEncryptedAppMetricaApiKey().isPresent();
    }
}
