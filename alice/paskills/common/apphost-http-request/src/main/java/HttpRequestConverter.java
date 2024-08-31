package ru.yandex.alice.paskills.common.apphost.http;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Collectors;

import NAppHostHttp.Http;
import NAppHostHttp.Http.THttpResponse;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.protobuf.ByteString;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.Message;

import static java.util.stream.Collectors.mapping;
import static java.util.stream.Collectors.toList;

public class HttpRequestConverter {

    private final ObjectMapper jsonMapper;

    private final ConcurrentHashMap<Class<?>, Message> cache = new ConcurrentHashMap<>();

    public HttpRequestConverter(ObjectMapper jsonMapper) {
        this.jsonMapper = jsonMapper;
    }

    public Http.THttpRequest httpRequestToProto(HttpRequest<?> request) {

        var httpRequestBuilder = Http.THttpRequest.newBuilder()
                .setMethod(request.method().protoMethod)
                .setScheme(request.scheme().protoScheme)
                .setPath(request.path());

        Set<String> headersKeys = request.headers().keySet().stream()
                .map(String::toLowerCase)
                .collect(Collectors.toUnmodifiableSet());


        var content = request.content();
        if (content != null) {
            ByteString contentByteString;
            String contentType;

            if (content instanceof byte[] bytes) {
                contentByteString = ByteString.copyFrom(bytes);
                contentType = "application/octet-stream";
            } else if (content instanceof Message msg) {
                contentByteString = msg.toByteString();
                contentType = "application/protobuf";
            } else if (content instanceof String s) {
                contentByteString = ByteString.copyFromUtf8(s);
                contentType = "application/text";
            } else if (content instanceof ByteBuffer bb) {
                contentByteString = ByteString.copyFrom(bb);
                contentType = "application/octet-stream";
            } else {
                try {
                    String jsonString = jsonMapper.writeValueAsString(content);
                    contentByteString = ByteString.copyFromUtf8(jsonString);
                    contentType = "application/json";
                } catch (JsonProcessingException e) {
                    throw new RuntimeException("Failed to convert HttpRequest body to json");
                }
            }

            httpRequestBuilder.setContent(contentByteString);
            httpRequestBuilder.addHeaders(Http.THeader.newBuilder()
                    .setName("Content-Length")
                    .setValue(String.valueOf(contentByteString.size()))
                    .build());

            if (!headersKeys.contains("content-type")) {
                httpRequestBuilder.addHeaders(Http.THeader.newBuilder()
                        .setName("Content-Type")
                        .setValue(contentType)
                        .build());
            }
        }

        request.headers().forEach((key, values) -> {
            if (!"Content-Length".equalsIgnoreCase(key.toLowerCase().trim())) {
                if (values != null && !values.isEmpty()) {
                    for (String value : values) {
                        httpRequestBuilder.addHeaders(Http.THeader.newBuilder()
                                .setName(key)
                                .setValue(value)
                                .build());
                    }
                } else {
                    httpRequestBuilder.addHeaders(Http.THeader.newBuilder()
                            .setName(key)
                            .build());
                }
            }
        });

        if (request.remoteIp() != null) {
            httpRequestBuilder.setRemoteIP(request.remoteIp());
        }

        return httpRequestBuilder.build();

    }

    @SuppressWarnings("unchecked")
    public <T> HttpResponse<T> protoToHttpResponse(THttpResponse protoResponse, Class<T> clazz) {

        Map<String, List<String>> headers = getHeaders(protoResponse);

        T content;
        if (!protoResponse.getContent().isEmpty() &&
                protoResponse.getStatusCode() >= 200 && protoResponse.getStatusCode() < 300) {
            if (Message.class.isAssignableFrom(clazz)) {
                try {

                    Message defaultInstance = cache.computeIfAbsent(clazz,
                            it -> com.google.protobuf.Internal.getDefaultInstance((Class<Message>) it));

                    content = (T) defaultInstance.newBuilderForType()
                            .mergeFrom(protoResponse.getContent())
                            .build();
                } catch (InvalidProtocolBufferException e) {
                    throw new RuntimeException(e);
                }
            } else {
                try {
                    content = jsonMapper.readValue(protoResponse.getContent().toByteArray(), clazz);
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }
            }
        } else {
            content = null;
        }

        return new HttpResponse<>(protoResponse.getStatusCode(),
                headers,
                content,
                protoResponse.getContent().toByteArray(),
                protoResponse.getIsSdchEncoded(),
                protoResponse.getFromHttpProxy());
    }

    private Map<String, List<String>> getHeaders(THttpResponse protoResponse) {
        Map<String, List<String>> headers = protoResponse.getHeadersList().stream()
                .collect(Collectors.groupingBy(Http.THeader::getName,
                        mapping(Http.THeader::getValue, toList())));
        return headers;
    }
}
