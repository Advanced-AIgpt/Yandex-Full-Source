package ru.yandex.quasar.billing.beans;

import java.io.IOException;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonValue;
import com.fasterxml.jackson.databind.ObjectMapper;

/**
 * Enumeration for types of billed content.
 * Defines API-readable name and class for {@link ProviderContentItem}.
 * <p>
 * TODO: rename film -> movie, collection -> tv_show eventually
 */
public enum ContentType {
    MOVIE("film", ProviderContentItem.JustIdProviderContentItem.class, "movie", true),
    TV_SHOW("collection", ProviderContentItem.JustIdProviderContentItem.class, "tv_show", true),
    SEASON("season", SeasonProviderContentItem.class, "tv_show_season", true),
    EPISODE("episode", EpisodeProviderContentItem.class, "tv_show_episode", true),
    SUBSCRIPTION("subscription", ProviderContentItem.JustIdProviderContentItem.class, "sub", false),
    ANTHOLOGY("anthology_movie", ProviderContentItem.JustIdProviderContentItem.class, "anthology_movie", false),
    ANTHOLOGY_EPISODE("anthology_movie_episode", ProviderContentItem.JustIdProviderContentItem.class,
            "anthology_movie_episode", true),
    TRAILER("trailer", ProviderContentItem.JustIdProviderContentItem.class, "trailer", true),
    OTHER("other", ProviderContentItem.JustIdProviderContentItem.class, "other", false);

    private final String name;

    private final Class<? extends ProviderContentItem> providerContentTypeClazz;
    private final String universalProviderName;
    // if user may
    private final boolean consumableContent;

    ContentType(String name, Class<? extends ProviderContentItem> provierContentTypeClazz,
                String universalProviderName, boolean consumableContent) {
        this.name = name;
        this.providerContentTypeClazz = provierContentTypeClazz;
        this.universalProviderName = universalProviderName;
        this.consumableContent = consumableContent;
    }

    @JsonCreator
    public static ContentType forName(String name) {
        for (ContentType contentType : ContentType.values()) {
            if (contentType.name.equals(name)) {
                return contentType;
            }
        }
        return null;
    }

    @Nullable
    public static ContentType forUniversalProviderName(String name) {
        for (ContentType contentType : ContentType.values()) {
            if (contentType.universalProviderName.equals(name)) {
                return contentType;
            }
        }
        return null;
    }

    /**
     * Reads a serialized ProviderContentItem from JSON string
     */
    static ProviderContentItem readProviderContentItem(ObjectMapper objectMapper, String serialized)
            throws IOException {
        return objectMapper.readValue(serialized, ProviderContentItem.class);
    }

    @JsonValue
    public String getName() {
        return name;
    }

    public Class<? extends ProviderContentItem> getProviderContentTypeClazz() {
        return providerContentTypeClazz;
    }

    public String getUniversalProviderName() {
        return universalProviderName;
    }

    public boolean isConsumableContent() {
        return consumableContent;
    }

}
