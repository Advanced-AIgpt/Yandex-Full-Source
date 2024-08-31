package ru.yandex.alice.paskills.my_alice.bunker.image_storage;

import java.util.Map;
import java.util.Optional;

import org.junit.jupiter.api.Test;
import org.springframework.core.ParameterizedTypeReference;
import org.springframework.http.MediaType;

import ru.yandex.alice.paskills.my_alice.bunker.client.BunkerClient;
import ru.yandex.alice.paskills.my_alice.bunker.client.NodeContent;
import ru.yandex.alice.paskills.my_alice.bunker.client.NodeInfo;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class ImageStorageTest {
    private final ImageStorage imageStorage = new ImageStorageImpl(new BunkerClientMock(), "", Map.of(
            "/emptyNode", new NodeInfo("/emptyNode", null),
            "/jsonNode", new

                    NodeInfo("/jsonNode", MediaType.APPLICATION_JSON),
            "/badPngNode", new

                    NodeInfo("/badPngNode", MediaType.IMAGE_PNG),
            "/noHrefImgNode", new

                    NodeInfo("/noHrefImgNode", MediaType.parseMediaType("image/svg+xml; " +
                    "original-mime=\"image/png; charset=binary\"")),
            "/imgNode", new

                    NodeInfo("/imgNode", MediaType.parseMediaType("image/svg+xml; " +
                    "avatar-href=\"https://avatars.mds.yandex.net" +
                    "/get-bunker/61205/7a5eae9275433b5fce9209f021e5d52bce863dae/orig\"; " +
                    "original-mime=\"image/png; charset=binary\""))
    ));

    @Test
    public void skipWrongSchema() {
        assertEquals(
                Optional.of("https://example.com/image.png"),
                imageStorage.getRealUrl("https://example.com/image.png")
        );
        assertEquals(
                Optional.of("/path"),
                imageStorage.getRealUrl("/path")
        );
    }

    @Test
    public void noData() {
        assertEquals(Optional.empty(), imageStorage.getRealUrl("bunker:/notExists"));
        assertEquals(Optional.empty(), imageStorage.getRealUrl("bunker:/emptyNode"));
        assertEquals(Optional.empty(), imageStorage.getRealUrl("bunker:/jsonNode"));
        assertEquals(Optional.empty(), imageStorage.getRealUrl("bunker:/badPngNode"));
        assertEquals(Optional.empty(), imageStorage.getRealUrl("bunker:/noHrefImgNode"));
    }

    @Test
    public void success() {
        assertEquals(
                Optional.of("https://avatars.mds.yandex.net/get-bunker/61205/7a5eae9275433b5fce9209f021e5d52bce863dae" +
                        "/orig"),
                imageStorage.getRealUrl("bunker:/imgNode")
        );
    }

    protected static class BunkerClientMock implements BunkerClient {
        @Override
        public <T> Optional<NodeContent<T>> get(String nodePath, Class<T> responseType) {
            return Optional.empty();
        }

        @Override
        public <T> Optional<NodeContent<T>> get(String nodePath, ParameterizedTypeReference<T> responseType) {
            return Optional.empty();
        }

        @Override
        public <T> Optional<NodeContent<T>> get(String nodePath, String version, Class<T> responseType) {
            return Optional.empty();
        }

        @Override
        public <T> Optional<NodeContent<T>> get(String nodePath, String version,
                                                ParameterizedTypeReference<T> responseType) {
            return Optional.empty();
        }

        @Override
        public Map<String, NodeInfo> tree(String rootNodePath) {
            return Map.of();
        }

        @Override
        public Map<String, NodeInfo> tree(String rootNodePath, String version) {
            return Map.of();
        }
    }
}
