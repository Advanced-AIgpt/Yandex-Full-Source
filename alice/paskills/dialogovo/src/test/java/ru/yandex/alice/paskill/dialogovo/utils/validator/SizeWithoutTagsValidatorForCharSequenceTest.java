package ru.yandex.alice.paskill.dialogovo.utils.validator;

import lombok.Data;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

class SizeWithoutTagsValidatorForCharSequenceTest {

    @Test
    void testNormalString() throws NoSuchFieldException {
        var test = new OK("aaa");
        var validator = new SizeWithoutTagsValidatorForCharSequence();
        validator.initialize(OK.class.getDeclaredField("val").getAnnotation(SizeWithoutTags.class));

        assertTrue(validator.isValid("aaa", null));
    }

    @Test
    void testNullString() throws NoSuchFieldException {
        var validator = new SizeWithoutTagsValidatorForCharSequence();
        validator.initialize(OK.class.getDeclaredField("val").getAnnotation(SizeWithoutTags.class));

        assertTrue(validator.isValid(null, null));
    }

    @Test
    void testTaggedString() throws NoSuchFieldException {
        var validator = new SizeWithoutTagsValidatorForCharSequence();
        validator.initialize(OK.class.getDeclaredField("val").getAnnotation(SizeWithoutTags.class));

        assertTrue(validator.isValid("<speaker voice=\"shitova.us\">aaa", null));
    }

    @Test
    void testWrongTaggedString() throws NoSuchFieldException {
        var validator = new SizeWithoutTagsValidatorForCharSequence();
        validator.initialize(OK.class.getDeclaredField("val").getAnnotation(SizeWithoutTags.class));

        assertFalse(validator.isValid("<blablabla voice=\"shitova.us\">aaa", null));
    }

    @Test
    void testWrongTaggedSpacedString() throws NoSuchFieldException {
        var validator = new SizeWithoutTagsValidatorForCharSequence();
        validator.initialize(OK.class.getDeclaredField("val").getAnnotation(SizeWithoutTags.class));

        assertFalse(validator.isValid("<blablabla voice=\"shitova.us\">                                  ", null));
    }

    @Test
    void testTaggedSpacedString() throws NoSuchFieldException {
        var validator = new SizeWithoutTagsValidatorForCharSequence();
        validator.initialize(OK.class.getDeclaredField("val").getAnnotation(SizeWithoutTags.class));

        assertTrue(validator.isValid("<speaker voice=\"shitova.us\">                    ", null));
    }

    @Data
    private static class OK {
        @SizeWithoutTags(max = 5)
        private final String val;
    }
}
