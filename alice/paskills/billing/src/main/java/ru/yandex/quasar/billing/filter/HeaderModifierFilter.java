package ru.yandex.quasar.billing.filter;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;

import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.core.annotation.Order;
import org.springframework.stereotype.Component;

import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.SecurityConfig;


@Component
@Order(4)
public class HeaderModifierFilter implements Filter {

    public static final String HEADER_X_CSRF_TOKEN = "x-csrf-token";
    private static final Logger log = LogManager.getLogger();
    private final SecurityConfig securityConfig;

    @Autowired
    public HeaderModifierFilter(BillingConfig billingConfig) {
        securityConfig = billingConfig.getSecurityConfig();
    }

    @Override
    public void init(FilterConfig filterConfig) {
    }

    @Override
    public void doFilter(ServletRequest request, ServletResponse response, FilterChain chain) throws IOException,
            ServletException {
        String origin = ((HttpServletRequest) request).getHeader("Origin");
        if (origin != null) {
            try {
                URI uri = new URI(origin);

                if (uri.getHost() != null
                        && uri.getScheme().equalsIgnoreCase("https")
                        && allowedHost(uri.getHost())
                ) {
                    ((HttpServletResponse) response).addHeader("Access-Control-Allow-Credentials", "true");
                    ((HttpServletResponse) response).addHeader("Access-Control-Allow-Origin", origin);
                    ((HttpServletResponse) response).addHeader("Access-Control-Allow-Headers", HEADER_X_CSRF_TOKEN);
                    ((HttpServletResponse) response).addHeader("Vary", "Origin");
                }
            } catch (URISyntaxException e) {
                log.warn("Could not parse Origin URI: " + origin, e);
            }
        }

        chain.doFilter(request, response);
    }

    boolean allowedHost(String host) {
        return securityConfig.getCorsAllowedDomains().contains(host.toLowerCase()) ||
                securityConfig.getAllowedHostPatterns().stream()
                        .anyMatch(pattern -> pattern.matcher(host.toLowerCase()).matches());
    }

    @Override
    public void destroy() {
    }
}
