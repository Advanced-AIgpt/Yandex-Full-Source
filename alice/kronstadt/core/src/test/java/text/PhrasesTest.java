package ru.yandex.alice.kronstadt.core.text;

import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.Set;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static ru.yandex.alice.kronstadt.core.text.InflectedString.RuCase.CREATIVE;
import static ru.yandex.alice.kronstadt.core.text.InflectedString.RuCase.GENITIVE;
import static ru.yandex.alice.kronstadt.core.text.InflectedString.RuCase.NOMINATIVE;
import static ru.yandex.alice.kronstadt.core.text.InflectedString.RuCase.PREPOSITIONAL;

class PhrasesTest {

    private final Phrases phrases = new Phrases(List.of("phrases/test"));
    private final int seed = 11;

    @Test
    public void testOneWithoutTemplate() {
        assertEquals(phrases.get("station.discovery.suggest.simple.1"), "Ух ты, а я и не знаю. Но у меня есть навык " +
                "{0}. {1}?");
    }

    @Test
    public void testMultipleReplacements() {
        assertEquals(phrases.get("test.multiple", List.of("Вести ФМ")), "Чтобы и дальше следить за событиями на Вести" +
                " ФМ, скажите мне «Что нового на Вести ФМ?»");
    }

    @Test
    public void testMultipleInflectedWithSimpleString() {
        assertEquals(phrases.get("test.multiple.inflected", List.of("Вести ФМ")), "Чтобы и дальше следить за " +
                "событиями на Вести" +
                " ФМ, скажите мне «Что нового на Вести ФМ?»");
    }

    @Test
    public void testMultipleInflectedWithInflectedString() {
        assertEquals(phrases.get("test.multiple.inflected", InflectedString.cons("Вести ФМ")),
                "Чтобы и дальше следить за событиями на Вести ФМ, скажите мне «Что нового на Вести ФМ?»");
    }

    @Test
    public void testMultipleInflectedWithInflectedStringWithDat() {
        assertEquals(phrases.get("test.multiple.inflected",
                InflectedString.cons(Map.of(NOMINATIVE, "Вести ФМ", CREATIVE, "Вестях ФМ"))),
                "Чтобы и дальше следить за событиями на Вести ФМ, скажите мне «Что нового на Вести ФМ?»");
    }

    @Test
    public void testMultipleInflectedWithInflectedStringWithGen() {
        assertEquals(phrases.get("test.multiple.inflected",
                InflectedString.cons(Map.of(NOMINATIVE, "Вести ФМ", GENITIVE, "Вестях ФМ"))),
                "Чтобы и дальше следить за событиями на Вестях ФМ, скажите мне «Что нового на Вести ФМ?»");
    }

    @Test
    public void testMultipleInflectedWithInflectedStringWithDiffCases() {
        assertEquals(phrases.get("test.multiple.inflected",
                InflectedString.cons(Map.of(
                        NOMINATIVE, "Вести ФМ",
                        GENITIVE, "Вестей ФМ",
                        PREPOSITIONAL, "Вестях ФМ"))),
                "Чтобы и дальше следить за событиями на Вестей ФМ, скажите мне «Что нового на Вестях ФМ?»");
    }

    @Test
    public void testMultipleInflectedWithInflectedStringWithDiff2VarsCases() {
        assertEquals(phrases.get("test.multiple.inflected.2.vars",
                InflectedString.cons(Map.of(
                        NOMINATIVE, "Вести ФМ",
                        GENITIVE, "Вестях ФМ",
                        PREPOSITIONAL, "Вестям ФМ")),
                InflectedString.cons(Map.of(
                        NOMINATIVE, "Коммерсант ФМ",
                        GENITIVE, "Коммерсанта ФМ",
                        PREPOSITIONAL, "Коммерсанте ФМ"))),
                "Чтобы и дальше следить за событиями на Вестях ФМ, скажите мне «Что нового на Коммерсанте ФМ?»");
    }

    @Test
    public void testMultipleInflectedWithInflectedStringWithDiff3VarsCases() {
        assertEquals(phrases.get("test.multiple.inflected.3.vars.2.same",
                InflectedString.cons(Map.of(
                        NOMINATIVE, "Вести ФМ",
                        GENITIVE, "Вестях ФМ",
                        PREPOSITIONAL, "Вестям ФМ")),
                InflectedString.cons(Map.of(
                        NOMINATIVE, "Коммерсант ФМ",
                        GENITIVE, "Коммерсанта ФМ",
                        PREPOSITIONAL, "Коммерсанте ФМ"))),
                "Чтобы и дальше следить за событиями на Вестях ФМ, скажите мне «Что нового на Вестях ФМ?» И на " +
                        "Коммерсанте ФМ");
    }

    @Test
    public void testMultipleInflectedWithSimpleStringWithDiff2VarsCases() {
        assertEquals(phrases.get("test.multiple.inflected.2.vars",
                List.of("Вести ФМ", "Коммерсант ФМ")),
                "Чтобы и дальше следить за событиями на Вести ФМ, скажите мне «Что нового на Коммерсант ФМ?»");
    }

    @Test
    public void testOneWithoutTemplateWithDefault() {
        assertEquals(phrases.get("station.discovery.suggest.simple.1", "default"), "Ух ты, а я и не знаю. Но у меня " +
                "есть навык {0}. {1}?");
    }

    @Test
    public void testGetAllWithVariables() {
        assertEquals(
                Set.copyOf(phrases.getAll("news.content.postroll.next.provider.approve.phrase.prefix", "Вести ФМ")),
                Set.copyOf(List.of("запускай Вести ФМ", "включай Вести ФМ", "Вести ФМ")));
    }

    @Test
    public void testOneNotExistingWitEmptyDefaultWithoutTemplate() {
        assertEquals(phrases.get("station.discovery.suggest.simple.not.existing"), "");
    }

    @Test
    public void testOneNotExistingWitDefaultWithoutTemplate() {
        assertEquals(phrases.get("station.discovery.suggest.simple.not.existing", "default"), "default");
    }

    @Test
    public void testOneNotExistingWithoutTemplateUnknownLang() {
        assertEquals(phrases.get("station.discovery.suggest.simple.not.existing", "default", Language.UNKNOWN),
                "default");
    }

    @Test
    public void testOneWithTemplate() {
        assertEquals(phrases.get("station.discovery.suggest.simple.1", "", "Простые рецепты", "Запустить для вас"),
                "Ух ты, а я и не знаю. Но у меня есть навык Простые рецепты. Запустить для вас?");
    }

    @Test
    public void testOneNotExistingWitEmptyDefaultWithTemplate() {
        assertEquals(phrases.get("station.discovery.suggest.simple.not.existing"), "");
    }

    @Test
    public void testOneNotExistingWitDefaultWithTemplate() {
        assertEquals(phrases.get("station.discovery.suggest.simple.not.existing", "default"), "default");
    }

    @Test
    public void testOneNotExistingWitTemplateUnknownLang() {
        assertEquals(phrases.get("station.discovery.suggest.simple.not.existing", "default", Language.UNKNOWN),
                "default");
    }

    @Test
    public void testOneWithPartTemplate() {
        assertEquals(phrases.get("station.discovery.suggest.simple.1", "", "Простые рецепты"), "Ух ты, а я и не знаю." +
                " Но у меня есть навык Простые рецепты. {1}?");
    }

    @Test
    public void testOneWithTemplateRandomFromPool() {
        Random random = new Random(seed);

        assertEquals(phrases.getRandom("station.discovery.suggest", random, "Простые рецепты",
                "Запустить для вас"),
                "О, я тоже недавно спрашивала об этом сама у себя и нашла ответ в навыке \"Простые рецепты\". " +
                        "Запустить?");
    }

    @Test
    public void testOneWithoutTemplateRandomFromPool() {
        Random random = new Random(seed);

        assertEquals(phrases.getRandom("station.discovery.suggest", random),
                "О, я тоже недавно спрашивала об этом сама у себя и нашла ответ в навыке \"{0}\". Запустить?");
    }

    @Test
    public void testOneWithoutTemplateWithDefaultRandomFromPool() {
        Random random = new Random(seed);

        assertEquals(phrases.getRandom("station.discovery.suggest", "default", random),
                "О, я тоже недавно спрашивала об этом сама у себя и нашла ответ в навыке \"{0}\". Запустить?");
    }

    @Test
    public void testOneNotExistingWitEmptyDefaultWithoutTemplateRandomFromPool() {
        Random random = new Random(seed);

        assertEquals(phrases.getRandom("station.discovery.suggest.simple.not.existing", random), "");
    }

    @Test
    public void testOneNotExistingWitDefaultWithoutTemplateRandomFromPool() {
        Random random = new Random(seed);

        assertEquals(phrases.getRandom("station.discovery.suggest.simple.not.existing", "default", random), "default");
    }

    @Test
    public void testOneNotExistingWithoutTemplateUnknownLangRandomFromPool() {
        Random random = new Random(seed);

        assertEquals(phrases.getRandom("station.discovery.suggest.simple.not.existing", "default", random,
                Language.UNKNOWN), "default");
    }

    @Test
    public void testOneNotExistingWitEmptyDefaultWithTemplateRandomFromPool() {
        Random random = new Random(seed);

        assertEquals(phrases.getRandom("station.discovery.suggest.simple.not.existing", random), "");
    }

    @Test
    public void testOneNotExistingWitDefaultWithTemplateRandomFromPool() {
        Random random = new Random(seed);

        assertEquals(phrases.getRandom("station.discovery.suggest.simple.not.existing", "default", random), "default");
    }

    @Test
    public void testOneNotExistingWitTemplateUnknownLangRandomFromPool() {
        Random random = new Random(seed);

        assertEquals(phrases.getRandom("station.discovery.suggest.simple.not.existing", "default", random,
                Language.UNKNOWN), "default");
    }

    @Test
    public void testOneWithTemplateRandomFromPoolRandomNext() {
        Random random = new Random(seed);

        assertEquals(phrases.getRandom("station.discovery.suggest", random, "Простые рецепты",
                "Запустить для вас"),
                "О, я тоже недавно спрашивала об этом сама у себя и нашла ответ в навыке \"Простые рецепты\". " +
                        "Запустить?");

        assertEquals(phrases.getRandom("station.discovery.suggest", random, "Простые рецепты",
                "Запустить для вас"),
                "О, я тоже недавно спрашивала об этом сама у себя и нашла ответ в навыке \"Простые рецепты\". " +
                        "Запустить?");

        assertEquals(phrases.getRandom("station.discovery.suggest", random, "Простые рецепты",
                "Запустить для вас"),
                "Хороший вопрос! У меня как раз есть навык \"Простые рецепты\". Запустить?");
    }

    @Test
    public void testReplaceWithDollar() {
        assertEquals("$", phrases.render("{0}", "$"));
        assertEquals("${5}", phrases.render("{0}", "${5}"));
    }
}
