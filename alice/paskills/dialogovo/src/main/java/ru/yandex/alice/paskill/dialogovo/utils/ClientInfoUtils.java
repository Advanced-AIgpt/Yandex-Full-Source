package ru.yandex.alice.paskill.dialogovo.utils;

import java.time.Instant;
import java.time.LocalDate;
import java.time.ZoneId;
import java.util.Map;
import java.util.Set;
import java.util.function.Function;

import lombok.Data;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;

import static java.util.stream.Collectors.toMap;

public class ClientInfoUtils {

    private static final String STATION_STUB_ID = "ru.yandex.quasar.app/1.0 (Yandex Station; android 6.0.1)";
    private static final String STATIONMINI_STUB_ID = "aliced/1.0 (Yandex yandexmini; Linux 1.0)";

    private static final Map<String, UnreleasedDeviceStub> SECRET_DEVICES = Set.of(
            new UnreleasedDeviceStub("Station_2",
                    LocalDate.of(2020, 11, 26)
                            .atStartOfDay(ZoneId.of("UTC"))
                            .toInstant(),
                    STATION_STUB_ID),
            new UnreleasedDeviceStub("yandexmini_2",
                    Instant.MAX,
                    STATIONMINI_STUB_ID),
            new UnreleasedDeviceStub("centaur",
                    LocalDate.of(2023, 1, 1)
                            .atStartOfDay(ZoneId.of("UTC"))
                            .toInstant(),
                    STATIONMINI_STUB_ID)
    )
            .stream()
            .collect(toMap(UnreleasedDeviceStub::getModel, Function.identity()));

    private ClientInfoUtils() {
        throw new UnsupportedOperationException();
    }

    public static String printId(ClientInfo clientInfo, Instant requestTime) {
        // https://wiki.yandex-team.ru/yandexmobile/techdocs/useragent/#format
        // <Name>/<Version>.<Build> (<Device_Manufacturer> <Device_Name>; <OS_Name> <OS_Version>)

        if (SECRET_DEVICES.containsKey(clientInfo.getDeviceModel()) &&
                requestTime.isBefore(SECRET_DEVICES.get(clientInfo.getDeviceModel()).releaseDate)) {
            return SECRET_DEVICES.get(clientInfo.getDeviceModel()).stubClientId;
        }

        return clientInfo.getAppId() +
                "/" +
                clientInfo.getAppVersion() +
                " (" +
                clientInfo.getDeviceManufacturer() +
                " " +
                clientInfo.getDeviceModel() +
                "; " +
                clientInfo.getPlatform() +
                " " +
                clientInfo.getOsVersion() +
                ")";
    }

    @Data
    private static class UnreleasedDeviceStub {
        private final String model;
        private final Instant releaseDate;
        private final String stubClientId;
    }
}
