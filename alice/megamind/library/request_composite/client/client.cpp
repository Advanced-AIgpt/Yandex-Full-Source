#include "client.h"

namespace NAlice::NMegamind {

// TClientComponent::TView -----------------------------------------------------
bool TClientComponent::TView::HasExpFlag(TStringBuf name) const {
    const auto maybeFlag = ExpFlag(name);
    return maybeFlag.Defined() && maybeFlag.GetRef() != TStringBuf("0");
}

TMaybe<TString> TClientComponent::TView::ExpFlag(TStringBuf name) const {
    if (const auto* ptr = ExpFlags().FindPtr(name)) {
        return *ptr;
    }
    return Nothing();
}

const TClientComponent::TExpFlags& TClientComponent::TView::ExpFlags() const {
    return Client_.ExpFlags();
}

// TClientComponent -----------------------------------------------------------
// static
TClientComponent::TExpFlags TClientComponent::CreateExpFlags(const TExperimentsProto& proto) {
    return TExpFlagsConverter::Build(proto);
}

// static
TClientFeatures TClientComponent::CreateClientFeatures(const TSpeechKitRequestProto& proto, const TExpFlags& expFlags) {
    TClientFeatures clientFeatures{proto.GetApplication(), expFlags};
    for (const auto& feature : proto.GetRequest().GetAdditionalOptions().GetSupportedFeatures()) {
        clientFeatures.AddSupportedFeature(feature);
    }
    for (const auto& feature : proto.GetRequest().GetAdditionalOptions().GetUnsupportedFeatures()) {
        clientFeatures.AddUnsupportedFeature(feature);
    }
    return clientFeatures;
}

} // namespace NAlice::NMegamind
