package ru.yandex.quasar.billing.filter;

import java.io.IOException;
import java.nio.charset.Charset;
import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import org.apache.http.NameValuePair;
import org.apache.http.client.HttpResponseException;
import org.apache.http.client.utils.URLEncodedUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.HttpRequest;
import org.springframework.http.MediaType;
import org.springframework.http.client.ClientHttpRequestExecution;
import org.springframework.http.client.ClientHttpRequestInterceptor;
import org.springframework.http.client.ClientHttpResponse;
import org.springframework.stereotype.Component;
import org.springframework.util.StringUtils;

import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.YTLoggingService;
import ru.yandex.quasar.billing.services.YTRequestLogItem;

@Component
class YTLoggerHttpRequestInterceptor implements ClientHttpRequestInterceptor {
    private static final Logger log = LogManager.getLogger();

    private final AuthorizationContext authorizationContext;
    private final YTLoggingService ytLoggingService;

    YTLoggerHttpRequestInterceptor(AuthorizationContext authorizationContext,
                                   YTLoggingService ytLoggingService) {
        this.authorizationContext = authorizationContext;
        this.ytLoggingService = ytLoggingService;
    }

    @Override
    public ClientHttpResponse intercept(HttpRequest request, byte[] body, ClientHttpRequestExecution execution)
            throws IOException {
        long startTimeMillis = System.currentTimeMillis();

        ClientHttpResponse result = null;
        Exception ex = null;
        try {
            result = execution.execute(request, body);
            return result;
        } catch (Exception e) {
            ex = e;
            throw e;
        } finally {
            try {
                logToYT(request, body, result, startTimeMillis, ex);
            } catch (Exception e) {
                log.error("Failed to log request to YT", e);
            }
        }
    }

    private void logToYT(HttpRequest request, byte[] requestBody, @Nullable ClientHttpResponse response,
                         long startTimeMillis, @Nullable Throwable t) throws IOException {
        YTRequestLogItem logItem = new YTRequestLogItem();

        logItem.setUid(authorizationContext.getCurrentUid());

        logItem.setMethod(request.getMethod() != null ? request.getMethod().name() : "UNKNOWN");

        logItem.setScheme(request.getURI().getScheme());
        logItem.setHost(request.getURI().getHost());
        logItem.setPort(request.getURI().getPort());
        logItem.setPath(request.getURI().getPath());
        logItem.setRequestTime(Instant.ofEpochMilli(startTimeMillis));
        logItem.setResponseTime(Instant.now());

        MediaType contentType = request.getHeaders().getContentType();

        String query = request.getURI().getQuery();

        logItem.addParameters(getParametersMap(requestBody, contentType, query));

        Map<String, String> requestHeaders = new HashMap<>(request.getHeaders().toSingleValueMap());
        logItem.addRequestHeaders(requestHeaders);
        logItem.setRequestBody(
                new String(
                        requestBody,
                        contentType != null && contentType.getCharset() != null ? contentType.getCharset() :
                                Charset.defaultCharset()
                )
        );

        String xReqId = null;
        String xRequestId = null;
        for (Map.Entry<String, String> entry : requestHeaders.entrySet()) {
            String headerName = entry.getKey();
            if ("X-Req-Id".equalsIgnoreCase(headerName)) {
                xReqId = entry.getValue();
            } else if ("X-Request-Id".equalsIgnoreCase(headerName)) {
                xRequestId = entry.getValue();
            }
        }

        logItem.setRequestId(xReqId != null ? xReqId : xRequestId);

        if (response != null) {
            logItem.setResponseStatus(response.getRawStatusCode());
            try {
                Map<String, String> responseHeaders = new HashMap<>(response.getHeaders().toSingleValueMap());
                logItem.addResponseHeaders(responseHeaders);
                String responseBody = new String(response.getBody().readAllBytes(),
                        response.getHeaders().getContentType() != null &&
                                response.getHeaders().getContentType().getCharset() != null ?
                                response.getHeaders().getContentType().getCharset() :
                                Charset.defaultCharset());
                logItem.setResponseBody(responseBody);
            } catch (HttpResponseException e) {
                // When 401 error occurs body might not be accessible even with buffering enabled
                // the error is raised so simply leave the body null
                log.error(e.getMessage(), e);
            }

        }

        ytLoggingService.logRequestData("outgoing request: " + request.getURI().getHost(), logItem, t);
    }

    private Map<String, String[]> getParametersMap(byte[] requestBody, MediaType contentType, String query) {
        List<NameValuePair> parse = new ArrayList<>();

        if (!StringUtils.isEmpty(query)) {
            parse.addAll(URLEncodedUtils.parse(query, Charset.defaultCharset()));
        }

        if (contentType != null) {
            if ("application".equalsIgnoreCase(contentType.getType()) &&
                    "x-www-form-urlencoded".equalsIgnoreCase(contentType.getSubtype())) {
                parse.addAll(
                        URLEncodedUtils.parse(
                                new String(
                                        requestBody,
                                        contentType.getCharset() != null ? contentType.getCharset() :
                                                Charset.defaultCharset()
                                ),
                                Charset.defaultCharset()
                        )
                );
            }
        }

        return parse.stream()
                .collect(Collectors.groupingBy(NameValuePair::getName))
                .entrySet()
                .stream()
                .collect(
                        Collectors.toMap(
                                Map.Entry::getKey,
                                e -> e.getValue().stream().map(NameValuePair::getValue).toArray(String[]::new)
                        )
                );
    }


}
