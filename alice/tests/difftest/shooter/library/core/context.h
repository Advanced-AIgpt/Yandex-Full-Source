#pragma once

#include <alice/tests/difftest/shooter/library/core/config.h>
#include <alice/tests/difftest/shooter/library/ports/ports.h>
#include <alice/tests/difftest/shooter/library/yav/yav.h>

#include <util/folder/path.h>
#include <util/generic/maybe.h>

namespace NAlice::NShooter {

struct TJokerServerSettings {
    TString Host;
    ui16 Port;
};

struct TTokens {
    TString YavToken;
    TString TestUserToken;
};

class IContext : public NNonCopyable::TNonCopyable {
public:
    virtual ~IContext() = default;

    virtual const TConfig& Config() const = 0;
    virtual const TTokens& Tokens() const = 0;
    virtual const THolder<IYav>& Yav() const = 0;
    virtual const TMaybe<TJokerServerSettings>& JokerServerSettings() const = 0;
};

class TContext : public IContext {
public:
    TContext(const TFsPath& configFilePath, TTokens tokens = TTokens());

    const TConfig& Config() const override {
        return Config_;
    }

    const TTokens& Tokens() const override {
        return Tokens_;
    }

    const THolder<IYav>& Yav() const override {
        return Yav_;
    }

    const TMaybe<TJokerServerSettings>& JokerServerSettings() const override {
        return JokerServerSettings_;
    }

private:
    const TConfig Config_;
    const TTokens Tokens_;
    const THolder<IYav> Yav_;
    const TMaybe<TJokerServerSettings> JokerServerSettings_;
};

} // namespace NAlice::NShooter
