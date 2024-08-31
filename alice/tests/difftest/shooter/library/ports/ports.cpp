#include "ports.h"

#include <alice/joker/library/log/log.h>

#include <util/network/sock.h>

namespace NAlice::NShooter {

namespace {

static constexpr int PORT_RADIUS = 10;

bool IsPortFree(ui16 port) {
    TSockAddrInet6 addr("::", port);
    TInet6StreamSocket sock;
    return sock.Bind(&addr) == 0;
}

} // namespace

ui16 TPorts::Add(TStringBuf appName) {
    TWriteGuard g(Lock_);

    ui16 port = FreePort();
    Map_.emplace(appName, port);
    Ports_.emplace(port);

    LOG(INFO) << "Register port " << port << " for app \"" << appName << "\"" << Endl;
    return port;
}

ui16 TPorts::Get(TStringBuf appName) const {
    TReadGuard g(Lock_);

    auto ptr = Map_.find(appName);
    Y_ASSERT(ptr != Map_.end());
    return ptr->second;
}

bool TPorts::Has(TStringBuf appName) const {
    return Map_.contains(appName);
}

// The port returned is free (or registered) only in the moment of calling this function
ui16 TPorts::FreePort() {
    for (ui16 p = 15000; p <= 25000; ++p) {
        bool notRegistered = true;
        for (const auto reg : Ports_) {
            if (abs(reg - p) < PORT_RADIUS) {
                notRegistered = false;
                break;
            }
        }

        if (notRegistered && IsPortFree(p)) {
            return p;
        }
    }
    ythrow yexception() << "Can't get any free port";
}

THashSet<ui16> TPorts::Ports_;
TRWMutex TPorts::Lock_;

} // namespace NAlice::NShooter
