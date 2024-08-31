package ru.yandex.quasar.billing.controller;

import javax.annotation.Nonnull;

import lombok.Data;

@Data
class CreateMerchantAccessRequest {
    @Nonnull
    private final String token;
    @Nonnull
    private final String description;
}
