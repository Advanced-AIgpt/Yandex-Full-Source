package ru.yandex.alice.kronstadt.core.text;

import java.util.Collections;
import java.util.List;
import java.util.Optional;

import org.junit.jupiter.api.Test;

import ru.yandex.alice.kronstadt.core.layout.TextWithTts;

import static org.junit.jupiter.api.Assertions.assertEquals;


class ListRendererTest {

    @Test
    public void renderEmptyList() {
        assertEquals(
                new TextWithTts("", ""),
                ListRenderer.render(Collections.emptyList(), Optional.empty()));
    }

    @Test
    public void renderSingleItem() {
        var item = new TextWithTts("text", "tts");
        assertEquals(
                item,
                ListRenderer.render(List.of(item), Optional.empty())
        );
    }

    @Test
    public void renderTwoItems() {
        var a = new TextWithTts("a", "A");
        var b = new TextWithTts("b", "B");
        assertEquals(
                new TextWithTts("a и b", "A и B"),
                ListRenderer.render(List.of(a, b), Optional.empty())
        );
    }

    @Test
    public void renderThreeItems() {
        var a = new TextWithTts("a", "A");
        var b = new TextWithTts("b", "B");
        var c = new TextWithTts("c", "C");
        assertEquals(
                new TextWithTts("a, b и c", "A, B и C"),
                ListRenderer.render(List.of(a, b, c), Optional.empty())
        );
    }

    @Test
    public void renderListWithLineBreaks() {
        var a = new TextWithTts("a", "A");
        var b = new TextWithTts("b", "B");
        var c = new TextWithTts("c", "C");
        var expected = new TextWithTts(
                "- a\n- b\n- c",
                "A, B и C"
        );
        assertEquals(expected, ListRenderer.renderWithLineBreaks(List.of(a, b, c), Optional.empty()));
    }

    @Test
    public void renderWithPauses() {
        var a = new TextWithTts("a", "A");
        var b = new TextWithTts("b", "B");
        var c = new TextWithTts("c", "C");
        var expected = new TextWithTts(
                "- a\n- b\n- c",
                "A, sil<[100]> B и C"
        );
        assertEquals(expected, ListRenderer.renderWithLineBreaks(List.of(a, b, c), Optional.of(100)));
    }
}
