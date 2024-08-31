package ru.yandex.quasar.billing.util;

public class JsonUtil {
    private JsonUtil() throws IllegalAccessException {
        throw new IllegalAccessException();
    }

    /**
     * @param almostJson a string with single quotes
     * @return a string with double quotes
     */
    public static String toJsonQuotes(String almostJson) {
        return almostJson.replace("'", "\"");
    }
}
