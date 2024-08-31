package ru.yandex.quasar.billing.services.mediabilling;

import java.time.Instant;

import com.fasterxml.jackson.databind.util.StdConverter;

public class LongToInstantConverter extends StdConverter<Long, Instant> {

    @Override
    public Instant convert(Long value) {
        return Instant.ofEpochMilli(value);
    }
}
