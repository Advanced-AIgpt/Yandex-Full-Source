package ru.yandex.quasar.billing.controller;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class SkillPublicKeyResponse {
    @JsonProperty("public_key")
    private final String publicKey;
}
