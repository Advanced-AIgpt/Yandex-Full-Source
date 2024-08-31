package ru.yandex.alice.paskills.my_alice.blocks.music;

import java.util.List;
import java.util.stream.Collectors;

public interface WithArtists {

    List<Artist> getArtists();
    default String getArtistsAsString() {
        return getArtists().stream()
                .map(Artist::getName)
                .collect(Collectors.joining(", "));
    }

}
