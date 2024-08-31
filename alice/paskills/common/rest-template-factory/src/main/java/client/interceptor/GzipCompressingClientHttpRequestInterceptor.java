package ru.yandex.alice.paskills.common.resttemplate.factory.client.interceptor;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.zip.GZIPOutputStream;

import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpRequest;
import org.springframework.http.client.ClientHttpRequestExecution;
import org.springframework.http.client.ClientHttpRequestInterceptor;
import org.springframework.http.client.ClientHttpResponse;

public class GzipCompressingClientHttpRequestInterceptor implements ClientHttpRequestInterceptor {

    public static byte[] compress(byte[] body) throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        try (GZIPOutputStream gzipOutputStream = new GZIPOutputStream(baos)) {
            gzipOutputStream.write(body);
        }
        return baos.toByteArray();
    }

    public ClientHttpResponse intercept(HttpRequest request, byte[] body, ClientHttpRequestExecution exec)
            throws IOException {

        HttpHeaders httpHeaders = request.getHeaders();
        httpHeaders.add(HttpHeaders.CONTENT_ENCODING, "gzip");
        httpHeaders.add(HttpHeaders.ACCEPT_ENCODING, "gzip");
        return exec.execute(request, compress(body));
    }
}
