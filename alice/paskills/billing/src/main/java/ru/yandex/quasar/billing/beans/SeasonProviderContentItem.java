package ru.yandex.quasar.billing.beans;

import java.beans.ConstructorProperties;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * A {@link ProviderContentItem} for seasons. Contains ID of the TV show it is for.
 */
public class SeasonProviderContentItem extends ProviderContentItem {
    //Ivi has seasonless serials
    @JsonProperty(required = false)
    @Nullable
    private final String id;

    @JsonProperty(value = "tv_show_id")
    @Nullable
    private final String tvShowId;

    @JsonCreator
    @ConstructorProperties({"id", "tv_show_id"})
    SeasonProviderContentItem(String id, String tvShowId) {
        super(ContentType.SEASON);
        this.id = id;
        this.tvShowId = tvShowId;
    }

    @Override
    public String getId() {
        return id;
    }

    public String getTvShowId() {
        return tvShowId;
    }

    public ProviderContentItem toTvShow() {
        return ProviderContentItem.create(ContentType.TV_SHOW, getTvShowId());
    }
}
