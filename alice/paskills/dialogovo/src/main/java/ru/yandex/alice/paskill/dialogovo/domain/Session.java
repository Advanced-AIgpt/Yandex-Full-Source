package ru.yandex.alice.paskill.dialogovo.domain;

import java.time.Instant;
import java.util.Optional;
import java.util.UUID;

import javax.annotation.Nullable;

import lombok.Data;

import static java.lang.Math.max;

@Data
public class Session {
    private final String sessionId;
    private final long messageId;
    private final Optional<UUID> eventId;
    private final Instant startTimestamp;
    private final boolean ended;
    private final ProactiveSkillExitState proactiveSkillExitState;
    private final ActivationSourceType activationSourceType;
    private final long appMetricaEventCounter;
    private final long failCounter;

    @SuppressWarnings("ParameterNumber")
    private Session(
            String sessionId,
            long messageId,
            Optional<UUID> eventId,
            Instant startTimestamp,
            boolean isEnded,
            @Nullable ProactiveSkillExitState proactiveSkillExitState,
            ActivationSourceType activationSourceType,
            long appMetricaEventCounter,
            long failCounter
    ) {
        this.sessionId = sessionId;
        this.messageId = messageId;
        this.eventId = eventId;
        this.startTimestamp = startTimestamp;
        this.ended = isEnded;
        this.proactiveSkillExitState = proactiveSkillExitState != null
                ? proactiveSkillExitState
                : ProactiveSkillExitState.createEmpty();
        this.activationSourceType = activationSourceType;
        this.appMetricaEventCounter = max(1L, appMetricaEventCounter);
        this.failCounter = failCounter;
    }

    public String getSessionId() {
        return sessionId;
    }

    public long getMessageId() {
        return messageId;
    }

    public Optional<UUID> getEventId() {
        return eventId;
    }

    public Instant getStartTimestamp() {
        return startTimestamp;
    }

    public ProactiveSkillExitState getProactiveSkillExitState() {
        return proactiveSkillExitState;
    }

    public ActivationSourceType getActivationSourceType() {
        return activationSourceType;
    }

    public Long getAppMetricaEventCounter() {
        return appMetricaEventCounter;
    }

    public Long getFailCounter() {
        return failCounter;
    }

    public static Session create(ActivationSourceType activationSourceType, Instant requestTime) {
        return create(UUID.randomUUID().toString(), 0, requestTime, false, ProactiveSkillExitState.createEmpty(),
                activationSourceType);
    }

    public static Session create(ActivationSourceType activationSourceType, int messageId, Instant requestTime) {
        return create(UUID.randomUUID().toString(), messageId, requestTime, false,
                ProactiveSkillExitState.createEmpty(),
                activationSourceType);
    }

    public static Session create(ActivationSourceType activationSourceType, int messageId, Instant requestTime,
                                 Long appMetricaEventCounter) {
        return create(UUID.randomUUID().toString(), messageId, requestTime, false,
                ProactiveSkillExitState.createEmpty(), activationSourceType, appMetricaEventCounter, 0);
    }

    public static Session create(
            String sessionId,
            long messageId,
            Instant startTimestamp,
            boolean isEnded,
            @Nullable ProactiveSkillExitState proactiveSkillExitState,
            ActivationSourceType activationSourceType
    ) {
        return create(
                sessionId,
                messageId,
                Optional.empty(),
                startTimestamp,
                isEnded,
                proactiveSkillExitState,
                activationSourceType
        );
    }

    @SuppressWarnings("ParameterNumber")
    public static Session create(
            String sessionId,
            long messageId,
            Instant startTimestamp,
            boolean isEnded,
            @Nullable ProactiveSkillExitState proactiveSkillExitState,
            ActivationSourceType activationSourceType,
            long appMetricaEventCounter,
            long failCounter
    ) {
        return create(
                sessionId,
                messageId,
                Optional.empty(),
                startTimestamp,
                isEnded,
                proactiveSkillExitState,
                activationSourceType,
                appMetricaEventCounter,
                failCounter
        );
    }

    public static Session create(
            String sessionId,
            long messageId,
            Optional<UUID> eventId,
            Instant startTimestamp,
            boolean isEnded,
            @Nullable ProactiveSkillExitState proactiveSkillExitState,
            ActivationSourceType activationSourceType
    ) {
        //eventCounter начинаем с 1, резервируя 0 для newUserBySkillAppMetrica ивента
        return new Session(
                sessionId,
                messageId,
                eventId,
                startTimestamp,
                isEnded,
                proactiveSkillExitState,
                activationSourceType,
                1,
                0
        );
    }

    @SuppressWarnings("ParameterNumber")
    public static Session create(
            String sessionId,
            long messageId,
            Optional<UUID> eventId,
            Instant startTimestamp,
            boolean isEnded,
            @Nullable ProactiveSkillExitState proactiveSkillExitState,
            ActivationSourceType activationSourceType,
            long appMetricaEventCounter,
            long failCounter
    ) {
        //eventCounter начинаем с 1, резервируя 0 для newUserBySkillAppMetrica ивента
        return new Session(
                sessionId,
                messageId,
                eventId,
                startTimestamp,
                isEnded,
                proactiveSkillExitState,
                activationSourceType,
                appMetricaEventCounter,
                failCounter
        );
    }


    public Session getNext() {
        return new Session(
                sessionId,
                messageId + 1,
                Optional.empty(),
                startTimestamp,
                false,
                proactiveSkillExitState,
                activationSourceType,
                appMetricaEventCounter,
                0
        );
    }

    public Session getNext(ActivationSourceType activationSourceType) {
        return new Session(
                sessionId,
                messageId + 1,
                Optional.empty(),
                startTimestamp,
                false,
                proactiveSkillExitState,
                activationSourceType,
                appMetricaEventCounter,
                0
        );
    }

    public Session getNext(ActivationSourceType activationSourceType, long appmetricaEventCounter) {
        return new Session(
                sessionId,
                messageId + 1,
                Optional.empty(),
                startTimestamp,
                false,
                proactiveSkillExitState,
                activationSourceType,
                appmetricaEventCounter,
                0
        );
    }

    public Session getEnded() {
        return new Session(
                sessionId,
                messageId,
                Optional.empty(),
                startTimestamp,
                true,
                proactiveSkillExitState,
                activationSourceType,
                appMetricaEventCounter,
                failCounter
        );
    }

    public Session withProactiveSkillExitState(ProactiveSkillExitState newProactiveSkillExitState) {
        return new Session(
                sessionId,
                messageId,
                Optional.empty(),
                startTimestamp,
                ended,
                newProactiveSkillExitState,
                activationSourceType,
                appMetricaEventCounter,
                failCounter
        );
    }

    public Session withAppmetricaEventCounter(long newAppMetricaEventCounter) {
        return new Session(
                sessionId,
                messageId,
                Optional.empty(),
                startTimestamp,
                ended,
                proactiveSkillExitState,
                activationSourceType,
                newAppMetricaEventCounter,
                failCounter
        );
    }

    public Session withIncFailCounter() {
        return new Session(
                sessionId,
                messageId,
                Optional.empty(),
                startTimestamp,
                ended,
                proactiveSkillExitState,
                activationSourceType,
                appMetricaEventCounter,
                failCounter + 1
        );
    }

    public boolean isNew() {
        return messageId == 0;
    }

    // isEnded lombok getter is not generated because of getEnded method presence
    public boolean isEnded() {
        return ended;
    }

    // otherwise lombok toString leads to StackOverflowException
    @Override
    public String toString() {
        return "Session{" +
                "sessionId='" + sessionId + '\'' +
                ", messageId=" + messageId +
                ", eventId=" + eventId.map(UUID::toString).orElse("") +
                ", startTimestamp=" + startTimestamp +
                ", isEnded=" + ended +
                ", proactiveSkillExitState=" + proactiveSkillExitState.toString() +
                ", failCounter=" + failCounter +
                '}';
    }

    @Data
    public static class ProactiveSkillExitState {

        private final long suggestedExitAtMessageId;
        private final long doNotUnderstandReplyCounter;

        /**
         * a flag that disables all proactive suggests for skill responses
         * should only be used in VINS requests from developer console
         */

        private ProactiveSkillExitState(
                long suggestedExitAtMessageId,
                long doNotUnderstandReplyCounter
        ) {
            this.suggestedExitAtMessageId = suggestedExitAtMessageId;
            this.doNotUnderstandReplyCounter = doNotUnderstandReplyCounter;
        }

        public long getSuggestedExitAtMessageId() {
            return suggestedExitAtMessageId;
        }

        public long getDoNotUnderstandReplyCounter() {
            return doNotUnderstandReplyCounter;
        }

        public static ProactiveSkillExitState create(long suggestedExitAtMessageId, long doNotUnderstandReplyCounter) {
            return new ProactiveSkillExitState(suggestedExitAtMessageId, doNotUnderstandReplyCounter);
        }

        public static ProactiveSkillExitState createEmpty() {
            return new ProactiveSkillExitState(0, 0);
        }

        @Override
        public String toString() {
            return "ProactiveSkillExitState{ "
                    + "suggestedExitAtMessageId=" + suggestedExitAtMessageId
                    + ", doNotUnderstandReplyCounter=" + doNotUnderstandReplyCounter
                    + "}";
        }
    }
}
