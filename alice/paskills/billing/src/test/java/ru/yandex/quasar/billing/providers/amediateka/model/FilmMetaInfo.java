package ru.yandex.quasar.billing.providers.amediateka.model;

import ru.yandex.quasar.billing.beans.ContentMetaInfo;

public class FilmMetaInfo extends BaseMetaInfo {

    private Integer duration;
    private String[] location;

    public Integer getDuration() {
        return duration;
    }

    public String[] getLocation() {
        return location;
    }

    public ContentMetaInfo toContentMetaInfo() {
        return new ContentMetaInfo(
                getName(),
                getImages().get("v_banner_3_4").getSizedUrlTemplate().replace("%size%", "135x180"),
                Integer.valueOf(getYear()),
                getDuration() / 60,
                getRestriction(),
                getLocation()[0],
                null,
                getDescription()
        );
    }

    public static class FilmMetaInfoDTO {
        private FilmMetaInfo film;

        public FilmMetaInfo getFilm() {
            return film;
        }
    }
}
