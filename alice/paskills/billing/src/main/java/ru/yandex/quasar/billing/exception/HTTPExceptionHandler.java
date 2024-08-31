package ru.yandex.quasar.billing.exception;

import javax.servlet.http.HttpServletRequest;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Getter;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.core.Ordered;
import org.springframework.core.annotation.Order;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.ControllerAdvice;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.servlet.mvc.method.annotation.ResponseEntityExceptionHandler;


/**
 * Component to properly and custom log our exceptions.
 * <p>
 * Works with descendants of {@link AbstractHTTPException}, see it's Javadoc.
 * <p>
 * Returns to the caller an {@link ErrorInfo} instance.
 * <p>
 * Example::
 * <p>
 * 2018-06-29 14:04:36.062  WARN 9825 --- [o-auto-1-exec-1] r.y.q.b.exception.HTTPExceptionHandler   :
 * 400/BadRequestException/98ebf5e8-da71-4cf7-b46c-3df2b1f9dd36 at GET /billing/getContentMetaInfo: Unsupported type
 * film
 * Filtered trace:
 * ru.yandex.quasar.billing.controller.BillingControllerTest$Config$TestContentProvider.getContentMetaInfo
 * (BillingControllerTest.java:449)
 * ru.yandex.quasar.billing.controller.BillingController.getContentMetaInfo(BillingController.java:255)
 * ru.yandex.quasar.billing.filter.YTLoggingFilter.doFilter(YTLoggingFilter.java:52)
 * ru.yandex.quasar.billing.filter.StatsLoggingFilter.doFilter(StatsLoggingFilter.java:38)
 * ru.yandex.quasar.billing.filter.HeaderModifierFilter.doFilter(HeaderModifierFilter.java:59)
 */
@Order(Ordered.HIGHEST_PRECEDENCE)
@ControllerAdvice
public class HTTPExceptionHandler extends ResponseEntityExceptionHandler {
    private static final Logger log = LogManager.getLogger(HTTPExceptionHandler.class);

    @ExceptionHandler(AbstractHTTPException.class)
    ResponseEntity<Object> handleHTTPException(AbstractHTTPException ex, HttpServletRequest request) {
        log.warn(
                "{}/{}/{} at {} {}: {}\nFiltered trace:\n{}",
                ex.getStatus(),
                ex.getClass().getSimpleName(),
                ex.getExceptionId(),
                request.getMethod(),
                request.getRequestURI(),
                ex.getInternalMessage(),
                ExceptionUtils.getShortenedStackTrace(ex)
        );


        return ResponseEntity.status(ex.getStatus()).body(ErrorInfo.fromHTTPException(ex));
    }

    @Getter
    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    public static class ErrorInfo {
        private final String message;
        private final String time;
        private final String id;
        @JsonInclude(JsonInclude.Include.NON_NULL)
        private final String providerName;
        @JsonInclude(JsonInclude.Include.NON_NULL)
        private final Boolean socialApiSessionExists;

        static ErrorInfo fromHTTPException(AbstractHTTPException ex) {
            if (ex instanceof ProviderUnauthorizedException) {
                ProviderUnauthorizedException unauthorizedException = (ProviderUnauthorizedException) ex;
                return new ErrorInfo(ex.getExternalMessage(), ex.getExceptionTime(), ex.getExceptionId(),
                        unauthorizedException.getProviderName(), unauthorizedException.isSocialApiSessionFound());
            } else {
                return new ErrorInfo(ex.getExternalMessage(), ex.getExceptionTime(), ex.getExceptionId(), null, null);
            }
        }
    }
}
