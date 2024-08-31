package ru.yandex.quasar.billing.filter;

import java.io.IOException;
import java.time.Instant;
import java.util.Base64;
import java.util.Objects;
import java.util.Optional;
import java.util.UUID;

import javax.annotation.Nullable;
import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.http.HttpServletRequest;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.ThreadContext;
import org.json.JSONArray;
import org.springframework.core.annotation.Order;
import org.springframework.stereotype.Component;

import ru.yandex.quasar.billing.providers.UaasHeaders;
import ru.yandex.quasar.billing.providers.XDeviceHeaders;
import ru.yandex.quasar.billing.services.AuthorizationContext;

@Order(1)
@Component
public class ContextFilter implements Filter {

    private static final Logger logger = LogManager.getLogger();
    private final AuthorizationContext context;

    public ContextFilter(AuthorizationContext context) {
        this.context = context;
    }

    @Override
    public void init(FilterConfig filterConfig) {

    }

    @Override
    public void doFilter(ServletRequest request, ServletResponse response, FilterChain chain)
            throws IOException, ServletException {
        ThreadContext.clearAll();
        context.clearUserContext();
        try {
            var httpRequest = (HttpServletRequest) request;

            String requestId = Objects.requireNonNullElseGet(getRequestId(httpRequest),
                    () -> UUID.randomUUID() + "-generated");

            ThreadContext.put("requestId", requestId);
            context.setRequestId(requestId);
            context.setDeviceVideoCodecs(httpRequest.getHeader(XDeviceHeaders.X_DEVICE_VIDEO_CODECS));
            context.setDeviceAudioCodecs(httpRequest.getHeader(XDeviceHeaders.X_DEVICE_AUDIO_CODECS));
            context.setSupportsCurrentHDCPLevel(httpRequest.getHeader(XDeviceHeaders.SUPPORTS_CURRENT_HDCP_LEVEL));
            context.setDeviceDynamicRanges(httpRequest.getHeader(XDeviceHeaders.X_DEVICE_DYNAMIC_RANGES));
            context.setDeviceVideoFormats(httpRequest.getHeader(XDeviceHeaders.X_DEVICE_VIDEO_FORMATS));
            context.setRequestTimestamp(Instant.now());
            addRequestExperiments(httpRequest);

            chain.doFilter(request, response);
        } finally {
            ThreadContext.clearAll();
            context.clearUserContext();
        }
    }

    @Nullable
    private String getRequestId(HttpServletRequest request) {
        String xReqId = request.getHeader("X-Req-Id");
        return xReqId != null ? xReqId : request.getHeader("X-Request-Id");
    }

    void addRequestExperiments(HttpServletRequest request) {
        String header = request.getHeader(UaasHeaders.EXPERIMENT_FLAG_HEADER);
        if (header != null) {
            try {
                for (String experimentBase64 : header.split(",")) {
                    JSONArray jsonExperiments = Optional.of(new JSONArray(new String(
                            Base64.getDecoder().decode(experimentBase64))))
                            .map(it -> it.optJSONObject(0))
                            .map(it -> it.optJSONObject("CONTEXT"))
                            .map(it -> it.optJSONObject("MAIN"))
                            .map(it -> it.optJSONObject("VOICE"))
                            .map(it -> it.optJSONArray("flags"))
                            .orElse(new JSONArray());

                    for (int i = 0; i < jsonExperiments.length(); i++) {
                        String expFlag = jsonExperiments.optString(i);
                        if (expFlag != null && !expFlag.isBlank()) {
                            context.addRequestExperiments(expFlag);
                        }
                    }
                }
                logger.info("Got experiments: {}", context.getRequestExperiments());
            } catch (Exception e) {
                logger.error("Can't parse usaas' experiments: {}", header);
            }
        }
    }

    @Override
    public void destroy() {
    }
}
