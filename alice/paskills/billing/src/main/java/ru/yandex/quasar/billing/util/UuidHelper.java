package ru.yandex.quasar.billing.util;

import java.util.Optional;
import java.util.UUID;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

public final class UuidHelper {
    private static final Logger logger = LogManager.getLogger();

    private UuidHelper() {
        throw new UnsupportedOperationException();
    }

    public static Optional<UUID> fromString(String value) {
        try {
            return Optional.of(UUID.fromString(value));
        } catch (Exception ex) {
            logger.warn("can't parse value to uuid = {}", value);
            return Optional.empty();
        }

    }
}
