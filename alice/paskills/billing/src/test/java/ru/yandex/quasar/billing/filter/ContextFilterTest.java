package ru.yandex.quasar.billing.filter;

import java.util.Base64;
import java.util.Set;

import javax.servlet.http.HttpServletRequest;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.quasar.billing.providers.UaasHeaders;
import ru.yandex.quasar.billing.services.AuthorizationContext;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class ContextFilterTest {

    private ContextFilter filter;

    private AuthorizationContext authorizationContext;

    @BeforeEach
    void setUp() {
        authorizationContext = new AuthorizationContext();
        authorizationContext.clearUserContext();
        filter = new ContextFilter(authorizationContext);
    }

    @Test
    void testExperimentsDeserialization() {
        HttpServletRequest request = mock(HttpServletRequest.class);
        String header = "[{\"CONTEXT\": {\"MAIN\": {\"VOICE\": {\"flags\":[\"abc\", \"def\"]}}}}]";
        when(request.getHeader(UaasHeaders.EXPERIMENT_FLAG_HEADER))
                .thenReturn(Base64.getEncoder().encodeToString(header.getBytes()));
        filter.addRequestExperiments(request);
        Set<String> experiments = authorizationContext.getRequestExperiments();
        assertEquals(experiments, Set.of("abc", "def"));
    }
}
