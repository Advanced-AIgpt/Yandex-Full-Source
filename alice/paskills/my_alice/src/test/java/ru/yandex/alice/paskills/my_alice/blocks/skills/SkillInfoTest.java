package ru.yandex.alice.paskills.my_alice.blocks.skills;

import java.util.List;
import java.util.UUID;

import javax.annotation.Nullable;

import org.junit.jupiter.api.Test;
import org.springframework.util.StringUtils;

import static org.junit.jupiter.api.Assertions.assertEquals;

class SkillInfoTest {

    private SkillInfo createSkillInfo(String name, Category category, @Nullable List<String> examples) {
        return new SkillInfo(
                UUID.randomUUID().toString(),
                name,
                "description",
                category.getName(),
                examples,
                null,
                null,
                null,
                null,
                null
        );
    }

    @Test
    public void testDefaultVoiceSuggest() {
        SkillInfo skill = createSkillInfo("название навыка", Category.BUSINESS_FINANCE, null);
        assertEquals("Запусти навык название навыка", skill.voiceSuggest());
    }

    @Test
    public void testDefaultGameVoiceSuggest() {
        SkillInfo skill = createSkillInfo("название навыка", Category.GAMES_TRIVIA_ACCESSORIES, null);
        assertEquals("Играем в название навыка", skill.voiceSuggest());
    }

    @Test
    public void ignoreExamplesForGame() {
        String example = "запусти навык название навыка";
        List<String> examples = List.of(example);
        SkillInfo skill = createSkillInfo("название навыка", Category.GAMES_TRIVIA_ACCESSORIES, examples);
        assertEquals("Играем в название навыка", skill.voiceSuggest());
    }

    @Test
    public void testVoiceSuggestPicksSmallestExample() {
        String longExample = "давай поиграем в название навыка";
        String shortExample = "запусти навык название навыка";
        List<String> examples = List.of(shortExample, longExample);
        SkillInfo skill = createSkillInfo("название навыка", Category.BUSINESS_FINANCE, examples);
        assertEquals(StringUtils.capitalize(shortExample), skill.voiceSuggest());
    }

    @Test
    public void testWithSkillNameWithGameWord() {
        String name = "Игра угадай мелодию";
        SkillInfo skill = createSkillInfo(name, Category.GAMES_TRIVIA_ACCESSORIES, null);
        assertEquals("Играем в игру угадай мелодию", skill.voiceSuggest());
    }


}
