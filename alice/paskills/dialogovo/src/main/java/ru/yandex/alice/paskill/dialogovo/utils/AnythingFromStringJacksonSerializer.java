package ru.yandex.alice.paskill.dialogovo.utils;

import java.io.IOException;

import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.databind.JsonSerializer;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.SerializerProvider;

public class AnythingFromStringJacksonSerializer extends JsonSerializer<String> {

    @Override
    public void serialize(String value, JsonGenerator gen, SerializerProvider serializers) throws IOException {
        var mapper = new ObjectMapper();
        var tree = mapper.readTree(value);

        gen.writeObject(tree);
    }
}
