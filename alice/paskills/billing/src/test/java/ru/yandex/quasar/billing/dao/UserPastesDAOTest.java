package ru.yandex.quasar.billing.dao;

import java.io.IOException;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.ContentItem;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.config.TestConfigProvider;

import static org.junit.jupiter.api.Assertions.assertEquals;

@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = TestConfigProvider.class)
class UserPastesDAOTest {
    private final Long uid = 9999L;
    @Autowired
    private UserPastesDAO userPastesDAO;
    @Autowired
    private ObjectMapper objectMapper;

    @Test
    void testSaveGetObject() throws IOException {
        ContentItem contentItem = new ContentItem("provider", ProviderContentItem.createEpisode("id1", "s1", "show1"));

        Long pasteId = userPastesDAO.savePaste(uid, objectMapper.writeValueAsString(contentItem));

        ContentItem actual = objectMapper.readValue(userPastesDAO.getPaste(pasteId, uid), ContentItem.class);

        assertEquals(contentItem, actual);
    }

    @Test
    void testSaveGetString() {
        String expected = "some text";
        Long pasteId = userPastesDAO.savePaste(uid, expected);

        String actual = userPastesDAO.getPaste(pasteId, uid);

        assertEquals(expected, actual);
    }
}
