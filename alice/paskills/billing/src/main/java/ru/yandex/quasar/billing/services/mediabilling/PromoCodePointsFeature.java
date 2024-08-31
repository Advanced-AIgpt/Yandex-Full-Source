package ru.yandex.quasar.billing.services.mediabilling;

import java.math.BigDecimal;

import com.fasterxml.jackson.annotation.JsonFormat;

record PromoCodePointsFeature(@JsonFormat(shape = JsonFormat.Shape.STRING) BigDecimal amount) {
}
