package ru.yandex.quasar.billing.services.tvm;

import java.util.Arrays;
import java.util.Map;

import ru.yandex.passport.tvmauth.BlackboxEnv;
import ru.yandex.passport.tvmauth.CheckedServiceTicket;
import ru.yandex.passport.tvmauth.CheckedUserTicket;
import ru.yandex.passport.tvmauth.ClientStatus;
import ru.yandex.passport.tvmauth.TicketStatus;
import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.passport.tvmauth.Unittest;
import ru.yandex.passport.tvmauth.roles.Roles;
import ru.yandex.quasar.billing.config.SecretsConfig;

import static java.util.stream.Collectors.toMap;

public class TestTvmClientImpl implements TvmClient {

    public static final String UNIVERSAL_SERVICE_TICKET = "UNIVERSAL_SERVICE_TICKET";
    public static final String USER_TICKET = "user_ticket";
    public static final String UID = "999999";
    public static final String SERVICE_TICKET = "service_ticket";

    private final Map<String, String> predefinedTokens;
    private final Map<String, CheckedServiceTicket> predefinedServiceTickets;

    public TestTvmClientImpl(SecretsConfig secretsConfig) {
        this.predefinedTokens = Arrays.stream(TvmClientName.values())
                .collect(toMap(TvmClientName::getAlias, TestTvmClientImpl::getTestTicket));

        this.predefinedServiceTickets = Arrays.stream(TvmClientName.values())
                .collect(toMap(clientName -> predefinedTokens.get(clientName.getAlias()),
                        clientName -> Unittest.createServiceTicket(TicketStatus.OK,
                                secretsConfig.getTvmAliases().get(clientName.getAlias()))
                        )
                );
    }

    public static String getTestTicket(TvmClientName client) {
        return client.getAlias() + "_TICKET";
    }

    @Override
    public CheckedUserTicket checkUserTicket(String ticketBody) {
        if (USER_TICKET.equals(ticketBody)) {
            return Unittest.createUserTicket(TicketStatus.OK, Long.parseLong(UID), new String[0],
                    new long[]{Long.parseLong(UID)});
        } else {
            return Unittest.createUserTicket(TicketStatus.MALFORMED, 0, new String[0], new long[]{0});
        }
    }

    @Override
    public CheckedUserTicket checkUserTicket(String ticketBody, BlackboxEnv overridedBbEnv) {
        return checkUserTicket(ticketBody);
    }

    @Override
    public CheckedServiceTicket checkServiceTicket(String serviceTicket) {
        if (predefinedServiceTickets.containsKey(serviceTicket)) {
            return predefinedServiceTickets.get(serviceTicket);
        } else if (SERVICE_TICKET.equals(serviceTicket)) {
            return Unittest.createServiceTicket(TicketStatus.OK, 1);
        } else {
            return Unittest.createServiceTicket(TicketStatus.MALFORMED, 0);
        }
    }

    @Override
    public Roles getRoles() {
        return null;
    }

    @Override
    public void close() {

    }

    @Override
    public ClientStatus getStatus() {
        return new ClientStatus(ClientStatus.Code.OK, "");
    }

    @Override
    public String getServiceTicketFor(String alias) {
        return predefinedTokens.getOrDefault(alias, UNIVERSAL_SERVICE_TICKET);
    }

    @Override
    public String getServiceTicketFor(int tvmId) {
        return predefinedTokens.getOrDefault(TvmClientName.values()[tvmId], UNIVERSAL_SERVICE_TICKET);
    }


}
