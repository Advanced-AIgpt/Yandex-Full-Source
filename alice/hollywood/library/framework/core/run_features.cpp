#include "run_features.h"

#include <alice/hollywood/library/framework/proto/framework_state.pb.h>

#include <alice/megamind/protos/scenarios/features/music.pb.h>
#include <alice/megamind/protos/scenarios/features/gc.pb.h>
#include <alice/megamind/protos/scenarios/features/music.pb.h>
#include <alice/megamind/protos/scenarios/features/search.pb.h>
#include <alice/megamind/protos/scenarios/features/vins.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/library/video_common/protos/features.pb.h>

#include <util/generic/yexception.h>

namespace NAlice::NHollywoodFw {

namespace {

//
// Additional helper function to set oneof message inside TRunFeature
//
template <class TOneof>
bool AssignAsAClass(NScenarios::TScenarioRunResponse::TFeatures& features,
                    TOneof* (NScenarios::TScenarioRunResponse::TFeatures::*fn)(),
                    const google::protobuf::Message& obj) {
    TOneof copy;
    if (obj.GetTypeName() != copy.GetTypeName()) {
        return false;
    }
    const TOneof* upcastMessage = dynamic_cast<const TOneof*>(&obj);
    if (upcastMessage == nullptr) {
        return false;
    }
    *(features.*fn)() = *upcastMessage;
    return true;
}

} // anonimous namespace

struct TRunFeaturesImpl {
    NScenarios::TScenarioRunResponse::TFeatures Features;
};

TRunFeatures::TRunFeatures(const NScenarios::TScenarioRunResponse_TFeatures& proto) {
    Impl_.reset(new TRunFeaturesImpl);
    Impl_->Features.CopyFrom(proto);
}

void TRunFeatures::SetIntentName(const TString& intent) {
    if (!Impl_) {
        Impl_.reset(new TRunFeaturesImpl);
    }
    *Impl_->Features.MutableIntent() = intent;
}

void TRunFeatures::SetPlayerFeatures(bool restorePlayer, ui32 secondsSincePause) {
    if (!Impl_) {
        Impl_.reset(new TRunFeaturesImpl);
    }
    NScenarios::TScenarioRunResponse::TFeatures::TPlayerFeatures pf;
    pf.SetRestorePlayer(restorePlayer);
    pf.SetSecondsSincePause(secondsSincePause);
    *Impl_->Features.MutablePlayerFeatures() = std::move(pf);
}

void TRunFeatures::IgnoresExpectedRequest(bool f) {
    if (!Impl_) {
        Impl_.reset(new TRunFeaturesImpl);
    }
    Impl_->Features.SetIgnoresExpectedRequest(f);
}

void TRunFeatures::Set(const google::protobuf::Message& obj) {
    if (!Impl_) {
        Impl_.reset(new TRunFeaturesImpl);
    }
    if (AssignAsAClass(Impl_->Features, &NScenarios::TScenarioRunResponse::TFeatures::MutableMusicFeatures, obj) ||
        AssignAsAClass(Impl_->Features, &NScenarios::TScenarioRunResponse::TFeatures::MutableVideoFeatures, obj) ||
        AssignAsAClass(Impl_->Features, &NScenarios::TScenarioRunResponse::TFeatures::MutableVinsFeatures, obj) ||
        AssignAsAClass(Impl_->Features, &NScenarios::TScenarioRunResponse::TFeatures::MutableSearchFeatures, obj) ||
        AssignAsAClass(Impl_->Features, &NScenarios::TScenarioRunResponse::TFeatures::MutableGCFeatures, obj)) {
        return;
    }
    Y_ENSURE(false, "This class can't be passed into TFeatures::Set()");
}

void TRunFeatures::ExportToProto(TProtoHwScene& sceneResults) {
    if (Impl_ != nullptr) {
        *sceneResults.MutableRunFeatures() = Impl_->Features;
    }
}

void TRunFeatures::ExportToResponse(NScenarios::TScenarioRunResponse& response) {
    if (Impl_ != nullptr) {
        response.MutableFeatures()->CopyFrom(Impl_->Features);
    }
}


} // namespace NAlice::NHollywoodFw
