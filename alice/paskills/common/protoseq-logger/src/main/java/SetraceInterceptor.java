package ru.yandex.alice.paskills.common.logging.protoseq;

import javax.annotation.Nullable;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.web.servlet.HandlerInterceptor;

public class SetraceInterceptor implements HandlerInterceptor {

    private static final Logger logger = LogManager.getLogger();

    private static final String HEADER_RTLOG_TOKEN = "X-RTLog-Token";
    private static final String HEADER_APPHOST_REQID = "X-AppHost-Request-Reqid";
    private static final String HEADER_APPHOST_RUID = "X-AppHost-Request-Ruid";
    private static final String ATTR_NAME = SetraceInterceptor.class.getCanonicalName() + ".RTLog-Token";

    private static final SetraceEventLogger ACTIVATION_STARTED_LOGGER =
            new SetraceEventLogger(LogLevels.ACTIVATION_STARTED);
    private static final SetraceEventLogger ACTIVATION_FINISHED_LOGGER =
            new SetraceEventLogger(LogLevels.ACTIVATION_FINISHED);

    private final Setrace setrace;

    @Deprecated(forRemoval = true)
    public SetraceInterceptor(String serviceName) {
        setrace = new Setrace(serviceName);
    }

    // single instance of setrace must be used across all interceptors as it counts frames
    public SetraceInterceptor(Setrace setrace) {
        this.setrace = setrace;
    }

    @Override
    public boolean preHandle(
            HttpServletRequest request,
            HttpServletResponse response,
            Object handler
    ) {
        @Nullable String rtLogToken;
        @Nullable String apphostReqId = request.getHeader(HEADER_APPHOST_REQID);
        @Nullable String apphostRuid = request.getHeader(HEADER_APPHOST_RUID);
        if (apphostReqId != null && apphostRuid != null) {
            rtLogToken = apphostReqId + "-" + apphostRuid;
        } else {
            rtLogToken = request.getHeader(HEADER_RTLOG_TOKEN);
        }
        logger.debug("Rtlog token: {}", rtLogToken);
        if (rtLogToken != null) {
            request.setAttribute(ATTR_NAME, rtLogToken);
            setrace.setupThreadContext(rtLogToken);
            ACTIVATION_STARTED_LOGGER.log();
        }
        return true;
    }

    @Override
    public void afterCompletion(
            HttpServletRequest request,
            HttpServletResponse response,
            Object handler,
            @Nullable Exception ex
    ) {
        if (request.getAttribute(ATTR_NAME) != null) {
            ACTIVATION_FINISHED_LOGGER.log();
            setrace.clearThreadContext();
        }
    }

}
