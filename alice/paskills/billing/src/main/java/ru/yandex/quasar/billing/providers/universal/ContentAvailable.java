package ru.yandex.quasar.billing.providers.universal;

import java.time.Instant;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.datatype.jsr310.ser.InstantSerializer;
import lombok.Data;

import ru.yandex.quasar.billing.providers.StreamData;

@Data
class ContentAvailable {
    @JsonProperty(required = true)
    private final boolean available;

    @JsonProperty("consumption_data")
    @Nullable
    private final ConsumptionData consumptionData;

    @JsonProperty("stream_data")
    @Nullable
    private final StreamData streamData;

    @JsonProperty("rejection_reason")
    @Nullable
    private final UniversalProviderRejectionReason rejectionReason;

    @Data
    private static class ConsumptionData {

        private final boolean consumed;

        // date of the last consumption finished
        @JsonProperty("consumed_at")
        @JsonFormat(shape = JsonFormat.Shape.STRING)
        @JsonSerialize(using = InstantSerializer.class)
        @Nullable
        private final Instant consumedAt;

        // seconds consumed from the beginning
        @Nullable
        @JsonProperty("stopped_at")
        private final Integer stoppedAt;
    }
}
