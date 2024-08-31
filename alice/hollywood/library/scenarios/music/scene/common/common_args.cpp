#include "common_args.h"

#include <alice/megamind/protos/blackbox/blackbox.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

namespace {

TStringBuf GetUid(const TRunRequest& request) {
    if (const auto* ds = request.GetDataSource(EDataSourceType::BLACK_BOX)) {
        return ds->GetUserInfo().GetUid();
    }
    return TStringBuf{};
}

} // namespace

void FillCommonArgs(TMusicScenarioSceneArgsCommon& commonArgs, const TRunRequest& request) {
    const TStringBuf uid = GetUid(request);
    commonArgs.MutableAccountStatus()->SetUid(uid.data(), uid.size());
}

const NData::NMusic::TContentId* TryGetContentId(const TMusicScenarioSceneArgsCommon& commonArgs) {
    const NData::NMusic::TContentId* contentId = nullptr;
    if (commonArgs.GetFrame().HasContentId()) {
        contentId = &commonArgs.GetFrame().GetContentId();
    }
    return contentId;
}

} // namespace NAlice::NHollywoodFw::NMusic
