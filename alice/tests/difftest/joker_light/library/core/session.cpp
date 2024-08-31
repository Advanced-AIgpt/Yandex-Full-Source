#include "session.h"
#include "context.h"

#include <alice/joker/library/log/log.h>

namespace NAlice::NJokerLight {

namespace {

} // namespace

TSession::TSession(TContext& context, const TString& id)
    : Context_{context}
    , Id_{id}
{
}

void TSession::Init(TSettings settings) {
    LOG(INFO) << "Initing session " << Id_.Quote() << Endl;

    Settings_.ConstructInPlace(std::move(settings));
    auto& ydb = Context_.Ydb();
    ydb.UpsertSession(Id_, Settings_.GetRef());
}

TStatus TSession::Load() {
    LOG(INFO) << "Loading session " << Id_.Quote() << Endl;

    auto& ydb = Context_.Ydb();
    TMaybe<TSession::TSettings> setting = ydb.ObtainSession(Id_);
    if (setting.Defined()) {
        Settings_ = std::move(setting);
        return Success();
    }
    return TError{} << "Can't load session " << Id_.Quote();
}

const TSession::TSettings& TSession::Settings() const {
    Y_ASSERT(Settings_);
    return Settings_.GetRef();
}

const TString& TSession::Id() const {
    return Id_;
}

} // namespace NAlice::NJokerLight
