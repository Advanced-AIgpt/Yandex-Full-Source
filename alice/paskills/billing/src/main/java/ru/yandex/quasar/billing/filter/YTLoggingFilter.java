package ru.yandex.quasar.billing.filter;

import java.io.IOException;
import java.time.Instant;
import java.util.Collection;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;

import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.core.annotation.Order;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Component;
import org.springframework.web.util.ContentCachingRequestWrapper;
import org.springframework.web.util.ContentCachingResponseWrapper;

import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.YTLoggingService;
import ru.yandex.quasar.billing.services.YTRequestLogItem;

@Order(3)
@Component
public class YTLoggingFilter implements Filter {

    private final YTLoggingService ytLoggingService;

    private final AuthorizationContext authorizationContext;

    @Autowired
    public YTLoggingFilter(YTLoggingService ytLoggingService, AuthorizationContext authorizationContext) {
        this.ytLoggingService = ytLoggingService;
        this.authorizationContext = authorizationContext;
    }

    @Override
    public void init(FilterConfig filterConfig) {

    }

    @Override
    public void doFilter(ServletRequest request, ServletResponse response, FilterChain chain) throws IOException,
            ServletException {
        Instant requestInstant = Instant.now();
        Exception unhandledException = null;
        try {
            chain.doFilter(request, response);
        } catch (Exception e) {
            // if we got here means no custom Exception handlers worked out, {@code response#status} is still 200 as
            // nothing changed it
            unhandledException = e;
            throw e;
        } finally {
            YTRequestLogItem logItem = new YTRequestLogItem();

            logItem.setUid(authorizationContext.getCurrentUid());
            logItem.setRequestId(authorizationContext.getRequestId());
            logItem.addExperiments(authorizationContext.getRequestExperiments());

            logItem.setMethod(((HttpServletRequest) request).getMethod());

            logItem.setScheme(request.getScheme());
            logItem.setHost(request.getServerName());
            logItem.setPort(request.getServerPort());
            logItem.setPath(((HttpServletRequest) request).getServletPath());
            logItem.setRequestTime(requestInstant);

            logItem.addParameters(request.getParameterMap());
            Map<String, String> requestHeaders = new HashMap<>();
            for (Enumeration<String> e = ((HttpServletRequest) request).getHeaderNames(); e.hasMoreElements(); ) {
                String headerName = e.nextElement();
                requestHeaders.put(
                        headerName,
                        ((HttpServletRequest) request).getHeader(headerName)
                );
            }
            logItem.addRequestHeaders(requestHeaders);
            logItem.setRequestBody(
                    new String(
                            ((ContentCachingRequestWrapper) request).getContentAsByteArray(),
                            request.getCharacterEncoding()
                    )
            );

            Map<String, String> responseHeaders = new HashMap<>();
            for (String headerName : ((HttpServletResponse) response).getHeaderNames()) {
                Collection<String> headers = ((HttpServletResponse) response).getHeaders(headerName);
                responseHeaders.put(
                        headerName,
                        headers.stream().findFirst().orElse(null)
                );
            }
            logItem.addResponseHeaders(responseHeaders);
            // if exception is unhandled then 500 status is set by the servlet on the upper level
            logItem.setResponseStatus(unhandledException == null ? ((HttpServletResponse) response).getStatus() :
                    HttpStatus.INTERNAL_SERVER_ERROR.value());
            logItem.setResponseBody(
                    new String(
                            ((ContentCachingResponseWrapper) response).getContentAsByteArray(),
                            response.getCharacterEncoding()
                    )
            );
            ((ContentCachingResponseWrapper) response).copyBodyToResponse();
            logItem.setResponseTime(Instant.now());

            ytLoggingService.logRequestData("incoming request: " + ((HttpServletRequest) request).getServletPath(),
                    logItem, unhandledException);
        }
    }

    @Override
    public void destroy() {

    }
}
