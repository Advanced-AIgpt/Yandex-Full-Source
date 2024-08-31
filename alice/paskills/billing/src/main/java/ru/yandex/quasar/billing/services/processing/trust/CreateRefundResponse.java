package ru.yandex.quasar.billing.services.processing.trust;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class CreateRefundResponse {
    @Nonnull
    private final CreateRefundStatus status;
    @JsonProperty("trust_refund_id")
    @Nullable
    private final String trustRefundId;
}
