package ru.yandex.alice.paskills.my_alice.apphost.http;

import java.nio.charset.Charset;

import NAppHostHttp.Http;
import com.google.protobuf.ByteString;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NonNull;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.HttpHeaders;
import org.springframework.lang.Nullable;

@Data
@NonNull
@AllArgsConstructor(access = AccessLevel.PROTECTED)
public class Response {

    private static final Logger logger = LogManager.getLogger();
    private final Integer code;
    private final HttpHeaders headers;
    @Nullable
    private final String content;
    private final ByteString rawContent;

    protected static Response fromProto(Http.THttpResponse protoResponse) {
        HttpHeaders headers = new HttpHeaders();
        if (protoResponse.getHeadersList() != null) {
            for (var header : protoResponse.getHeadersList()) {
                headers.add(header.getName(), header.getValue());
            }
        }

        ByteString rawContent = protoResponse.getContent();
        String content = null;
        try {
            Charset charset = null;
            if (headers.getContentType() != null) {
                charset = headers.getContentType().getCharset();
            }

            if (charset != null) {
                content = rawContent.toString(charset);
            } else {
                content = rawContent.toStringUtf8();
            }
        } catch (Exception e) {
            logger.warn("Empty response, maybe wrong Request item format?", e);
        }

        return new Response(
                protoResponse.getStatusCode(),
                headers,
                content,
                rawContent
        );
    }
}
