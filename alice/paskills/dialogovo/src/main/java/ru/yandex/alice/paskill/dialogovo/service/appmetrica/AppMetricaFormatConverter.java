package ru.yandex.alice.paskill.dialogovo.service.appmetrica;

import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.TimeZone;

import com.google.common.collect.ImmutableMap;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.domain.LocationInfo;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.metrika.appmetrica.proto.AppMetricaProto;
import ru.yandex.metrika.appmetrica.proto.AppMetricaProto.ReportMessage;

class AppMetricaFormatConverter {

    private static final Map<String, String> APP_ID_TO_DEVICE_MAPPINGS = ImmutableMap.<String, String>builder()
            .put("ru.yandex.searchplugin", "yandex.search.app")
            .put("ru.yandex.mobile", "yandex.search.app")
            .put("ru.yandex.searchplugin.beta", "yandex.search.app")
            .put("ru.yandex.mobile.search", "yandex.browser")
            .put("ru.yandex.mobile.search.ipad", "yandex.browser")
            .put("com.yandex.browser", "yandex.browser")
            .put("com.yandex.browser.alpha", "yandex.browser")
            .put("com.yandex.browser.beta", "yandex.browser")
            .put("ru.yandex.yandexnavi", "yandex.navigator")
            .put("ru.yandex.mobile.navigator", "yandex.navigator")
            .put("ru.yandex.yandexmaps", "yandex.maps")
            .put("ru.yandex.traffic", "yandex.maps")
            .put("com.yandex.launcher", "yandex.launcher")
            .put("ru.yandex.quasar.services", "yandex.station")
            .put("yandex.auto", "yandex.auto")
            .put("ru.yandex.autolauncher", "yandex.auto")
            .put("ru.yandex.iosdk.elariwatch", "elariwatch")
            .put("aliced", "yandex.station.mini")
            .put("YaBro", "yandex.browser")
            .put("YaBro.beta", "yandex.browser")
            .put("ru.yandex.quasar.app", "yandex.station")
            .put("yandex.auto.old", "yandex.auto")
            .put("ru.yandex.mobile.music", "yandex.music")
            .put("ru.yandex.music", "yandex.music")
            .build();

    private AppMetricaFormatConverter() {
        throw new UnsupportedOperationException();
    }

    public static ReportMessage clientEventMessage(
            AppMetricaEvent appMetricaEvent,
            //TODO: use later for account.id
            Optional<String> profileId,
            List<AppMetricaProto.ReportMessage.Session.Event> events) {
        int offsetInSeconds = TimeZone.getTimeZone(appMetricaEvent.getClientInfo().getTimezone()).getRawOffset() / 1000;
        long sessionId = getSessionId(appMetricaEvent.getSession());

        ReportMessage.Session.SessionDesc sessionDescription = getSessionDescription(appMetricaEvent,
                offsetInSeconds);

        ReportMessage reportMessage = ReportMessage
                .newBuilder()
                .setSendTime(AppMetricaProto.Time.newBuilder()
                        .setTimestamp(appMetricaEvent.getEventTime().getEpochSecond())
                        .setTimeZone(offsetInSeconds)
                        .build())
                .addSessions(ReportMessage.Session.newBuilder()
                        .addAllEvents(events)
                        .setId(sessionId)
                        .setSessionDesc(sessionDescription)
                        .build())
                .build();

        return reportMessage;
    }

    // session id that robust to multiple sessions inside skill occured at one time
    // avoiding session-id hashcode collision by setting timestamp at first bits

    static long getSessionId(Session session) {
        return ((session.getStartTimestamp().getEpochSecond()) << 32) |
                (session.getSessionId().hashCode() & 0xffffffffL);
    }

    private static ReportMessage.Session.SessionDesc
    getSessionDescription(AppMetricaEvent appMetricaEvent, int offsetInSeconds) {
        ReportMessage.Session.SessionDesc.Builder sessionDescriptionBuilder =
                ReportMessage.Session.SessionDesc.newBuilder()
                        .setSessionType(ReportMessage.Session.SessionDesc.SessionType.SESSION_FOREGROUND)
                        .setStartTime(AppMetricaProto.Time.newBuilder()
                                .setTimestamp(appMetricaEvent.getSession().getStartTimestamp().getEpochSecond())
                                .setTimeZone(offsetInSeconds)
                                .build())
                        .setLocale(appMetricaEvent.getClientInfo().getLang());

        appMetricaEvent.getLocationInfo().ifPresent(location ->
                sessionDescriptionBuilder.setLocation(
                        ReportMessage.Session.SessionDesc.Location.newBuilder()
                                .setLat(location.getLat())
                                .setLon(location.getLon())
                                .build()));

        return sessionDescriptionBuilder.build();
    }

    private static ReportMessage.Session.SessionDesc
    getSessionDescription(long sessionStartTimestamp,
                          String lang,
                          Optional<LocationInfo> location,
                          int offsetInSeconds) {
        ReportMessage.Session.SessionDesc.Builder sessionDescriptionBuilder =
                ReportMessage.Session.SessionDesc.newBuilder()
                        .setSessionType(ReportMessage.Session.SessionDesc.SessionType.SESSION_FOREGROUND)
                        .setStartTime(AppMetricaProto.Time.newBuilder()
                                .setTimestamp(sessionStartTimestamp)
                                .setTimeZone(offsetInSeconds)
                                .build())
                        .setLocale(lang);

        location.ifPresent(it ->
                sessionDescriptionBuilder.setLocation(
                        ReportMessage.Session.SessionDesc.Location.newBuilder()
                                .setLat(it.getLat())
                                .setLon(it.getLon())
                                .build()));

        return sessionDescriptionBuilder.build();
    }

    // predefined list of values - one of phone | tablet | phablet | tv | desktop
    public static String mapDeviceType(ClientInfo clientInfo, SkillInfo.Look skillLook) {
        if (clientInfo.isDesktop()) {
            return "desktop";
        } else if (clientInfo.isTvDevice()) {
            return "tv";
        }

        return "phone";
    }

    public static String mapDeviceModel(ClientInfo clientInfo, SkillInfo.Look skillLook) {
        if (skillLook == SkillInfo.Look.INTERNAL) {
            return clientInfo.getDeviceModel();
        }

        if (clientInfo.isStationMini()) {
            return "yandex.station.mini";
        }

        return APP_ID_TO_DEVICE_MAPPINGS.getOrDefault(clientInfo.getAppId(), "");
    }

    public static String mapOsVersion(ClientInfo clientInfo, SkillInfo.Look skillLook) {
        if (skillLook == SkillInfo.Look.INTERNAL) {
            return clientInfo.getOsVersion();
        }

        return "";
    }

    public static String mapDeviceManufacturer(ClientInfo clientInfo, SkillInfo.Look skillLook) {
        if (skillLook == SkillInfo.Look.INTERNAL) {
            return clientInfo.getDeviceManufacturer();
        }

        if (clientInfo.getDeviceManufacturer().equals("Yandex")) {
            return "Yandex";
        }

        return "external";
    }

    public static String mapAppVersion(ClientInfo clientInfo, SkillInfo.Look skillLook) {
        if (skillLook == SkillInfo.Look.INTERNAL) {
            return clientInfo.getAppVersion();
        }

        return "";
    }

    public static String mapPlatform(ClientInfo clientInfo, SkillInfo.Look skillLook) {
        boolean showPlatform =
                skillLook == SkillInfo.Look.INTERNAL ||
                        (clientInfo.isAndroid() || clientInfo.isIOS() || clientInfo.isLinux()
                                || clientInfo.isWindows());
        if (!showPlatform) {
            return "";
        }

        if (clientInfo.isIOS()) {
            return "iOS";
        } else {
            return clientInfo.getPlatform();
        }
    }
}
