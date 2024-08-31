package ru.yandex.quasar.billing.controller;

import java.util.List;

import lombok.Data;

@Data
class SkillMerchantResponse {
    private final List<ServiceMerchant> merchants;

}
