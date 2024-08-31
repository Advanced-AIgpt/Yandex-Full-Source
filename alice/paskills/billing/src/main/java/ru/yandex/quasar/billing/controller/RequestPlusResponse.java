package ru.yandex.quasar.billing.controller;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.quasar.billing.services.promo.PromoState;

@Data
@JsonInclude(JsonInclude.Include.NON_NULL)
class RequestPlusResponse {
    private final PromoState result;
    @Nullable
    @JsonProperty("personal_card")
    private final PersonalCard personalCard;
    @Nullable
    @JsonProperty("activate_promo_uri")
    private final String activatePromoUri;
    @Nullable
    @JsonProperty("expiration_time")
    @JsonInclude(JsonInclude.Include.NON_NULL)
    private final String expirationTime;
    @Nullable
    @JsonInclude(JsonInclude.Include.NON_NULL)
    private final String experiment;

}
