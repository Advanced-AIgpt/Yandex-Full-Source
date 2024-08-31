#pragma once

#include "callback_directive_model.h"
#include "client_directive_model.h"

#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <google/protobuf/struct.pb.h>

namespace NAlice::NMegamind {

class TAlarmSetSoundDirectiveModel final : public TClientDirectiveModel {
public:
    using TSettings = NScenarios::TAlarmSetSoundDirective_TSettings;

    template <typename TAlarmSettings>
    TAlarmSetSoundDirectiveModel(const TString& analyticsType, TCallbackDirectiveModel&& callback,
                                 TAlarmSettings&& settings)
        : TClientDirectiveModel("alarm_set_sound", analyticsType)
        , Callback(std::move(callback))
        , Settings(std::forward<TAlarmSettings>(settings)) {
    }

    void Accept(IModelSerializer& serializer) const final {
        serializer.Visit(*this);
    }

    [[nodiscard]] const TCallbackDirectiveModel& GetCallback() const {
        return Callback;
    }

    [[nodiscard]] const TSettings& GetSettings() const {
        return Settings;
    }

private:
    TCallbackDirectiveModel Callback;
    TSettings Settings;
};

} // namespace NAlice::NMegamind
