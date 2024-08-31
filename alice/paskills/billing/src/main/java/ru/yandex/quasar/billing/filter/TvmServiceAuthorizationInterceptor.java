package ru.yandex.quasar.billing.filter;

import java.util.Arrays;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.stream.Collectors;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import com.google.common.base.Strings;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.method.HandlerMethod;
import org.springframework.web.servlet.HandlerInterceptor;

import ru.yandex.passport.tvmauth.CheckedServiceTicket;
import ru.yandex.passport.tvmauth.TicketStatus;
import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.config.SecretsConfig;
import ru.yandex.quasar.billing.services.tvm.TvmClientName;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

class TvmServiceAuthorizationInterceptor implements HandlerInterceptor {

    private static final Logger logger = LogManager.getLogger();
    private final TvmClient tvmClient;
    private final Map<String, Integer> tvmAliases;
    /**
     * if returns {@link Optional#empty} if no TVM restriction defined for a handler method
     */

    private final ConcurrentMap<HandlerMethod, Optional<Set<Integer>>> handlersCache = new ConcurrentHashMap<>();

    TvmServiceAuthorizationInterceptor(TvmClient tvmClient, SecretsConfig secretsConfig) {
        this.tvmClient = tvmClient;
        this.tvmAliases = secretsConfig.getTvmAliases();
    }

    @Override
    public boolean preHandle(HttpServletRequest request, HttpServletResponse response, Object handler)
            throws Exception {
        if (handler instanceof HandlerMethod) {
            HandlerMethod handlerMethod = (HandlerMethod) handler;

            Optional<Set<Integer>> restrictionConfig
                    = handlersCache.computeIfAbsent(handlerMethod, this::getAllowedServices);

            restrictionConfig.ifPresent(allowedClientIds -> validateRequest(request, allowedClientIds));

            return true;
        }

        return true;
    }

    private void validateRequest(HttpServletRequest request, Set<Integer> allowedClientIds) {
        String serviceTicketHeader = request.getHeader(TvmHeaders.SERVICE_TICKET_HEADER);
        if (!Strings.isNullOrEmpty(serviceTicketHeader)) {

            CheckedServiceTicket serviceTicket = tvmClient.checkServiceTicket(serviceTicketHeader);
            if (serviceTicket.getStatus() != TicketStatus.OK) {
                logger.error("Bad TVM service ticket: " + serviceTicket.getStatus() +
                        " debug: " + serviceTicket.debugInfo());
                throw new TvmServiceAuthorizationException();
            }
            if (!(allowedClientIds.isEmpty() || allowedClientIds.contains(serviceTicket.getSrc()))) {
                logger.error("Not allowed for ticket source: " + serviceTicket.getSrc());
                throw new TvmServiceAuthorizationException();
            }
        } else {
            // TVM ticket required
            throw new TvmServiceAuthorizationException();
        }
    }

    /**
     * {@link Optional#empty} is treated as no Tvm protection at all
     */
    private Optional<Set<Integer>> getAllowedServices(HandlerMethod method) {
        TvmRequired methodAnnotation = method.getMethodAnnotation(TvmRequired.class);
        if (methodAnnotation != null) {
            TvmClientName[] allowedServiceNames;
            if (methodAnnotation.allowed().length > 0) {
                allowedServiceNames = methodAnnotation.allowed();
            } else if (methodAnnotation.value().length > 0) {
                allowedServiceNames = methodAnnotation.value();
            } else {
                allowedServiceNames = new TvmClientName[0];
            }

            Set<Integer> allowedIds = Arrays.stream(allowedServiceNames)
                    .map(TvmClientName::getAlias)
                    .map(tvmAliases::get)
                    .filter(Objects::nonNull)
                    .collect(Collectors.toSet());
            return Optional.of(allowedIds);

        } else {
            return Optional.empty();
        }
    }

    @ResponseStatus(HttpStatus.FORBIDDEN)
    private static class TvmServiceAuthorizationException extends RuntimeException {

        TvmServiceAuthorizationException() {
        }

        TvmServiceAuthorizationException(Throwable cause) {
            super(cause);
        }
    }
}
