package ru.yandex.quasar.billing.filter;


import java.io.IOException;

import com.google.common.net.HttpHeaders;
import org.springframework.http.HttpRequest;
import org.springframework.http.client.ClientHttpRequestExecution;
import org.springframework.http.client.ClientHttpRequestInterceptor;
import org.springframework.http.client.ClientHttpResponse;
import org.springframework.stereotype.Component;

import ru.yandex.quasar.billing.services.AuthorizationContext;

import static ru.yandex.quasar.billing.services.AuthorizationService.X_REQUEST_ID;

@Component
class NginxHeadersInterceptor implements ClientHttpRequestInterceptor {

    private final AuthorizationContext authorizationContext;

    NginxHeadersInterceptor(AuthorizationContext authorizationContext) {
        this.authorizationContext = authorizationContext;
    }

    @Override
    public ClientHttpResponse intercept(HttpRequest request, byte[] body, ClientHttpRequestExecution execution)
            throws IOException {
        String forwardedFor;
        //check is case-insensitive
        if (!request.getHeaders().containsKey(HttpHeaders.X_FORWARDED_FOR) &&
                (forwardedFor = authorizationContext.getForwardedFor()) != null) {
            request.getHeaders().add(HttpHeaders.X_FORWARDED_FOR, forwardedFor);
        }

        String userAgent;
        if (!request.getHeaders().containsKey(HttpHeaders.USER_AGENT) &&
                (userAgent = authorizationContext.getUserAgent()) != null) {
            request.getHeaders().add(HttpHeaders.USER_AGENT, userAgent);
        }

        String requestId;
        if (!request.getHeaders().containsKey(X_REQUEST_ID) &&
                (requestId = authorizationContext.getRequestId()) != null) {
            request.getHeaders().add(X_REQUEST_ID, requestId);
        }

        return execution.execute(request, body);
    }
}
