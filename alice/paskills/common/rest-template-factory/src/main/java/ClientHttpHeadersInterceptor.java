package ru.yandex.alice.paskills.common.resttemplate.factory;

import java.io.IOException;

import com.google.common.net.HttpHeaders;
import org.springframework.http.HttpRequest;
import org.springframework.http.client.ClientHttpRequestExecution;
import org.springframework.http.client.ClientHttpRequestInterceptor;
import org.springframework.http.client.ClientHttpResponse;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.paskill.dialogovo.utils.Headers;

@Component
public class ClientHttpHeadersInterceptor implements ClientHttpRequestInterceptor {

    private final RequestContext requestContext;

    public ClientHttpHeadersInterceptor(RequestContext requestContext) {
        this.requestContext = requestContext;
    }

    @Override
    public ClientHttpResponse intercept(HttpRequest request, byte[] body, ClientHttpRequestExecution execution)
            throws IOException {
        String forwardedFor;
        //check is case-insensitive
        if (!request.getHeaders().containsKey(HttpHeaders.X_FORWARDED_FOR) &&
                (forwardedFor = requestContext.getForwardedFor()) != null) {
            request.getHeaders().add(HttpHeaders.X_FORWARDED_FOR, forwardedFor);
        }

        String userAgent;
        if (!request.getHeaders().containsKey(HttpHeaders.USER_AGENT) &&
                (userAgent = requestContext.getUserAgent()) != null) {
            request.getHeaders().add(HttpHeaders.USER_AGENT, userAgent);
        }

        String requestId;
        if (!request.getHeaders().containsKey(Headers.X_REQUEST_ID) &&
                (requestId = requestContext.getRequestId()) != null) {
            request.getHeaders().add(Headers.X_REQUEST_ID, requestId);
        }

        return execution.execute(request, body);
    }
}

