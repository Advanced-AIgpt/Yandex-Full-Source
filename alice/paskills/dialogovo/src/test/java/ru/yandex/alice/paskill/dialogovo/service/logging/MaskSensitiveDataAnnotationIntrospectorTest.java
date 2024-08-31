package ru.yandex.alice.paskill.dialogovo.service.logging;

import java.util.Optional;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.AnnotationIntrospector;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.introspect.AnnotationIntrospectorPair;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;

import static org.junit.jupiter.api.Assertions.assertEquals;

@JsonTest
class MaskSensitiveDataAnnotationIntrospectorTest {

    @Autowired
    private ObjectMapper mapperOriginal;
    private ObjectMapper mapper;

    @BeforeEach
    void setUp() {

        mapper = mapperOriginal.copy();
        AnnotationIntrospector origIntrospector = mapper.getSerializationConfig().getAnnotationIntrospector();
        AnnotationIntrospector newIntrospector = AnnotationIntrospectorPair.pair(origIntrospector,
                new MaskSensitiveDataAnnotationIntrospector());
        mapper.setAnnotationIntrospectors(newIntrospector,
                mapper.getDeserializationConfig().getAnnotationIntrospector());
    }

    @Test
    void testSerializeString() throws JsonProcessingException {
        assertEquals("{\"a\":\"test\"}", mapper.writeValueAsString(new TestClassA("test")));
    }

    @Test
    void serializeMaskedString() throws JsonProcessingException {
        assertEquals("{\"a\":\"****\"}", mapper.writeValueAsString(new TestClassB("test")));
    }

    @Test
    void serializeMaskedOptString() throws JsonProcessingException {
        assertEquals("{\"c\":\"********\"}", mapper.writeValueAsString(new TestClassC(Optional.of("testtest"))));
    }

    @Test
    void serializeMaskedEmptyOpt() throws JsonProcessingException {
        assertEquals("{\"c\":null}", mapper.writeValueAsString(new TestClassC(Optional.empty())));
    }


    private record TestClassA(String a) {
    }


    private record TestClassB(@MaskSensitiveData String a) {
    }


    private record TestClassC(@MaskSensitiveData Optional<String> c) {
    }


}
