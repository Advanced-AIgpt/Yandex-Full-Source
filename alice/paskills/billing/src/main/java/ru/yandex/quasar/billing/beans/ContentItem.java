package ru.yandex.quasar.billing.beans;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

import com.fasterxml.jackson.annotation.JsonAnyGetter;
import com.fasterxml.jackson.annotation.JsonAnySetter;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.deser.std.StdDeserializer;
import lombok.Data;


/**
 * A DTO-container for a single content item, like an exact episode or a film.
 * <p>
 * Contains type of the item and providers' representation for it, for each provider that should know it.
 * Actual type for {@link ProviderContentItem} is decided only by the `contentType`.
 * <p>
 * Провайдер бывает, что не умеет по ИД эпизода понимать к какому сезону он относится, аналогично по сезону понять
 * сериал
 * В результате используется этот класс и его наследники, как составной (композитный) ключ, однозачно определяющий
 * контент.
 * БАСС также присылает нам этот композитный объект
 * <p>
 * TODO: allow each provider to have custom {@link ProviderContentItem} impl per contentType.
 */
@JsonDeserialize(using = ContentItem.ContentItemDeserializer.class)
public class ContentItem {
    @JsonProperty("type")
    private ContentType contentType;

    // Map by provider name
    private Map<String, ProviderContentItem> providersInfo; // all the other fields

    // internal use only
    protected ContentItem() {
        this.providersInfo = new HashMap<>();
    }

    public ContentItem(ContentType contentType, Map<String, ProviderContentItem> providersInfo) {
        this.contentType = contentType;
        this.providersInfo = providersInfo;
    }

    /**
     * Content item for a single {@link ProviderContentItem}
     * ContentType is obtained from providerInfo parameter
     *
     * @param providerName provider name
     * @param providerInfo provider's content item
     */
    public ContentItem(String providerName, ProviderContentItem providerInfo) {
        this.contentType = providerInfo.getContentType();
        Map<String, ProviderContentItem> providerContentItemMap = new HashMap<>();
        providerContentItemMap.put(providerName, providerInfo);
        this.providersInfo = providerContentItemMap;
    }


    public ContentType getContentType() {
        return contentType;
    }

    public ProviderContentItem getProviderInfo(String providerName) {
        return providersInfo.get(providerName);
    }

    @JsonIgnore
    public Set<ProviderContentItemEntry> getProviderEntries() {
        return providersInfo.entrySet().stream()
                .map(entry -> new ProviderContentItemEntry(entry.getKey(), entry.getValue()))
                .collect(Collectors.toSet());
    }

    @JsonIgnore
    Map<String, ProviderContentItem> getProvidersInfo() {
        return providersInfo;
    }

    @JsonAnyGetter
    Map<String, ?> getDeserializeMap() {
        return providersInfo;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }
        ContentItem that = (ContentItem) o;
        return contentType == that.contentType &&
                Objects.equals(providersInfo, that.providersInfo);
    }

    @Override
    public int hashCode() {
        int result = contentType != null ? contentType.hashCode() : 0;
        result = 31 * result + (providersInfo != null ? providersInfo.hashCode() : 0);
        return result;
    }

    /**
     * Custom deserializer to implement polymorphism of ProviderContentItem
     * <p>
     * First reads fields as generic Objects and then converts them to the required type given by  ContentItem's own
     * type.
     */
    static class ContentItemDeserializer extends StdDeserializer<ContentItem> {
        ContentItemDeserializer() {
            this(null);
        }

        ContentItemDeserializer(Class<?> vc) {
            super(vc);
        }

        @Override
        public ContentItem deserialize(JsonParser p, DeserializationContext ctxt) throws IOException {
            ContentItemPojo contentItem = p.readValueAs(ContentItemPojo.class);
            ObjectMapper mapper = (ObjectMapper) p.getCodec();

            for (String key : contentItem.items.keySet()) {
                Map<String, Object> item = contentItem.items.get(key);
                item.putIfAbsent("contentType", contentItem.getContentType().getName());
                ProviderContentItem providerContentItem = mapper.convertValue(item,
                        contentItem.getContentType().getProviderContentTypeClazz());

                contentItem.getProvidersInfo().put(key, providerContentItem);
            }

            return new ContentItem(contentItem.getContentType(), contentItem.getProvidersInfo());
        }

        @JsonDeserialize
        private static final class ContentItemPojo extends ContentItem {
            Map<String, Map<String, Object>> items = new HashMap<>();

            @JsonAnySetter
            public void put(String name, Map<String, Object> value) {
                this.items.put(name, value);
            }

        }
    }

    @Data
    public static final class ProviderContentItemEntry {
        private final String providerName;
        private final ProviderContentItem providerContentItem;
    }
}
