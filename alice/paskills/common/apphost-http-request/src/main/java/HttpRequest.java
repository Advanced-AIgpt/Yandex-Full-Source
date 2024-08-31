package ru.yandex.alice.paskills.common.apphost.http;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;

import javax.annotation.Nullable;

import NAppHostHttp.Http.THttpRequest.EMethod;
import NAppHostHttp.Http.THttpRequest.EScheme;

public record HttpRequest<T>(
        Method method,
        Scheme scheme,
        String path,
        Map<String, List<String>> headers,
        @Nullable T content,
        @Nullable String remoteIp
) {

    public static <T> Builder<T> builder(String path) {
        return new Builder<>(path);
    }

    public enum Method {

        GET(EMethod.Get),
        POST(EMethod.Post),
        PUT(EMethod.Put),
        DELETE(EMethod.Delete),
        HEAD(EMethod.Head),
        CONNECT(EMethod.Connect),
        OPTIONS(EMethod.Options),
        TRACE(EMethod.Trace),
        PATCH(EMethod.Patch);

        final EMethod protoMethod;

        Method(EMethod protoMethod) {
            this.protoMethod = protoMethod;
        }
    }

    public enum Scheme {
        NONE(EScheme.None),
        HTTP(EScheme.Http),
        HTTPS(EScheme.Https);

        final EScheme protoScheme;

        Scheme(EScheme protoScheme) {
            this.protoScheme = protoScheme;
        }
    }

    public static class Builder<T> {
        private Method method = Method.GET;
        private Scheme scheme = Scheme.HTTPS;
        private final String path;
        private final Map<String, List<String>> headers = new HashMap<>();
        @Nullable
        private T content;
        @Nullable
        private String remoteIp;

        private Builder(String path) {
            this.path = Objects.requireNonNull(path);
        }

        public Builder<T> method(Method method) {
            this.method = Objects.requireNonNull(method);
            return this;
        }

        public Builder<T> scheme(Scheme scheme) {
            this.scheme = Objects.requireNonNull(scheme);
            return this;
        }

        public Builder<T> header(String key, String value) {
            this.headers.computeIfAbsent(key, __ -> new ArrayList<>()).add(value);
            return this;
        }

        public Builder<T> headerIfPresent(String key, @Nullable String value) {
            if (value != null) {
                this.headers.computeIfAbsent(key, __ -> new ArrayList<>()).add(value);
            }
            return this;
        }

        public Builder<T> headerIfPresent(String key, Optional<String> value) {
            if (value.isPresent()) {
                this.headers.computeIfAbsent(key, __ -> new ArrayList<>()).add(value.get());
            }
            return this;
        }

        public Builder<T> headers(Map<String, String> headers) {
            Objects.requireNonNull(headers);
            this.headers.clear();

            headers.forEach((key, value) -> {
                var v = new ArrayList<String>(1);
                v.add(value);
                this.headers.put(key, v);
            });

            return this;
        }

        public Builder<T> content(T content) {
            this.content = content;
            return this;
        }

        public Builder<T> remoteIp(String remoteIp) {
            this.remoteIp = remoteIp;
            return this;
        }

        public HttpRequest<T> build() {
            return new HttpRequest<>(method, scheme, path, headers, content, remoteIp);
        }

    }

}
