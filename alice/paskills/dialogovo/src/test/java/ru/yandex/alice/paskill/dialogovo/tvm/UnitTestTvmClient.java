package ru.yandex.alice.paskill.dialogovo.tvm;

import ru.yandex.passport.tvmauth.BlackboxEnv;
import ru.yandex.passport.tvmauth.CheckedServiceTicket;
import ru.yandex.passport.tvmauth.CheckedUserTicket;
import ru.yandex.passport.tvmauth.ClientStatus;
import ru.yandex.passport.tvmauth.TicketStatus;
import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.passport.tvmauth.Unittest;
import ru.yandex.passport.tvmauth.roles.Roles;

public class UnitTestTvmClient implements TvmClient {

    public static final String SERVICE_TICKET = "ticket1";

    @Override
    public ClientStatus getStatus() {
        return new ClientStatus(ClientStatus.Code.OK, "OK");
    }

    @Override
    public String getServiceTicketFor(String alias) {
        return "ticket";
    }

    @Override
    public String getServiceTicketFor(int clientId) {
        return "ticket";
    }

    @Override
    public CheckedServiceTicket checkServiceTicket(String ticketBody) {
        return Unittest.createServiceTicket(TicketStatus.OK, 0);
    }

    @Override
    public CheckedUserTicket checkUserTicket(String ticketBody) {
        return Unittest.createUserTicket(TicketStatus.OK, 1, new String[0], new long[0]);
    }

    @Override
    public CheckedUserTicket checkUserTicket(String ticketBody, BlackboxEnv env) {
        throw new UnsupportedOperationException("Not implemented, yet");
    }

    @Override
    public Roles getRoles() {
        throw new UnsupportedOperationException("Not implemented, yet");
    }

    @Override
    public void close() {

    }
}
