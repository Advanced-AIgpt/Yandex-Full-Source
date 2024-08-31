package ru.yandex.alice.paskills.my_alice.blocks.music;

import lombok.Data;

@Data
class Playlist implements BlockEntity {

    private final String id;
    private final Data data;

    @Override
    public String getImageUri() {
        return data.getCover().getUri();
    }

    @Override
    public String getSuggest() {
        return "Включи плейлист " + data.getTitle();
    }

    @Override
    public String getTitle() {
        return data.getTitle();
    }

    @lombok.Data
    public static class Data {
        private final String title;
        private final Cover cover;
    }

    @lombok.Data
    public static class Cover {
        private final String uri;
    }
}
