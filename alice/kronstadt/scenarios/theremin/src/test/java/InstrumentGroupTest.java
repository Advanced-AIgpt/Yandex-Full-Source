package ru.yandex.alice.paskill.dialogovo.scenarios.theremin;

import java.util.stream.Stream;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertAll;
import static org.junit.jupiter.api.Assertions.assertFalse;

class InstrumentGroupTest {

    @Test
    void testNoEmptyInstrumentGroup() {
        assertAll(Stream.of(InstrumentGroup.values())
                //.filter(group -> group.getInstruments().isEmpty())
                .map(group -> (() -> assertFalse(group.getInstruments().isEmpty(), "Group '" + group.name() + "' " +
                        "contains no instruments!")))

        );
    }
}
