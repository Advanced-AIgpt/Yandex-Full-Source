package ru.yandex.alice.paskill.dialogovo.utils;

import java.io.IOException;
import java.nio.charset.StandardCharsets;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.io.Files;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;

public class ResourceUtils {
    private static final ObjectMapper MAPPER = new ObjectMapper();

    private ResourceUtils() {
    }

    public static String getStringResource(String location) {
        var resolver = new PathMatchingResourcePatternResolver();
        try {
            return Files.asCharSource(resolver.getResource(location).getFile(), StandardCharsets.UTF_8).read();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public static <T> T readObject(String resourcePath, Class<T> clazz) {
        try {
            return MAPPER.readValue(getStringResource(resourcePath), clazz);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }
}
