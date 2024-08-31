package ru.yandex.alice.paskills.my_alice.blocks.music;

import lombok.Data;

@Data
class MixLink implements BlockEntity {

    private final String id;
    private final Data data;

    @Override
    public String getImageUri() {
        return data.getBackgroundImageUri();
    }

    @Override
    public String getSuggest() {
        return "Включи подборку " + data.getTitle();
    }

    @Override
    public String getTitle() {
        return data.getTitle();
    }

    @lombok.Data
    public static class Data {
        private final String title;
        private final String backgroundImageUri;
    }
}
