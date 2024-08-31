package ru.yandex.alice.paskill.dialogovo.domain.div;

import org.junit.jupiter.api.Test;

import ru.yandex.alice.paskill.dialogovo.utils.TextUtils;

import static org.junit.jupiter.api.Assertions.assertEquals;

class TextUtilsTest {

    @Test
    public void endWithoutTerminal() {
        assertEquals(TextUtils.endWithoutTerminal("a"), "a");
        assertEquals(TextUtils.endWithoutTerminal("a!"), "a");
        assertEquals(TextUtils.endWithoutTerminal(""), "");
        assertEquals(TextUtils.endWithoutTerminal("aa   "), "aa");
    }

    @Test
    public void endWithDot() {
        assertEquals(TextUtils.endWithDot("a"), "a.");
        assertEquals(TextUtils.endWithDot("a!"), "a!");
        assertEquals(TextUtils.endWithDot(""), ".");
    }

    @Test
    public void htmlEscape() {
        assertEquals(TextUtils.htmlEscape("Давай сыграем в «Шар судьбы»."), "Давай сыграем в «Шар судьбы».");
        assertEquals(TextUtils.htmlEscape("Давай сыграем в \"Фантастический квест\""), "Давай сыграем в &quot;" +
                "Фантастический " +
                "квест&quot;");
    }
}
