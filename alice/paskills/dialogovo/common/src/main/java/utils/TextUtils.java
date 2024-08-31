package ru.yandex.alice.paskill.dialogovo.utils;

import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;

import com.google.common.html.HtmlEscapers;
import org.springframework.util.StringUtils;

public class TextUtils {
    private TextUtils() {
    }

    public static String htmlEscape(String text) {
        return HtmlEscapers.htmlEscaper().escape(text);
    }

    public static String capitalizeFirst(String text) {
        return StringUtils.capitalize(text);
    }

    /**
     * Removes a terminal (?!.) at the end of sentence in case it is there.
     * Also trims spaces at the end.
     */
    public static String endWithoutTerminal(String text) {
        text = text.stripTrailing();
        if (text.length() > 1) {
            char last = text.toCharArray()[text.length() - 1];
            if (last == '.' || last == '!' || last == '?') {
                return text.substring(0, text.length() - 1);
            }
        }

        return text;
    }

    /**
     * Adds a dot at the end of sentence in case it has none. Also trims
     * spaces at the end.
     */
    public static String endWithDot(String text) {
        text = text.stripTrailing();
        if (text.length() > 1) {
            char last = text.toCharArray()[text.length() - 1];
            if (last == '.' || last == '!' || last == '?') {
                return text;
            }
        }

        return text + ".";
    }

    public static String urlEncode(String text) {
        return URLEncoder.encode(text, StandardCharsets.UTF_8);
    }
}
