package ru.yandex.alice.kronstadt.server.http.middleware;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.springframework.stereotype.Component;
import org.springframework.web.servlet.HandlerInterceptor;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.log.LoggingContext;
import ru.yandex.alice.kronstadt.core.log.LoggingContextHolder;

@Component
class LoggingInterceptor implements HandlerInterceptor {

    private final RequestContext requestContext;

    LoggingInterceptor(RequestContext requestContext) {
        this.requestContext = requestContext;
    }

    @Override
    public boolean preHandle(HttpServletRequest request, HttpServletResponse response, Object handler) {
        LoggingContextHolder.setCurrent(LoggingContext.builder()
                .user(requestContext.getCurrentUserId())
                .requestId(requestContext.getRequestId())
                .build());

        return true;
    }

    @Override
    public void afterCompletion(HttpServletRequest request, HttpServletResponse response, Object handler,
                                Exception ex) {
        LoggingContextHolder.clearCurrent();
    }
}
