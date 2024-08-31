package ru.yandex.alice.paskill.dialogovo.controller;

import java.io.IOException;

import javax.annotation.Nullable;
import javax.servlet.ServletInputStream;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import com.google.common.io.ByteStreams;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.converter.HttpMessageNotReadableException;
import org.springframework.stereotype.Component;
import org.springframework.web.HttpRequestMethodNotSupportedException;
import org.springframework.web.bind.annotation.ControllerAdvice;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.servlet.ModelAndView;
import org.springframework.web.servlet.mvc.support.DefaultHandlerExceptionResolver;
import org.springframework.web.util.ContentCachingRequestWrapper;

import ru.yandex.alice.paskill.dialogovo.midleware.AccessLogger;

@Component
@ControllerAdvice
class UnhandledRequestExceptionHandler extends DefaultHandlerExceptionResolver {
    private static final Logger logger = LogManager.getLogger();
    private static final int MAX_PAYLOAD_LEN = 5000000;

    private final AccessLogger accessLogger;

    UnhandledRequestExceptionHandler(AccessLogger accessLogger) {
        this.accessLogger = accessLogger;
    }


    @ExceptionHandler(value = HttpRequestMethodNotSupportedException.class)
    protected ModelAndView handleMethodNotSupportedException(HttpRequestMethodNotSupportedException ex,
                                                             HttpServletRequest request,
                                                             HttpServletResponse response,
                                                             @Nullable Object handler) throws IOException {
        accessLogger.log(request, response, ex);
        logRequestBody(request);
        return handleHttpRequestMethodNotSupported(ex, request, response, handler);
    }

    @ExceptionHandler(value = HttpMessageNotReadableException.class)
    protected ModelAndView handleHttpMessageNotReadableException(HttpMessageNotReadableException ex,
                                                                 HttpServletRequest request,
                                                                 HttpServletResponse response,
                                                                 @Nullable Object handler) throws IOException {
        accessLogger.log(request, response, ex);
        logRequestBody(request);
        return handleHttpMessageNotReadable(ex, request, response, handler);
    }

    private void logRequestBody(HttpServletRequest request) {
        try {
            logger.info("Got unhandled request with body={}", getRequestPayload(request));
        } catch (Exception ex) {
            logger.error("Error while logging incorrect request", ex);
        }
    }

    @Nullable
    protected String getRequestPayload(HttpServletRequest request) throws IOException {
        ContentCachingRequestWrapper wrapper = wrapCached(request);
        if (wrapper.getContentLength() > 0) {
            try (ServletInputStream inputStream = wrapper.getInputStream()) {
                byte[] buf = ByteStreams.toByteArray(inputStream);
                return new String(buf, 0, Math.min(buf.length, MAX_PAYLOAD_LEN), wrapper.getCharacterEncoding());
            }
        }

        return null;
    }

    private ContentCachingRequestWrapper wrapCached(HttpServletRequest request) {
        ContentCachingRequestWrapper wrapper;
        if (request instanceof ContentCachingRequestWrapper) {
            wrapper = (ContentCachingRequestWrapper) request;
        } else {
            wrapper = new ContentCachingRequestWrapper(request, MAX_PAYLOAD_LEN);
        }
        return wrapper;
    }

}
