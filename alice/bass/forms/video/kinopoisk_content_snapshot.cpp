#include "kinopoisk_content_snapshot.h"

#include <library/cpp/resource/resource.h>

namespace NBASS {
namespace NVideo {

namespace {

class TKinopoiskContentSnapshot : private TMoveOnly {
public:
    TKinopoiskContentSnapshot() {
        auto json = NSc::TValue::FromJson(NResource::Find("kinopoisk.json"));
        for (const NSc::TValue& elem : json.GetArray()) {
            AddItem(TKinopoiskContentItem(elem));
        }
    }

    void AddItem(TKinopoiskContentItem&& item) {
        TString id = TString{item->FilmKpId().Get()};
        ByKinopoiskId.insert(std::make_pair(id, std::move(item)));
    }

    const TKinopoiskContentItem* FindByKinopoiskId(TStringBuf kpId) const {
        return ByKinopoiskId.FindPtr(kpId);
    }

private:
    THashMap<TString, TKinopoiskContentItem> ByKinopoiskId;
};

} // namespace anonymous

const TKinopoiskContentItem* FindKpContentItemByKpId(TStringBuf kpId) {
    return Singleton<TKinopoiskContentSnapshot>()->FindByKinopoiskId(kpId);
}

void InitKpContentSnapshot() {
    const TKinopoiskContentItem* dummy = FindKpContentItemByKpId(TStringBuf());
    Y_UNUSED(dummy);
}

} // namespace NVideo
} // namespace NBASS
