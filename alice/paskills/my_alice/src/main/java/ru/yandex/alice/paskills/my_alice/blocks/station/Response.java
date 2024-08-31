package ru.yandex.alice.paskills.my_alice.blocks.station;

import java.util.Collections;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.EqualsAndHashCode;

@EqualsAndHashCode
@AllArgsConstructor
class Response {

    @Nullable
    private final Payload payload;

    public boolean hasStation() {
        return hasDevice(Device.STATION);
    }

    public boolean hasStationMini() {
        return hasDevice(Device.STATION_MINI);
    }

    private boolean hasDevice(String deviceType) {
        if (payload == null) {
            return false;
        }
        return payload.getDevices().stream().anyMatch(d -> deviceType.equals(d.getType()));
    }

    @Data
    private static class Payload {
        @Nullable
        private final List<Device> devices;

        public List<Device> getDevices() {
            return Objects.requireNonNullElse(devices, Collections.emptyList());
        }
    }

    @Data
    private static class Device {

        public static final String STATION = "devices.types.smart_speaker.yandex.station";
        public static final String STATION_MINI = "devices.types.smart_speaker.yandex.station.mini";

        private final String id;
        private final String externalId; // device_id + suffix
        private final String name;
        private final String type;
    }
}
