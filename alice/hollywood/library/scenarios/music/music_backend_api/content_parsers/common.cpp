#include "common.h"

namespace NAlice::NHollywood::NMusic {

TMaybe<NData::NMusic::TContentInfo> TryConstructContentInfo(const NJson::TJsonValue& resultJson) {
    if (const TString& title = resultJson[TITLE].GetString(); !title.empty()) {
        NData::NMusic::TContentInfo contentInfo;
        contentInfo.SetTitle(title);
        return contentInfo;
    }
    return Nothing();
}

} // namespace NAlice::NHollywood::NMusic
