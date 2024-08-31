package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors;

import java.util.List;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

class UtteranceComparatorTest {

    static List<Arguments> getTestData() {
        return List.of(
                Arguments.of("дальше", "дальше", true),
                Arguments.of("дальше", "дальше.", true),
                Arguments.of("Дальше, ", "дальше.", true),
                Arguments.of("Дальше, ", "а дальше.", false)
        );
    }

    @ParameterizedTest(name = "compare utterances {0}, {1}, {2}")
    @MethodSource("getTestData")
    void testEquals(String s1, String s2, boolean shouldBeEqual) {
        Assertions.assertEquals(UtteranceComparator.equal(s1, s2), shouldBeEqual);
    }

}
