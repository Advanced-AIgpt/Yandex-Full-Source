package ru.yandex.quasar.billing.providers.amediateka.model;

import java.util.List;

import lombok.Data;
import lombok.Getter;

/**
 * Payload for various `GET /something/{id}/play_url.json` methods in amediateka API
 */
@Data
public class Stream {
    private String url;

    @Getter
    public static class StreamsDTO extends MultipleDTO<Stream> {
        private List<Stream> streams;

        @Override
        public List<Stream> getPayload() {
            return null;
        }
    }
}
