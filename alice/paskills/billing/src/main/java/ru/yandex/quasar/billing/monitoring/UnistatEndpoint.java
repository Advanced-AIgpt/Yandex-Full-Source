package ru.yandex.quasar.billing.monitoring;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.actuate.endpoint.annotation.Endpoint;
import org.springframework.boot.actuate.endpoint.annotation.ReadOperation;
import org.springframework.stereotype.Component;

import ru.yandex.quasar.billing.services.UnistatService;

@Component
@Endpoint(id = "unistat")
class UnistatEndpoint {

    private final UnistatService unistatService;

    @Autowired
    UnistatEndpoint(UnistatService unistatService) {
        this.unistatService = unistatService;
    }


    @ReadOperation
    public String unistat() {
        return unistatService.getAllInUnistatFormat();
    }
}
