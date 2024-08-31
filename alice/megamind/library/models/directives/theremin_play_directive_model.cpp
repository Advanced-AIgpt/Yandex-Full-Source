#include "theremin_play_directive_model.h"

#include <util/generic/yexception.h>
#include <utility>

namespace NAlice::NMegamind {

// TThereminPlayDirectiveModel -------------------------------------------------
TThereminPlayDirectiveModel::TThereminPlayDirectiveModel(const TString& analyticsType,
                                                         TIntrusivePtr<IThereminPlayDirectiveSetModel> set)
    : TClientDirectiveModel("theremin_play", analyticsType)
    , Set(std::move(set)) {
    if (!Set) {
        ythrow yexception() << "Set can't be null";
    }
}

void TThereminPlayDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TIntrusivePtr<IThereminPlayDirectiveSetModel>& TThereminPlayDirectiveModel::GetSet() const {
    return Set;
}

// TThereminPlayDirectiveExternalSetModel --------------------------------------
TThereminPlayDirectiveExternalSetModel::TThereminPlayDirectiveExternalSetModel(
    bool noOverlaySamples, bool repeatSoundInside, bool stopOnCeil,
    TVector<TThereminPlayDirectiveExternalSetSampleModel> samples)
    : NoOverlaySamples(noOverlaySamples)
    , RepeatSoundInside(repeatSoundInside)
    , StopOnCeil(stopOnCeil)
    , Samples(std::move(samples)) {
}

void TThereminPlayDirectiveExternalSetModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

EThereminPlayDirectiveSet TThereminPlayDirectiveExternalSetModel::GetType() const {
    return EThereminPlayDirectiveSet::ExternalSet;
}

bool TThereminPlayDirectiveExternalSetModel::GetRepeatSoundInside() const {
    return RepeatSoundInside;
}

bool TThereminPlayDirectiveExternalSetModel::GetNoOverlaySamples() const {
    return NoOverlaySamples;
}

bool TThereminPlayDirectiveExternalSetModel::GetStopOnCeil() const {
    return StopOnCeil;
}

const TVector<TThereminPlayDirectiveExternalSetSampleModel>&
TThereminPlayDirectiveExternalSetModel::GetSamples() const {
    return Samples;
}

// TThereminPlayDirectiveExternalSetSampleModel --------------------------------
TThereminPlayDirectiveExternalSetSampleModel::TThereminPlayDirectiveExternalSetSampleModel(const TString& url)
    : Url(url) {
}

const TString& TThereminPlayDirectiveExternalSetSampleModel::GetUrl() const {
    return Url;
}

// TThereminPlayDirectiveInternalSetModel --------------------------------------
TThereminPlayDirectiveInternalSetModel::TThereminPlayDirectiveInternalSetModel(int mode)
    : Mode(mode) {
}

EThereminPlayDirectiveSet TThereminPlayDirectiveInternalSetModel::GetType() const {
    return EThereminPlayDirectiveSet::InternalSet;
}

void TThereminPlayDirectiveInternalSetModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

int TThereminPlayDirectiveInternalSetModel::GetMode() const {
    return Mode;
}

} // namespace NAlice::NMegamind
