#include "formulas.h"

#include "has_good_result/factors.h"
#include "show_or_gallery/factors.h"

#include <library/cpp/resource/resource.h>

#include <util/generic/strbuf.h>
#include <util/stream/str.h>

namespace NVideoCommon {

namespace {
std::unique_ptr<TFullModel> LoadModel(TStringBuf name) {
    try {
        TString data;
        if (!NResource::FindExact(name, &data)) {
            LOG(ERR) << "Can't load " << name << Endl;
            return {};
        }

        return std::make_unique<TFullModel>(DeserializeModel(data));
    } catch (...) {
        LOG(ERR) << "Failed to load " << name << Endl;
        return {};
    }
}
} // namespace

void TFormulas::Init() {
    HasGoodResult = LoadModel("has_good_result.cbm");
    ShowOrGallery = LoadModel("show_or_gallery.cbm");
}

TMaybe<double> TFormulas::GetProb(const NHasGoodResult::TFactors& factors) const {
    return GetProbImpl(HasGoodResult, factors);
}

TMaybe<double> TFormulas::GetProb(const NShowOrGallery::TFactors& factors) const {
    return GetProbImpl(ShowOrGallery, factors);
}
} // namespace NVideoCommon
