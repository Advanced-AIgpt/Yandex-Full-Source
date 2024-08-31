#include "joker_app.h"

#include <alice/joker/library/log/log.h>
#include <alice/tests/difftest/shooter/library/core/context.h>

#include <library/cpp/neh/neh.h>
#include <library/cpp/uri/uri.h>

#include <util/string/builder.h>

namespace NAlice::NShooter {

namespace {

static inline constexpr TStringBuf NAME = "joker";

} // namespace

TJokerApp::TJokerApp(const IContext& ctx)
    : Ctx_{ctx}
{
}

TStringBuf TJokerApp::Name() const {
    return NAME;
}

TStatus TJokerApp::Ping() {
    const auto& jokerSettings = Ctx_.JokerServerSettings();
    NNeh::TResponseRef r = AppRequest(Name(), {jokerSettings->Host, jokerSettings->Port, "/admin", "action=ping"});
    if (r && r->Data == "pong") {
        return Success();
    }
    return TError();
}

TStatus TJokerApp::Run() {
    // init Joker session
    Y_ASSERT(Ctx_.Config().HasJokerConfig());
    const auto& jokerConfig = Ctx_.Config().JokerConfig();
    const auto& jokerSettings = Ctx_.JokerServerSettings();

    TStringBuilder initSettings;
    initSettings << "id=" << jokerConfig.SessionId();
    if (jokerConfig.HasSessionSettings()) {
        initSettings << "&" << jokerConfig.SessionSettings();
    }
    AppRequest(Name(), {jokerSettings->Host, jokerSettings->Port, "/session", initSettings});

    return Success();
}

void TJokerApp::Init() {}
void TJokerApp::Stop() {}

} // namespace NAlice::NShooter
