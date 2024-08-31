package ru.yandex.quasar.billing.services.processing.trust;

import java.time.Instant;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Data;

@JsonIgnoreProperties(ignoreUnknown = true)
@Data
public class SubscriptionShortInfo {

    @JsonProperty("status")
    private final String status;
    @JsonProperty("subs_until_ts")
    @JsonSerialize(using = TrustDateSerializer.class)
    @JsonDeserialize(using = TrustDateDeserializer.class)
    private final Instant subsUntilTs;
    @JsonProperty("finish_ts")
    @JsonSerialize(using = TrustDateSerializer.class)
    @JsonDeserialize(using = TrustDateDeserializer.class)
    private final Instant finishTs;

}
