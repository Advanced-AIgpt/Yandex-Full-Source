package ru.yandex.quasar.billing.providers.amediateka.model;

import java.util.Map;

import com.fasterxml.jackson.annotation.JsonProperty;

public class BaseMetaInfo extends BaseObject {

    private String name;
    private Map<String, Image> images;
    private String year;
    private String restriction;
    private String description;

    public String getName() {
        return name;
    }

    public Map<String, FilmMetaInfo.Image> getImages() {
        return images;
    }

    public String getYear() {
        return year;
    }

    public String getRestriction() {
        return restriction;
    }

    public String getDescription() {
        return description;
    }

    public static class Image {
        private String url;
        @JsonProperty("sized_url_template")
        private String sizedUrlTemplate;

        public String getUrl() {
            return url;
        }

        public String getSizedUrlTemplate() {
            return sizedUrlTemplate;
        }
    }
}
