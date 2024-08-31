package ru.yandex.quasar.billing.filter;

import java.io.IOException;

import javax.servlet.FilterChain;
import javax.servlet.Servlet;
import javax.servlet.ServletException;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.mock.web.MockFilterChain;
import org.springframework.mock.web.MockHttpServletRequest;
import org.springframework.mock.web.MockHttpServletResponse;
import org.springframework.test.context.TestPropertySource;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;

import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.YTLoggingService;
import ru.yandex.quasar.billing.services.YTRequestLogItem;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.isNull;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

@SpringJUnitConfig(classes = {TestConfigProvider.class, YTLoggingService.class, WrappingFilter.class})
@TestPropertySource("/application.properties")
class YTLoggingFilterTest {

    private AuthorizationContext authorizationContext;
    @SpyBean
    private YTLoggingService ytLoggingService;
    private YTLoggingFilter filter;
    private WrappingFilter wrappingFilter;

    private MockHttpServletRequest httpServletRequest = new MockHttpServletRequest();
    private MockHttpServletResponse httpServletResponse = new MockHttpServletResponse();
    private MockFilterChain filterChain;
    private Servlet servlet = mock(Servlet.class);
    private FilterChain filterChainMock = mock(FilterChain.class);

    @BeforeEach
    void setUp() {
        authorizationContext = new AuthorizationContext();
        filter = spy(new YTLoggingFilter(ytLoggingService, authorizationContext));
        wrappingFilter = new WrappingFilter();
        httpServletRequest.setServerName("ya.ru");

        filterChain = new MockFilterChain(servlet, wrappingFilter, filter);
    }

    @Test
    void doFilter() throws IOException, ServletException {
        doThrow(new DummyException())
                .when(servlet).service(any(), any());

        assertThrows(DummyException.class,
                () -> filterChain.doFilter(httpServletRequest, httpServletResponse));
        ArgumentCaptor<YTRequestLogItem> captor = ArgumentCaptor.forClass(YTRequestLogItem.class);

        verify(ytLoggingService).logRequestData(any(), captor.capture(), any(Exception.class));
        assertEquals(500, captor.getValue().getResponseStatus());
    }

    @Test
    void doFilterOnHandledException() throws IOException, ServletException {
        doNothing().when(servlet).service(any(), any());
        httpServletResponse.setStatus(500);
        filterChain.doFilter(httpServletRequest, httpServletResponse);
        ArgumentCaptor<YTRequestLogItem> captor = ArgumentCaptor.forClass(YTRequestLogItem.class);
        verify(ytLoggingService).logRequestData(any(), captor.capture(), isNull());
        assertEquals(500, captor.getValue().getResponseStatus());
    }

    private class DummyException extends RuntimeException {

    }
}
