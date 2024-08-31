#include "config_ut_helpers.h"

#include <alice/megamind/library/protos/all_typed_callbacks.pb.h>
#include <alice/protos/extensions/extensions.pb.h>

#include <google/protobuf/descriptor.pb.h>

namespace NAlice::NMegamind::NTesting {

TFsPath GetConfigPath(const TString& env) {
    return TFsPath(ArcadiaSourceRoot()) / "alice/megamind/configs" / env;
}

TFsPath GetCombinatorConfigPath(const TString& env) {
    return GetConfigPath(env) / "combinators";
}

TFsPath GetScenarioConfigPath(const TString& env) {
    return GetConfigPath(env) / "scenarios";
}

TFsPath GetServerConfigPath(const TString& env) {
    return GetConfigPath(env) / "megamind.pb.txt";
}

bool IsExistingTypedCallback(const TString& typedCallbackName) {
    TTypedCallback allCallbacks;
    const auto* descriptor = allCallbacks.GetDescriptor();
    const auto* callbacksOneofDescriptor = descriptor->FindOneofByName("Callback");
    if (!callbacksOneofDescriptor) {
        return false;
    }
    for (auto idx = 0; idx < callbacksOneofDescriptor->field_count(); ++idx) {
        const auto* callbackDescriptor = callbacksOneofDescriptor->field(idx)->message_type();
        if (callbackDescriptor->options().GetExtension(NAlice::MMTypedCallbackName) == typedCallbackName) {
            return true;
        }
    }
    return false;
}

} //NAlice::NMegamind::NTesting
