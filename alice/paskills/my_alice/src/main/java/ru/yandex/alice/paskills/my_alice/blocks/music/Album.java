package ru.yandex.alice.paskills.my_alice.blocks.music;

import java.util.Collections;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;

import lombok.Data;

@Data
class Album implements BlockEntity {

    private final String id;
    private final Data data;

    @Override
    public String getImageUri() {
        return data.getCoverUri();
    }

    @Override
    public String getSuggest() {
        String albumWord = isSingle() ? "сингл" : "альбом";
        String artist = !data.getArtists().isEmpty() ? " " + data.getArtists().get(0).getName() : "";
        return "Включи " + albumWord + " " + data.getTitle() + artist;
    }

    @Override
    public String getTitle() {
        String prefix = isSingle() ? "" : "Альбом ";
        return prefix + data.getArtistsAsString() + " – " + data.getTitle();
    }

    private boolean isSingle() {
        return "single".equals(data.getType());
    }

    @lombok.Data
    private static class Data implements WithArtists {
        private final String title;
        private final String coverUri;
        @Nullable
        private final String type;
        private final List<Artist> artists;

        public List<Artist> getArtists() {
            return Objects.requireNonNullElse(artists, Collections.emptyList());
        }

    }
}
