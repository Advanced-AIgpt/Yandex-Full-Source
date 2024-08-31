package ru.yandex.quasar.billing.services.mediabilling;

import java.util.Collections;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;


record OfferShortDto(String id, @Nullable List<String> features, boolean familySub) {
    public List<String> features() {
        return Objects.requireNonNullElse(this.features, Collections.emptyList());
    }
}
