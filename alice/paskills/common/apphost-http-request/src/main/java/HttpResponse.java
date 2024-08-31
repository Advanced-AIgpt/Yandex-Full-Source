package ru.yandex.alice.paskills.common.apphost.http;

import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

public record HttpResponse<T>(
        int statusCode,
        Map<String, List<String>> headers,
        @Nullable T content,
        @Nullable byte[] rawContent,
        boolean sdchEncoded,
        boolean fromHttpProxy
) {

    public boolean is1xxInformational() {
        return statusCode >= 100 && statusCode < 200;
    }

    public boolean is2xxSuccessful() {
        return statusCode >= 200 && statusCode <= 300;
    }

    public boolean is3xxRedirection() {
        return statusCode >= 300 && statusCode < 400;
    }

    public boolean is4xxClientError() {
        return statusCode >= 400 && statusCode < 500;
    }

    public boolean is5xxServerError() {
        return statusCode >= 500;
    }

    public boolean isError() {
        return (is4xxClientError() || is5xxServerError());
    }

    @Override
    @Nullable
    public byte[] rawContent() {
        if (rawContent != null) {
            return Arrays.copyOf(rawContent, rawContent.length);
        } else {
            return null;
        }
    }

    @Nullable
    public String rawContentAsString() {
        return rawContentAsString(StandardCharsets.UTF_8);
    }

    @Nullable
    public String rawContentAsString(Charset charset) {
        if (rawContent != null) {
            return new String(rawContent, charset);
        }
        return null;
    }
}
