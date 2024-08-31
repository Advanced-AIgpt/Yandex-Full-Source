package ru.yandex.quasar.billing.filter;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.quasar.billing.services.UnistatService;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.Mockito.mock;

class StatsLoggingFilterTest {

    private StatsLoggingFilter filter;

    @BeforeEach
    void setUp() {
        filter = new StatsLoggingFilter(mock(UnistatService.class));
    }

    @Test
    void getMethod() {
        String actual = filter.getMethodSignal("/foo/bar");
        String expected = "foo-bar";
        assertEquals(expected, actual);
    }

    @Test
    void getMethodWithTemplate() {
        String actual = filter.getMethodSignal("billing/purchase_offer/123123123");
        String expected = "billing-purchase_offer-@";
        assertEquals(expected, actual);
    }

    @Test
    void testGetMethodTrailingSlash() {
        String actual = filter.getMethodSignal("/foo/bar//ree/");
        String expected = "foo-bar--ree";
        assertEquals(expected, actual);
    }

    @Test
    void testGetMethodSingleSlash() {
        String actual = filter.getMethodSignal("/");
        String expected = "";
        assertEquals(expected, actual);
    }

    @Test
    void testGetMethodDoubleSlash() {
        String actual = filter.getMethodSignal("//");
        String expected = "";
        assertEquals(expected, actual);
    }

    @Test
    void testGetMethodTripleSlash() {
        String actual = filter.getMethodSignal("///");
        String expected = "";
        assertEquals(expected, actual);
    }

    @Test
    void testGetMethodEmpty() {
        String actual = filter.getMethodSignal("");
        String expected = "";
        assertEquals(expected, actual);
    }
}
