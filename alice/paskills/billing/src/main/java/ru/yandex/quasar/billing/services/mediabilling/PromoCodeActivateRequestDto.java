package ru.yandex.quasar.billing.services.mediabilling;

import java.util.List;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;


record PromoCodeActivateRequestDto(
        String code,
        long uid,
        String paymentMethodId,
        String ip,
        @Nullable String platform,
        @Nullable String source,
        String origin,
        @Nullable Integer region,
        @JsonInclude(JsonInclude.Include.NON_EMPTY)
        @Nullable List<String> supportedFeatures
) {
}
