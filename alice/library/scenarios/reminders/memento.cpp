#include "memento.h"

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/memento/proto/api.pb.h>

#include <google/protobuf/any.pb.h>

#include <alice/library/proto/protobuf.h>

namespace NAlice::NRemindersApi {
namespace {

namespace NMemento = ru::yandex::alice::memento::proto;

template <typename T>
NMemento::TConfigKeyAnyPair CreateUserConfigs(NMemento::EConfigKey key, const T& proto) {
    NMemento::TConfigKeyAnyPair configPair;
    configPair.SetKey(key);

    ::google::protobuf::Any configAny;
    configAny.PackFrom(proto);
    *configPair.MutableValue() = configAny;

    return configPair;
}

} // namespace


TMaybe<TRemindersState> RemindersFromMemento(TStringBuf mementoBase64) {
    NScenarios::TMementoData mementoProto;
    ProtoFromBase64String(mementoBase64, mementoProto);
    return RemindersFromMemento(mementoProto);
}

TMaybe<TRemindersState> RemindersFromMemento(const NScenarios::TMementoData& mementoProto) {
    if (mementoProto.GetUserConfigs().HasReminders()) {
        return mementoProto.GetUserConfigs().GetReminders();
    }
    return Nothing();
}

TMaybe<TRemindersState> RemindersFromMemento(NScenarios::TMementoData&& mementoProto) {
    if (mementoProto.GetUserConfigs().HasReminders()) {
        return std::move(*mementoProto.MutableUserConfigs()->MutableReminders());
    }
    return Nothing();
}

// TMementoReminderDirectiveBuilder ---------------------------------------------------------------
NScenarios::TServerDirective TMementoReminderDirectiveBuilder::BuildSaveServerDirective(
    const TRemindersState& state)
{
    NScenarios::TServerDirective sd;
    sd.MutableMementoChangeUserObjectsDirective()->MutableUserObjects()->MutableUserConfigs()->Add(
        CreateUserConfigs(NMemento::EConfigKey::CK_REMINDERS, state)
    );
    return sd;
}

} // namespace NAlice::NRemindersApi
