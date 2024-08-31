package ru.yandex.quasar.billing.services;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import lombok.Getter;
import lombok.Setter;

@Setter
@Getter
public class YTRequestLogItem {

    private final Map<String, String[]> parameters = new HashMap<>();
    private final Map<String, String> requestHeaders = new HashMap<>();
    private final Map<String, String> responseHeaders = new HashMap<>();
    private String uid;
    private String method;
    private String scheme;
    private String host;
    private int port;
    private String path;
    private String requestBody;
    private String responseBody;

    private int responseStatus;

    private String requestId;
    private Instant requestTime;
    private Instant responseTime;
    private String exceptionStackTrace;
    private final List<String> experiments = new ArrayList<>();

    public void addRequestHeader(String name, String value) {
        requestHeaders.put(name.toLowerCase(), value);
    }

    public void addRequestHeaders(Map<String, String> rawHeaders) {
        for (Map.Entry<String, String> entry : rawHeaders.entrySet()) {
            addRequestHeader(entry.getKey(), entry.getValue());
        }
    }

    public void addResponseHeader(String name, String value) {
        responseHeaders.put(name.toLowerCase(), value);
    }

    public void addResponseHeaders(Map<String, String> rawHeaders) {
        for (Map.Entry<String, String> entry : rawHeaders.entrySet()) {
            addResponseHeader(entry.getKey(), entry.getValue());
        }
    }

    public void addParameters(Map<String, String[]> newParams) {
        parameters.putAll(newParams);
    }

    public void addExperiments(Collection<String> exps) {
        this.experiments.addAll(exps);
    }

}
