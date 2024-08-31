package ru.yandex.alice.paskills.common.tvm.spring.handler;

import java.util.Map;
import java.util.Set;

class SecurityAccessProvider {
    private final Map<String, Set<Integer>> allowedServiceClientIds;

    SecurityAccessProvider(Map<String, Set<Integer>> allowedServiceClientIds) {
        this.allowedServiceClientIds = allowedServiceClientIds;
    }

    public boolean isAllowed(Set<String> services, int clientId) {
        return services.stream().anyMatch(service -> isAllowed(service, clientId));
    }

    private boolean isAllowed(String service, int clientId) {
        if (allowedServiceClientIds.containsKey(service)) {
            return allowedServiceClientIds.get(service).contains(clientId);
        }
        return false;
    }
}
