package ru.yandex.quasar.billing.services;

import java.time.Instant;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Setter;
import org.springframework.stereotype.Component;

@Component
public class AuthorizationContext {

    private final ThreadLocal<UserContext> currentUserContext = ThreadLocal.withInitial(UserContext::new);

    public UserContext getUserContext() {
        return currentUserContext.get();
    }

    public void setUserContext(UserContext userContext) {
        currentUserContext.set(userContext);
    }

    @Nullable
    public String getCurrentUid() {
        return currentUserContext.get().getUid();
    }

    public void setCurrentUid(@Nullable String currentUid) {
        this.currentUserContext.get().setUid(currentUid);
    }

    @Nullable
    public String getCurrentUserTicket() {
        return this.currentUserContext.get().getUserTicket();
    }

    void setCurrentUserTicket(@Nullable String userTicket) {
        this.currentUserContext.get().setUserTicket(userTicket);
    }

    public void clearUserContext() {
        this.currentUserContext.get().clear();
    }

    @Nullable
    public String getSessionId() {
        return this.currentUserContext.get().getSessionId();
    }

    void setSessionId(@Nullable String sessionId) {
        this.currentUserContext.get().setSessionId(sessionId);
    }

    @Nullable
    public String getOauthToken() {
        return this.currentUserContext.get().getOauthToken();
    }

    void setOauthToken(@Nullable String oauthToken) {
        this.currentUserContext.get().setOauthToken(oauthToken);
    }

    @Nullable
    public String getUserIp() {
        return this.currentUserContext.get().getUserIp();
    }

    void setUserIp(@Nullable String userIp) {
        this.currentUserContext.get().setUserIp(userIp);
    }

    @Nullable
    public String getUserAgent() {
        return this.currentUserContext.get().getUserAgent();
    }

    void setUserAgent(@Nullable String userAgent) {
        this.currentUserContext.get().setUserAgent(userAgent);
    }

    @Nullable
    public String getForwardedFor() {
        return this.currentUserContext.get().getForwardedFor();
    }

    void setForwardedFor(@Nullable String forwardedFor) {
        this.currentUserContext.get().setForwardedFor(forwardedFor);
    }

    @Nullable
    public String getRequestId() {
        return this.currentUserContext.get().getRequestId();
    }

    public void setRequestId(@Nullable String requestId) {
        this.currentUserContext.get().setRequestId(requestId);
    }

    @Nullable
    public String getHost() {
        return this.currentUserContext.get().getHost();
    }

    void setHost(@Nullable String host) {
        this.currentUserContext.get().setHost(host);
    }

    @Nullable
    public String getYandexUid() {
        return this.currentUserContext.get().getYandexUid();
    }

    void setYandexUid(@Nullable String yandexUID) {
        currentUserContext.get().setYandexUid(yandexUID);
    }

    Map<String, String> getProviderTokens() {
        return this.currentUserContext.get().getProviderTokens();
    }

    @Nullable
    public String getDeviceVideoCodecs() {
        return this.currentUserContext.get().getDeviceVideoCodecs();
    }

    public void setDeviceVideoCodecs(@Nullable String deviceVideoCodecs) {
        currentUserContext.get().setDeviceVideoCodecs(deviceVideoCodecs);
    }

    @Nullable
    public String getDeviceAudioCodecs() {
        return this.currentUserContext.get().getDeviceAudioCodecs();
    }

    public void setDeviceAudioCodecs(@Nullable String deviceAudioCodecs) {
        currentUserContext.get().setDeviceAudioCodecs(deviceAudioCodecs);
    }

    @Nullable
    public String getSupportsCurrentHDCPLevel() {
        return this.currentUserContext.get().getSupportsCurrentHDCPLevel();
    }

    public void setSupportsCurrentHDCPLevel(@Nullable String supportsCurrentHDCPLevel) {
        currentUserContext.get().setSupportsCurrentHDCPLevel(supportsCurrentHDCPLevel);
    }

    @Nullable
    public String getDeviceDynamicRanges() {
        return this.currentUserContext.get().getDeviceDynamicRanges();
    }

    public void setDeviceDynamicRanges(@Nullable String deviceDynamicRanges) {
        currentUserContext.get().setDeviceDynamicRanges(deviceDynamicRanges);
    }

    @Nullable
    public String getDeviceVideoFormats() {
        return this.currentUserContext.get().getDeviceVideoFormats();
    }

    public void setDeviceVideoFormats(@Nullable String deviceVideoFormats) {
        currentUserContext.get().setDeviceVideoFormats(deviceVideoFormats);
    }

    public Set<String> getRequestExperiments() {
        return Collections.unmodifiableSet(currentUserContext.get().getRequestExperiments());
    }

    public boolean hasExperiment(String exp) {
        return getRequestExperiments().contains(exp);
    }

    public void addRequestExperiments(String experiment) {
        currentUserContext.get().getRequestExperiments().add(experiment);
    }

    public Instant getRequestTimestamp() {
        return currentUserContext.get().getRequestTimestamp();
    }

    public void setRequestTimestamp(Instant requestTimestamp) {
        currentUserContext.get().requestTimestamp = requestTimestamp;
    }

    @Getter
    @Setter(AccessLevel.PRIVATE)
    @AllArgsConstructor
    public static class UserContext {
        @Nullable
        private String uid;
        @Nullable
        private String userTicket;
        @Nullable
        private String sessionId;
        @Nullable
        private String oauthToken;
        @Nullable
        private String userIp;
        @Nullable
        private String userAgent;
        @Nullable
        private String forwardedFor;
        @Nullable
        private String requestId;
        @Nullable
        private String host;
        @Nullable
        private String yandexUid;
        private Map<String, String> providerTokens;
        @Nullable
        private String deviceVideoCodecs;
        @Nullable
        private String deviceAudioCodecs;
        @Nullable
        private String supportsCurrentHDCPLevel;
        @Nullable
        private String deviceDynamicRanges;
        @Nullable
        private String deviceVideoFormats;
        private Set<String> requestExperiments;
        @Nullable
        private Instant requestTimestamp;

        private UserContext() {
            providerTokens = new HashMap<>();
            requestExperiments = new HashSet<>();
        }

        public void clear() {
            uid = null;
            userTicket = null;
            sessionId = null;
            oauthToken = null;
            userIp = null;
            userAgent = null;
            forwardedFor = null;
            requestId = null;
            host = null;
            yandexUid = null;
            providerTokens.clear();
            deviceVideoCodecs = null;
            deviceAudioCodecs = null;
            supportsCurrentHDCPLevel = null;
            deviceDynamicRanges = null;
            deviceVideoFormats = null;
            requestExperiments.clear();
            requestTimestamp = null;
        }

        public UserContext makeCopy() {
            return new UserContext(uid, userTicket, sessionId, oauthToken, userIp,
                    userAgent, forwardedFor, requestId, host, yandexUid, new HashMap<>(providerTokens),
                    deviceVideoCodecs, deviceAudioCodecs, supportsCurrentHDCPLevel, deviceDynamicRanges,
                    deviceVideoFormats, new HashSet<>(requestExperiments), requestTimestamp);
        }
    }

}
