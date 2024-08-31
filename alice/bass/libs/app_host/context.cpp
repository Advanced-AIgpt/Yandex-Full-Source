#include "context.h"

namespace NBASS {

namespace {
constexpr TStringBuf ANSWERS = "answers";
}

void TAppHostInitContext::AddSourceInit(const IAppHostSource& appHostSource) {
    AppHostInitData.Push(appHostSource.GetSourceInit());
}

NSc::TValue TAppHostInitContext::GetInitData() const {
    NSc::TValue initData;
    initData["name"].SetString("INIT");
    initData["results"].CopyFrom(AppHostInitData);
    NSc::TValue wrapper;
    wrapper.Push(initData);
    return wrapper;
}

TAppHostResultContext::TAppHostResultContext(NSc::TValue resultsFromSource)
    : RawResponse(std::move(resultsFromSource))
{
}

TMaybe<NSc::TValue> TAppHostResultContext::GetItemRef(TStringBuf name) const {
    if (!RawResponse.Has(ANSWERS)) {
        return Nothing();
    }
    for (const auto& item : RawResponse[ANSWERS].GetArray()) {
        const NSc::TValue& itemName = item.TrySelect("name");
        if (itemName.GetString() == name) {
            return item;
        }
    }
    return Nothing();
}

} // NBASS
