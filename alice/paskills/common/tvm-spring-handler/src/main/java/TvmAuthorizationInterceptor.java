package ru.yandex.alice.paskills.common.tvm.spring.handler;

import java.io.IOException;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Map;
import java.util.Optional;
import java.util.OptionalInt;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.regex.Pattern;

import javax.annotation.Nullable;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.http.HttpStatus;
import org.springframework.web.method.HandlerMethod;
import org.springframework.web.servlet.HandlerInterceptor;

import ru.yandex.passport.tvmauth.CheckedServiceTicket;
import ru.yandex.passport.tvmauth.CheckedUserTicket;
import ru.yandex.passport.tvmauth.TicketStatus;
import ru.yandex.passport.tvmauth.TvmClient;

import static ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityRequestAttributes.TVM_CLIENT_ID_REQUEST_ATTR;
import static ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityRequestAttributes.UID_REQUEST_ATTR;
import static ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityRequestAttributes.USER_TICKET_REQUEST_ATTR;

public class TvmAuthorizationInterceptor implements HandlerInterceptor {
    private static final Logger logger = LogManager.getLogger();

    private final TvmClient tvmClient;
    private final SecurityAccessProvider securityAccessProvider;
    @Nullable
    private final String developerTrustedToken;
    private final ConcurrentMap<HandlerMethod, Optional<HandlerRequirements>> handlersCache = new ConcurrentHashMap<>();
    private final boolean allowXUid;

    private final Pattern userTicketPattern = Pattern.compile(":.+?$");
    private final boolean validateServiceTicket;


    public TvmAuthorizationInterceptor(
            TvmClient tvmClient,
            Map<String, Set<Integer>> allowedServiceClientIds,
            @Nullable String developerTrustedToken,
            boolean allowXUid
    ) {
        this(tvmClient, allowedServiceClientIds, developerTrustedToken, allowXUid, true);
    }

    public TvmAuthorizationInterceptor(
            TvmClient tvmClient,
            Map<String, Set<Integer>> allowedServiceClientIds,
            @Nullable String developerTrustedToken,
            boolean allowXUid,
            boolean validateServiceTicket
    ) {
        this.tvmClient = tvmClient;
        this.developerTrustedToken = developerTrustedToken;
        this.securityAccessProvider = new SecurityAccessProvider(allowedServiceClientIds);
        this.allowXUid = allowXUid;
        this.validateServiceTicket = validateServiceTicket;
    }

    @Override
    public boolean preHandle(HttpServletRequest request, HttpServletResponse response, Object handler)
            throws IOException {

        if (handler instanceof HandlerMethod) {
            HandlerMethod handlerMethod = (HandlerMethod) handler;

            // check authorization only if specified on method
            Optional<HandlerRequirements> handlerRequirements =
                    handlersCache.computeIfAbsent(handlerMethod, this::handlerRequirements);
            if (handlerRequirements.isPresent()) {
                return validateAccess(handlerRequirements.get(), request, response);
            }
        }

        return true;
    }

    private boolean validateAccess(HandlerRequirements requirements,
                                   HttpServletRequest request,
                                   HttpServletResponse response
    ) throws IOException {
        boolean ok = processServiceTicket(request, requirements.allowed, response);

        if (ok) {
            return processUserTicket(request, requirements.userRequired, response);
        } else {
            return false;
        }

    }

    private boolean processServiceTicket(HttpServletRequest request,
                                         Set<String> authorizedServices,
                                         HttpServletResponse response
    ) throws IOException {
        OptionalInt sourceServiceTvmClientIdO;
        if (isTrustedDeveloperMode(request)) {
            logger.debug("Check trusted developer mode auth");
            sourceServiceTvmClientIdO = tryGetServiceClientIdFromTrustedDeveloperMode(request);
        } else {
            logger.debug("Check TVM auth");
            sourceServiceTvmClientIdO = tryGetServiceClientIdFromTvmHeaders(request);
        }

        if (!validateServiceTicket) {
            return true;
        } else if (sourceServiceTvmClientIdO.isEmpty()) {
            logger.info("Authorization for services = [{}] required but " +
                    "sourceTvmClientId not set in request context", authorizedServices);
            response.sendError(HttpStatus.FORBIDDEN.value(), "authorization not found");
            return false;
        }
        int sourceServiceTvmClientId = sourceServiceTvmClientIdO.getAsInt();
        logger.debug("Source client id: {}", sourceServiceTvmClientId);

        // check for whitelist if specified in the annotation
        if (!authorizedServices.isEmpty()) {
            logger.debug("Check source client id in allowed clients list: {}", authorizedServices);

            if (!securityAccessProvider.isAllowed(authorizedServices, sourceServiceTvmClientId)) {
                logger.warn("Authorization for services = [{}] required " +
                                "but sourceTvmClientId=[{}] is not configured as authorized",
                        authorizedServices, sourceServiceTvmClientId);

                response.sendError(HttpStatus.FORBIDDEN.value(), "authorized client does not have access");
                return false;
            }
        }
        request.setAttribute(TVM_CLIENT_ID_REQUEST_ATTR, String.valueOf(sourceServiceTvmClientId));
        return true;
    }

    private boolean processUserTicket(HttpServletRequest request, boolean userRequired, HttpServletResponse response)
            throws IOException {

        if (allowXUid && request.getHeader(SecurityHeaders.X_UID) != null) {
            String uid = request.getHeader(SecurityHeaders.X_UID);
            logger.info("User provided via developer X-Uid header: {}", uid);
            request.setAttribute(UID_REQUEST_ATTR, Long.parseLong(uid));
            return true;
        }

        String userTicketHeader = request.getHeader(SecurityHeaders.USER_TICKET_HEADER);
        if (userTicketHeader != null && !userTicketHeader.isBlank()) {
            logger.debug("User ticket provided: {}",
                    () -> userTicketPattern.matcher(userTicketHeader).replaceAll(":###"));

            CheckedUserTicket userTicket = tvmClient.checkUserTicket(userTicketHeader);
            TicketStatus status = userTicket.getStatus();
            if (status == TicketStatus.OK) {
                logger.debug("User ticket status: {}", status);
                request.setAttribute(UID_REQUEST_ATTR, userTicket.getDefaultUid());
                request.setAttribute(USER_TICKET_REQUEST_ATTR, userTicketHeader);
                return true;
            } else {
                logger.warn("Invalid user ticket. status: {}", () -> status.name());
                response.sendError(HttpStatus.FORBIDDEN.value(), "invalid TVM user ticket");
                return false;
            }
        }

        if (userRequired) {
            logger.debug("Method handler requires authorized user but no tvm user ticket provided");
            response.sendError(HttpStatus.FORBIDDEN.value(), "Authorization is required");
            return false;
        } else {
            return true;
        }
    }

    private boolean isTrustedDeveloperMode(HttpServletRequest request) {
        var token = request.getHeader(SecurityHeaders.X_DEVELOPER_TRUSTED_TOKEN);
        return token != null && token.equals(developerTrustedToken);
    }

    private OptionalInt tryGetServiceClientIdFromTvmHeaders(HttpServletRequest request) {
        // validate service ticket if exists in request
        String serviceTicketHeader = request.getHeader(SecurityHeaders.SERVICE_TICKET_HEADER);
        if (serviceTicketHeader != null && !serviceTicketHeader.isEmpty()) {
            logger.debug("Checking TVM service ticket header");

            try {
                CheckedServiceTicket serviceTicket = tvmClient.checkServiceTicket(serviceTicketHeader);
                if (serviceTicket.getStatus() == TicketStatus.OK) {
                    return OptionalInt.of(serviceTicket.getSrc());
                } else if (serviceTicket.getStatus() == TicketStatus.INVALID_DST && !validateServiceTicket) {
                    logger.warn("Invalid TVM service ticket, wrong Dst. " +
                            "Skipping due to disabled service ticket validation. " +
                            "DebugInfo:{}", serviceTicket.debugInfo());

                    // serviceTicket.getSrc() leads to NotAllowedException on invalid ticket
                    return OptionalInt.of(1);
                } else {
                    logger.warn("Invalid TVM service ticket. Status: {}, debugInfo: {}",
                            serviceTicket.getStatus(), serviceTicket.debugInfo());
                    return OptionalInt.empty();
                }
            } catch (Exception e) {
                logger.error("Exception on TVM service ticket check", e);
                throw new TvmAuthorizationException(e);
            }
        }

        logger.debug("No TVM Service ticket header provided");
        return OptionalInt.empty();
    }

    private OptionalInt tryGetServiceClientIdFromTrustedDeveloperMode(HttpServletRequest request) {
        String header = request.getHeader(SecurityHeaders.X_TRUSTED_SERVICE_TVM_CLIENT_ID);
        if (header != null && !header.isEmpty()) {
            try {
                return OptionalInt.of(Integer.parseInt(header.trim()));
            } catch (NumberFormatException e) {
                logger.warn("Incorrect trusted service tvm client id - {}", header);
                return OptionalInt.empty();
            }
        } else {
            logger.warn("{} header expected for trusted developer authorization, but none found",
                    SecurityHeaders.X_DEVELOPER_TRUSTED_TOKEN);
            return OptionalInt.empty();
        }
    }

    private Optional<HandlerRequirements> handlerRequirements(HandlerMethod handler) {
        TvmRequired methodAnnotation = handler.getMethodAnnotation(TvmRequired.class);
        if (methodAnnotation != null) {
            return Optional.of(new HandlerRequirements(methodAnnotation));
        }

        var classAnnotation = handler.getBeanType().getAnnotation(TvmRequired.class);
        if (classAnnotation != null) {
            return Optional.of(new HandlerRequirements(classAnnotation));
        }

        return Optional.empty();
    }

    private static class HandlerRequirements {
        private final boolean userRequired;
        private final Set<String> allowed;

        HandlerRequirements(TvmRequired tvmRequired) {
            this.userRequired = tvmRequired.userRequired();
            this.allowed = new HashSet<>(Arrays.asList(tvmRequired.allowed()));
        }
    }
}
