package ru.yandex.quasar.billing.services.processing.yapay;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.List;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonValue;
import lombok.Builder;
import lombok.Data;

import ru.yandex.quasar.billing.services.processing.TrustCurrency;

@Data
@Builder
class Order {
    @JsonProperty("order_id")
    private final Long orderId;
    private final boolean active;
    private final boolean verified;
    @JsonProperty("user_email")
    private final String userEmail;
    private final BigDecimal price;
    private final String description;
    @JsonProperty("receipt_url")
    private final String receiptUrl;
    private final String kind;
    private final TrustCurrency currency;
    @JsonProperty("pay_status")
    private final Status payStatus;

    @Nullable
    private final Instant closed;
    private final List<OrderItem> items;
    @JsonFormat(shape = JsonFormat.Shape.STRING)
    private final Instant created;
    private final String caption;
    private final Integer revision;
    // merchant uid
    private final Long uid;
    @JsonProperty("user_description")
    private final String userDescription;
    @JsonFormat(shape = JsonFormat.Shape.STRING)
    private final Instant updated;

    enum Status {
        created("new"),
        in_moderation("in_moderation"),
        held("held"),
        in_progress("in_progress"),
        moderation_negative("moderation_negative"),
        paid("paid"),
        in_cancel("in_cancel"),
        canceled("canceled"),
        rejected("rejected");


        @JsonValue
        private final String code;

        Status(String code) {
            this.code = code;
        }

        static Status forCode(String v) {
            for (Status cc : Status.values()) {
                if (cc.code.equals(v)) {
                    return cc;
                }
            }
            throw new IllegalArgumentException("Enum value not found for " + v);
        }

        public String getCode() {
            return code;
        }
    }

}
