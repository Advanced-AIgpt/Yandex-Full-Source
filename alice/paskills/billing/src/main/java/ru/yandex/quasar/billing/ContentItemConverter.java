package ru.yandex.quasar.billing;

import java.io.IOException;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.core.convert.converter.Converter;
import org.springframework.stereotype.Component;

import ru.yandex.quasar.billing.beans.ContentItem;
import ru.yandex.quasar.billing.exception.InternalErrorException;

@Component
public class ContentItemConverter implements Converter<String, ContentItem> {
    private final ObjectMapper objectMapper;

    public ContentItemConverter(ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;
    }

    @Override
    public ContentItem convert(String source) {
        try {
            return objectMapper.readValue(source, ContentItem.class);
        } catch (IOException e) {
            throw new InternalErrorException("failed to read ContentItem from " + source, e);
        }
    }
}
