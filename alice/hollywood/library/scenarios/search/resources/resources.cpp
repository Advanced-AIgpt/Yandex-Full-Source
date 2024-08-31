#include "resources.h"

#include <util/stream/file.h>

namespace NAlice::NHollywood::NSearch {

void TSearchScenarioResources::LoadFromPath(const TFsPath& dirPath) {
    BnoApps = MakeHolder<TBnoApps>(dirPath);
    NavigationFixList = MakeHolder<TNavigationFixList>(dirPath, TEmptyLogAdapter());
}

const TBnoApps* TSearchScenarioResources::GetBnoApps() const {
    return BnoApps.Get();
}

const TNavigationFixList* TSearchScenarioResources::GetNavigationFixList() const {
    return NavigationFixList.Get();
}

} // namespace NAlice::NHollywood::NSearch
