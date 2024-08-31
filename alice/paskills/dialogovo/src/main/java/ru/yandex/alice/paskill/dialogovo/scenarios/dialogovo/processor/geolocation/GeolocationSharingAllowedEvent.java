package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.geolocation;

import java.time.Instant;

import lombok.Data;

@Data
public class GeolocationSharingAllowedEvent {
    private final Instant untilTime;
}
