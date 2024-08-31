package ru.yandex.quasar.billing.providers.universal;

import java.math.BigDecimal;
import java.util.Collections;
import java.util.List;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

@Data
@Builder
class ContentItemInfo {

    @JsonProperty("content_item_id")
    private final String contentItemId;
    @JsonProperty("content_type")
    private final ContentItemType contentType;
    private final String title;
    @Nullable
    private final String description;
    @JsonProperty("description_short")
    @Nullable
    private final String descriptionShort;
    @JsonProperty("url_to_item_page")
    @Nullable
    private final String urlToItemPage;

    @JsonProperty("cover_url_2x3")
    @Nullable
    private final String coverUrl2x3;
    @JsonProperty("cover_url_16x9")
    @Nullable
    private final String coverUrl16x9;

    @JsonProperty("thumbnail_url_2x3_small")
    @Nullable
    private final String thumbnailUrl2x3Small;
    @JsonProperty("thumbnail_url_16x9")
    @Nullable
    private final String thumbnailUrl16x9;
    @JsonProperty("thumbnail_url_16x9_small")
    @Nullable
    private final String thumbnailUrl16x9Small;

    @JsonProperty("release_year")
    private final Integer releaseYear;
    // duration in seconds
    @Nullable
    private final Integer duration;

    @JsonInclude(value = JsonInclude.Include.NON_EMPTY)
    @Nullable
    private final List<String> genres;

    @Nullable
    //rating from 0 to 10
    private final BigDecimal rating;

    @JsonInclude(value = JsonInclude.Include.NON_EMPTY)
    @Nullable
    private final List<String> directors;
    @JsonInclude(value = JsonInclude.Include.NON_EMPTY)
    @Nullable
    private final List<String> actors;

    @JsonProperty("misc_ids")
    @Nullable
    private final MiscIds miscIds;

    //children elements like seasons in TV SHOW
    @JsonInclude(value = JsonInclude.Include.NON_EMPTY)
    @Nullable
    private final List<String> children;

    // parent item for seasons/episodes and other children items
    @JsonProperty("parent_item_id")
    private final String parentItemId;

    @JsonProperty("min_age")
    @Nullable
    private final Integer minAge;

    @JsonProperty("sequence_number")
    @Nullable
    private final Integer sequenceNumber;

    @Nullable
    private final String country;

    public static ContentItemInfoBuilder builder(String contentItemId, ContentItemType contentType, String title) {
        return new ContentItemInfoBuilder()
                .contentItemId(contentItemId)
                .contentType(contentType)
                .title(title);
    }

    public List<String> getGenres() {
        return genres != null ? genres : Collections.emptyList();
    }


    public List<String> getDirectors() {
        return directors != null ? directors : Collections.emptyList();
    }


    public List<String> getActors() {
        return actors != null ? actors : Collections.emptyList();
    }


    public List<String> getChildren() {
        return children != null ? children : Collections.emptyList();
    }

    @Data
    private static class MiscIds {
        @Nullable
        private final String kinopoisk;
        @Nullable
        private final String imdb;
    }
}
