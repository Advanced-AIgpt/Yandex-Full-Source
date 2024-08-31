package ru.yandex.quasar.billing.services;

import java.net.URI;
import java.net.URISyntaxException;

import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.ThreadContext;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.filter.HeaderModifierFilter;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

@Component
public class YTLoggingService {

    /**
     * Через qloud-ный pipeline доставки логов пролезают записи максимум 8Кб (cм. https://wiki.yandex-team
     * .ru/qloud/doc/logs/#std).
     * Т.к. у нас могут попадаться двухбайтовые русские символы, приходится контролировать размер строк в байтах
     */
    private static final Logger requestLogger = LogManager.getLogger("REQUEST_LOGGER");
    private static final Logger logger = LogManager.getLogger();

    private final String blackboxHost;

    private final String socialApiHost;
    private final ObjectMapper objectMapper;

    @Autowired
    public YTLoggingService(BillingConfig billingConfig, ObjectMapper objectMapper) throws URISyntaxException {
        this(
                new URI(billingConfig.getSocialAPIClientConfig().getBlackboxBaseUrl()).getHost(),
                new URI(billingConfig.getSocialAPIClientConfig().getSocialApiBaseUrl()).getHost(),
                objectMapper
        );
    }

    YTLoggingService(String blackboxHost, String socialApiHost, ObjectMapper objectMapper) {
        this.blackboxHost = blackboxHost;
        this.socialApiHost = socialApiHost;

        this.objectMapper = objectMapper;
    }

    private void log(String message, YTRequestLogItem logItem, @Nullable Throwable t) {
        try {
            //ThreadContext.put("fields", objectMapper.writeValueAsString(logItem));
            ThreadContext.put("request_type", message);

            ObjectNode treeObject = objectMapper.valueToTree(logItem);
            if (t == null) {
                requestLogger.info(treeObject);
            } else {
                requestLogger.error(treeObject, t);
            }
        } finally {
            //ThreadContext.remove("fields");
            ThreadContext.remove("request_type");
        }

    }

    public void logRequestData(String message, YTRequestLogItem logItem, @Nullable Throwable t) {

        logItem = filterPrivateData(logItem);

        if (logItem != null) {
            log(message, logItem, t);
        }
    }

    @Nullable
    private YTRequestLogItem filterPrivateData(YTRequestLogItem logItem) {
        if (blackboxHost.equals(logItem.getHost()) ||
                socialApiHost.equals(logItem.getHost()) ||
                ("localhost".equals(logItem.getHost()) && logItem.getPath() != null && logItem.getPath().startsWith(
                        "/tvm/"))) {
            return null;
        }

        if ("/unistat".equals(logItem.getPath()) || "/healthcheck".equals(logItem.getPath())) {
            return null;
        }

        logItem.getParameters().remove("client_secret");
        logItem.getParameters().remove("userEmail");
        logItem.getParameters().remove("session");
        logItem.getParameters().remove("access_token");

        logItem.getRequestHeaders().remove(TvmHeaders.SERVICE_TICKET_HEADER.toLowerCase());
        logItem.getRequestHeaders().remove(TvmHeaders.USER_TICKET_HEADER.toLowerCase());
        logItem.getRequestHeaders().remove("authorization");
        logItem.getRequestHeaders().remove("yandexuid");
        logItem.getRequestHeaders().remove("x-user-ip");
        logItem.getRequestHeaders().remove("cookie");
        logItem.getRequestHeaders().remove(HeaderModifierFilter.HEADER_X_CSRF_TOKEN);

        return logItem;
    }

}
