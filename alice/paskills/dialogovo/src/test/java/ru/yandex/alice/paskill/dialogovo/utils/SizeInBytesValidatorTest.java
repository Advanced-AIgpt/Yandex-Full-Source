package ru.yandex.alice.paskill.dialogovo.utils;

import javax.validation.Validation;
import javax.validation.Validator;

import lombok.AllArgsConstructor;
import lombok.Data;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class SizeInBytesValidatorTest {

    private Validator validator = Validation.buildDefaultValidatorFactory().getValidator();

    @Test
    void validateTestClass() {
        var obj17Bytes = new TestClassMax("x".repeat(17));
        var obj17BytesRes = validator.validate(obj17Bytes);
        assertEquals(1, obj17BytesRes.size());
        assertEquals("размер не должен превышать 16 байт", obj17BytesRes.stream().findFirst().get().getMessage());

        assertEquals(0, validator.validate(new TestClassMax("x".repeat(16))).size());
        assertEquals(0, validator.validate(new TestClassMax(null)).size());
        assertEquals(0, validator.validate(new TestClassMaxMin("\uD83D\uDC4D")).size());
        assertEquals(1, validator.validate(new TestClassMaxMin("x")).size());
    }

    @Data
    @AllArgsConstructor
    static class TestClassMax {
        @SizeInBytes(max = 16)
        private String text;
    }

    @Data
    @AllArgsConstructor
    static class TestClassMaxMin {
        @SizeInBytes(max = 5, min = 3)
        private String text;
    }
}
