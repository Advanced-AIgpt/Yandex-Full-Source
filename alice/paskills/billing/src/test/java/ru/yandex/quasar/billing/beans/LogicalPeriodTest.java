package ru.yandex.quasar.billing.beans;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertAll;
import static org.junit.jupiter.api.Assertions.assertEquals;

class LogicalPeriodTest {

    @Test
    void testParsePeriod() {
        assertAll(
                () -> assertEquals(LogicalPeriod.ofYears(2), LogicalPeriod.parse("P2Y")),
                () -> assertEquals(LogicalPeriod.ofMonths(3), LogicalPeriod.parse("P3M")),
                () -> assertEquals(LogicalPeriod.ofDays(21), LogicalPeriod.parse("P3W")),
                () -> assertEquals(LogicalPeriod.ofDays(5), LogicalPeriod.parse("P5D"))
        );
    }

    @Test
    void testParseDuration() {
        assertAll(
                () -> assertEquals(LogicalPeriod.ofSeconds(20), LogicalPeriod.parse("PT20S")),
                () -> assertEquals(LogicalPeriod.ofMinutes(15), LogicalPeriod.parse("PT15M")),
                () -> assertEquals(LogicalPeriod.ofHours(10), LogicalPeriod.parse("PT10H"))
        );
    }

    @Test
    void testParseComplex() {
        assertAll(
                () -> assertEquals(LogicalPeriod.of(1, 2, 3), LogicalPeriod.parse("P1Y2M3D")),
                () -> assertEquals(LogicalPeriod.of(1, 2, 3, 4, 5, 6), LogicalPeriod.parse("P1Y2M3DT4H5M6S"))
        );
    }

    @Test
    void testToStringPeriod() {
        assertAll(
                () -> assertEquals("P2Y", LogicalPeriod.ofYears(2).toString()),
                () -> assertEquals("P3M", LogicalPeriod.ofMonths(3).toString()),
                () -> assertEquals("P21D", LogicalPeriod.ofWeeks(3).toString()),
                () -> assertEquals("P5D", LogicalPeriod.ofDays(5).toString())
        );
    }

    @Test
    void testToStringDuration() {
        assertAll(
                () -> assertEquals("PT20S", LogicalPeriod.ofSeconds(20).toString()),
                () -> assertEquals("PT15M", LogicalPeriod.ofMinutes(15).toString()),
                () -> assertEquals("PT10H", LogicalPeriod.ofHours(10).toString())
        );
    }

    @Test
    void testToStringComplex() {
        assertAll(
                () -> assertEquals("P1Y2M3D", LogicalPeriod.of(1, 2, 3).toString()),
                () -> assertEquals("P1Y2M3DT4H5M6S", LogicalPeriod.of(1, 2, 3, 4, 5, 6).toString())
        );
    }

    @Test
    void testOfMethod() {
        assertEquals(LogicalPeriod.of(1, 2, 3), LogicalPeriod.of(1, 2, 3, 0, 0, 0));
    }

}
