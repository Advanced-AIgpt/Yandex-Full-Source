package ru.yandex.alice.paskill.dialogovo.external.v1.request.geolocation;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.RequestBase;

import static ru.yandex.alice.paskill.dialogovo.external.v1.request.InputType.GEOLOCATION_REJECTED;

public class GeolocationSharingRejectedRequest extends RequestBase {
    public GeolocationSharingRejectedRequest() {
        super(GEOLOCATION_REJECTED);
    }
}
