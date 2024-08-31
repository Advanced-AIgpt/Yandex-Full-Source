package ru.yandex.alice.paskill.dialogovo.service.appmetrica;

import java.net.URI;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.CacheStats;
import com.google.common.cache.LoadingCache;
import lombok.Builder;
import lombok.Data;
import org.apache.commons.codec.digest.DigestUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.util.StringUtils;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.paskill.dialogovo.config.AppMetricaConfig;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.processor.AppmetricaCommitArgs;
import ru.yandex.alice.paskill.dialogovo.utils.CryptoUtils;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.metrika.appmetrica.proto.AppMetricaProto;

class AppMetricaServiceImpl implements AppMetricaService {
    private static final Logger logger = LogManager.getLogger();
    private static final Duration CACHE_EXPIRATION = Duration.ofDays(1L);
    private final DialogovoInstrumentedExecutorService executor;
    private final RestTemplate restTemplate;
    private final String secret;
    private final AppMetricaFirstUserEventDao appMetricaFirstUserEventDao;
    private final LoadingCache<String, String> apiKeyDecryptCache;
    private final LoadingCache<ApiKeySkillUserCacheKey, Boolean> apiKeyNewUserEventCache;
    private final AppMetricaConfig appMetricaConfig;

    AppMetricaServiceImpl(
            AppMetricaConfig appMetricaConfig,
            String secret,
            DialogovoInstrumentedExecutorService executor,
            RestTemplate restTemplate,
            AppMetricaFirstUserEventDao appMetricaFirstUserEventDao,
            long cacheSize
    ) {
        this.appMetricaConfig = appMetricaConfig;
        this.secret = secret;
        this.executor = executor;
        this.restTemplate = restTemplate;
        this.appMetricaFirstUserEventDao = appMetricaFirstUserEventDao;
        this.apiKeyDecryptCache = CacheBuilder.newBuilder()
                .expireAfterWrite(CACHE_EXPIRATION)
                .maximumSize(cacheSize)
                .recordStats()
                .build(new CacheLoader<>() {
                    @Override
                    public String load(String apiKeyEncrypted) {
                        return decrypt(apiKeyEncrypted);
                    }
                });

        this.apiKeyNewUserEventCache = CacheBuilder.newBuilder()
                .expireAfterWrite(CACHE_EXPIRATION)
                .maximumSize(cacheSize)
                .recordStats()
                .build(new CacheLoader<>() {
                    @Override
                    public Boolean load(ApiKeySkillUserCacheKey apiKeySkillUserCacheKey) {
                        return !appMetricaFirstUserEventExists(apiKeySkillUserCacheKey);
                    }
                });
    }

    CacheStats getApiKeyDecryptCacheStats() {
        return apiKeyDecryptCache.stats();
    }

    CacheStats getApiKeyNewUserEventCacheStats() {
        return apiKeyNewUserEventCache.stats();
    }

    @Override
    public CompletableFuture<AppMetricaResponse> sendEventsAsync(
            String uuid,
            String apiKeyEncrypted,
            AppMetricaEvent appMetricaEvent,
            List<AppMetricaProto.ReportMessage.Session.Event> events,
            boolean useTimestampCounter
    ) {
        return executor.supplyAsyncInstrumentedWithoutTimeout(() -> {
            try {
                return sendEvents(uuid, apiKeyEncrypted, appMetricaEvent,
                        events, useTimestampCounter);
            } catch (AppMetricaServiceException e) {
                logger.error("Failed sending appmetrica events: {}", e.getMessage());
                return AppMetricaResponse.FAILED_RESPONSE;
            }
        });
    }

    @Override
    public CompletableFuture<AppMetricaResponse> sendEventsAsync(String uuid, String skillId, String apiKeyEncrypted,
                                                                 String uri, Long eventEpochTme,
                                                                 AppMetricaProto.ReportMessage reportMessage,
                                                                 boolean useTimestampCounter) {
        return executor.supplyAsyncInstrumentedWithoutTimeout(() -> {
            try {
                return sendEvents(uuid, skillId, apiKeyEncrypted, uri,
                        eventEpochTme, reportMessage, useTimestampCounter);
            } catch (AppMetricaServiceException e) {
                logger.error("Failed sending appmetrica events: {}", e.getMessage());
                return AppMetricaResponse.FAILED_RESPONSE;
            }
        });
    }

    @Override
    public AppmetricaCommitArgs prepareClientEventsForCommit(String uuid,
                                                             String apiKeyEncrypted,
                                                             AppMetricaEvent appMetricaEvent,
                                                             List<AppMetricaProto.ReportMessage.Session.Event> events)
            throws AppMetricaServiceException {
        try {
            String apiKeyDecrypted = apiKeyDecryptCache.get(apiKeyEncrypted);
            String deviceId = appMetricaEvent.getClientInfo().getDeviceIdO().orElseGet(() -> DigestUtils.md5Hex(uuid));

            ClientInfo clientInfo = appMetricaEvent.getClientInfo();
            SkillInfo.Look skillLook = appMetricaEvent.getSkillLook();
            URI uri = UriComponentsBuilder.fromUriString(appMetricaConfig.getUrl())
                    .path("report")
                    .queryParam("api_key_128", apiKeyDecrypted)
                    .queryParam("uuid", uuid)
                    .queryParam("deviceid", deviceId)
                    .queryParam("analytics_sdk_version", 123)
                    .queryParam("locale", clientInfo.getLang())
                    .queryParam("app_id", "ru.yandex.alice.paskill.dialogovo")
                    .queryParam("app_platform", AppMetricaFormatConverter.mapPlatform(clientInfo, skillLook))
                    .queryParam("os_version", AppMetricaFormatConverter.mapOsVersion(clientInfo, skillLook))
                    .queryParam("model", AppMetricaFormatConverter.mapDeviceModel(clientInfo, skillLook))
                    .queryParam("manufacturer", AppMetricaFormatConverter.mapDeviceManufacturer(clientInfo, skillLook))
                    .queryParam("device_type", AppMetricaFormatConverter.mapDeviceType(clientInfo, skillLook))
                    .queryParam("app_version_name", AppMetricaFormatConverter.mapAppVersion(clientInfo, skillLook))
                    .build()
                    .toUri();

            AppMetricaProto.ReportMessage reportMessage = AppMetricaFormatConverter
                    .clientEventMessage(appMetricaEvent, appMetricaEvent.getUid(), events);

            return new AppmetricaCommitArgs(apiKeyEncrypted, uri.toASCIIString(),
                    appMetricaEvent.getEventTime().getEpochSecond(), reportMessage);
        } catch (Exception ex) {
            logger.error("Error while preparing app metrica events "
                    + appMetricaEvent + " for uuid [" + uuid + "]", ex);
            throw new AppMetricaServiceException("Error while preparing app metrica events", ex);
        }
    }

    AppMetricaResponse sendEvents(String uuid,
                                  String apiKeyEncrypted,
                                  AppMetricaEvent appMetricaEvent,
                                  List<AppMetricaProto.ReportMessage.Session.Event> events,
                                  boolean useTimestampCounter) throws AppMetricaServiceException {
        try {
            String apiKeyDecrypted = apiKeyDecryptCache.get(apiKeyEncrypted);
            String deviceId = appMetricaEvent.getClientInfo().getDeviceIdO().orElseGet(() -> DigestUtils.md5Hex(uuid));

            ClientInfo clientInfo = appMetricaEvent.getClientInfo();
            SkillInfo.Look skillLook = appMetricaEvent.getSkillLook();
            URI uri = UriComponentsBuilder.fromUriString(appMetricaConfig.getUrl())
                    .path("report")
                    .queryParam("api_key_128", apiKeyDecrypted)
                    .queryParam("uuid", uuid)
                    .queryParam("deviceid", deviceId)
                    .queryParam("analytics_sdk_version", 123)
                    .queryParam("locale", clientInfo.getLang())
                    .queryParam("app_id", "ru.yandex.alice.paskill.dialogovo")
                    .queryParam("app_platform", AppMetricaFormatConverter.mapPlatform(clientInfo, skillLook))
                    .queryParam("os_version", AppMetricaFormatConverter.mapOsVersion(clientInfo, skillLook))
                    .queryParam("model", AppMetricaFormatConverter.mapDeviceModel(clientInfo, skillLook))
                    .queryParam("manufacturer", AppMetricaFormatConverter.mapDeviceManufacturer(clientInfo, skillLook))
                    .queryParam("device_type", AppMetricaFormatConverter.mapDeviceType(clientInfo, skillLook))
                    .queryParam("app_version_name", AppMetricaFormatConverter.mapAppVersion(clientInfo, skillLook))
                    .build()
                    .toUri();

            ApiKeySkillUserCacheKey apiKeySkillUserCacheKey = ApiKeySkillUserCacheKey
                    .builder()
                    .apiKeyEncrypted(apiKeyEncrypted)
                    .skillId(appMetricaEvent.getSkillId())
                    .uuid(uuid)
                    .build();
            boolean newUserBySkillAppMetricaApiKey = apiKeyNewUserEventCache.get(apiKeySkillUserCacheKey);

            List<AppMetricaProto.ReportMessage.Session.Event> eventsToSend = new ArrayList<>(events);

            if (newUserBySkillAppMetricaApiKey) {
                //New client event will always has 0 number
                AppMetricaProto.ReportMessage.Session.Event.Builder builder =
                        AppMetricaProto.ReportMessage.Session.Event.newBuilder()
                                .setNumberInSession(useTimestampCounter
                                        ? appMetricaEvent.getEventTime().toEpochMilli() : 0L)
                                .setType(AppMetricaProto.ReportMessage.Session.Event.EventType.EVENT_INIT_VALUE)
                                .setTime(0);
                AppMetricaProto.ReportMessage.Session.Event newProfileEvent = builder.build();

                eventsToSend.add(0, newProfileEvent);
            }

            AppMetricaProto.ReportMessage reportMessage = AppMetricaFormatConverter
                    .clientEventMessage(appMetricaEvent, appMetricaEvent.getUid(), eventsToSend);

            HttpHeaders headers = new HttpHeaders();
            headers.add(HttpHeaders.CONTENT_TYPE, "application/x-protobuf");
            HttpEntity<AppMetricaProto.ReportMessage> httpEntity = new HttpEntity<>(reportMessage, headers);
            logger.debug("Send appmetrica event uri: [{}] report-message: [{}] headers: [{}]",
                    () -> uri.toASCIIString().replace(apiKeyDecrypted, "***"),
                    reportMessage::toString,
                    httpEntity::getHeaders);

            AppMetricaResponse appMetricaResponse = restTemplate.postForObject(uri, httpEntity,
                    AppMetricaResponse.class);

            if (!appMetricaResponse.isOk()) {
                throw new Exception("Received incorrect appmetrica response status = [" +
                        appMetricaResponse.getStatus() + "]");
            }

            if (newUserBySkillAppMetricaApiKey) {
                appMetricaFirstUserEventDao.saveFirstUserEvent(
                        apiKeyEncrypted,
                        appMetricaEvent.getSkillId(),
                        uuid,
                        appMetricaEvent.getEventTime());
                // if new user - do not cache newUserBySkillAppMetricaApiKey=true result
                apiKeyNewUserEventCache.invalidate(apiKeySkillUserCacheKey);
            }

            return appMetricaResponse;
        } catch (Exception ex) {
            logger.error("Error while sending app metrica events " + appMetricaEvent + " for uuid [" + uuid + "]", ex);
            throw new AppMetricaServiceException("Error while sending app metrica events", ex);
        }
    }

    @SuppressWarnings("ParameterCount")
    AppMetricaResponse sendEvents(String uuid,
                                  String skillId,
                                  String apiKeyEncrypted,
                                  String uri,
                                  long eventEpochTime,
                                  AppMetricaProto.ReportMessage reportMessage,
                                  boolean useTimestampCounter) throws AppMetricaServiceException {
        try {
            String apiKeyDecrypted = apiKeyDecryptCache.get(apiKeyEncrypted);

            ApiKeySkillUserCacheKey apiKeySkillUserCacheKey = ApiKeySkillUserCacheKey
                    .builder()
                    .apiKeyEncrypted(apiKeyEncrypted)
                    .skillId(skillId)
                    .uuid(uuid)
                    .build();
            boolean newUserBySkillAppMetricaApiKey = apiKeyNewUserEventCache.get(apiKeySkillUserCacheKey);

            AppMetricaProto.ReportMessage.Builder reportMessage2 = reportMessage.toBuilder();

            if (newUserBySkillAppMetricaApiKey) {
                //New client event will always has 0 number
                AppMetricaProto.ReportMessage.Session.Event.Builder builder =
                        AppMetricaProto.ReportMessage.Session.Event.newBuilder()
                                .setNumberInSession(useTimestampCounter ? eventEpochTime : 0L)
                                .setType(AppMetricaProto.ReportMessage.Session.Event.EventType.EVENT_INIT_VALUE)
                                .setTime(0);
                AppMetricaProto.ReportMessage.Session.Event newProfileEvent = builder.build();

                reportMessage2.setSessions(0, reportMessage2.getSessions(0).toBuilder().addEvents(0, newProfileEvent));
            }

            HttpHeaders headers = new HttpHeaders();
            headers.add(HttpHeaders.CONTENT_TYPE, "application/x-protobuf");
            HttpEntity<AppMetricaProto.ReportMessage> httpEntity = new HttpEntity<>(reportMessage2.build(), headers);
            logger.info("Send appmetrica event uri: [{}] report-message: [{}] headers: [{}]",
                    () -> uri.replace(apiKeyDecrypted, "***"),
                    reportMessage2::toString,
                    httpEntity::getHeaders);

            AppMetricaResponse appMetricaResponse = restTemplate.postForObject(uri, httpEntity,
                    AppMetricaResponse.class);

            if (!appMetricaResponse.isOk()) {
                throw new Exception("Received incorrect appmetrica event response status = [" +
                        appMetricaResponse.getStatus() + "]");
            }

            if (newUserBySkillAppMetricaApiKey) {
                appMetricaFirstUserEventDao.saveFirstUserEvent(
                        apiKeyEncrypted,
                        skillId,
                        uuid,
                        Instant.ofEpochSecond(eventEpochTime));
                // if new user - do not cache newUserBySkillAppMetricaApiKey=true result
                apiKeyNewUserEventCache.invalidate(apiKeySkillUserCacheKey);
            }

            return appMetricaResponse;
        } catch (Exception ex) {
            logger.error("Error while sending app metrica events for uuid [" + uuid + "]", ex);
            throw new AppMetricaServiceException("Error while sending app metrica events for uuid [" + uuid + "]", ex);
        }
    }

    private String decrypt(String apiKeyEncrypted) {
        return CryptoUtils.decrypt(secret, apiKeyEncrypted);
    }

    private boolean appMetricaFirstUserEventExists(ApiKeySkillUserCacheKey apiKeySkillUserCacheKey) {
        return appMetricaFirstUserEventDao.firstUserEventExists(
                apiKeySkillUserCacheKey.apiKeyEncrypted,
                apiKeySkillUserCacheKey.getSkillId(),
                apiKeySkillUserCacheKey.getUuid()
        );
    }

    @Data
    static class AppMetricaResponse {
        public static final String SUCCESS_STATUS = "accepted";
        public static final AppMetricaResponse FAILED_RESPONSE = new AppMetricaResponse("failed");

        private final String status;

        @JsonIgnore
        public boolean isOk() {
            return !StringUtils.isEmpty(status) && status.equals(SUCCESS_STATUS);
        }
    }

    @Data
    @Builder
    static class ApiKeySkillUserCacheKey {
        private final String apiKeyEncrypted;
        private final String skillId;
        private final String uuid;
    }
}
