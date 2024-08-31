package ru.yandex.alice.paskill.dialogovo.utils;

import java.util.regex.Pattern;

public class VoiceUtils {

    private static final Pattern NOT_ALLOWED_SYMBOL_TYPES_PATTERN = Pattern.compile(
            "[^\\p{L}\\p{N}\\p{P}\\p{Z}]",
            Pattern.UNICODE_CHARACTER_CLASS);

    private VoiceUtils() {
        throw new UnsupportedOperationException();
    }

    public static String normalize(String text) {
        return NOT_ALLOWED_SYMBOL_TYPES_PATTERN.matcher(text).replaceAll("");
    }

}
