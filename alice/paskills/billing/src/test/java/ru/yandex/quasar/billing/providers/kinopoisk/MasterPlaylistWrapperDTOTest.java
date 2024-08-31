package ru.yandex.quasar.billing.providers.kinopoisk;

import java.io.IOException;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.JacksonTester;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import static ru.yandex.quasar.billing.providers.kinopoisk.MasterPlaylistWrapperDTO.WatchingRejectionReason.GEO_CONSTRAINT_VIOLATION;

@SpringJUnitConfig
@JsonTest
class MasterPlaylistWrapperDTOTest {

    private static final String EXAMPLE = "{\n" +
            "    \"status\": \"APPROVED\",\n" +
            "    \"masterPlaylist\": {}\n" +
            "}";
    private static final String EXAMPLE2 = "{" +
            "  \"status\":\"REJECTED\"," +
            "  \"hasFaceRecognition\":false," +
            "  \"watchingRejectionReason\":\"GEO_CONSTRAINT_VIOLATION\"" +
            "}";
    private static final String EXAMPLE3 = "{" +
            "  \"status\":\"UNKNOWN ERROR\"," +
            "  \"hasFaceRecognition\":false," +
            "  \"watchingRejectionReason\":\"GEO_CONSTRAINT_VIOLATION\"" +
            "}";
    @Autowired
    private JacksonTester<MasterPlaylistWrapperDTO> parser;

    @Test
    void testDeserialize() throws IOException {
        parser.parse(EXAMPLE).assertThat().isEqualTo(
                new MasterPlaylistWrapperDTO(MasterPlaylistWrapperDTO.MasterPlaylistStatus.APPROVED, "{}", null)
        );
    }

    @Test
    void testDeserializeRejected() throws IOException {
        parser.parse(EXAMPLE2).assertThat().isEqualTo(
                new MasterPlaylistWrapperDTO(MasterPlaylistWrapperDTO.MasterPlaylistStatus.REJECTED, null,
                        GEO_CONSTRAINT_VIOLATION)
        );
    }

    @Test
    void testDeserializeUnknown() throws IOException {
        parser.parse(EXAMPLE3).assertThat().isEqualTo(
                new MasterPlaylistWrapperDTO(MasterPlaylistWrapperDTO.MasterPlaylistStatus.UNKNOWN, null,
                        GEO_CONSTRAINT_VIOLATION)
        );
    }
}
