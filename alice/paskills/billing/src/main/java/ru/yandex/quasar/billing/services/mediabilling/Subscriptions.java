package ru.yandex.quasar.billing.services.mediabilling;

import java.math.BigDecimal;
import java.time.Instant;
import java.time.Period;
import java.util.List;
import java.util.Set;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.deser.std.NumberDeserializers;
import com.fasterxml.jackson.datatype.jsr310.ser.InstantSerializer;
import lombok.Data;

@Data
public class Subscriptions {

    private final Set<String> availableFeatures;
    private final List<RenewableInfo> autoRenewable;
    // private final Map<String, Object> info
    private final List<NonAutoRenewableRemainder> nonAutoRenewableRemainders;
    private final List<NonAutoRenewable> nonAutoRenewable;
    private final List<TrialAvailability> trialAvailability;

    @Data
    public static class RenewableInfo {
        private final String productId;
        @JsonFormat(shape = JsonFormat.Shape.STRING)
        @JsonSerialize(using = InstantSerializer.class)
        private final Instant nextCharge;
        @JsonFormat(shape = JsonFormat.Shape.STRING)
        @JsonSerialize(using = InstantSerializer.class)
        private final Instant expires;
        private final Period commonPeriodDuration;
        private final Price price;
        private final boolean finished;
        private final String vendor;
        private final Set<String> features;
        private final Integer region;
        private final boolean debug;
    }

    @Data
    public static class Price {
        @JsonDeserialize(using = NumberDeserializers.BigDecimalDeserializer.class)
        @JsonFormat(shape = JsonFormat.Shape.STRING)
        private final BigDecimal amount;
        private final String currency;
    }

    @Data
    public static class NonAutoRenewableRemainder {
        private final Set<String> features;
        private final Integer remainderDays;
        private final Integer region;
    }

    @Data
    public static class NonAutoRenewable {
        private final String feature;
        @JsonFormat(shape = JsonFormat.Shape.STRING, pattern = "yyyy-MM-dd'T'HH:mm:ssXXX")
        @JsonSerialize(using = InstantSerializer.class)
        private final Instant start;
        @JsonFormat(shape = JsonFormat.Shape.STRING, pattern = "yyyy-MM-dd'T'HH:mm:ssXXX")
        @JsonSerialize(using = InstantSerializer.class)
        private final Instant end;
        private final Integer region;

    }

    @Data
    public static class TrialAvailability {
        private final String feature;
        private final Boolean available;
    }

    @Data
    static class Wrapper {
        private final InvocationInfo invocationInfo;
        private final Subscriptions result;

    }
}
