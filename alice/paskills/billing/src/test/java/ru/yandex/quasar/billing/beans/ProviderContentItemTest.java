package ru.yandex.quasar.billing.beans;

import java.io.IOException;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringExtension;

@ExtendWith(SpringExtension.class)
@JsonTest
public class ProviderContentItemTest {
    @Autowired
    private JacksonTester<ProviderContentItem> tester;

    @Test
    public void testDeserializeJustIdSubscription() throws IOException {
        String json = "{\"contentType\":\"subscription\",\"id\":\"1d\"}";
        ProviderContentItem obj = ProviderContentItem.create(ContentType.SUBSCRIPTION, "1d");

        Assertions.assertThat(tester.parse(json)).isEqualTo(obj);
        Assertions.assertThat(tester.write(obj))
                .isEqualToJson(json);
    }

    @Test
    public void testDeserializeJustIdMovie() throws IOException {
        String json = "{\"contentType\":\"film\",\"id\":\"1d\"}";
        ProviderContentItem obj = ProviderContentItem.create(ContentType.MOVIE, "1d");

        Assertions.assertThat(tester.parse(json)).isEqualTo(obj);
        Assertions.assertThat(tester.write(obj))
                .isEqualToJson(json);
    }

    @Test
    public void testDeserializeSeason() throws IOException {
        String json = "{\"contentType\":\"season\",\"id\":\"1d\",\"tv_show_id\":\"show1\"}";
        ProviderContentItem obj = ProviderContentItem.createSeason("1d", "show1");

        Assertions.assertThat(tester.parse(json)).isEqualTo(obj);
        Assertions.assertThat(tester.write(obj))
                .isEqualToJson(json);
    }

    @Test
    public void testDeserializeSEpisode() throws IOException {
        String json = "{\"contentType\":\"episode\",\"id\":\"1d\",\"tv_show_id\":\"show1\", \"season_id\":\"season1\"}";
        ProviderContentItem obj = ProviderContentItem.createEpisode("1d", "season1", "show1");

        Assertions.assertThat(tester.parse(json)).isEqualTo(obj);
        Assertions.assertThat(tester.write(obj))
                .isEqualToJson(json);
    }

}
