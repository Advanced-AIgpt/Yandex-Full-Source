package ru.yandex.alice.paskills.my_alice.bunker.image_storage;

import java.util.Optional;

import org.springframework.lang.Nullable;

public interface ImageStorage {
    Optional<String> getRealUrl(@Nullable String url);

    boolean isReady();
}
