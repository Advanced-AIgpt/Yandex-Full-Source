package ru.yandex.quasar.billing.services.mediabilling;

import java.math.BigDecimal;
import java.time.Instant;

import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;
import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonValue;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.datatype.jsr310.ser.InstantSerializer;
import lombok.Data;

@Data
public class OrderInfo {

    private final Long orderId;
    private final BigDecimal paidDays;
    private final BigDecimal debitAmount;
    private final String currency;
    @JsonFormat(shape = JsonFormat.Shape.STRING, pattern = "yyyy-MM-dd'T'HH:mm:ssXXX")
    @JsonSerialize(using = InstantSerializer.class)
    private final Instant created;
    private final String type;
    private final BigDecimal paidAmount;
    private final BigDecimal paidQuantity;
    private final Boolean trialPayment;
    private final OrderStatus status;

    //https://a.yandex-team.ru/arc/trunk/arcadia/devtools/experimental/mediabillingtst/common/src/main/java/ru/yandex
    // /music/support/billing/OrderStatus.java
    public enum OrderStatus {
        OK,
        PENDING,
        @JsonEnumDefaultValue
        ERROR,
        CANCELLED,
        REFUND;

        @JsonValue
        public String getSerializationName() {
            return name().toLowerCase();
        }
    }

    @Data
    static class Wrapper {
        private final InvocationInfo invocationInfo;
        private final OrderInfo result;
    }
}
