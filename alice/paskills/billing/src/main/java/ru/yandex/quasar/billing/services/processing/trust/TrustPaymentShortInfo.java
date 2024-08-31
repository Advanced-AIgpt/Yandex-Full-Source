package ru.yandex.quasar.billing.services.processing.trust;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.List;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.deser.std.NumberDeserializers;
import lombok.Data;

import ru.yandex.quasar.billing.services.processing.TrustCurrency;

@Data
@JsonIgnoreProperties(ignoreUnknown = true)
public class TrustPaymentShortInfo {

    @JsonProperty("payment_resp_desc")
    @Nullable
    private final String paymentRespDesc;
    @JsonProperty("payment_resp_code")
    @Nullable
    private final String paymentRespCode;
    @JsonProperty("payment_status")
    private final String paymentStatus;
    @JsonProperty("user_account")
    private final String maskedCardNumber;
    @JsonProperty("card_type")
    private final String cardType;
    @JsonDeserialize(using = NumberDeserializers.BigDecimalDeserializer.class)
    @JsonFormat(shape = JsonFormat.Shape.STRING)
    private final BigDecimal amount;
    private final TrustCurrency currency;
    @JsonDeserialize(using = TrustDateDeserializer.class)
    @JsonSerialize(using = TrustDateSerializer.class)
    @JsonProperty("payment_ts")
    @Nullable // on failed purchases
    private final Instant paymentTs;
    @JsonProperty("paymethod_id")
    private final String paymethodId;

    @JsonDeserialize(using = TrustDateDeserializer.class)
    @JsonSerialize(using = TrustDateSerializer.class)
    @JsonProperty("clear_real_ts")
    @Nullable
    private final Instant clearRealTs;

    private final List<TrustOrderInfo> orders;

    @Data
    public static class TrustOrderInfo {
        @JsonProperty("order_id")
        private final String orderId;

        @JsonProperty("orig_amount")
        @JsonDeserialize(using = NumberDeserializers.BigDecimalDeserializer.class)
        @JsonFormat(shape = JsonFormat.Shape.STRING)
        private final BigDecimal origAmount;
    }

}
