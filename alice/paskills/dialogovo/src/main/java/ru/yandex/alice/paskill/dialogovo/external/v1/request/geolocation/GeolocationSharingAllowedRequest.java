package ru.yandex.alice.paskill.dialogovo.external.v1.request.geolocation;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.RequestBase;

import static ru.yandex.alice.paskill.dialogovo.external.v1.request.InputType.GEOLOCATION_ALLOWED;

public class GeolocationSharingAllowedRequest extends RequestBase {
    public GeolocationSharingAllowedRequest() {
        super(GEOLOCATION_ALLOWED);
    }
}
