package ru.yandex.quasar.billing.services.processing.trust;

import java.time.Instant;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Data;

@JsonInclude(JsonInclude.Include.NON_NULL)
@Data
class UpdateSubscriptionRequest {
    @JsonProperty("finish_ts")
    @JsonSerialize(using = TrustDateSerializer.class)
    @JsonDeserialize(using = TrustDateDeserializer.class)
    private final Instant finishTs;
}
