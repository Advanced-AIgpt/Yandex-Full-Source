package ru.yandex.quasar.billing.beans;

import java.beans.ConstructorProperties;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * A {@link ProviderContentItem} for episodes. Contains IDs of the TV show and season it is for.
 */
public class EpisodeProviderContentItem extends ProviderContentItem {
    @JsonProperty(required = true)
    @Nonnull
    private final String id;

    @JsonProperty(value = "season_id")
    @Nullable
    private final String seasonId;

    @JsonProperty(value = "tv_show_id")
    @Nullable
    private final String tvShowId;

    @JsonCreator
    @ConstructorProperties({"id", "season_id", "tv_show_id"})
    EpisodeProviderContentItem(String id, @Nullable String seasonId, String tvShowId) {
        super(ContentType.EPISODE);

        this.id = id;
        this.seasonId = seasonId;
        this.tvShowId = tvShowId;
    }

    @Override
    public String getId() {
        return id;
    }

    @Nullable
    public String getSeasonId() {
        return seasonId;
    }

    public String getTvShowId() {
        return tvShowId;
    }

    public ProviderContentItem toSeason() {
        return ProviderContentItem.create(ContentType.SEASON, seasonId);
    }

    public ProviderContentItem toTvShow() {
        return create(ContentType.TV_SHOW, getTvShowId());
    }
}
