package ru.yandex.quasar.billing.beans;

import java.beans.ConstructorProperties;
import java.io.IOException;
import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonTypeInfo;
import com.fasterxml.jackson.databind.DatabindContext;
import com.fasterxml.jackson.databind.JavaType;
import com.fasterxml.jackson.databind.annotation.JsonTypeIdResolver;
import com.fasterxml.jackson.databind.jsontype.impl.TypeIdResolverBase;

import ru.yandex.quasar.billing.exception.InternalErrorException;

/**
 * Base class for a provider's representation of a {@link ContentItem}.
 */
@JsonTypeInfo(use = JsonTypeInfo.Id.CUSTOM, include = JsonTypeInfo.As.EXISTING_PROPERTY, property = "contentType",
        visible = true)
@JsonTypeIdResolver(ProviderContentItem.ProviderContentItemTypeResolver.class)
public abstract class ProviderContentItem {

    private final ContentType contentType;

    ProviderContentItem(ContentType contentType) {
        this.contentType = contentType;
    }

    public static ProviderContentItem createSeason(String id) {
        return createSeason(id, null);
    }

    public static SeasonProviderContentItem createSeason(String id, @Nullable String tvShowId) {
        return new SeasonProviderContentItem(id, tvShowId);
    }

    public static EpisodeProviderContentItem createEpisode(String id, @Nullable String seasonId,
                                                           @Nullable String tvShowId) {
        return new EpisodeProviderContentItem(id, seasonId, tvShowId);
    }

    public static ProviderContentItem create(ContentType type, String id) {
        return new JustIdProviderContentItem(type, id);
    }

    /**
     * Child classes should implement this.
     * Should return an "own" id for the sake of backward compatibility.
     */
    public abstract String getId();

    public ContentType getContentType() {
        return contentType;
    }

    /**
     * NB: children should override these in case they use a different equality
     * For example, if id is non-unique on it's own
     */
    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (!(o instanceof ProviderContentItem)) {
            return false;
        }

        ProviderContentItem that = (ProviderContentItem) o;
        return getContentType() == that.getContentType()
                && Objects.equals(getId(), that.getId());
    }

    @Override
    public int hashCode() {
        int result = contentType.hashCode();
        result = 31 * result + (getId() != null ? getId().hashCode() : 0);
        return result;
    }

    /**
     * Kinda-safer-than-plain-casting type narrowing. Use this instead of good-old-casting.
     */
    public <T extends ProviderContentItem> T narrowTo(Class<T> clazz) {
        if (getContentType().getProviderContentTypeClazz() != clazz) {
            throw new InternalErrorException("Trying to convert " + getContentType().getName() + " to " +
                    clazz.getName() + " instead of " + getContentType().getProviderContentTypeClazz().getName());
        }
        if (clazz.isInstance(this)) {
            return clazz.cast(this);
        } else {
            throw new InternalErrorException("Cant convert " + getContentType().getName() + " to " + clazz.getName());
        }
    }

    public ProviderContentItem asJustItContentItem() {
        return create(contentType, getId());
    }

    /**
     * Basic implementation for contentItem that has just object's id as metadata.
     */

    static class JustIdProviderContentItem extends ProviderContentItem {

        @JsonProperty(required = true)
        private final String id;

        @JsonCreator
        @ConstructorProperties({"contentType", "id"})
        JustIdProviderContentItem(ContentType contentType, String id) {
            super(contentType);
            this.id = id;
        }

        @Override
        public String getId() {
            return id;
        }

        @Override
        public JustIdProviderContentItem asJustItContentItem() {
            return this;
        }

        @Override
        public String toString() {
            return "JustIdProviderContentItem{" +
                    getContentType() + ":'" + getId() + '\'' +
                    '}';
        }
    }

    static class ProviderContentItemTypeResolver extends TypeIdResolverBase {

        private JavaType superType;

        @Override
        public void init(JavaType bt) {
            super.init(bt);
            superType = bt;
        }

        @Override
        public JavaType typeFromId(DatabindContext context, String id) throws IOException {
            return context.getConfig().constructType(ContentType.forName(id).getProviderContentTypeClazz());
        }


        @Override
        public String idFromBaseType() {
            return super.idFromBaseType();
        }

        @Override
        public String idFromValue(Object value) {
            return idFromValueAndType(value, value.getClass());
        }

        @Override
        public String idFromValueAndType(Object value, Class<?> suggestedType) {
            return ((ProviderContentItem) value).getContentType().getName();
        }

        @Override
        public JsonTypeInfo.Id getMechanism() {
            return JsonTypeInfo.Id.NONE;
        }

    }
}
