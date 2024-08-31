package ru.yandex.quasar.billing.services.mediabilling;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

/**
 * All calls to mediabilling API contain such meta-element in response
 */
@Data
class InvocationInfo {
    private final String hostname;
    private final String action;
    @JsonProperty("app-name")
    private final String appName;
    @JsonProperty("app-version")
    private final String appVersion;
    @JsonProperty("req-id")
    private final String reqId;
    @JsonProperty("exec-duration-millis")
    private final String execDurationMillis;
}
