package ru.yandex.quasar.billing.beans;

import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Getter;
import lombok.ToString;
import org.springframework.util.StringUtils;

@Getter
@ToString
@Builder
public class ContentMetaInfo {

    private final String title;
    @Nullable
    private final String imageUrl;
    @Nullable
    private final Integer year;
    @Nullable
    private final Integer durationMinutes;
    @Nullable
    private final String ageRestriction;
    @Nullable
    private final String country;

    /**
     * Номер сезона. Сейчас имеет смысл только для сезона сериала - у остальных будет {@code null}
     */
    @Nullable
    private final Integer seasonNumber;
    @Nullable
    private final String description;

    @SuppressWarnings("ParameterNumber")
    @JsonCreator
    public ContentMetaInfo(
            @JsonProperty("title") String title,
            @JsonProperty("imageUrl") @Nullable String imageUrl,
            @JsonProperty("year") @Nullable Integer year,
            @JsonProperty("durationMinutes") @Nullable Integer durationMinutes,
            @JsonProperty("ageRestriction") @Nullable String ageRestriction,
            @JsonProperty("country") @Nullable String country,
            @JsonProperty("seasonNumber") @Nullable Integer seasonNumber,
            @JsonProperty("description") @Nullable String description
    ) {
        this.title = title;
        this.imageUrl = imageUrl;
        this.year = year;
        this.durationMinutes = durationMinutes;
        this.ageRestriction = ageRestriction;
        this.country = country;
        this.seasonNumber = seasonNumber;
        this.description = description;
    }

    private static String coalesce(String a, String b) {
        return StringUtils.hasText(a) ? a : b;
    }

    private static Integer coalesce(Integer a, Integer b) {
        return a != null ? a : b;
    }

    public static ContentMetaInfo merge(ContentMetaInfo a, ContentMetaInfo b) {
        if (a == null) {
            return b;
        }

        String title = coalesce(a.getTitle(), b.getTitle());
        String imageUrl = coalesce(a.getImageUrl(), b.getImageUrl());
        Integer year = coalesce(a.getYear(), b.getYear());
        Integer durationMinutes = coalesce(a.getDurationMinutes(), b.getDurationMinutes());
        String ageRestriction = coalesce(a.getAgeRestriction(), b.getAgeRestriction());
        String country = coalesce(a.getCountry(), b.getCountry());
        Integer seasonNumber = coalesce(a.getSeasonNumber(), b.getSeasonNumber());
        String description = coalesce(a.getDescription(), b.getDescription());

        return new ContentMetaInfo(title, imageUrl, year, durationMinutes, ageRestriction, country, seasonNumber,
                description);
    }

    public static ContentMetaInfoBuilder builder(String title) {
        return new ContentMetaInfoBuilder().title(title);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }
        ContentMetaInfo that = (ContentMetaInfo) o;
        return Objects.equals(title, that.title) &&
                Objects.equals(imageUrl, that.imageUrl) &&
                Objects.equals(year, that.year) &&
                Objects.equals(durationMinutes, that.durationMinutes) &&
                Objects.equals(ageRestriction, that.ageRestriction) &&
                Objects.equals(country, that.country) &&
                Objects.equals(seasonNumber, that.seasonNumber) &&
                Objects.equals(description, that.description);
    }

    @Override
    public int hashCode() {
        int result = title != null ? title.hashCode() : 0;
        result = 31 * result + (year != null ? year.hashCode() : 0);
        result = 31 * result + (seasonNumber != null ? seasonNumber.hashCode() : 0);
        return result;
    }

}
