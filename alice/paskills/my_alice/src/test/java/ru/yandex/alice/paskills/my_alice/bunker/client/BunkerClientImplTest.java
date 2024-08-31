package ru.yandex.alice.paskills.my_alice.bunker.client;

import java.io.IOException;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.TimeUnit;

import com.google.gson.annotations.SerializedName;
import lombok.Data;
import okhttp3.mockwebserver.MockResponse;
import okhttp3.mockwebserver.MockWebServer;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.core.ParameterizedTypeReference;
import org.springframework.http.MediaType;
import org.springframework.lang.Nullable;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;

public class BunkerClientImplTest {

    private MockWebServer mockWebServer;
    private BunkerClient bunkerClient;

    @BeforeEach
    void setUp() throws IOException {
        mockWebServer = new MockWebServer();
        mockWebServer.start();

        bunkerClient = new BunkerClientImpl(mockWebServer.url("").toString(), "defVersion");
    }

    @Test
    void get() {
        mockWebServer.enqueue(
                new MockResponse()
                        .addHeader("Content-Type", "application/json")
                        .setBody("{}")
        );

        Optional<NodeContent<String>> result = bunkerClient.get("/node", new ParameterizedTypeReference<>() {
        });


        assertEquals(Optional.of(new NodeContent<>(
                MediaType.parseMediaType("application/json"),
                "{}"
        )), result);

        var req = assertDoesNotThrow(
                () -> mockWebServer.takeRequest(1L, TimeUnit.MILLISECONDS)
        );
        assertNotNull(req);
        assertEquals("GET", req.getMethod());
        assertEquals("/v1/cat?node=/node&version=defVersion", req.getPath());
    }

    @Test
    void getWithVersion() {
        mockWebServer.enqueue(
                new MockResponse()
                        .addHeader("Content-Type", "application/json")
                        .setBody("{}")
        );

        Optional<NodeContent<String>> result = bunkerClient.get("/node", "version", new ParameterizedTypeReference<>() {
        });


        assertEquals(Optional.of(new NodeContent<>(
                MediaType.parseMediaType("application/json"),
                "{}"
        )), result);

        var req = assertDoesNotThrow(
                () -> mockWebServer.takeRequest(1L, TimeUnit.MILLISECONDS)
        );
        assertNotNull(req);
        assertEquals("GET", req.getMethod());
        assertEquals("/v1/cat?node=/node&version=version", req.getPath());
    }


    @Test
    void getJson_ParameterizedTypeReference() {
        mockWebServer.enqueue(
                new MockResponse()
                        .addHeader("Content-Type", "application/json")
                        .setBody("{\"key\":\"value\"}")
        );

        Optional<NodeContent<TestJson>> result = bunkerClient.get("/node", new ParameterizedTypeReference<>() {
        });

        assertEquals(Optional.of(new NodeContent<>(
                MediaType.parseMediaType("application/json"),
                new TestJson("value")
        )), result);

        var req = assertDoesNotThrow(
                () -> mockWebServer.takeRequest(1L, TimeUnit.MILLISECONDS)
        );
        assertNotNull(req);
        assertEquals("GET", req.getMethod());
        assertEquals("/v1/cat?node=/node&version=defVersion", req.getPath());
    }

    @Test
    void getJson_Class() {
        mockWebServer.enqueue(
                new MockResponse()
                        .addHeader("Content-Type", "application/json")
                        .setBody("{\"key\":\"value\"}")
        );

        Optional<NodeContent<TestJson>> result = bunkerClient.get("/node", TestJson.class);

        assertEquals(Optional.of(new NodeContent<>(
                MediaType.parseMediaType("application/json"),
                new TestJson("value")
        )), result);

        var req = assertDoesNotThrow(
                () -> mockWebServer.takeRequest(1L, TimeUnit.MILLISECONDS)
        );
        assertNotNull(req);
        assertEquals("GET", req.getMethod());
        assertEquals("/v1/cat?node=/node&version=defVersion", req.getPath());
    }

    @Test
    void getBadCode() {
        mockWebServer.enqueue(
                new MockResponse()
                        .setResponseCode(404)
                        .setBody("ERROR: Node does not exist (SQLSTATE 46801)")
        );

        Optional<NodeContent<String>> result = bunkerClient.get("/node", new ParameterizedTypeReference<>() {
        });

        assertEquals(Optional.empty(), result);

        var req = assertDoesNotThrow(
                () -> mockWebServer.takeRequest(1L, TimeUnit.MILLISECONDS)
        );
        assertNotNull(req);
        assertEquals("GET", req.getMethod());
        assertEquals("/v1/cat?node=/node&version=defVersion", req.getPath());
    }

    @Test
    void getNoContentType() {
        mockWebServer.enqueue(
                new MockResponse()
                        .setBody("ERROR: Node does not exist (SQLSTATE 46801)")
        );

        Optional<NodeContent<String>> result = bunkerClient.get("/node", new ParameterizedTypeReference<>() {
        });

        assertEquals(Optional.empty(), result);

        var req = assertDoesNotThrow(
                () -> mockWebServer.takeRequest(1L, TimeUnit.MILLISECONDS)
        );
        assertNotNull(req);
        assertEquals("GET", req.getMethod());
        assertEquals("/v1/cat?node=/node&version=defVersion", req.getPath());
    }

    @Test
    void getBadResponseFormat() {
        mockWebServer.enqueue(
                new MockResponse()
                        .addHeader("Content-Type", "application/json")
                        .setBody("{\"key\":\"valu")
        );

        Optional<NodeContent<TestJson>> result = bunkerClient.get("/node", new ParameterizedTypeReference<>() {
        });

        assertEquals(Optional.empty(), result);

        var req = assertDoesNotThrow(
                () -> mockWebServer.takeRequest(1L, TimeUnit.MILLISECONDS)
        );
        assertNotNull(req);
        assertEquals("GET", req.getMethod());
        assertEquals("/v1/cat?node=/node&version=defVersion", req.getPath());
    }

    @Test
    void tree() {
        mockWebServer.enqueue(
                new MockResponse()
                        .addHeader("Content-Type", "application/json")
                        .setBody("[{\"name\":\".schema\",\"fullName\":\"/my-alice/.schema\"," +
                                "\"flushDate\":\"2020-06-24T08:40:44.417Z\"},{\"name\":\"main-promo\"," +
                                "\"fullName\":\"/my-alice/.schema/main-promo\",\"version\":6,\"isDeleted\":false," +
                                "\"mime\":\"application/schema+json; charset=utf-8; schema=\\\"bunker:/" +
                                ".schema/base#\\\"\",\"saveDate\":\"2020-06-24T16:16:30.080Z\"," +
                                "\"flushDate\":\"2020-06-24T08:41:05.037Z\"},{\"name\":\"images\"," +
                                "\"fullName\":\"/my-alice/images\",\"version\":1,\"isDeleted\":true," +
                                "\"saveDate\":\"2020-06-24T14:08:30.613Z\",\"flushDate\":\"2020-06-24T14:06:01" +
                                ".247Z\"},{\"name\":\"main-promo\",\"fullName\":\"/my-alice/images/main-promo\"," +
                                "\"version\":1,\"isDeleted\":true,\"saveDate\":\"2020-06-24T14:08:30.613Z\"," +
                                "\"flushDate\":\"2020-06-24T14:06:25.271Z\"},{\"name\":\"stable\"," +
                                "\"fullName\":\"/my-alice/stable\",\"flushDate\":\"2020-06-24T13:48:34.712Z\"}," +
                                "{\"name\":\"main-promo\",\"fullName\":\"/my-alice/stable/main-promo\"," +
                                "\"version\":50,\"isDeleted\":false,\"mime\":\"application/json; charset=utf-8; " +
                                "schema=\\\"bunker:/my-alice/.schema/main-promo#\\\"\"," +
                                "\"saveDate\":\"2020-07-06T15:38:45.449Z\",\"flushDate\":\"2020-06-24T13:53:02" +
                                ".871Z\"},{\"name\":\"images\",\"fullName\":\"/my-alice/stable/main-promo/images\"," +
                                "\"flushDate\":\"2020-06-24T14:07:56.932Z\"},{\"name\":\"skazki-na-noch.png\"," +
                                "\"fullName\":\"/my-alice/stable/main-promo/skazki-na-noch.png\",\"version\":1," +
                                "\"isDeleted\":false,\"mime\":\"image/svg+xml; avatar-href=\\\"https://avatars.mds" +
                                ".yandex.net/get-bunker/61205/7a5eae9275433b5fce9209f021e5d52bce863dae/orig\\\"; " +
                                "original-mime=\\\"image/png; charset=binary\\\"\",\"saveDate\":\"2020-06-24T14:10:41" +
                                ".121Z\",\"flushDate\":\"2020-06-24T14:10:41.121Z\"}]")
        );

        Map<String, NodeInfo> expected = Map.of(
                "/my-alice/.schema", new NodeInfo("/my-alice/.schema", null),
                "/my-alice/.schema/main-promo", new NodeInfo("/my-alice/.schema/main-promo", MediaType.parseMediaType(
                        "application/schema+json; charset=utf-8; schema=\"bunker:/.schema/base#\""
                )),
                "/my-alice/images", new NodeInfo("/my-alice/images", null),
                "/my-alice/images/main-promo", new NodeInfo("/my-alice/images/main-promo", null),
                "/my-alice/stable", new NodeInfo("/my-alice/stable", null),
                "/my-alice/stable/main-promo", new NodeInfo("/my-alice/stable/main-promo", MediaType.parseMediaType(
                        "application/json; charset=utf-8; schema=\"bunker:/my-alice/.schema/main-promo#\""
                )),
                "/my-alice/stable/main-promo/images", new NodeInfo("/my-alice/stable/main-promo/images", null),
                "/my-alice/stable/main-promo/skazki-na-noch.png", new NodeInfo(
                        "/my-alice/stable/main-promo/skazki-na-noch.png",
                        MediaType.parseMediaType(
                                "image/svg+xml; " +
                                        "avatar-href=\"https://avatars.mds.yandex.net/get-bunker/61205/" +
                                        "7a5eae9275433b5fce9209f021e5d52bce863dae/orig\"; " +
                                        "original-mime=\"image/png; charset=binary\""
                        )
                )
        );
        Map<String, NodeInfo> result = bunkerClient.tree("/my-alice");

        assertEquals(expected, result);

        var req = assertDoesNotThrow(
                () -> mockWebServer.takeRequest(1L, TimeUnit.MILLISECONDS)
        );
        assertNotNull(req);
        assertEquals("GET", req.getMethod());
        assertEquals("/v1/tree?node=/my-alice&version=defVersion", req.getPath());
    }

    @Test
    void treeWithVersion() {
        mockWebServer.enqueue(
                new MockResponse()
                        .addHeader("Content-Type", "application/json")
                        .setBody("[]")
        );

        Map<String, NodeInfo> expected = Map.of();
        Map<String, NodeInfo> result = bunkerClient.tree("/my-alice", "someVersion");

        assertEquals(expected, result);

        var req = assertDoesNotThrow(
                () -> mockWebServer.takeRequest(1L, TimeUnit.MILLISECONDS)
        );
        assertNotNull(req);
        assertEquals("GET", req.getMethod());
        assertEquals("/v1/tree?node=/my-alice&version=someVersion", req.getPath());
    }

    @Test
    void treeBadCode() {
        mockWebServer.enqueue(
                new MockResponse()
                        .setResponseCode(404)
                        .setBody("ERROR: Node does not exist (SQLSTATE 46801)")
        );

        Map<String, NodeInfo> result = bunkerClient.tree("/my-alice");

        assertEquals(Map.<String, NodeInfo>of(), result);

        var req = assertDoesNotThrow(
                () -> mockWebServer.takeRequest(1L, TimeUnit.MILLISECONDS)
        );
        assertNotNull(req);
        assertEquals("GET", req.getMethod());
        assertEquals("/v1/tree?node=/my-alice&version=defVersion", req.getPath());
    }

    @Test
    void treeBadResponse() {
        mockWebServer.enqueue(
                new MockResponse()
                        .setBody("[")
        );

        Map<String, NodeInfo> result = bunkerClient.tree("/my-alice");

        assertEquals(Map.<String, NodeInfo>of(), result);

        var req = assertDoesNotThrow(
                () -> mockWebServer.takeRequest(1L, TimeUnit.MILLISECONDS)
        );
        assertNotNull(req);
        assertEquals("GET", req.getMethod());
        assertEquals("/v1/tree?node=/my-alice&version=defVersion", req.getPath());
    }

    @Data
    static class TestJson {
        @Nullable
        @SerializedName("key")
        private final String someValue;
    }
}
