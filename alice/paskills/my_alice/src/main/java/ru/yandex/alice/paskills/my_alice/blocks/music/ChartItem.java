package ru.yandex.alice.paskills.my_alice.blocks.music;

import java.util.List;

import javax.annotation.Nullable;

import lombok.Data;

@Data
class ChartItem implements BlockEntity {

    private final String id;
    private final Data data;

    @Override
    public String getImageUri() {
        return data.getTrack().getCoverUri();
    }

    @Override
    public String getSuggest() {
        return "Включи " + getArtist() + data.track.title;
    }

    @Override
    public String getTitle() {
        return data.getTrack().getArtistsAsString() + " – " + data.getTrack().getTitle();
    }

    private String getArtist() {
        return data.track.artists != null && !data.track.artists.isEmpty()
                ? data.track.artists.get(0).getName() + " "
                : "";
    }

    @lombok.Data
    public static class Data {
        private final Track track;
    }

    @lombok.Data
    public static class Track implements WithArtists {
        private final String title;
        private final String coverUri;
        @Nullable
        private final List<Artist> artists;
    }

}
