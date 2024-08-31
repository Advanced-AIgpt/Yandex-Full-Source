package ru.yandex.alice.paskill.dialogovo.service.billing;

import java.time.Instant;
import java.util.Optional;
import java.util.Set;
import java.util.UUID;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.domain.Interfaces;
import ru.yandex.alice.kronstadt.core.domain.LocationInfo;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;

@Data
@JsonInclude(JsonInclude.Include.NON_ABSENT)
public class PurchaseWebhookRequest {

    @JsonProperty("session")
    private final Optional<PurchaseRequestSession> purchaseRequestSession;

    @JsonProperty("location_info")
    private final Optional<PurchaseRequestLocationInfo> purchaseRequestLocationInfo;

    private final Set<String> experiments;

    @JsonProperty("client_info")
    private final PurchaseRequestClientInfo purchaseRequestClientInfo;

    @Data
    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    public static class PurchaseRequestSession {
        @JsonProperty("session_id")
        private final String sessionId;

        @JsonProperty("message_id")
        private final long messageId;

        @JsonProperty("event_id")
        private final Optional<UUID> eventId;

        @JsonProperty("start_timestamp")
        private final long startTimestamp;

        @JsonProperty("is_ended")
        private final boolean isEnded;

        @JsonProperty("activation_source_type")
        private final ActivationSourceType activationSourceType;

        public static Optional<PurchaseRequestSession> from(Optional<Session> session) {
            return session.map(s -> new PurchaseRequestSession(
                    s.getSessionId(),
                    s.getMessageId(),
                    s.getEventId(),
                    s.getStartTimestamp().toEpochMilli(),
                    s.isEnded(),
                    s.getActivationSourceType()
            ));
        }
    }

    public Optional<Session> convertToSession(UUID eventId) {
        return purchaseRequestSession.map(session -> Session.create(
                session.sessionId,
                session.messageId,
                Optional.of(eventId),
                Instant.ofEpochMilli(session.startTimestamp),
                session.isEnded,
                null,
                session.activationSourceType
        ));
    }

    @Data
    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    public static class PurchaseRequestLocationInfo {
        private final double lat;
        private final double lon;
        private final double accuracy;

        public static Optional<PurchaseRequestLocationInfo> from(Optional<LocationInfo> locationInfo) {
            return locationInfo.map(location -> new PurchaseRequestLocationInfo(
                    location.getLat(),
                    location.getLon(),
                    location.getAccuracy()
            ));
        }
    }

    public Optional<LocationInfo> convertToLocationInfo() {
        return purchaseRequestLocationInfo
                .map(location -> new LocationInfo(location.lat, location.lon, location.accuracy));
    }

    @Data
    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    public static class PurchaseRequestClientInfo {
        @JsonProperty("app_id")
        private final String appId;

        @JsonProperty("app_version")
        private final String appVersion;

        private final String platform;

        @JsonProperty("os_version")
        private final String osVersion;

        private final String uuid;

        @JsonProperty("device_id")
        private final Optional<String> deviceId;

        private final String lang;

        private final String timezone;

        @JsonProperty("device_model")
        private final String deviceModel;

        @JsonProperty("device_manufacturer")
        private final String deviceManufacturer;

        @JsonProperty("interfaces")
        private final Interfaces interfaces;

        public static PurchaseRequestClientInfo from(ClientInfo clientInfo) {
            return new PurchaseRequestClientInfo(
                    clientInfo.getAppId(),
                    clientInfo.getAppVersion(),
                    clientInfo.getPlatform(),
                    clientInfo.getOsVersion(),
                    clientInfo.getUuid(),
                    clientInfo.getDeviceIdO(),
                    clientInfo.getLang(),
                    clientInfo.getTimezone(),
                    clientInfo.getDeviceModel(),
                    clientInfo.getDeviceManufacturer(),
                    clientInfo.getInterfaces()
            );
        }
    }

    public ClientInfo convertToClientInfo() {
        return new ClientInfo(
                purchaseRequestClientInfo.appId,
                purchaseRequestClientInfo.appVersion,
                purchaseRequestClientInfo.platform,
                purchaseRequestClientInfo.osVersion,
                purchaseRequestClientInfo.uuid,
                purchaseRequestClientInfo.deviceId.orElse(null),
                purchaseRequestClientInfo.lang,
                purchaseRequestClientInfo.timezone,
                purchaseRequestClientInfo.deviceModel,
                purchaseRequestClientInfo.deviceManufacturer,
                purchaseRequestClientInfo.interfaces
        );
    }
}
