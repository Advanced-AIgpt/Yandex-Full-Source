package ru.yandex.alice.paskills.my_alice.blocks;

import java.util.Optional;

import NAppHostHttp.Http;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.Data;
import lombok.EqualsAndHashCode;

import ru.yandex.alice.paskills.my_alice.blackbox.SessionId;
import ru.yandex.web.apphost.api.request.RequestContext;

public interface WithHttpRequest extends BlockRender {
    Optional<Http.THttpRequest> prepare(RequestContext context, SessionId.Response blackboxResponse);
    String getRequestContextKey();

    default <T> T parseHttpResponse(Http.THttpResponse httpResponse, Class<T> clazz, ObjectMapper objectMapper)
            throws InvalidResponseCode, JsonProcessingException {
        if (httpResponse.getStatusCode() < 200 || httpResponse.getStatusCode() >= 300) {
            throw new InvalidResponseCode(httpResponse.getStatusCode(), httpResponse.getContent().toStringUtf8());
        }
        return objectMapper.readValue(httpResponse.getContent().toStringUtf8(), clazz);
    }

    default <T> T parseHttpResponse(
            RequestContext context,
            String key,
            Class<T> responseClass,
            ObjectMapper objectMapper
    ) throws JsonProcessingException, InvalidResponseCode {
        Http.THttpResponse httpResponse = context
                .getSingleRequestItem(key)
                .getProtobufData(Http.THttpResponse.getDefaultInstance());
        return parseHttpResponse(httpResponse, responseClass, objectMapper);
    }

    @Data
    @EqualsAndHashCode(callSuper = true)
    class InvalidResponseCode extends Exception {

        private final int statusCode;
        private final String responseBody;

        public InvalidResponseCode(int statusCode, String responseBody) {
            super("Failed to parse http response: status code " + Integer.toString(statusCode));
            this.statusCode = statusCode;
            this.responseBody = responseBody;
        }

    }
}
