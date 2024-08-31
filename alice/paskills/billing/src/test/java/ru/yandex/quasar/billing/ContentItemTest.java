package ru.yandex.quasar.billing;

import java.io.IOException;
import java.util.Arrays;
import java.util.HashSet;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.json.JSONObject;
import org.junit.jupiter.api.Test;

import ru.yandex.quasar.billing.beans.ContentItem;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static ru.yandex.quasar.billing.util.JsonUtil.toJsonQuotes;

class ContentItemTest {

    private ObjectMapper objectMapper = new ObjectMapper();

    @Test
    void testEnumDeserialize() throws IOException {
        assertEquals(ContentType.MOVIE, objectMapper.readValue("\"film\"", ContentType.class));
    }

    @Test
    void testSmokeDeserialize() throws IOException {
        ContentItem contentItem = objectMapper.readValue("{\"type\": \"film\", \"amediateka\": {\"id\": \"ololo\"}}",
                ContentItem.class);

        assertEquals(contentItem.getContentType(), ContentType.MOVIE);
        assertEquals(contentItem.getProviderInfo("amediateka").getContentType(), ContentType.MOVIE);
        assertEquals((contentItem.getProviderInfo("amediateka")).getId(), "ololo");
    }

    @Test
    void testDeserializeSeason() throws IOException {
        ContentItem contentItem = objectMapper.readValue("{\"type\": \"season\", \"amediateka\": {\"id\": \"ololo\", " +
                "\"tv_show_id\":\"show1\"}}", ContentItem.class);

        assertEquals(new ContentItem("amediateka", ProviderContentItem.createSeason("ololo", "show1")), contentItem);
    }

    @Test
    void testDeserializeSeasonWithoutShow() throws IOException {
        ContentItem contentItem = objectMapper.readValue("{\"type\": \"season\", \"amediateka\": {\"id\": " +
                "\"ololo\"}}", ContentItem.class);

        assertEquals(new ContentItem("amediateka", ProviderContentItem.createSeason("ololo", null)), contentItem);
    }

    @Test
    void testDeserializeEpisode() throws IOException {
        ContentItem contentItem = objectMapper.readValue("{\"type\": \"episode\", \"amediateka\": {\"id\": \"ololo\"," +
                " \"season_id\": \"s1\", \"tv_show_id\":\"show1\"}}", ContentItem.class);

        assertEquals(new ContentItem("amediateka", ProviderContentItem.createEpisode("ololo", "s1", "show1")),
                contentItem);
    }

    @Test
    void testSmokeSerialize() throws JsonProcessingException {
        ContentItem contentItem = new ContentItem("test", ProviderContentItem.createSeason("1", "2"));

        String asString = objectMapper.writeValueAsString(contentItem);

        JSONObject jsonObject = new JSONObject(asString);

        assertEquals(jsonObject.keySet(), new HashSet<>(Arrays.asList("type", "test"))); // check only proper keys here
        assertEquals(jsonObject.get("type"), "season");
        assertTrue(jsonObject.getJSONObject("test").similar(new JSONObject(toJsonQuotes("{'id': '1', 'tv_show_id': " +
                "'2', 'contentType': 'season'}"))));
        // TODO: get rid of that nested `contentType`
    }

    @Test
    void testContentItemId() {
        ContentItem contentItem = new ContentItem("test", ProviderContentItem.createSeason("1", "2"));
        ContentItem contentItem2 = new ContentItem("test2", ProviderContentItem.createSeason("1", "2"));
        ContentItem contentItem3 = new ContentItem("test", ProviderContentItem.createEpisode("1", "2", "3"));
        ContentItem contentItem4 = new ContentItem("test", ProviderContentItem.createSeason("1", "2"));


        assertNotEquals(contentItem.hashCode(), contentItem2.hashCode());
        assertNotEquals(contentItem.hashCode(), contentItem3.hashCode());
        assertNotEquals(contentItem3.hashCode(), contentItem2.hashCode());
        assertEquals(contentItem.hashCode(), contentItem4.hashCode());
    }

    @Test
    void testDeserializeSeasonlessSerial() throws IOException {
        ContentItem contentItem = objectMapper.readValue("{\"type\": \"season\", \"ivi\":{\"tv_show_id\":\"123\"}})",
                ContentItem.class);
        ContentItem expected = new ContentItem("ivi", ProviderContentItem.createSeason(null, "123"));
        assertEquals(expected.hashCode(), contentItem.hashCode());
    }
}
