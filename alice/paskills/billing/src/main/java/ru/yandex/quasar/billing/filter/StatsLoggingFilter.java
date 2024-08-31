package ru.yandex.quasar.billing.filter;

import java.io.IOException;
import java.util.List;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.json.JSONObject;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.core.annotation.Order;
import org.springframework.stereotype.Component;
import org.springframework.util.StringUtils;
import org.springframework.web.util.ContentCachingResponseWrapper;

import ru.yandex.quasar.billing.services.UnistatService;

import static java.util.stream.Collectors.toList;
import static org.springframework.http.HttpStatus.UNAUTHORIZED;
import static org.springframework.util.StringUtils.trimLeadingCharacter;
import static org.springframework.util.StringUtils.trimTrailingCharacter;

@Order(2)
@Component
public class StatsLoggingFilter implements Filter {

    private final UnistatService unistatService;
    private final List<PathRule> replaceRules;

    @Autowired
    public StatsLoggingFilter(UnistatService unistatService) {
        this.unistatService = unistatService;
        this.replaceRules = Stream.of(
                "billing/purchase_offer/@",
                "billing/purchase_offer/@/bind",
                "billing/purchase_offer/@/start",
                "billing/purchase_offer/@/status",
                "billing/skill/@/merchants",
                "billing/skill/@",
                "billing/skill/@/public_key",
                "billing/skill/@/request_merchant_access",
                "billing/internal/payment/@",
                "billing/internal/payment/@/refund",
                "billing/internal/refund/@",
                "billing/v2/transaction/@",
                "billing/user/skill_product/@",
                "billing/user/skill_product/@/@"
        )
                .map(PathRule::createForTemplate)
                .collect(toList());
    }

    @Override
    public void init(FilterConfig filterConfig) {
    }

    @Override
    public void doFilter(ServletRequest request, ServletResponse response, FilterChain chain) throws IOException,
            ServletException {
        long startTimeMillis = System.currentTimeMillis();

        boolean exceptionThrown = false;
        try {
            chain.doFilter(request, response);
        } catch (RuntimeException | IOException | ServletException e) {
            exceptionThrown = true;
            throw e;
        } finally {
            if (!(response instanceof ContentCachingResponseWrapper)) {
                response = new ContentCachingResponseWrapper((HttpServletResponse) response);
            }

            String servletPath = ((HttpServletRequest) request).getServletPath();
            String method = getMethodSignal(servletPath);

            unistatService.logOperationDurationHist("quasar_billing_method_" + method + "_duration_dhhh",
                    System.currentTimeMillis() - startTimeMillis);

            int responseStatus = ((HttpServletResponse) response).getStatus();

            unistatService.incrementStatValue("quasar_billing_method_" + method + "_calls_dmmm");

            if (responseStatus >= 400 || exceptionThrown) {
                unistatService.incrementStatValue("quasar_billing_method_" + method + "_failures_dmmm");
            }
            if (responseStatus == UNAUTHORIZED.value()) {
                ContentCachingResponseWrapper wrappedResponse = (ContentCachingResponseWrapper) response;
                String body = new String(wrappedResponse.getContentAsByteArray(),
                        wrappedResponse.getCharacterEncoding());
                try {
                    JSONObject bodyJson = new JSONObject(body);
                    String providerName = bodyJson.optString("providerName");
                    if (StringUtils.hasText(providerName)) {
                        unistatService.incrementStatValue("quasar_billing_method_" + method + "_provider_" +
                                providerName + "_unauth_failures_dmmm");
                        unistatService.incrementStatValue("quasar_billing_total_" + providerName +
                                "_unauth_failures_dmmm");
                    }
                } catch (RuntimeException ignored) {

                }
            }
        }
    }

    @Nonnull
    String getMethodSignal(String servletPath) {
        String path = trimTrailingCharacter(trimLeadingCharacter(servletPath, '/'), '/');
        path = simplifyPath(path);
        return path.replace("/", "-");
    }

    private String simplifyPath(String path) {
        for (PathRule pathRule : replaceRules) {
            if (pathRule.getPattern().matcher(path).matches()) {
                return pathRule.getReplacement();
            }
        }

        return path;
    }

    @Override
    public void destroy() {
    }
}
