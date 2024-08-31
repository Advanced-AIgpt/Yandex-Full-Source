package ru.yandex.alice.paskill.dialogovo.utils;

import java.net.UnknownHostException;

import javax.servlet.http.HttpServletRequest;

import org.junit.jupiter.api.Test;
import org.mockito.Mockito;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.when;

class ControllerUtilsTest {

    @Test
    void testRemote() throws UnknownHostException {
        HttpServletRequest httpServletRequest = Mockito.mock(HttpServletRequest.class);
        when(httpServletRequest.getLocalAddr()).thenReturn("192.13.14.2");
        assertTrue(ControllerUtils.external(httpServletRequest));
    }

    @Test
    void testLocal() throws UnknownHostException {
        HttpServletRequest httpServletRequest = Mockito.mock(HttpServletRequest.class);
        when(httpServletRequest.getLocalAddr()).thenReturn("127.0.0.1");
        assertFalse(ControllerUtils.external(httpServletRequest));
    }
}
