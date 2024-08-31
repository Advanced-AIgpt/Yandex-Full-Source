package ru.yandex.alice.paskills.my_alice.apphost.request_init;

import java.io.IOException;
import java.time.Instant;
import java.util.List;
import java.util.Map;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.GsonTester;
import org.springframework.http.HttpHeaders;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;

import static org.junit.jupiter.api.Assertions.assertEquals;

@JsonTest
class RequestTest {

    @Autowired
    private GsonTester<Request.Raw> gsonTester;

    @Test
    void defaults() throws IOException {
        var expected = new Request(
                "GET",
                "/",
                "yandex.ru",
                HttpHeaders.EMPTY,
                new LinkedMultiValueMap<>(),
                Map.of(),
                "ru",
                null,
                null,
                null,
                Instant.ofEpochSecond(0)
        );

        assertEquals(
                expected,
                Request.fromRaw(gsonTester.parseObject(
                        "{}"
                ))
        );
        assertEquals(
                expected,
                Request.fromRaw(gsonTester.parseObject(
                        "{" +
                                "\"ycookie\": null, " +
                                "\"mycookie\": null, " +
                                "\"cookies\": null, " +
                                "\"gsmop\": null, " +
                                "\"ip\": null, " +
                                "\"is_internal\": null, " +
                                "\"reqid\": null, " +
                                "\"path\": null, " +
                                "\"proto\": null, " +
                                "\"hostname\": null, " +
                                "\"params\": null, " +
                                "\"is_suspected_robot\": null, " +
                                "\"scheme\": null, " +
                                "\"method\": null, " +
                                "\"body\": null, " +
                                "\"cookies_parsed\": null, " +
                                "\"xff\": null, " +
                                "\"referer\": null, " +
                                "\"connection_ip\": null, " +
                                "\"referer_is_ya\": null, " +
                                "\"time_epoch\": null, " +
                                "\"uri\": null, " +
                                "\"headers\": null, " +
                                "\"tld\": null, " +
                                "\"ua\": null" +
                                "}"
                ))
        );
    }

    @Test
    void stringFields() throws IOException {
        assertEquals(
                new Request(
                        "METHOD",
                        "URI",
                        "HOSTNAME",
                        HttpHeaders.EMPTY,
                        new LinkedMultiValueMap<>(),
                        Map.of(),
                        "TLD",
                        "IP",
                        "REQ_ID",
                        "BODY",
                        Instant.ofEpochSecond(0)
                ),
                Request.fromRaw(gsonTester.parseObject(
                        "{\n" +
                                "  " +
                                "\"method\": \"METHOD\",\n" +
                                "  " +
                                "\"uri\": \"URI\",\n" +
                                "  \"path\": \"PATH\",\n" +
                                "  \"proto\": \"PROTO\",\n" +
                                "  \"scheme\": \"SCHEME\",\n" +
                                "  \"hostname\": \"HOSTNAME\",\n" +
                                "  \"cookies\": \"COOKIES\",\n" +
                                "  \"referer\": \"REFERER\",\n" +
                                "  \"ua\": \"UA\",\n" +
                                "  \"xff\": \"XFF\",\n" +
                                "  \"tld\": \"TLD\",\n" +
                                "  \"ip\": \"IP\",\n" +
                                "  \"reqid\": \"REQ_ID\",\n" +
                                "  \"connection_ip\": \"CONNECTION_IP\",\n" +
                                "  \"body\": \"BODY\",\n" +
                                "  \"gsmop\": \"GSM_OP\"\n" +
                                "}"
                ))
        );
    }

    @Test
    void headers() throws IOException {
        HttpHeaders expected = new HttpHeaders();
        expected.add("b", "value");

        assertEquals(
                expected,
                Request.fromRaw(gsonTester.parseObject(
                        "{\"headers\":{" +
                                "\"a\": null," +
                                "\"b\": \"value\"" +
                                "}}"
                )).getHeaders()
        );
    }

    @Test
    void params() throws IOException {
        MultiValueMap<String, String> expected = new LinkedMultiValueMap<>();
        expected.add("empty1", null);
        expected.add("empty2", null);
        expected.addAll("empty3", List.of(""));
        expected.addAll("single", List.of("1"));
        expected.addAll("multi", List.of("1", "2"));

        assertEquals(
                expected,
                Request.fromRaw(gsonTester.parseObject(
                        "{\n" +
                                "  \"params\": {\n" +
                                "    \"empty1\": null,\n" +
                                "    \"empty2\": [],\n" +
                                "    \"empty3\": [\n" +
                                "      \"\"\n" +
                                "    ],\n" +
                                "    \"single\": [\n" +
                                "      \"1\"\n" +
                                "    ],\n" +
                                "    \"multi\": [\n" +
                                "      \"1\",\n" +
                                "      \"2\"\n" +
                                "    ]\n" +
                                "  " +
                                "}\n" +
                                "}"
                )).getParams()
        );
    }

    @Test
    void cookiesParsed() throws IOException {
        assertEquals(
                Map.of("a", "value", "empty", ""),
                Request.fromRaw(gsonTester.parseObject(
                        "{\n" +
                                "  \"cookies_parsed\": {\n" +
                                "    \"a\": \"value\",\n" +
                                "    \"b\": null,\n" +
                                "    \"empty\": \"\"\n" +
                                "  }\n" +
                                "}"
                )).getCookiesParsed()
        );
    }

//    @Test
//    public void files() {
//        assertEquals(
//                Map.of(),
//                Request.fromRaw(GSON.fromJson(
//                        "{\"files\": {\"empty\": null}}",
//                        Request.RawRequest.class
//                )).getFiles()
//        );
//
//        assertEquals(
//                Map.of("upfile", new Request.File(
//                        "upfile",
//                        "hello world" .getBytes(),
//                        Optional.empty(),
//                        Map.of()
//                )),
//                Request.fromRaw(GSON.fromJson(
//                        "{\"files\": {\"upfile\": {\"content\": \"aGVsbG8gd29ybGQ=\"}}}",
//                        Request.RawRequest.class
//                )).getFiles()
//        );
//
//        assertEquals(
//                Map.of(
//                        "hello.txt",
//                        new Request.File(
//                                "hello.txt",
//                                "привет!" .getBytes(),
//                                Optional.of("text/plain"),
//                                Map.of("size", "14")
//                        )
//                ),
//                Request.fromRaw(GSON.fromJson(
//                        "{\"files\": {\n" +
//                                "  \"hidden.name\": {\n" +
//                                "    \"name\": \"hello.txt\",\n" +
//                                "    \"content\": \"0L/RgNC40LLQtdGCIQ==\",\n" +
//                                "    \"size\": \"14\",\n" +
//                                "    \"content_type\": \"text/plain\"\n" +
//                                "  }\n" +
//                                "}}",
//                        Request.RawRequest.class
//                )).getFiles()
//        );
//    }

    @Test
    void timeEpoch() throws IOException {
        assertEquals(
                Instant.ofEpochSecond(123),
                Request.fromRaw(gsonTester.parseObject(
                        "{\"time_epoch\": 123}"
                )).getTimeEpoch()
        );
    }

//    @Test
//    public void yCookie() {
//        assertEquals(
//                Request.YCookie.EMPTY,
//                Request.fromRaw(GSON.fromJson(
//                        "{\"ycookie\": {\n" +
//                                "  \"ys\": null,\n" +
//                                "  \"yp\": null,\n" +
//                                "  \"yc\": null,\n" +
//                                "  \"yt\": null,\n" +
//                                "  \"ypszm\": null\n" +
//                                "}}",
//                        Request.RawRequest.class
//                )).getYCookie()
//        );
//
//        assertEquals(
//                Request.YCookie.EMPTY,
//                Request.fromRaw(GSON.fromJson(
//                        "{\"ycookie\": {\n" +
//                                "  \"ys\": {\"empty\": null},\n" +
//                                "  \"yp\": {\"empty\": null},\n" +
//                                "  \"yc\": {\"empty\": null},\n" +
//                                "  \"yt\": {\"empty\": null},\n" +
//                                "  \"ypszm\": {\"empty\": null}\n" +
//                                "}}",
//                        Request.RawRequest.class
//                )).getYCookie()
//        );
//
//        assertEquals(
//                new Request.YCookie(
//                        Map.of("musicchrome", "0-0-473-1", "mclid", "1955454", "def_bro", "1"),
//                        Map.of("gpauto", "55_197182:36_748096:140:1:1591800840", "org_id", "33790", "2fa", "1"),
//                        Map.of("sad", "1578644773:1591346390:11"),
//                        Map.of("org_id", 1902745295, "sad", 1906706390),
//                        Map.of("ratio", 2)
//                ),
//                Request.fromRaw(GSON.fromJson(
//                        "{\"ycookie\": {\n" +
//                                "  \"ys\": {\n" +
//                                "    \"musicchrome\": \"0-0-473-1\",\n" +
//                                "    \"mclid\": \"1955454\",\n" +
//                                "    \"def_bro\": \"1\"\n" +
//                                "  },\n" +
//                                "  \"yp\": {\n" +
//                                "    \"gpauto\": \"55_197182:36_748096:140:1:1591800840\",\n" +
//                                "    \"org_id\": \"33790\",\n" +
//                                "    \"2fa\": \"1\"\n" +
//                                "  },\n" +
//                                "  \"yc\": {\n" +
//                                "    \"sad\": \"1578644773:1591346390:11\"\n" +
//                                "  },\n" +
//                                "  \"yt\": {\n" +
//                                "    \"org_id\": 1902745295,\n" +
//                                "    \"sad\": 1906706390\n" +
//                                "  },\n" +
//                                "  \"ypszm\": {\n" +
//                                "    \"ratio\": 2\n" +
//                                "  }\n" +
//                                "}}",
//                        Request.RawRequest.class
//                )).getYCookie()
//        );
//    }

//    @Test
//    public void myCookie() {
//        assertEquals(
//                List.of(List.of(List.of(43, 213, 118209), List.of(1))),
//                Request.fromRaw(GSON.fromJson(
//                        "{\"mycookie\": [[[null, 43, 213, 118209, null], [], null, [1]]]}",
//                        Request.RawRequest.class
//                )).getMyCookie()
//        );
//    }
}
