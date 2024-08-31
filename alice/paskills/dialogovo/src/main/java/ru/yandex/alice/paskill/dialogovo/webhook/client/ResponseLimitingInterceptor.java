package ru.yandex.alice.paskill.dialogovo.webhook.client;

import java.io.IOException;
import java.io.InputStream;
import java.math.BigDecimal;

import javax.annotation.Nullable;

import com.google.common.base.Strings;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpRequest;
import org.springframework.http.client.AbstractClientHttpResponse;
import org.springframework.http.client.ClientHttpRequestExecution;
import org.springframework.http.client.ClientHttpRequestInterceptor;
import org.springframework.http.client.ClientHttpResponse;
import org.springframework.stereotype.Component;

@Component
class ResponseLimitingInterceptor implements ClientHttpRequestInterceptor {
    private final long maxSize;
    private final BigDecimal maxSizeDec;

    ResponseLimitingInterceptor(@Value("${webhookClientConfig.maxResponseSize}") long maxSize) {
        this.maxSize = maxSize;
        this.maxSizeDec = BigDecimal.valueOf(this.maxSize);
    }

    @Override
    public ClientHttpResponse intercept(HttpRequest request, byte[] body, ClientHttpRequestExecution execution)
            throws IOException {

        ClientHttpResponse response = new WrappingClientHttpResponse(execution.execute(request, body));
        String contentLengthStr = response.getHeaders().getFirst(HttpHeaders.CONTENT_LENGTH);
        if (!Strings.isNullOrEmpty(contentLengthStr)) {
            try {
                BigDecimal size = new BigDecimal(contentLengthStr);
                if (size.compareTo(maxSizeDec) > 0) {
                    response.close();
                    throw new ResponseTooLargeException(size);
                }
                return response;
            } catch (NumberFormatException e) {
                response.close();
                throw new ResponseTooLargeException(e);
            }
        } else {
            return response;
        }
    }

    static class ResponseTooLargeException extends IOException {
        @Nullable
        private final BigDecimal size;

        ResponseTooLargeException(Throwable cause) {
            super(cause);
            this.size = null;
        }

        ResponseTooLargeException(@Nullable BigDecimal size) {
            this.size = size;
        }

        @Nullable
        public BigDecimal getSize() {
            return size;
        }
    }

    private class WrappingClientHttpResponse extends AbstractClientHttpResponse {
        private final ClientHttpResponse response;

        WrappingClientHttpResponse(ClientHttpResponse response) {
            this.response = response;
        }

        @Override
        public int getRawStatusCode() throws IOException {
            return response.getRawStatusCode();
        }

        @Override
        public String getStatusText() throws IOException {
            return response.getStatusText();
        }

        @Override
        public void close() {
            response.close();
        }

        @Override
        public InputStream getBody() throws IOException {
            return new LimitedInputStream(response.getBody(), maxSize) {
                @Override
                protected void raiseError(long pSizeMax, long pCount) throws IOException {
                    response.close();
                    throw new ResponseTooLargeException(BigDecimal.valueOf(pCount));
                }
            };
        }

        @Override
        public HttpHeaders getHeaders() {
            return response.getHeaders();
        }
    }

}
