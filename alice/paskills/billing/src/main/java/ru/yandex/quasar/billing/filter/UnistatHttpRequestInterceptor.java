package ru.yandex.quasar.billing.filter;

import java.io.IOException;
import java.net.SocketTimeoutException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;

import javax.annotation.Nullable;

import com.google.common.collect.Lists;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.HttpRequest;
import org.springframework.http.client.ClientHttpRequestExecution;
import org.springframework.http.client.ClientHttpRequestInterceptor;
import org.springframework.http.client.ClientHttpResponse;
import org.springframework.stereotype.Component;

import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.UniversalProviderConfig;
import ru.yandex.quasar.billing.services.UnistatService;

@Component
class UnistatHttpRequestInterceptor implements ClientHttpRequestInterceptor {
    private static final Logger log = LogManager.getLogger();
    private final Map<String, List<PathRule>> dynamicPathSimplificationRules = new HashMap<>();

    private final UnistatService unistatService;

    UnistatHttpRequestInterceptor(UnistatService unistatService,
                                  BillingConfig billingConfig) throws URISyntaxException {
        this.unistatService = unistatService;
        configureSimplificationRules(billingConfig);
    }

    @Override
    public ClientHttpResponse intercept(HttpRequest request, byte[] body, ClientHttpRequestExecution execution)
            throws IOException {
        long startTimeMillis = System.currentTimeMillis();
        ClientHttpResponse result = null;
        boolean wasTimeout = false;
        try {
            result = execution.execute(request, body);
            return result;
        } catch (SocketTimeoutException e) {
            wasTimeout = true;
            throw e;
        } finally {
            try {
                logToUnistat(request, result, startTimeMillis, wasTimeout);
            } catch (Exception e) {
                log.error("Failed to log request to unistat", e);
            }
        }
    }

    private void logToUnistat(
            HttpRequest request,
            @Nullable ClientHttpResponse result,
            long startTimeMillis,
            boolean wasTimeout
    ) throws IOException {
        URI uri = request.getURI();
        String host = uri.getHost();
        String path = uri.getPath();
        if (path.endsWith("/")) {
            path = path.substring(1, path.length() - 1);
        } else if (!path.isEmpty()) {
            path = path.substring(1);
        }

        path = simplifyPath(host, path);

        String method = host.replace('.', '-')
                + "_" + path.replace('/', '-').replace('.', '-');

        unistatService.logOperationDurationHist(
                "quasar_billing_remote_method_" + method + "_duration_dhhh",
                System.currentTimeMillis() - startTimeMillis
        );

        unistatService.incrementStatValue("quasar_billing_remote_method_" + method + "_calls_dmmm");
        if (wasTimeout) {
            unistatService.incrementStatValue("quasar_billing_remote_method_" + method + "_timeout_dmmm");
        }

        // Mediabilling responds with 451 code on some request from unsupported countries for example from CG
        if (result == null || (result.getRawStatusCode() >= 400 && result.getRawStatusCode() != 451)) {
            unistatService.incrementStatValue("quasar_billing_remote_method_" + method + "_failures_dmmm");
        }
    }


    private String simplifyPath(String host, String path) {

        List<PathRule> pathRules = dynamicPathSimplificationRules.getOrDefault(host, Collections.emptyList());

        for (PathRule pathRule : pathRules) {
            if (pathRule.getPattern().matcher(path).matches()) {
                return pathRule.getReplacement();
            }
        }

        return path;
    }

    private void configureSimplificationRules(BillingConfig billingConfig) throws URISyntaxException {
        dynamicPathSimplificationRules
                .computeIfAbsent(new URI(billingConfig.getAmediatekaConfig().getApiUrl()).getHost(),
                        x -> new ArrayList<>())
                .addAll(
                        Lists.newArrayList(
                                createPathRule("v1/bundles/@/items.json"),
                                createPathRule("external/v1/serials/@"),
                                createPathRule("external/v1/seasons/@/episodes.json"),
                                createPathRule("v1/films/@/streams.json"),
                                createPathRule("v1/episodes/@/streams.json"),
                                createPathRule("v1/seasons/@/streams.json"),
                                createPathRule("v1/serials/@/streams.json"),
                                createPathRule("v1/films/@/availability.json"),
                                createPathRule("v1/episodes/@/availability.json"),
                                createPathRule("v1/seasons/@/availability.json"),
                                createPathRule("v1/serials/@/availability.json"),
                                createPathRule("external/v1/films/@"),
                                createPathRule("external/v1/serials/@"),
                                createPathRule("external/v1/pay/promo_code/@")
                        )
                );

        dynamicPathSimplificationRules
                .computeIfAbsent(new URI(billingConfig.getTrustBillingConfig().getApiBaseUrl()).getHost(),
                        x -> new ArrayList<>())
                .addAll(

                        Lists.newArrayList(
                                createPathRule("trust-payments/v2/payments/@"),
                                createPathRule("trust-payments/v2/payments/@/start"),
                                createPathRule("trust-payments/v2/payments/@/clear"),
                                createPathRule("trust-payments/v2/payments/@/unhold"),
                                createPathRule("trust-payments/v2/subscriptions/@")
                        )
                );

        dynamicPathSimplificationRules
                .computeIfAbsent(new URI(billingConfig.getSocialAPIClientConfig().getSocialApiBaseUrl()).getHost(),
                        x -> new ArrayList<>())
                .addAll(Lists.newArrayList(
                        createCustomPathRule("api/token/\\d+", "api/token/@")
                        )
                );

        dynamicPathSimplificationRules
                .computeIfAbsent(new URI(billingConfig.getYaPayConfig().getApiBaseUrl()).getHost(),
                        x -> new ArrayList<>())
                .addAll(
                        Lists.newArrayList(
                                createPathRule("v1/merchant_by_key/@"),
                                createPathRule("v1/internal/order/@"),
                                createPathRule("v1/internal/order/@/@"),
                                createPathRule("v1/internal/order/@/@/start"),
                                createPathRule("v1/internal/order/@/@/clear"),
                                createPathRule("v1/internal/order/@/@/unhold"),
                                createPathRule("v1/internal/service/@")
                        )
                );

        for (UniversalProviderConfig providerConfig : billingConfig.getUniversalProviders().values()) {
            var baseUrl = new URI(providerConfig.getBaseUrl());
            // The goal is no remove only leading slash from the base URL and preserve (or add if missing) trailing
            // slash
            // before UniversalProvider methods
            // So remove both trailing and leading slashes and add back trailing slash if path was not empty
            String basePath = baseUrl.getPath().replaceAll("(^/)|(/$)", "");
            if (!basePath.isEmpty()) {
                basePath = basePath + "/";
            }

            dynamicPathSimplificationRules
                    .computeIfAbsent(baseUrl.getHost(), x -> new ArrayList<>())
                    .addAll(
                            List.of(
                                    createPathRule(basePath + "content/@/available"),
                                    createPathRule(basePath + "content/@/stream"),
                                    createPathRule(basePath + "content/@/options"),
                                    createPathRule(basePath + "products/@/@"),
                                    createPathRule(basePath + "products/@/@/purchase"),
                                    createPathRule(basePath + "content/@"),
                                    createPathRule(basePath + "promo/@"),
                                    createPathRule(basePath + "products/@")
                            )
                    );
        }

    }

    private PathRule createPathRule(String replacement) {
        return PathRule.createForTemplate(replacement);
    }

    private PathRule createCustomPathRule(String regex, String replacement) {
        return new PathRule(Pattern.compile(regex), replacement);
    }

}
