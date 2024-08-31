package ru.yandex.quasar.billing.services;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class UnistatServiceTest {

    @Test
    public void testRangeLowerBound() {
        assertEquals(0.0, UnistatService.rangeLowerBound(0));
        assertEquals(1.0, UnistatService.rangeLowerBound(1));
        assertEquals(1.5, UnistatService.rangeLowerBound(2));
        assertEquals(2.25, UnistatService.rangeLowerBound(3));
        assertEquals(3.375, UnistatService.rangeLowerBound(4));
        assertEquals(3.375, UnistatService.rangeLowerBound(5));
        assertEquals(3325.0, Math.floor(UnistatService.rangeLowerBound(3500)));
        assertEquals(4987.0, Math.floor(UnistatService.rangeLowerBound(6542)));
        assertEquals(7481.0, Math.floor(UnistatService.rangeLowerBound(9382)));
        assertEquals(11222.0, Math.floor(UnistatService.rangeLowerBound(13000)));
    }
}
