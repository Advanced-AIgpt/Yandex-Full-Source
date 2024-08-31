package ru.yandex.alice.paskills.common.logging.protoseq;

import java.io.IOException;

import org.springframework.http.HttpRequest;
import org.springframework.http.client.ClientHttpRequestExecution;
import org.springframework.http.client.ClientHttpRequestInterceptor;
import org.springframework.http.client.ClientHttpResponse;

public class ClientSetraceInterceptor implements ClientHttpRequestInterceptor {

    private final String description;
    private final boolean sendRtLogToken;

    public ClientSetraceInterceptor(String description, boolean sendRtLogToken) {
        this.description = description;
        this.sendRtLogToken = sendRtLogToken;
    }

    @Override
    public ClientHttpResponse intercept(
            HttpRequest request,
            byte[] body,
            ClientHttpRequestExecution execution) throws IOException {
        ChildActivation childActivation = new ChildActivation(description);
        if (sendRtLogToken) {
            request.getHeaders().add("X-RTLog-Token", childActivation.rtLogToken());
        }
        try {
            return childActivation.run(() -> execution.execute(request, body));
        } catch (IOException e) {
            throw e;
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
