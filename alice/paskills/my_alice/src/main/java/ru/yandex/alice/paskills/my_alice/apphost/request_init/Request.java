package ru.yandex.alice.paskills.my_alice.apphost.request_init;

import java.time.Instant;
import java.util.Base64;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

import com.google.common.collect.Maps;
import com.google.gson.annotations.SerializedName;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NonNull;
import org.apache.logging.log4j.util.Strings;
import org.springframework.http.HttpHeaders;
import org.springframework.lang.Nullable;

import static java.util.Objects.requireNonNullElse;

/**
 * Parse arcadia/web/src_setup/lib/setup/request/setup.cpp format
 */
@Data
@NonNull
@AllArgsConstructor(access = AccessLevel.PACKAGE)
public class Request {
    protected static final String ITEM_TYPE = "request";

    private final String method;
    private final String uri;
    private final String hostname;
    private final HttpHeaders headers;
    private final Map<String, List<String>> params;
    private final Map<String, String> cookiesParsed;
    private final String tld;
    @Nullable
    private final String ip;
    @Nullable
    private final String reqId;
    @Nullable
    private final String body;
    private final Instant timeEpoch;

    protected static Request fromRaw(Raw raw) {
        return new Request(
                requireNonNullElse(raw.getMethod(), "GET"),
                requireNonNullElse(raw.getUri(), "/"),
                requireNonNullElse(raw.getHostname(), "yandex.ru"),
                headersFromRaw(requireNonNullElse(raw.getHeaders(), Map.of())),
                Maps.transformValues(
                        raw.getParams(),
                        v -> v != null && !v.isEmpty() ? v : Collections.singletonList(null)
                ),
                Maps.filterValues(raw.getCookiesParsed(), Objects::nonNull),
                requireNonNullElse(raw.getTld(), "ru"),
                Strings.trimToNull(raw.getIp()),
                Strings.trimToNull(raw.getReqid()),
                Strings.trimToNull(raw.getBody()),
                Instant.ofEpochSecond(raw.getTimeEpoch())
        );
    }

    private static HttpHeaders headersFromRaw(Map<String, String> rawHeaders) {
        HttpHeaders headers = new HttpHeaders();

        for (var entry : rawHeaders.entrySet()) {
            if (entry.getValue() != null) {
                headers.add(entry.getKey(), entry.getValue());
            }
        }

        return headers;
    }

    @Data
    public static class File {
        private final String name;
        private final byte[] content;
        private final String contentType;
        private final Map<String, String> attributes;

        private static File fromRaw(String defaultName, Map<String, String> rawAttrs) {
            String name = defaultName;
            byte[] content = {};
            String contentType = null;
            Map<String, String> attributes = new HashMap<>();

            for (var attr : rawAttrs.entrySet()) {
                switch (attr.getKey().toLowerCase()) {
                    case "name":
                        name = attr.getValue();
                        break;
                    case "content_type":
                        contentType = attr.getValue();
                        break;
                    case "content":
                        content = Base64.getDecoder().decode(attr.getValue());
                        break;
                    default:
                        attributes.put(attr.getKey(), attr.getValue());
                        break;
                }
            }

            return new File(name, content, contentType, attributes);
        }
    }

    @Data
    public static class YCookie {
        public static final YCookie EMPTY = new YCookie(Map.of(), Map.of(), Map.of(), Map.of(), Map.of());

        private final Map<String, String> ys;
        private final Map<String, String> yp;
        private final Map<String, String> yc;
        private final Map<String, String> yt;
        private final Map<String, String> ypszm;

        protected static YCookie fromRaw(YCookie rawValue) {
            return new YCookie(
                    copyNotNullValues(rawValue.getYs()),
                    copyNotNullValues(rawValue.getYp()),
                    copyNotNullValues(rawValue.getYc()),
                    copyNotNullValues(rawValue.getYt()),
                    copyNotNullValues(rawValue.getYpszm())
            );
        }

        protected static <T> Map<String, T> copyNotNullValues(@Nullable Map<String, T> src) {
            Map<String, T> dst = new HashMap<>();

            if (src != null) {
                for (var entry : src.entrySet()) {
                    if (entry.getValue() != null) {
                        dst.put(entry.getKey(), entry.getValue());
                    }
                }
            }

            return dst;
        }

        public Map<String, String> getYs() {
            return requireNonNullElse(ys, Map.of());
        }

        public Map<String, String> getYp() {
            return requireNonNullElse(yp, Map.of());
        }

        public Map<String, String> getYc() {
            return requireNonNullElse(yc, Map.of());
        }

        public Map<String, String> getYt() {
            return requireNonNullElse(yt, Map.of());
        }

        public Map<String, String> getYpszm() {
            return requireNonNullElse(ypszm, Map.of());
        }
    }

    @Data
    protected static class Raw {
        @Nullable
        private final String method;
        @Nullable
        private final String uri;
        @Nullable
        private final String path;
        @Nullable
        private final String proto;
        @Nullable
        private final String scheme;
        @Nullable
        private final String hostname;
        @Nullable
        private final Map<String, String> headers;
        @Nullable
        private final Map<String, List<String>> params;
        @Nullable
        private final String cookies;
        @SerializedName("cookies_parsed")
        @Nullable
        private final Map<String, String> cookiesParsed;
        @SerializedName("is_internal")
        private final short isInternal;
        @SerializedName("is_suspected_robot")
        private final short isSuspectedRobot;
        @Nullable
        private final String referer;
        @Nullable
        private final String ua;
        @Nullable
        private final String xff;
        @Nullable
        private final String tld;
        @Nullable
        private final String ip;
        @Nullable
        private final String reqid;
        @SerializedName("connection_ip")
        @Nullable
        private final String connectionIp;
        @SerializedName("referer_is_ya")
        private final short refererIsYa;
        @Nullable
        private final String body;
        @Nullable
        private final Map<String, Map<String, String>> files;
        @SerializedName("time_epoch")
        private final long timeEpoch;
        @Nullable
        private final YCookie ycookie;
        @Nullable
        private final List<List<List<String>>> mycookie;
        @Nullable
        private final String gsmop;

        public Map<String, String> getHeaders() {
            return requireNonNullElse(headers, Map.of());
        }

        public Map<String, List<String>> getParams() {
            return requireNonNullElse(params, Map.of());
        }

        public Map<String, String> getCookiesParsed() {
            return requireNonNullElse(cookiesParsed, Map.of());
        }

        public Map<String, Map<String, String>> getFiles() {
            return requireNonNullElse(files, Map.of());
        }

        public YCookie getYcookie() {
            return requireNonNullElse(ycookie, YCookie.EMPTY);
        }

        public List<List<List<String>>> getMycookie() {
            return requireNonNullElse(mycookie, Collections.emptyList());
        }
    }
}
